// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef BATORRENT_SINGLEINSTANCE_H
#define BATORRENT_SINGLEINSTANCE_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <memory>

class QLocalServer;
class QmlSessionBridge;

// Local-socket single-instance claim + torrent/magnet forward to the live session.
class SingleInstance
{
public:
    static QString serverName();
    static QString collectTorrentArgs(const QStringList &args);
    static bool forwardToRunning(const QString &message);

    // Claim the socket before SessionManager/resume so a second launch during
    // boot forwards here instead of fighting the same resume dir.
    void claim(QObject *parent);

    // One routing rule for every way a torrent reaches us: argv, the
    // single-instance socket, and macOS's open-file event. Queues when the
    // bridge is not up yet — macOS can deliver a double-click before exec().
    void deliver(const QString &line);

    void setSessionBridge(QmlSessionBridge *bridge);
    void setRootWindow(QObject *root);
    void flushPending();

private:
    QLocalServer *m_server = nullptr;
    std::shared_ptr<QmlSessionBridge *> m_bridge;
    std::shared_ptr<QObject *> m_root;
    std::shared_ptr<QStringList> m_pending;
};

#endif
