// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "ipc/mediafeed.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <algorithm>

namespace {

constexpr qint64 kWindow = 16LL * 1024 * 1024;     // bytes held ahead of the reader
constexpr qint64 kKeepBehind = 2LL * 1024 * 1024;  // a short seek back stays in the buffer
constexpr qint64 kNearAhead = 4LL * 1024 * 1024;   // closer than this: wait, don't reconnect
constexpr qint32 kMinAnswer = 64 * 1024;

bool isStreamServer(const QUrl &u)
{
    return u.scheme() == QLatin1String("http") && u.host() == QLatin1String("127.0.0.1");
}

}

MediaFeed::MediaFeed(const QUrl &source, QObject *parent) : QObject(parent), m_source(source) {}

MediaFeed::~MediaFeed()
{
    if (m_reply) { m_reply->disconnect(this); m_reply->abort(); m_reply->deleteLater(); }
}

void MediaFeed::start()
{
    if (m_source.isLocalFile()) {
        m_file.setFileName(m_source.toLocalFile());
        if (!m_file.open(QIODevice::ReadOnly)) { emit failed(QStringLiteral("cannot open file")); return; }
        m_size = m_file.size();
        emit ready(m_size);
        return;
    }
    if (!isStreamServer(m_source)) { emit failed(QStringLiteral("source not allowed")); return; }
    startHttp();
}

void MediaFeed::startHttp()
{
    m_nam = new QNetworkAccessManager(this);
    QNetworkReply *head = m_nam->head(QNetworkRequest(m_source));
    connect(head, &QNetworkReply::finished, this, [this, head] {
        head->deleteLater();
        const qint64 len = head->header(QNetworkRequest::ContentLengthHeader).toLongLong();
        if (head->error() != QNetworkReply::NoError || len <= 0) {
            emit failed(QStringLiteral("stream unavailable"));
            return;
        }
        m_size = len;
        emit ready(m_size);
    });
}

void MediaFeed::request(quint32 id, qint64 offset, qint32 length)
{
    if (m_size < 0 || offset < 0 || offset >= m_size || length <= 0 || length > kMaxRequest
        || m_pending.size() >= kMaxPending) {
        emit refused(id);
        return;
    }
    length = qint32(std::min<qint64>(length, m_size - offset));

    if (m_file.isOpen()) {
        QByteArray bytes;
        if (m_file.seek(offset)) bytes = m_file.read(length);
        if (bytes.isEmpty()) emit refused(id);
        else emit data(id, offset, bytes);
        return;
    }

    m_pending.append({ id, offset, length });
    const qint64 bufEnd = m_bufStart + m_buf.size();
    const bool inWindow = offset >= m_bufStart && offset < bufEnd;
    const bool comingSoon = m_reply && offset >= bufEnd && offset <= bufEnd + kNearAhead;
    if (inWindow || comingSoon) servePending();   // also frees room for what's coming
    else streamFrom(offset);
}

// One open-ended range request at a time, the way a player reads a stream:
// sequential reads ride it, a seek replaces it.
void MediaFeed::streamFrom(qint64 offset)
{
    if (m_reply) { m_reply->disconnect(this); m_reply->abort(); m_reply->deleteLater(); }
    m_buf.clear();
    m_bufStart = offset;
    m_replyChecked = false;
    QNetworkRequest req(m_source);
    req.setRawHeader("Range", "bytes=" + QByteArray::number(offset) + "-");
    m_reply = m_nam->get(req);
    m_reply->setReadBufferSize(kWindow);
    connect(m_reply, &QNetworkReply::readyRead, this, &MediaFeed::onHttpData);
    connect(m_reply, &QNetworkReply::finished, this, &MediaFeed::onHttpData);
}

// A server that ignores Range answers 200 from byte 0: taking that as the
// bytes at `offset` would hand the decoder the wrong file position.
bool MediaFeed::replyStartsAtBuffer()
{
    if (m_replyChecked) return true;
    const int status = m_reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    bool ok = status == 200 && m_bufStart == 0;
    if (status == 206) {
        const QByteArray range = m_reply->rawHeader("Content-Range");   // bytes START-END/TOTAL
        const int dash = range.indexOf('-');
        ok = range.startsWith("bytes ") && dash > 6 && range.mid(6, dash - 6).toLongLong() == m_bufStart;
    }
    if (!ok) {
        m_reply->disconnect(this);
        m_reply->abort();
        m_reply->deleteLater();
        for (const Pending &p : std::as_const(m_pending)) emit refused(p.id);
        m_pending.clear();
        return false;
    }
    m_replyChecked = true;
    return true;
}

void MediaFeed::onHttpData()
{
    if (!m_reply || !replyStartsAtBuffer()) return;
    while (m_buf.size() < kWindow && m_reply->bytesAvailable() > 0)
        m_buf.append(m_reply->read(kWindow - m_buf.size()));
    servePending();
}

void MediaFeed::servePending()
{
    // Finished is not the same as read: the reply can still hold bytes the
    // window had no room for yet.
    const bool drained = !m_reply || (m_reply->isFinished() && m_reply->bytesAvailable() == 0);
    const qint64 bufEnd = m_bufStart + m_buf.size();
    for (int i = 0; i < m_pending.size();) {
        const Pending p = m_pending.at(i);
        if (p.offset >= m_bufStart && p.offset < bufEnd) {
            const qint64 n = std::min<qint64>(p.length, bufEnd - p.offset);
            if (n >= std::min(p.length, kMinAnswer) || drained || p.offset + n >= m_size) {
                m_pending.removeAt(i);
                emit data(p.id, p.offset, m_buf.mid(int(p.offset - m_bufStart), int(n)));
                continue;
            }
        } else if (drained) {
            m_pending.removeAt(i);
            emit refused(p.id);
            continue;
        }
        ++i;
    }
    trimBuffer();
    if (m_reply && m_reply->bytesAvailable() > 0 && m_buf.size() < kWindow)
        QMetaObject::invokeMethod(this, &MediaFeed::onHttpData, Qt::QueuedConnection);
}

void MediaFeed::trimBuffer()
{
    qint64 keepFrom = m_bufStart + m_buf.size() - kKeepBehind;
    for (const Pending &p : m_pending) keepFrom = std::min(keepFrom, p.offset);
    const qint64 drop = std::clamp<qint64>(keepFrom - m_bufStart, 0, m_buf.size());
    if (drop < kWindow / 4) return;   // trimming is a copy: do it in big steps
    m_buf.remove(0, int(drop));
    m_bufStart += drop;
}
