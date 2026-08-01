// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "app/singleinstance.h"

#include "bridges/session/qmlsessionbridge.h"

#include <QLocalServer>
#include <QLocalSocket>
#include <QWindow>

QString SingleInstance::serverName()
{
    return QStringLiteral("BATorrent-SingleInstance");
}

QString SingleInstance::collectTorrentArgs(const QStringList &args)
{
    QStringList relevant;
    for (int i = 1; i < args.size(); ++i) {
        const QString &a = args[i];
        if (a.endsWith(".torrent") || a.startsWith("magnet:") || a.startsWith("bittorrent:"))
            relevant << a;
    }
    return relevant.join('\n');
}

bool SingleInstance::forwardToRunning(const QString &message)
{
    QLocalSocket socket;
    socket.connectToServer(serverName());
    if (!socket.waitForConnected(1000))
        return false;
    socket.write(message.toUtf8());
    socket.waitForBytesWritten(1000);
    socket.disconnectFromServer();
    return true;
}

void SingleInstance::claim(QObject *parent)
{
    m_bridge = std::make_shared<QmlSessionBridge *>(nullptr);
    m_root = std::make_shared<QObject *>(nullptr);
    m_pending = std::make_shared<QStringList>();

    QLocalServer::removeServer(serverName());
    m_server = new QLocalServer(parent);
    if (!m_server->listen(serverName()))
        qWarning() << "[instance] listen failed:" << m_server->errorString();

    QObject::connect(m_server, &QLocalServer::newConnection, parent,
                     [this]() {
        QLocalSocket *client = m_server->nextPendingConnection();
        if (!client) return;
        if (auto *w = qobject_cast<QWindow *>(*m_root)) {
            if (w->visibility() == QWindow::Hidden) w->show();
            else if (w->windowStates() & Qt::WindowMinimized)
                w->setWindowStates(w->windowStates() & ~Qt::WindowMinimized);
            w->raise();
            w->requestActivate();
        }
        QObject::connect(client, &QLocalSocket::readyRead, client,
                         [this, client]() {
            const QStringList lines = QString::fromUtf8(client->readAll())
                                          .split('\n', Qt::SkipEmptyParts);
            for (const QString &line : lines)
                deliver(line);
            client->deleteLater();
        });
    });
}

void SingleInstance::setSessionBridge(QmlSessionBridge *bridge)
{
    if (m_bridge) *m_bridge = bridge;
}

void SingleInstance::setRootWindow(QObject *root)
{
    if (m_root) *m_root = root;
}

namespace {
void route(QmlSessionBridge *bridge, const QString &line)
{
    if (line.endsWith(QLatin1String(".torrent"))) bridge->requestAddTorrentFile(line);
    else if (line.startsWith(QLatin1String("magnet:"))
             || line.startsWith(QLatin1String("bittorrent:")))
        bridge->addMagnetUri(line);
}
} // namespace

void SingleInstance::deliver(const QString &line)
{
    if (line.isEmpty()) return;
    if (m_bridge && *m_bridge) route(*m_bridge, line);
    else if (m_pending)        *m_pending << line;
}

void SingleInstance::flushPending()
{
    if (!m_pending || !m_bridge || !*m_bridge) return;
    QmlSessionBridge *bridge = *m_bridge;
    for (const QString &line : *m_pending)
        route(bridge, line);
    m_pending->clear();
}
