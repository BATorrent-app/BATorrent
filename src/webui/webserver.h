// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <QObject>
#include <QTcpServer>

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
    void handleClient(QTcpSocket *socket);
    void sendResponse(QTcpSocket *socket, int status, const QString &statusText,
                      const QByteArray &body, const QByteArray &contentType);
    void sendJson(QTcpSocket *socket, int status, const QByteArray &json);
    void sendError(QTcpSocket *socket, int status, const QString &message);
    bool checkAuth(const QByteArray &header);

    QByteArray handleGetTorrents();
    QByteArray handleGetTorrentPeers(const QString &hash);
    QByteArray handleGetTorrentFiles(const QString &hash);
    QByteArray handleGetStatus();
    bool handleAddTorrent(const QByteArray &body);
    bool handleUploadTorrent(const QByteArray &requestData);
    bool handleRemoveTorrent(const QString &hash);
    bool handlePauseTorrent(const QString &hash);
    bool handleResumeTorrent(const QString &hash);

    int findTorrentByHash(const QString &hash);
    QString torrentHash(int index);

    QTcpServer *m_server = nullptr;
    SessionManager *m_session;
    QString m_user;
    QString m_passwordHash;
};

#endif
