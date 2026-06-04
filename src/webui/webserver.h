// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <QObject>
#include <QTcpServer>
#include <QByteArray>
#include <QDateTime>
#include <QHash>
#include <QSet>
#include <QString>

class SessionManager;
class QTcpSocket;

class WebServer : public QObject
{
    Q_OBJECT
public:
    explicit WebServer(SessionManager *session, QObject *parent = nullptr);
    ~WebServer();

    bool start(quint16 port, bool remoteAccess);
    void stop();
    bool isRunning() const;

    void setCredentials(const QString &user, const QString &passwordHash);

private slots:
    void onNewConnection();

private:
    struct PendingRequest {
        QByteArray buffer;
        int contentLength = -1;     // -1 until parsed
        int headerEnd = -1;         // index of body start; -1 until parsed
        bool headersParsed = false;
        // Gate further reads after a request has been handed to dispatch.
        // Without this, trailing TCP bytes can re-enter readMore and run the
        // handler twice (request smuggling / duplicate side effects).
        bool dispatched = false;
    };

    struct FailedAuthState {
        int attempts = 0;
        QDateTime lockedUntil;
    };

    void readMore(QTcpSocket *socket);
    void dispatch(QTcpSocket *socket, const QByteArray &requestData);

    void sendResponse(QTcpSocket *socket, int status, const QString &statusText,
                      const QByteArray &body, const QByteArray &contentType,
                      const QByteArray &extraHeaders = {});
    void sendJson(QTcpSocket *socket, int status, const QByteArray &json,
                  const QByteArray &extraHeaders = {});
    void sendError(QTcpSocket *socket, int status, const QString &message);
    bool checkAuth(const QByteArray &headersOnly, const QString &clientIp);
    bool checkSession(const QByteArray &headersOnly);
    QByteArray issueSessionCookie();
    static QByteArray headerValue(const QByteArray &headersOnly, const QByteArray &name);
    static bool constantTimeEquals(const QByteArray &a, const QByteArray &b);

    QByteArray handleGetTorrents();
    QByteArray handleGetTorrentPeers(const QString &hash);
    QByteArray handleGetTorrentFiles(const QString &hash);
    QByteArray handleGetStatus();
    bool handleAddTorrent(const QByteArray &body);
    bool handleUploadTorrent(const QByteArray &requestData, const QByteArray &boundary);
    bool handleRemoveTorrent(const QString &hash);
    bool handlePauseTorrent(const QString &hash);
    bool handleResumeTorrent(const QString &hash);

    int findTorrentByHash(const QString &hash);
    QString torrentHash(int index);

    QTcpServer *m_server = nullptr;
    SessionManager *m_session;
    QString m_user;
    QString m_passwordHash;

    // Per-socket request accumulation so handlers see complete bodies even
    // when the kernel delivers the HTTP request across multiple reads (large
    // .torrent uploads especially).
    QHash<QTcpSocket *, PendingRequest> m_pending;

    // Active session tokens with their issuance time (epoch seconds). Tokens
    // expire after kSessionMaxAgeSec; checkSession evicts expired entries.
    QHash<QByteArray, qint64> m_sessionTokens;
    static constexpr qint64 kSessionMaxAgeSec = 24 * 60 * 60; // 24h sliding cap

    // Failed-auth tracking per IP for login throttling. Capped + evicted to
    // defeat IP-spoofed memory-exhaustion attacks.
    QHash<QString, FailedAuthState> m_failedAuth;
    void evictExpiredFailedAuth();
    static constexpr int kFailedAuthMax = 4096;
};

#endif
