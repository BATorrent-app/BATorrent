// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef QMLNOTIFICATIONBRIDGE_H
#define QMLNOTIFICATIONBRIDGE_H

#include "bridges/bridgecommon.h"

class QmlNotificationBridge : public QObject
{
    Q_OBJECT
public:
    explicit QmlNotificationBridge(QObject *parent = nullptr) : QObject(parent) {}
    void setSession(IEngine *s) { m_session = s; }   // to skip the generic toast for movies (Play-now handles them)

public slots:
    void onTorrentAdded(const QString &name);
    void onTorrentFinished(const QString &name, const QString &infoHash);
    void onTorrentError(const QString &message);
    void onKillSwitchTriggered();
    void onRssAutoDownloaded(const QString &feedName, const QString &itemTitle);
    void onSuspiciousFilesDetected(const QString &name, const QStringList &files);

signals:
    // level: 0 = information, 1 = warning, 2 = critical
    void notify(const QString &title, const QString &body, int level);

private:
    IEngine *m_session = nullptr;
};

#endif // QMLNOTIFICATIONBRIDGE_H
