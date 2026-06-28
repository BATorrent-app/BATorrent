// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "streamserver.h"
#include "../torrent/sessionmanager.h"

#include <QTcpServer>
#include <QTcpSocket>
#include <QHostAddress>
#include <QFile>
#include <QFileInfo>
#include <QTimer>
#include <QRegularExpression>
#include <QElapsedTimer>

namespace {

constexpr qint64 kChunk      = 256 * 1024;        // bytes read+written per pump step
constexpr qint64 kWriteHigh  = 4 * 1024 * 1024;   // pause reading above this socket backlog
constexpr int    kRetryMs    = 200;               // re-check cadence while buffering
constexpr int    kGiveUpMs   = 90 * 1000;         // no-progress timeout (dead torrent)

QByteArray contentType(const QString &path)
{
    QString p = path;
    if (p.endsWith(QStringLiteral(".!bt"))) p.chop(4);
    p = p.toLower();
    if (p.endsWith(".mp4") || p.endsWith(".m4v")) return "video/mp4";
    if (p.endsWith(".mkv"))  return "video/x-matroska";
    if (p.endsWith(".webm")) return "video/webm";
    if (p.endsWith(".mov"))  return "video/quicktime";
    if (p.endsWith(".avi"))  return "video/x-msvideo";
    if (p.endsWith(".ts"))   return "video/mp2t";
    if (p.endsWith(".mp3"))  return "audio/mpeg";
    if (p.endsWith(".flac")) return "audio/flac";
    return "application/octet-stream";
}

// One client connection: parses the request, sends headers, then pumps bytes
// off disk as pieces complete. Deletes itself when finished or on disconnect.
class StreamConnection : public QObject
{
public:
    StreamConnection(QTcpSocket *sock, IEngine *session)
        : QObject(sock), m_sock(sock), m_session(session)
    {
        connect(m_sock, &QTcpSocket::readyRead, this, &StreamConnection::onReadyRead);
        connect(m_sock, &QTcpSocket::disconnected, m_sock, &QObject::deleteLater);
        connect(m_sock, &QTcpSocket::bytesWritten, this, [this](qint64){ pump(); });
        m_noProgress.start();
    }

private:
    void onReadyRead()
    {
        if (m_started) { m_sock->readAll(); return; }   // ignore further input
        m_req += m_sock->readAll();
        const int end = m_req.indexOf("\r\n\r\n");
        if (end < 0) { if (m_req.size() > 16 * 1024) m_sock->disconnectFromHost(); return; }
        begin(m_req.left(end));
    }

    void begin(const QByteArray &headers)
    {
        m_started = true;
        const QList<QByteArray> lines = headers.split('\n');
        if (lines.isEmpty()) { fail(400); return; }
        const QList<QByteArray> req = lines.first().trimmed().split(' ');
        if (req.size() < 2) { fail(400); return; }
        const QByteArray method = req[0];
        const QString path = QString::fromUtf8(req[1]);
        const bool head = (method == "HEAD");
        if (method != "GET" && !head) { fail(405); return; }

        // /stream/<hash>/<fileIndex>
        const QRegularExpression re(QStringLiteral("^/stream/([0-9a-fA-F]+)/(\\d+)"));
        const auto m = re.match(path);
        if (!m.hasMatch()) { fail(404); return; }
        m_hash = m.captured(1);
        m_fileIdx = m.captured(2).toInt();
        m_torIdx = m_session->torrentIndexByInfoHash(m_hash);
        m_size = (m_torIdx >= 0) ? m_session->streamFileSize(m_torIdx, m_fileIdx) : 0;
        const QString fp = (m_torIdx >= 0) ? m_session->streamFilePath(m_torIdx, m_fileIdx) : QString();
        if (m_torIdx < 0 || m_size <= 0 || fp.isEmpty()) { fail(404); return; }

        // Range: bytes=start-end (end optional)
        qint64 start = 0, end = m_size - 1;
        bool ranged = false;
        for (const QByteArray &ln : lines) {
            const QByteArray l = ln.trimmed();
            if (l.toLower().startsWith("range:")) {
                const QRegularExpression rr(QStringLiteral("bytes=(\\d*)-(\\d*)"));
                const auto rm = rr.match(QString::fromUtf8(l));
                if (rm.hasMatch()) {
                    ranged = true;
                    if (!rm.captured(1).isEmpty()) start = rm.captured(1).toLongLong();
                    if (!rm.captured(2).isEmpty()) end = rm.captured(2).toLongLong();
                }
                break;
            }
        }
        if (start < 0) start = 0;
        if (end >= m_size) end = m_size - 1;
        if (start > end) { fail(416); return; }

        m_pos = start; m_end = end;

        QByteArray hdr;
        const qint64 len = end - start + 1;
        if (ranged) {
            hdr  = "HTTP/1.1 206 Partial Content\r\n";
            hdr += "Content-Range: bytes " + QByteArray::number(start) + "-"
                 + QByteArray::number(end) + "/" + QByteArray::number(m_size) + "\r\n";
        } else {
            hdr  = "HTTP/1.1 200 OK\r\n";
        }
        hdr += "Content-Type: " + contentType(fp) + "\r\n";
        hdr += "Content-Length: " + QByteArray::number(len) + "\r\n";
        hdr += "Accept-Ranges: bytes\r\n";
        hdr += "Connection: close\r\n\r\n";
        m_sock->write(hdr);

        if (head) { m_sock->flush(); m_sock->disconnectFromHost(); return; }

        // kick libtorrent to fetch this window first, then start serving
        m_session->streamSetDeadlineWindow(m_torIdx, m_fileIdx, m_pos);
        m_retry = new QTimer(this);
        m_retry->setInterval(kRetryMs);
        connect(m_retry, &QTimer::timeout, this, &StreamConnection::pump);
        pump();
    }

    void pump()
    {
        if (!m_sock || m_sock->state() != QAbstractSocket::ConnectedState) return;
        if (m_pos > m_end) { finishOk(); return; }
        if (m_sock->bytesToWrite() > kWriteHigh) return;   // let the socket drain first

        // re-resolve the index each tick — it can shift if torrents are
        // removed/reordered mid-stream; the hash is the stable identity
        m_torIdx = m_session->torrentIndexByInfoHash(m_hash);
        if (m_torIdx < 0) { m_sock->disconnectFromHost(); return; }

        qint64 avail = m_session->streamContiguousAvailableBytes(m_torIdx, m_fileIdx, m_pos, kChunk);
        if (avail <= 0) {
            // not downloaded yet — prioritize and wait
            m_session->streamSetDeadlineWindow(m_torIdx, m_fileIdx, m_pos);
            if (m_noProgress.elapsed() > kGiveUpMs) { m_sock->disconnectFromHost(); return; }
            if (m_retry && !m_retry->isActive()) m_retry->start();
            return;
        }
        if (m_retry && m_retry->isActive()) m_retry->stop();

        if (!ensureOpen()) {
            if (m_noProgress.elapsed() > kGiveUpMs) { m_sock->disconnectFromHost(); return; }
            if (m_retry && !m_retry->isActive()) m_retry->start();
            return;
        }

        const qint64 want = qMin(avail, m_end - m_pos + 1);
        QByteArray buf = m_file.read(want);
        if (buf.isEmpty()) {                 // file lagged / renamed under us — reopen next tick
            m_file.close();
            if (m_retry && !m_retry->isActive()) m_retry->start();
            return;
        }
        m_sock->write(buf);
        m_pos += buf.size();
        m_noProgress.restart();
        if (m_pos > m_end) { finishOk(); return; }
        // keep going this tick while data is ready and the buffer has room
        if (m_sock->bytesToWrite() <= kWriteHigh) QTimer::singleShot(0, this, &StreamConnection::pump);
    }

    bool ensureOpen()
    {
        const QString path = m_session->streamFilePath(m_torIdx, m_fileIdx);
        if (path.isEmpty()) return false;
        if (!m_file.isOpen() || m_file.fileName() != path) {
            m_file.close();
            m_file.setFileName(path);
            if (!m_file.open(QIODevice::ReadOnly)) return false;
        }
        return m_file.seek(m_pos);
    }

    void finishOk()
    {
        if (m_retry) m_retry->stop();
        m_sock->flush();
        m_sock->disconnectFromHost();
    }

    void fail(int code)
    {
        const char *txt = code == 404 ? "Not Found" : code == 416 ? "Range Not Satisfiable"
                        : code == 405 ? "Method Not Allowed" : "Bad Request";
        QByteArray r = "HTTP/1.1 " + QByteArray::number(code) + " " + txt + "\r\n"
                       "Content-Length: 0\r\nConnection: close\r\n\r\n";
        m_sock->write(r);
        m_sock->flush();
        m_sock->disconnectFromHost();
    }

    QTcpSocket *m_sock;
    IEngine *m_session;
    QByteArray m_req;
    bool m_started = false;
    QString m_hash;
    int m_torIdx = -1, m_fileIdx = -1;
    qint64 m_size = 0, m_pos = 0, m_end = 0;
    QFile m_file;
    QTimer *m_retry = nullptr;
    QElapsedTimer m_noProgress;
};

} // namespace

StreamServer::StreamServer(IEngine *session, QObject *parent)
    : QObject(parent), m_session(session) {}

StreamServer::~StreamServer() { stop(); }

bool StreamServer::start()
{
    if (m_server) return true;
    m_server = new QTcpServer(this);
    connect(m_server, &QTcpServer::newConnection, this, &StreamServer::onNewConnection);
    if (!m_server->listen(QHostAddress::LocalHost, 0)) {   // 127.0.0.1, ephemeral
        delete m_server; m_server = nullptr;
        return false;
    }
    m_port = m_server->serverPort();
    return true;
}

void StreamServer::stop()
{
    if (m_server) { m_server->close(); m_server->deleteLater(); m_server = nullptr; }
    m_port = 0;
}

bool StreamServer::isRunning() const { return m_server && m_server->isListening(); }

void StreamServer::onNewConnection()
{
    while (m_server && m_server->hasPendingConnections()) {
        QTcpSocket *sock = m_server->nextPendingConnection();
        new StreamConnection(sock, m_session);   // self-owned (parented to sock), self-deletes
    }
}
