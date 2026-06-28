// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "enginehost.h"
#include "ipcprotocol.h"
#include "../torrent/sessionmanager.h"
#include <QLocalServer>
#include <QLocalSocket>
#include <QDataStream>
#include <QDebug>

EngineHost::EngineHost(SessionManager *session, const QString &serverName, QObject *parent)
    : QObject(parent), m_session(session), m_serverName(serverName), m_server(new QLocalServer(this))
{
    connect(m_server, &QLocalServer::newConnection, this, &EngineHost::onNewConnection);
}

bool EngineHost::listen()
{
    QLocalServer::removeServer(m_serverName);   // clear a stale socket from a previous crash
    if (!m_server->listen(m_serverName)) {
        qWarning() << "[engine] failed to listen on" << m_serverName << m_server->errorString();
        return false;
    }
    qInfo() << "[engine] listening on" << m_serverName;
    return true;
}

void EngineHost::onNewConnection()
{
    QLocalSocket *sock = m_server->nextPendingConnection();
    if (!sock) return;
    // One UI at a time; a reconnect (after a UI restart) replaces the old socket.
    if (m_client) m_client->deleteLater();
    m_client = sock;
    m_buf.clear();
    connect(sock, &QLocalSocket::readyRead, this, &EngineHost::onReadyRead);
    connect(sock, &QLocalSocket::disconnected, sock, &QLocalSocket::deleteLater);
    ipc::writeFrame(sock, ipc::Kind::Hello, {});
}

void EngineHost::onReadyRead()
{
    if (!m_client) return;
    m_buf.append(m_client->readAll());
    ipc::drainFrames(m_buf, [this](ipc::Kind kind, const QByteArray &payload) {
        if (kind == ipc::Kind::Ping) {
            ipc::writeFrame(m_client, ipc::Kind::Pong, {});
            return;
        }
        if (kind == ipc::Kind::Request) {
            QDataStream in(payload);
            in.setVersion(ipc::kStreamVersion);
            quint32 id = 0; QString method; QByteArray args;
            in >> id >> method >> args;
            const QByteArray result = dispatch(method, args);
            QByteArray reply;
            QDataStream out(&reply, QIODevice::WriteOnly);
            out.setVersion(ipc::kStreamVersion);
            out << id << result;
            ipc::writeFrame(m_client, ipc::Kind::Reply, reply);
        }
    });
}

// Representative slice of the IEngine surface. The remaining methods follow the
// same pattern (decode args → call m_session → encode result) and land next.
QByteArray EngineHost::dispatch(const QString &method, const QByteArray &args)
{
    QByteArray result;
    QDataStream out(&result, QIODevice::WriteOnly);
    out.setVersion(ipc::kStreamVersion);

    QDataStream in(args);
    in.setVersion(ipc::kStreamVersion);

    if (method == QLatin1String("torrentCount")) {
        out << qint32(m_session->torrentCount());
    } else if (method == QLatin1String("pauseAll")) {
        m_session->pauseAll();
    } else if (method == QLatin1String("resumeAll")) {
        m_session->resumeAll();
    } else if (method == QLatin1String("addMagnet")) {
        QString uri, savePath, coverHint; qint32 coverType = -1;
        in >> uri >> savePath >> coverHint >> coverType;
        m_session->addMagnet(uri, savePath, coverHint, coverType);
    } else {
        qWarning() << "[engine] unhandled method" << method;
    }
    return result;
}
