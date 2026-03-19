// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef RSSMANAGER_H
#define RSSMANAGER_H

#include <QObject>
#include <QStringList>
#include <QDateTime>
#include <QTimer>

class QNetworkAccessManager;
class SessionManager;

struct RssFeed {
    QString url;
    QString name;
    QString savePath;
    QString filterPattern; // regex
    bool enabled = true;
    bool autoDownload = false;
    int checkIntervalMin = 30;
    QDateTime lastChecked;
};

struct RssItem {
    QString title;
    QString link;       // .torrent URL or magnet
    QString description;
    QDateTime pubDate;
    qint64 size = 0;
    bool downloaded = false;
};

class RssManager : public QObject
{
    Q_OBJECT
public:
    static RssManager &instance();

    void setSession(SessionManager *session, const QString &savePath);

    // Feed management
    void addFeed(const QString &url);
    void removeFeed(int index);
    void setFeedEnabled(int index, bool enabled);
    void updateFeed(int index, const RssFeed &feed);
    QList<RssFeed> feeds() const;

    // Check feeds
    void checkAllFeeds();
    void checkFeed(int index);

    // Items
    QList<RssItem> itemsForFeed(int index) const;
    void downloadItem(int feedIndex, int itemIndex);
    void markDownloaded(const QString &feedUrl, const QString &link);

    // Persistence
    void loadFeeds();
    void saveFeeds();

signals:
    void feedAdded(const RssFeed &feed);
    void feedUpdated(int index, const QList<RssItem> &items);
    void feedError(const QString &error);
    void itemAutoDownloaded(const QString &feedName, const QString &itemTitle);

private:
    RssManager();
    void parseFeedXml(int feedIndex, const QByteArray &data);
    void autoDownloadMatching(int feedIndex);
    void setupTimers();

    QNetworkAccessManager *m_net;
    SessionManager *m_session = nullptr;
    QString m_defaultSavePath;
    QList<RssFeed> m_feeds;
    QMap<int, QList<RssItem>> m_items; // feedIndex -> items
    QSet<QString> m_downloadedLinks;   // track already downloaded
    QTimer *m_checkTimer;
};

#endif
