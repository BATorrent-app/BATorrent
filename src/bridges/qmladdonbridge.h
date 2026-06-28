// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef QMLADDONBRIDGE_H
#define QMLADDONBRIDGE_H

#include "bridges/bridgecommon.h"

class QmlAddonBridge : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList installed READ installed NOTIFY changed)
    Q_PROPERTY(QVariantList suggested READ suggested NOTIFY changed)
    Q_PROPERTY(bool autoTrackers READ autoTrackers WRITE setAutoTrackers NOTIFY changed)
    Q_PROPERTY(int trackerCount READ trackerCount NOTIFY changed)
    Q_PROPERTY(bool torrentSearchEnabled READ torrentSearchEnabled WRITE setTorrentSearchEnabled NOTIFY changed)
    Q_PROPERTY(QString torrentSearchUrl READ torrentSearchUrl WRITE setTorrentSearchUrl NOTIFY changed)
public:
    explicit QmlAddonBridge(QObject *parent = nullptr);

    QVariantList installed() const;
    QVariantList suggested() const;
    bool autoTrackers() const;
    void setAutoTrackers(bool on);
    int trackerCount() const;
    bool torrentSearchEnabled() const;
    void setTorrentSearchEnabled(bool on);
    QString torrentSearchUrl() const;
    void setTorrentSearchUrl(const QString &url);

    Q_INVOKABLE void addAddon(const QString &url);
    Q_INVOKABLE void removeAddon(int index);
    Q_INVOKABLE void setEnabled(int index, bool on);
    Q_INVOKABLE void refreshTrackers();
    Q_INVOKABLE bool isInstalled(const QString &url) const;

signals:
    void changed();
    void error(const QString &message);
};

#endif // QMLADDONBRIDGE_H
