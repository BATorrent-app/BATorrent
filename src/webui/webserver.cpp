// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "webserver.h"
#include "../torrent/sessionmanager.h"
#include <QTcpSocket>
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QCryptographicHash>
#include <sstream>

WebServer::WebServer(SessionManager *session, QObject *parent)
    : QObject(parent), m_session(session)
{
}

WebServer::~WebServer()
{
    stop();
}

bool WebServer::start(quint16 port, bool remoteAccess)
{
    stop();

    m_server = new QTcpServer(this);
    connect(m_server, &QTcpServer::newConnection, this, &WebServer::onNewConnection);

    QHostAddress addr = remoteAccess ? QHostAddress::Any : QHostAddress::LocalHost;
    if (!m_server->listen(addr, port)) {
        delete m_server;
        m_server = nullptr;
        return false;
    }
    return true;
}

void WebServer::stop()
{
    if (m_server) {
        m_server->close();
        delete m_server;
        m_server = nullptr;
    }
}

bool WebServer::isRunning() const
{
    return m_server && m_server->isListening();
}

void WebServer::setCredentials(const QString &user, const QString &passwordHash)
{
    m_user = user;
    m_passwordHash = passwordHash;
}

void WebServer::onNewConnection()
{
    while (m_server->hasPendingConnections()) {
        QTcpSocket *socket = m_server->nextPendingConnection();
        connect(socket, &QTcpSocket::readyRead, this, [this, socket]() {
            handleClient(socket);
        });
        connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
    }
}

void WebServer::handleClient(QTcpSocket *socket)
{
    if (!socket->canReadLine())
        return;

    QByteArray requestData = socket->readAll();
    QString request = QString::fromUtf8(requestData);

    int firstNewline = request.indexOf('\n');
    if (firstNewline < 0) {
        sendError(socket, 400, "Bad Request");
        return;
    }

    QString requestLine = request.left(firstNewline).trimmed();
    QStringList parts = requestLine.split(' ');
    if (parts.size() < 2) {
        sendError(socket, 400, "Bad Request");
        return;
    }

    QString method = parts[0];
    QString path = parts[1];

    if (!m_user.isEmpty() && !checkAuth(requestData)) {
        QByteArray body = "Unauthorized";
        QByteArray response = "HTTP/1.1 401 Unauthorized\r\n"
                              "WWW-Authenticate: Basic realm=\"BATorrent WebUI\"\r\n"
                              "Content-Type: text/plain\r\n"
                              "Content-Length: " + QByteArray::number(body.size()) + "\r\n"
                              "Connection: close\r\n\r\n" + body;
        socket->write(response);
        socket->flush();
        socket->disconnectFromHost();
        return;
    }

    int bodyStart = request.indexOf("\r\n\r\n");
    QByteArray body;
    if (bodyStart >= 0)
        body = requestData.mid(bodyStart + 4);

    if (method == "GET" && path == "/") {
        QFile file(":/webui/index.html");
        if (file.open(QIODevice::ReadOnly)) {
            sendResponse(socket, 200, "OK", file.readAll(), "text/html; charset=utf-8");
        } else {
            sendError(socket, 500, "Could not load WebUI");
        }
    }
    else if (method == "GET" && path == "/api/torrents") {
        sendJson(socket, 200, handleGetTorrents());
    }
    else if (method == "POST" && path == "/api/torrents") {
        if (handleAddTorrent(body))
            sendJson(socket, 200, R"({"status":"ok"})");
        else
            sendError(socket, 400, "Failed to add torrent");
    }
    else if (method == "GET" && path == "/api/status") {
        sendJson(socket, 200, handleGetStatus());
    }
    else if (method == "DELETE" && path.startsWith("/api/torrents/")) {
        QString hash = path.mid(QString("/api/torrents/").length());
        if (handleRemoveTorrent(hash))
            sendJson(socket, 200, R"({"status":"ok"})");
        else
            sendError(socket, 404, "Torrent not found");
    }
    else if (method == "POST" && path.endsWith("/pause")) {
        QString segment = path.mid(QString("/api/torrents/").length());
        segment.chop(QString("/pause").length());
        if (handlePauseTorrent(segment))
            sendJson(socket, 200, R"({"status":"ok"})");
        else
            sendError(socket, 404, "Torrent not found");
    }
    else if (method == "POST" && path.endsWith("/resume")) {
        QString segment = path.mid(QString("/api/torrents/").length());
        segment.chop(QString("/resume").length());
        if (handleResumeTorrent(segment))
            sendJson(socket, 200, R"({"status":"ok"})");
        else
            sendError(socket, 404, "Torrent not found");
    }
    else if (method == "GET" && path.contains("/peers")) {
        QString segment = path.mid(QString("/api/torrents/").length());
        segment.chop(QString("/peers").length());
        sendJson(socket, 200, handleGetTorrentPeers(segment));
    }
    else if (method == "GET" && path.contains("/files")) {
        QString segment = path.mid(QString("/api/torrents/").length());
        segment.chop(QString("/files").length());
        sendJson(socket, 200, handleGetTorrentFiles(segment));
    }
    else {
        sendError(socket, 404, "Not Found");
    }
}

void WebServer::sendResponse(QTcpSocket *socket, int status, const QString &statusText,
                              const QByteArray &body, const QByteArray &contentType)
{
    QByteArray response = "HTTP/1.1 " + QByteArray::number(status) + " " + statusText.toUtf8() + "\r\n"
                          "Content-Type: " + contentType + "\r\n"
                          "Content-Length: " + QByteArray::number(body.size()) + "\r\n"
                          "Connection: close\r\n\r\n" + body;
    socket->write(response);
    socket->flush();
    socket->disconnectFromHost();
}

void WebServer::sendJson(QTcpSocket *socket, int status, const QByteArray &json)
{
    sendResponse(socket, status, status == 200 ? "OK" : "Error", json, "application/json");
}

void WebServer::sendError(QTcpSocket *socket, int status, const QString &message)
{
    QJsonObject obj;
    obj["error"] = message;
    sendJson(socket, status, QJsonDocument(obj).toJson(QJsonDocument::Compact));
}

bool WebServer::checkAuth(const QByteArray &header)
{
    int idx = header.indexOf("Authorization: Basic ");
    if (idx < 0) return false;

    int start = idx + 21;
    int end = header.indexOf('\r', start);
    if (end < 0) end = header.indexOf('\n', start);
    if (end < 0) return false;

    QByteArray encoded = header.mid(start, end - start).trimmed();
    QByteArray decoded = QByteArray::fromBase64(encoded);
    int colon = decoded.indexOf(':');
    if (colon < 0) return false;

    QString user = QString::fromUtf8(decoded.left(colon));
    QString pass = QString::fromUtf8(decoded.mid(colon + 1));
    QString passHash = QString::fromUtf8(
        QCryptographicHash::hash(pass.toUtf8(), QCryptographicHash::Sha256).toHex());

    return user == m_user && passHash == m_passwordHash;
}

QString WebServer::torrentHash(int index)
{
    if (index < 0 || index >= m_session->torrentCount())
        return {};
    TorrentInfo info = m_session->torrentAt(index);
    if (!info.handle.is_valid())
        return {};
    lt::torrent_status st = info.handle.status();
    return QString::fromStdString(
        (std::ostringstream() << st.info_hashes.get_best()).str());
}

int WebServer::findTorrentByHash(const QString &hash)
{
    for (int i = 0; i < m_session->torrentCount(); ++i) {
        if (torrentHash(i) == hash)
            return i;
    }
    return -1;
}

QByteArray WebServer::handleGetTorrents()
{
    QJsonArray arr;
    for (int i = 0; i < m_session->torrentCount(); ++i) {
        TorrentInfo info = m_session->torrentAt(i);
        QJsonObject obj;
        obj["hash"] = torrentHash(i);
        obj["name"] = info.name;
        obj["size"] = info.totalSize;
        obj["done"] = info.totalDone;
        obj["progress"] = static_cast<double>(info.progress);
        obj["downloadRate"] = info.downloadRate;
        obj["uploadRate"] = info.uploadRate;
        obj["peers"] = info.numPeers;
        obj["seeds"] = info.numSeeds;
        obj["state"] = info.stateString;
        obj["paused"] = info.paused;
        obj["ratio"] = static_cast<double>(info.ratio);
        obj["savePath"] = info.savePath;
        arr.append(obj);
    }
    return QJsonDocument(arr).toJson(QJsonDocument::Compact);
}

QByteArray WebServer::handleGetTorrentPeers(const QString &hash)
{
    int idx = findTorrentByHash(hash);
    if (idx < 0) return "[]";

    auto peers = m_session->peersAt(idx);
    QJsonArray arr;
    for (const auto &p : peers) {
        QJsonObject obj;
        obj["ip"] = p.ip;
        obj["port"] = p.port;
        obj["downloadRate"] = p.downloadRate;
        obj["uploadRate"] = p.uploadRate;
        obj["progress"] = static_cast<double>(p.progress);
        obj["client"] = p.client;
        arr.append(obj);
    }
    return QJsonDocument(arr).toJson(QJsonDocument::Compact);
}

QByteArray WebServer::handleGetTorrentFiles(const QString &hash)
{
    int idx = findTorrentByHash(hash);
    if (idx < 0) return "[]";

    auto files = m_session->filesAt(idx);
    QJsonArray arr;
    for (const auto &f : files) {
        QJsonObject obj;
        obj["path"] = f.path;
        obj["size"] = f.size;
        obj["progress"] = static_cast<double>(f.progress);
        obj["priority"] = f.priority;
        arr.append(obj);
    }
    return QJsonDocument(arr).toJson(QJsonDocument::Compact);
}

QByteArray WebServer::handleGetStatus()
{
    QJsonObject obj;
    int count = m_session->torrentCount();
    int totalDown = 0, totalUp = 0;
    for (int i = 0; i < count; ++i) {
        TorrentInfo info = m_session->torrentAt(i);
        totalDown += info.downloadRate;
        totalUp += info.uploadRate;
    }
    obj["torrentCount"] = count;
    obj["downloadRate"] = totalDown;
    obj["uploadRate"] = totalUp;
    obj["downloadLimit"] = m_session->downloadLimit();
    obj["uploadLimit"] = m_session->uploadLimit();
    return QJsonDocument(obj).toJson(QJsonDocument::Compact);
}

bool WebServer::handleAddTorrent(const QByteArray &body)
{
    QJsonDocument doc = QJsonDocument::fromJson(body);
    if (!doc.isObject()) return false;

    QJsonObject obj = doc.object();
    QString magnet = obj.value("magnet").toString();
    QString savePath = obj.value("savePath").toString();

    if (magnet.isEmpty() || !magnet.startsWith("magnet:"))
        return false;

    if (savePath.isEmpty()) {
        if (m_session->torrentCount() > 0)
            savePath = m_session->torrentAt(0).savePath;
        else
            savePath = QDir::homePath() + "/Downloads";
    }

    m_session->addMagnet(magnet, savePath);
    return true;
}

bool WebServer::handleRemoveTorrent(const QString &hash)
{
    int idx = findTorrentByHash(hash);
    if (idx < 0) return false;
    m_session->removeTorrent(idx, false);
    return true;
}

bool WebServer::handlePauseTorrent(const QString &hash)
{
    int idx = findTorrentByHash(hash);
    if (idx < 0) return false;
    m_session->pauseTorrent(idx);
    return true;
}

bool WebServer::handleResumeTorrent(const QString &hash)
{
    int idx = findTorrentByHash(hash);
    if (idx < 0) return false;
    m_session->resumeTorrent(idx);
    return true;
}
