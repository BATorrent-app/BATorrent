// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "bridges/qmlrssbridge.h"

#include "services/integrations/rssmanager.h"
#include "services/platform/utils.h"
#include "torrent/iengine.h"

#include <libtorrent/magnet_uri.hpp>
#include <QDateTime>
#include <QRegularExpression>
#include <QSettings>
#include <QUrl>
#include <sstream>

namespace {
QString seenKey(const QString &url) { return QStringLiteral("rssSeen/") + QString::fromLatin1(url.toUtf8().toBase64(QByteArray::Base64UrlEncoding)); }
QString squash(const QString &s)
{
    QString out;
    for (const QChar c : s) if (c.isLetterOrNumber()) out += c.toLower();
    return out;
}
// the library row this feed item became, by hash for a magnet, else by name
int libraryIndex(IEngine *eng, const QString &link, const QString &title)
{
    if (!eng) return -1;
    if (link.startsWith(QLatin1String("magnet:"))) {
        try {
            const auto atp = lt::parse_magnet_uri(link.toStdString());
            const QString h = QString::fromStdString((std::ostringstream() << atp.info_hashes.get_best()).str());
            const int i = eng->torrentIndexByInfoHash(h);
            if (i >= 0) return i;
        } catch (const std::exception &) {}
    }
    const QString key = squash(title);
    if (key.isEmpty()) return -1;
    for (int i = 0; i < eng->torrentCount(); ++i)
        if (squash(eng->torrentAt(i).name) == key) return i;
    return -1;
}
}

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
        m["lastCheckedSecs"] = f.lastChecked.isValid() ? f.lastChecked.toSecsSinceEpoch() : 0;
        const auto items = RssManager::instance().itemsForFeed(i);
        m["count"] = items.size();
        // new: published since the feed was last opened here
        const qint64 seen = QSettings().value(seenKey(f.url), 0).toLongLong();
        int fresh = 0;
        for (const RssItem &it : items)
            if (it.pubDate.isValid() && it.pubDate.toSecsSinceEpoch() > seen) ++fresh;
        m["newCount"] = seen > 0 ? fresh : 0;
        out << m;
    }
    return out;
}

QVariantList QmlRssBridge::itemsForFeed(int feedIndex) const
{
    QVariantList out;
    auto &rss = RssManager::instance();
    const auto items = rss.itemsForFeed(feedIndex);
    const auto feeds = rss.feeds();
    const RssFeed feed = feedIndex >= 0 && feedIndex < feeds.size() ? feeds[feedIndex] : RssFeed();
    const QRegularExpression re(feed.filterPattern, QRegularExpression::CaseInsensitiveOption);
    const bool filtering = !feed.filterPattern.isEmpty() && re.isValid();
    for (const RssItem &it : items) {
        QVariantMap m;
        m["title"] = it.title;
        m["link"] = it.link;
        m["size"] = it.size > 0 ? formatSize(it.size) : QString();
        m["date"] = it.pubDate.isValid() ? it.pubDate.toString("yyyy-MM-dd hh:mm") : QString();
        m["pubSecs"] = it.pubDate.isValid() ? it.pubDate.toSecsSinceEpoch() : 0;
        m["downloaded"] = it.downloaded;
        // what happened to it: downloading, downloaded, or why it was left
        QString state, reason;
        if (it.downloaded) {
            const int i = libraryIndex(rss.session(), it.link, it.title);
            if (i >= 0) {
                const TorrentInfo info = rss.session()->torrentAt(i);
                state = info.completed || info.finished ? QStringLiteral("done") : QStringLiteral("downloading");
            } else {
                state = QStringLiteral("done");
            }
        } else if (libraryIndex(rss.session(), it.link, it.title) >= 0) {
            state = QStringLiteral("ignored");
            reason = QStringLiteral("library");
        } else if (feed.autoDownload && filtering && !re.match(it.title).hasMatch()) {
            state = QStringLiteral("ignored");
            reason = QStringLiteral("filter");
        }
        m["state"] = state;
        m["reason"] = reason;
        out << m;
    }
    return out;
}

void QmlRssBridge::markFeedSeen(int index)
{
    const auto feeds = RssManager::instance().feeds();
    if (index < 0 || index >= feeds.size()) return;
    const QString key = seenKey(feeds[index].url);
    const bool first = !QSettings().contains(key);
    QSettings().setValue(key, QDateTime::currentSecsSinceEpoch());
    if (!first) emit feedsChanged();
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
