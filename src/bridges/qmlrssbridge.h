// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef QMLRSSBRIDGE_H
#define QMLRSSBRIDGE_H

#include "bridges/bridgecommon.h"

class QmlRssBridge : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList feeds READ feeds NOTIFY feedsChanged)
public:
    explicit QmlRssBridge(QObject *parent = nullptr);

    QVariantList feeds() const;
    Q_INVOKABLE QVariantList itemsForFeed(int feedIndex) const;
    Q_INVOKABLE void addFeed(const QString &url);
    Q_INVOKABLE void removeFeed(int index);
    Q_INVOKABLE void setFeedEnabled(int index, bool enabled);
    Q_INVOKABLE void setAutoDownload(int index, bool on);
    Q_INVOKABLE void checkAllFeeds();
    Q_INVOKABLE void checkFeed(int index);
    Q_INVOKABLE void downloadItem(int feedIndex, int itemIndex);
    Q_INVOKABLE void updateFeedSettings(int index, const QString &filterPattern,
                                        const QString &savePath, int checkInterval,
                                        bool enabled, bool autoDownload);

signals:
    void feedsChanged();
    void errorOccurred(const QString &message);
    void autoDownloaded(const QString &feedName, const QString &itemTitle);
};

#endif // QMLRSSBRIDGE_H
