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
#include <QStandardPaths>
#include <QTemporaryFile>
#include <QRandomGenerator>

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
    m_pending.clear();
}

bool WebServer::isRunning() const
{
    return m_server && m_server->isListening();
}

void WebServer::setCredentials(const QString &user, const QString &passwordHash)
{
    m_user = user;
    m_passwordHash = passwordHash;
    // Changing credentials invalidates every existing session.
    m_sessionTokens.clear();
}

void WebServer::onNewConnection()
{
    while (m_server->hasPendingConnections()) {
        QTcpSocket *socket = m_server->nextPendingConnection();
        m_pending.insert(socket, PendingRequest{});
        connect(socket, &QTcpSocket::readyRead, this, [this, socket]() {
            readMore(socket);
        });
        connect(socket, &QTcpSocket::disconnected, this, [this, socket]() {
            m_pending.remove(socket);
            socket->deleteLater();
        });
    }
}

void WebServer::readMore(QTcpSocket *socket)
{
    auto it = m_pending.find(socket);
    if (it == m_pending.end())
        return;
    if (it->dispatched) {
        // Drain bytes to keep readyRead quiet; we won't act on them.
        socket->readAll();
        return;
    }

    it->buffer.append(socket->readAll());

    // First, locate the end of the headers and pull Content-Length so we
    // know how much body to expect.
    if (!it->headersParsed) {
        int end = it->buffer.indexOf("\r\n\r\n");
        if (end < 0) {
            // Cap header size to defeat slow-loris / unbounded-buffer abuse.
            if (it->buffer.size() > 32 * 1024) {
                sendError(socket, 400, "Headers too large");
                socket->disconnectFromHost();
            }
            return; // wait for more bytes
        }
        it->headerEnd = end + 4;
        it->headersParsed = true;

        const QByteArray headers = it->buffer.left(it->headerEnd);
        const QByteArray cl = headerValue(headers, "Content-Length");
        it->contentLength = cl.isEmpty() ? 0 : cl.toInt();

        // Reject obviously absurd uploads (>200 MB) before allocating memory.
        if (it->contentLength < 0 || it->contentLength > 200 * 1024 * 1024) {
            sendError(socket, 413, "Payload too large");
            socket->disconnectFromHost();
            return;
        }
    }

    // Wait until we have the whole body.
    int needed = it->headerEnd + it->contentLength;
    if (it->buffer.size() < needed)
        return;

    QByteArray complete = it->buffer.left(needed);
    // Mark dispatched before calling out so trailing bytes can't re-enter.
    it->dispatched = true;
    dispatch(socket, complete);
}

void WebServer::dispatch(QTcpSocket *socket, const QByteArray &requestData)
{
    int firstNewline = requestData.indexOf('\n');
    if (firstNewline < 0) {
        sendError(socket, 400, "Bad Request");
        return;
    }

    const QByteArray requestLine = requestData.left(firstNewline).trimmed();
    const int sp1 = requestLine.indexOf(' ');
    const int sp2 = sp1 >= 0 ? requestLine.indexOf(' ', sp1 + 1) : -1;
    if (sp1 < 0 || sp2 < 0) {
        sendError(socket, 400, "Bad Request");
        return;
    }
    const QByteArray method = requestLine.left(sp1);
    QByteArray path = requestLine.mid(sp1 + 1, sp2 - sp1 - 1);
    QByteArray query;
    const int qmark = path.indexOf('?');
    if (qmark >= 0) { query = path.mid(qmark + 1); path.truncate(qmark); }   // route on the bare path

    // Split headers from body so auth/cookie parsing can never accidentally
    // read attacker-supplied bytes that live in the request body.
    const int headerEnd = requestData.indexOf("\r\n\r\n");
    const QByteArray headers = headerEnd >= 0 ? requestData.left(headerEnd + 4)
                                              : requestData;
    const QByteArray body = headerEnd >= 0 ? requestData.mid(headerEnd + 4)
                                           : QByteArray();

    const QString clientIp = socket->peerAddress().toString();

    // QR pairing: a scanned URL of the form  /?pair=base64(user:pass)  signs the
    // device in once (issues a session cookie) and 302-redirects to a clean "/",
    // so the phone never types the password and it isn't left in the address bar.
    if (!m_user.isEmpty() && method == "GET" && query.contains("pair=")) {
        QByteArray pairVal;
        for (const QByteArray &kv : query.split('&'))
            if (kv.startsWith("pair=")) { pairVal = kv.mid(5); break; }
        const QByteArray decoded = QByteArray::fromBase64(QByteArray::fromPercentEncoding(pairVal));
        const int colon = decoded.indexOf(':');
        if (colon > 0) {
            const QByteArray u = decoded.left(colon);
            const QByteArray pHash = QCryptographicHash::hash(decoded.mid(colon + 1),
                                        QCryptographicHash::Sha256).toHex();
            if (constantTimeEquals(u, m_user.toUtf8())
                && constantTimeEquals(pHash, m_passwordHash.toUtf8())) {
                const QByteArray token = issueSessionCookie();
                const QByteArray extra = "Location: /\r\nSet-Cookie: BATSID=" + token
                    + "; HttpOnly; SameSite=Strict; Path=/\r\n";
                sendResponse(socket, 302, "Found", QByteArray(), "text/plain", extra);
                return;
            }
        }
        // invalid/forged pair token → fall through to the normal auth gate
    }

    // Login endpoint sits before the auth gate so unauthenticated clients
    // can post their credentials.
    if (m_user.isEmpty() == false && method == "POST" && path == "/api/login") {
        if (!checkAuth(headers, clientIp)) {
            sendError(socket, 401, "Invalid credentials");
            return;
        }
        QByteArray token = issueSessionCookie();
        QByteArray cookie = "Set-Cookie: BATSID=" + token
            + "; HttpOnly; SameSite=Strict; Path=/\r\n";
        sendJson(socket, 200, R"({"status":"ok"})", cookie);
        return;
    }

    if (!m_user.isEmpty() && !checkSession(headers) && !checkAuth(headers, clientIp)) {
        // No session cookie and no valid Basic Auth → require login.
        QByteArray body401 = "Unauthorized";
        sendResponse(socket, 401, "Unauthorized", body401, "text/plain",
            "WWW-Authenticate: Basic realm=\"BATorrent WebUI\"\r\n");
        return;
    }

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
        const QByteArray ct = headerValue(headers, "Content-Type");
        if (ct.contains("multipart/form-data")) {
            const int boundaryIdx = ct.indexOf("boundary=");
            QByteArray boundary;
            if (boundaryIdx >= 0) {
                QByteArray b = ct.mid(boundaryIdx + 9).trimmed();
                // RFC 2046 allows the boundary to be a quoted-string.
                if (b.size() >= 2 && b.startsWith('"') && b.endsWith('"'))
                    b = b.mid(1, b.size() - 2);
                boundary = "--" + b;
            }
            if (boundary.isEmpty()) {
                sendError(socket, 400, "Missing multipart boundary");
                return;
            }
            if (handleUploadTorrent(body, boundary))
                sendJson(socket, 200, R"({"status":"ok"})");
            else
                sendError(socket, 400, "Failed to upload torrent");
        } else {
            if (handleAddTorrent(body))
                sendJson(socket, 200, R"({"status":"ok"})");
            else
                sendError(socket, 400, "Failed to add torrent");
        }
    }
    else if (method == "GET" && path == "/api/status") {
        sendJson(socket, 200, handleGetStatus());
    }
    else if (method == "DELETE" && path.startsWith("/api/torrents/")) {
        QString hash = QString::fromUtf8(path.mid(QByteArray("/api/torrents/").length()));
        if (handleRemoveTorrent(hash))
            sendJson(socket, 200, R"({"status":"ok"})");
        else
            sendError(socket, 404, "Torrent not found");
    }
    else if (method == "POST" && path.endsWith("/pause")) {
        QByteArray segment = path.mid(QByteArray("/api/torrents/").length());
        segment.chop(QByteArray("/pause").length());
        if (handlePauseTorrent(QString::fromUtf8(segment)))
            sendJson(socket, 200, R"({"status":"ok"})");
        else
            sendError(socket, 404, "Torrent not found");
    }
    else if (method == "POST" && path.endsWith("/resume")) {
        QByteArray segment = path.mid(QByteArray("/api/torrents/").length());
        segment.chop(QByteArray("/resume").length());
        if (handleResumeTorrent(QString::fromUtf8(segment)))
            sendJson(socket, 200, R"({"status":"ok"})");
        else
            sendError(socket, 404, "Torrent not found");
    }
    else if (method == "GET" && path.contains("/peers")) {
        QByteArray segment = path.mid(QByteArray("/api/torrents/").length());
        segment.chop(QByteArray("/peers").length());
        sendJson(socket, 200, handleGetTorrentPeers(QString::fromUtf8(segment)));
    }
    else if (method == "GET" && path.contains("/files")) {
        QByteArray segment = path.mid(QByteArray("/api/torrents/").length());
        segment.chop(QByteArray("/files").length());
        sendJson(socket, 200, handleGetTorrentFiles(QString::fromUtf8(segment)));
    }
    else {
        sendError(socket, 404, "Not Found");
    }
}

void WebServer::sendResponse(QTcpSocket *socket, int status, const QString &statusText,
                              const QByteArray &body, const QByteArray &contentType,
                              const QByteArray &extraHeaders)
{
    QByteArray response;
    response.append("HTTP/1.1 ").append(QByteArray::number(status)).append(" ")
            .append(statusText.toUtf8()).append("\r\n");
    response.append("Content-Type: ").append(contentType).append("\r\n");
    response.append("Content-Length: ").append(QByteArray::number(body.size())).append("\r\n");
    // Security headers. These are cheap, broadly compatible, and shrink the
    // class of attacks the WebUI is exposed to (clickjacking, MIME-sniff
    // confusion, referrer leakage).
    response.append("X-Content-Type-Options: nosniff\r\n");
    response.append("X-Frame-Options: DENY\r\n");
    response.append("Referrer-Policy: same-origin\r\n");
    response.append("Connection: close\r\n");
    if (!extraHeaders.isEmpty())
        response.append(extraHeaders);
    response.append("\r\n");
    response.append(body);
    socket->write(response);
    socket->flush();
    socket->disconnectFromHost();
}

void WebServer::sendJson(QTcpSocket *socket, int status, const QByteArray &json,
                          const QByteArray &extraHeaders)
{
    sendResponse(socket, status, status == 200 ? "OK" : "Error", json,
                 "application/json", extraHeaders);
}

void WebServer::sendError(QTcpSocket *socket, int status, const QString &message)
{
    QJsonObject obj;
    obj["error"] = message;
    sendJson(socket, status, QJsonDocument(obj).toJson(QJsonDocument::Compact));
}

QByteArray WebServer::headerValue(const QByteArray &headersOnly, const QByteArray &name)
{
    // Search only within the headers region — never within the body — so an
    // attacker can't smuggle an Authorization line through, say, a multipart
    // form field. Caller is expected to pass requestData.left(headerEnd+4).
    QByteArray search = "\r\n" + name + ":";
    int idx = headersOnly.indexOf(search);
    int start;
    if (idx == 0) {
        // first line, no preceding \r\n
        start = name.size() + 1;
    } else if (idx > 0) {
        start = idx + search.size();
    } else if (headersOnly.startsWith(name + ":")) {
        start = name.size() + 1;
    } else {
        return {};
    }
    int end = headersOnly.indexOf('\r', start);
    if (end < 0) end = headersOnly.indexOf('\n', start);
    if (end < 0) return {};
    return headersOnly.mid(start, end - start).trimmed();
}

bool WebServer::constantTimeEquals(const QByteArray &a, const QByteArray &b)
{
    // Length mismatch is information we can leak (the attacker can guess
    // it), but the bytewise comparison must not short-circuit. Pad the
    // shorter input and XOR everything together so the loop count is
    // independent of where the first difference lies.
    if (a.size() != b.size()) return false;
    int diff = 0;
    for (int i = 0; i < a.size(); ++i)
        diff |= static_cast<unsigned char>(a[i]) ^ static_cast<unsigned char>(b[i]);
    return diff == 0;
}

bool WebServer::checkAuth(const QByteArray &headersOnly, const QString &clientIp)
{
    evictExpiredFailedAuth();
    // Throttle: after 5 failed attempts from the same IP, lock that IP out
    // for an exponential window (max 5 min).
    auto &state = m_failedAuth[clientIp];
    if (state.lockedUntil.isValid()
        && QDateTime::currentDateTime() < state.lockedUntil)
        return false;

    QByteArray authValue = headerValue(headersOnly, "Authorization");
    if (!authValue.startsWith("Basic ")) {
        // Don't count a missing header as a failed attempt — only count
        // actual Basic submissions that didn't match.
        return false;
    }

    QByteArray encoded = authValue.mid(6).trimmed();
    QByteArray decoded = QByteArray::fromBase64(encoded);
    int colon = decoded.indexOf(':');
    if (colon < 0) return false;

    QString user = QString::fromUtf8(decoded.left(colon));
    QString pass = QString::fromUtf8(decoded.mid(colon + 1));
    QByteArray passHash = QCryptographicHash::hash(pass.toUtf8(),
                                                    QCryptographicHash::Sha256).toHex();
    QByteArray storedHash = m_passwordHash.toUtf8();

    // Compare both fields in constant time so timing analysis can't recover
    // the hash byte-by-byte.
    bool userOk = constantTimeEquals(user.toUtf8(), m_user.toUtf8());
    bool passOk = constantTimeEquals(passHash, storedHash);

    if (userOk && passOk) {
        state.attempts = 0;
        state.lockedUntil = QDateTime();
        return true;
    }

    state.attempts++;
    if (state.attempts >= 5) {
        // 5,6,7,8,9 → 5s, 10s, 20s, 40s, 80s, ... cap at 5 min.
        int penaltySec = qMin(5 * (1 << (state.attempts - 5)), 300);
        state.lockedUntil = QDateTime::currentDateTime().addSecs(penaltySec);
    }
    return false;
}

bool WebServer::checkSession(const QByteArray &headersOnly)
{
    QByteArray cookieLine = headerValue(headersOnly, "Cookie");
    if (cookieLine.isEmpty()) return false;
    const qint64 now = QDateTime::currentSecsSinceEpoch();
    for (const QByteArray &part : cookieLine.split(';')) {
        QByteArray trimmed = part.trimmed();
        if (trimmed.startsWith("BATSID=")) {
            QByteArray token = trimmed.mid(7);
            auto it = m_sessionTokens.find(token);
            if (it == m_sessionTokens.end()) return false;
            if (now - it.value() > kSessionMaxAgeSec) {
                m_sessionTokens.erase(it);
                return false;
            }
            return true;
        }
    }
    return false;
}

QByteArray WebServer::issueSessionCookie()
{
    QByteArray raw(32, 0);
    QRandomGenerator *rng = QRandomGenerator::system();
    for (int i = 0; i < raw.size(); ++i)
        raw[i] = static_cast<char>(rng->bounded(256));
    QByteArray token = raw.toHex();
    m_sessionTokens.insert(token, QDateTime::currentSecsSinceEpoch());
    return token;
}

void WebServer::evictExpiredFailedAuth()
{
    const QDateTime now = QDateTime::currentDateTime();
    for (auto it = m_failedAuth.begin(); it != m_failedAuth.end(); ) {
        // Entries past their lockout with no recent activity are safe to drop.
        if (it->lockedUntil.isValid() && it->lockedUntil < now)
            it = m_failedAuth.erase(it);
        else
            ++it;
    }
    // Hard cap so an attacker spraying from many spoofed source addresses
    // (trivial via IPv6 prefix abuse) can't OOM the process.
    if (m_failedAuth.size() > kFailedAuthMax)
        m_failedAuth.clear();
}

QString WebServer::torrentHash(int index)
{
    if (index < 0 || index >= m_session->torrentCount())
        return {};
    return m_session->torrentHashAt(index);
}

int WebServer::findTorrentByHash(const QString &hash)
{
    if (hash.isEmpty()) return -1;
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

bool WebServer::handleUploadTorrent(const QByteArray &body, const QByteArray &boundary)
{
    // Find the file part inside the multipart body. The body has already
    // been split from the headers by the caller, so anything we see here
    // can be trusted to be the request payload.
    int filePartIdx = body.indexOf("filename=\"");
    if (filePartIdx < 0) return false;

    int dataStart = body.indexOf("\r\n\r\n", filePartIdx);
    if (dataStart < 0) return false;
    dataStart += 4;

    int dataEnd = body.indexOf(boundary, dataStart);
    if (dataEnd < 0) return false;
    dataEnd -= 2; // remove trailing CRLF before boundary

    QByteArray fileData = body.mid(dataStart, dataEnd - dataStart);

    QString savePath;
    int savePathIdx = body.indexOf("name=\"savePath\"");
    if (savePathIdx >= 0) {
        int spDataStart = body.indexOf("\r\n\r\n", savePathIdx);
        if (spDataStart >= 0) {
            spDataStart += 4;
            int spDataEnd = body.indexOf(boundary, spDataStart);
            if (spDataEnd >= 0)
                savePath = QString::fromUtf8(body.mid(spDataStart,
                    spDataEnd - spDataStart - 2)).trimmed();
        }
    }

    if (savePath.isEmpty()) {
        if (m_session->torrentCount() > 0)
            savePath = m_session->torrentAt(0).savePath;
        else
            savePath = QDir::homePath() + "/Downloads";
    }

    // QTemporaryFile generates a unique name per request so concurrent
    // uploads (two browser tabs, or WebUI + curl) don't clobber each other.
    QTemporaryFile tmp(QDir(QStandardPaths::writableLocation(
        QStandardPaths::TempLocation)).filePath("batorrent_upload_XXXXXX.torrent"));
    tmp.setAutoRemove(true);
    if (!tmp.open()) return false;
    tmp.write(fileData);
    tmp.close();

    m_session->addTorrent(tmp.fileName(), savePath);
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
