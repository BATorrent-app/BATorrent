// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "bridges/qmlrssbridge.h"

#include "services/integrations/rssmanager.h"
#include "services/platform/utils.h"

#include <QUrl>

QmlRssBridge::QmlRssBridge(QObject *parent) : QObject(parent)
{
    auto &rss = RssManager::instance();
    connect(&rss, &RssManager::feedAdded, this, [this]() { emit feedsChanged(); });
    connect(&rss, &RssManager::feedUpdated, this, [this]() { emit feedsChanged(); });
    connect(&rss, &RssManager::feedError, this, &QmlRssBridge::errorOccurred);
    connect(&rss, &RssManager::itemAutoDownloaded, this, &QmlRssBridge::autoDownloaded);
    rss.loadFeeds();
}

QVariantList QmlRssBridge::feeds() const
{
    QVariantList out;
    const auto feeds = RssManager::instance().feeds();
    for (int i = 0; i < feeds.size(); ++i) {
        const RssFeed &f = feeds[i];
        QVariantMap m;
        m["index"] = i;
        m["name"] = f.name.isEmpty() ? f.url : f.name;
        m["url"] = f.url;
        m["enabled"] = f.enabled;
        m["autoDownload"] = f.autoDownload;
        m["filterPattern"] = f.filterPattern;
        m["savePath"] = f.savePath;
        m["checkInterval"] = f.checkIntervalMin;
        m["lastChecked"] = f.lastChecked.isValid()
            ? f.lastChecked.toString("yyyy-MM-dd hh:mm") : QString();
        m["count"] = RssManager::instance().itemsForFeed(i).size();
        out << m;
    }
    return out;
}

QVariantList QmlRssBridge::itemsForFeed(int feedIndex) const
{
    QVariantList out;
    const auto items = RssManager::instance().itemsForFeed(feedIndex);
    for (const RssItem &it : items) {
        QVariantMap m;
        m["title"] = it.title;
        m["link"] = it.link;
        m["size"] = it.size > 0 ? formatSize(it.size) : QString();
        m["date"] = it.pubDate.isValid() ? it.pubDate.toString("yyyy-MM-dd hh:mm") : QString();
        m["downloaded"] = it.downloaded;
        out << m;
    }
    return out;
}

void QmlRssBridge::addFeed(const QString &url)
{
    if (url.trimmed().isEmpty()) return;
    RssManager::instance().addFeed(url.trimmed());
    RssManager::instance().saveFeeds();
    emit feedsChanged();
}

void QmlRssBridge::removeFeed(int index)
{
    RssManager::instance().removeFeed(index);
    RssManager::instance().saveFeeds();
    emit feedsChanged();
}

void QmlRssBridge::setFeedEnabled(int index, bool enabled)
{
    RssManager::instance().setFeedEnabled(index, enabled);
    RssManager::instance().saveFeeds();
    emit feedsChanged();
}

void QmlRssBridge::setAutoDownload(int index, bool on)
{
    auto feeds = RssManager::instance().feeds();
    if (index < 0 || index >= feeds.size()) return;
    RssFeed copy = feeds[index];
    copy.autoDownload = on;
    RssManager::instance().updateFeed(index, copy);
    RssManager::instance().saveFeeds();
    emit feedsChanged();
}

void QmlRssBridge::checkAllFeeds()
{
    RssManager::instance().checkAllFeeds();
}

void QmlRssBridge::checkFeed(int index)
{
    RssManager::instance().checkFeed(index);
}

void QmlRssBridge::updateFeedSettings(int index, const QString &filterPattern,
                                      const QString &savePath, int checkInterval,
                                      bool enabled, bool autoDownload)
{
    auto feeds = RssManager::instance().feeds();
    if (index < 0 || index >= feeds.size()) return;
    RssFeed f = feeds[index];
    f.filterPattern = filterPattern;
    f.savePath = savePath.startsWith("file://") ? QUrl(savePath).toLocalFile() : savePath;
    f.checkIntervalMin = qBound(5, checkInterval, 1440);
    f.enabled = enabled;
    f.autoDownload = autoDownload;
    RssManager::instance().updateFeed(index, f);
    RssManager::instance().saveFeeds();
    emit feedsChanged();
}

void QmlRssBridge::downloadItem(int feedIndex, int itemIndex)
{
    RssManager::instance().downloadItem(feedIndex, itemIndex);
}
