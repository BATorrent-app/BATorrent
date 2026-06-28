// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Engine-side IPC host: runs in the `--engine` child process, owns the real
// SessionManager, and serves the IEngine surface over a QLocalServer. The UI's
// IpcEngine is the client. See internal/ENGINE_SPLIT_PLAN.md.
#ifndef BATORRENT_ENGINEHOST_H
#define BATORRENT_ENGINEHOST_H

#include <QObject>
#include <QByteArray>

class SessionManager;
class QLocalServer;
class QLocalSocket;

class EngineHost : public QObject
{
    Q_OBJECT
public:
    EngineHost(SessionManager *session, const QString &serverName, QObject *parent = nullptr);
    bool listen();

private slots:
    void onNewConnection();
    void onReadyRead();

private:
    QByteArray dispatch(const QString &method, const QByteArray &args);   // → resultBlob

    SessionManager *m_session;
    QString m_serverName;
    QLocalServer *m_server;
    QLocalSocket *m_client = nullptr;   // single UI client
    QByteArray m_buf;
};

#endif
