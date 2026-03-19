// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "rssmanager.h"
#include "../torrent/sessionmanager.h"
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QXmlStreamReader>
#include <QSettings>
#include <QUrl>
#include <QRegularExpression>
#include <QDir>
#include <QFile>

RssManager &RssManager::instance()
{
    static RssManager mgr;
    return mgr;
}

RssManager::RssManager()
    : m_net(new QNetworkAccessManager(this))
    , m_checkTimer(new QTimer(this))
{
    loadFeeds();
    connect(m_checkTimer, &QTimer::timeout, this, &RssManager::checkAllFeeds);
    setupTimers();
}

void RssManager::setSession(SessionManager *session, const QString &savePath)
{
    m_session = session;
    m_defaultSavePath = savePath;
}

void RssManager::setupTimers()
{
    // Find minimum interval across all feeds, default 30 min
    int minInterval = 30;
    for (const auto &f : m_feeds) {
        if (f.enabled && f.checkIntervalMin > 0 && f.checkIntervalMin < minInterval)
            minInterval = f.checkIntervalMin;
    }
    m_checkTimer->start(minInterval * 60 * 1000);
}

// --- Feed management ---

void RssManager::addFeed(const QString &url)
{
    QString trimmed = url.trimmed();
    if (trimmed.isEmpty()) return;

    // Check duplicates
    for (const auto &f : m_feeds) {
        if (f.url == trimmed) {
            emit feedError("Feed already exists.");
            return;
        }
    }

    RssFeed feed;
    feed.url = trimmed;
    feed.name = trimmed; // will be updated from feed title
    m_feeds.append(feed);
    saveFeeds();
    emit feedAdded(feed);

    // Fetch immediately
    checkFeed(m_feeds.size() - 1);
}

void RssManager::removeFeed(int index)
{
    if (index < 0 || index >= m_feeds.size()) return;
    m_feeds.removeAt(index);
    m_items.remove(index);

    // Rebuild items map with shifted indices
    QMap<int, QList<RssItem>> newItems;
    for (auto it = m_items.begin(); it != m_items.end(); ++it) {
        int key = it.key();
        if (key > index)
            newItems[key - 1] = it.value();
        else
            newItems[key] = it.value();
    }
    m_items = newItems;

    saveFeeds();
}

void RssManager::setFeedEnabled(int index, bool enabled)
{
    if (index < 0 || index >= m_feeds.size()) return;
    m_feeds[index].enabled = enabled;
    saveFeeds();
}

void RssManager::updateFeed(int index, const RssFeed &feed)
{
    if (index < 0 || index >= m_feeds.size()) return;
    m_feeds[index] = feed;
    saveFeeds();
    setupTimers();
}

QList<RssFeed> RssManager::feeds() const
{
    return m_feeds;
}

// --- Checking feeds ---

void RssManager::checkAllFeeds()
{
    for (int i = 0; i < m_feeds.size(); ++i) {
        if (m_feeds[i].enabled)
            checkFeed(i);
    }
}

void RssManager::checkFeed(int index)
{
    if (index < 0 || index >= m_feeds.size()) return;

    QNetworkRequest req(QUrl(m_feeds[index].url));
    req.setHeader(QNetworkRequest::UserAgentHeader, "BATorrent/1.9");
    auto *reply = m_net->get(req);

    connect(reply, &QNetworkReply::finished, this, [this, reply, index]() {
        reply->deleteLater();

        if (index >= m_feeds.size()) return;

        if (reply->error() != QNetworkReply::NoError) {
            emit feedError(QString("%1: %2").arg(m_feeds[index].name, reply->errorString()));
            return;
        }

        m_feeds[index].lastChecked = QDateTime::currentDateTime();
        parseFeedXml(index, reply->readAll());
        saveFeeds();
    });
}

// --- XML parsing ---

void RssManager::parseFeedXml(int feedIndex, const QByteArray &data)
{
    QXmlStreamReader xml(data);
    QList<RssItem> items;
    bool inItem = false;
    bool inChannel = false;
    RssItem currentItem;
    QString feedTitle;

    while (!xml.atEnd()) {
        xml.readNext();

        if (xml.isStartElement()) {
            QString tag = xml.name().toString();

            if (tag == "channel")
                inChannel = true;
            else if (tag == "item" || tag == "entry") {
                inItem = true;
                currentItem = RssItem();
            }
            else if (inItem) {
                if (tag == "title")
                    currentItem.title = xml.readElementText();
                else if (tag == "link") {
                    // Atom feeds use href attribute
                    if (xml.attributes().hasAttribute("href"))
                        currentItem.link = xml.attributes().value("href").toString();
                    else
                        currentItem.link = xml.readElementText();
                }
                else if (tag == "description" || tag == "summary")
                    currentItem.description = xml.readElementText();
                else if (tag == "pubDate" || tag == "published" || tag == "updated")
                    currentItem.pubDate = QDateTime::fromString(xml.readElementText().trimmed(), Qt::RFC2822Date);
                else if (tag == "enclosure") {
                    // Torrent RSS feeds often use enclosure for .torrent URL
                    QString encUrl = xml.attributes().value("url").toString();
                    if (!encUrl.isEmpty())
                        currentItem.link = encUrl;
                    QString length = xml.attributes().value("length").toString();
                    if (!length.isEmpty())
                        currentItem.size = length.toLongLong();
                }
                else if (tag == "torrent:magnetURI" || tag == "magnetURI")
                    currentItem.link = xml.readElementText();
                else if (tag == "torrent:contentLength" || tag == "contentLength")
                    currentItem.size = xml.readElementText().toLongLong();
            }
            else if (inChannel && !inItem && tag == "title")
                feedTitle = xml.readElementText();
        }
        else if (xml.isEndElement()) {
            QString tag = xml.name().toString();
            if (tag == "item" || tag == "entry") {
                inItem = false;
                if (!currentItem.link.isEmpty()) {
                    // Check if already downloaded
                    if (m_downloadedLinks.contains(currentItem.link))
                        currentItem.downloaded = true;
                    items.append(currentItem);
                }
            }
            else if (tag == "channel")
                inChannel = false;
        }
    }

    if (xml.hasError()) {
        emit feedError(QString("XML parse error: %1").arg(xml.errorString()));
        return;
    }

    // Update feed name from channel title
    if (!feedTitle.isEmpty() && feedIndex < m_feeds.size())
        m_feeds[feedIndex].name = feedTitle;

    m_items[feedIndex] = items;
    emit feedUpdated(feedIndex, items);

    // Auto-download if enabled
    if (feedIndex < m_feeds.size() && m_feeds[feedIndex].autoDownload)
        autoDownloadMatching(feedIndex);
}

void RssManager::autoDownloadMatching(int feedIndex)
{
    if (!m_session || feedIndex >= m_feeds.size()) return;

    const auto &feed = m_feeds[feedIndex];
    auto &items = m_items[feedIndex];
    QRegularExpression regex;

    if (!feed.filterPattern.isEmpty()) {
        regex = QRegularExpression(feed.filterPattern, QRegularExpression::CaseInsensitiveOption);
        if (!regex.isValid()) return;
    }

    QString savePath = feed.savePath.isEmpty() ? m_defaultSavePath : feed.savePath;

    for (auto &item : items) {
        if (item.downloaded) continue;

        // Check filter
        if (regex.isValid() && !regex.pattern().isEmpty()) {
            if (!regex.match(item.title).hasMatch())
                continue;
        }

        // Download it
        if (item.link.startsWith("magnet:")) {
            m_session->addMagnet(item.link, savePath);
        } else if (item.link.endsWith(".torrent") || item.link.contains("download")) {
            // Fetch .torrent file and add it
            QNetworkRequest req(QUrl(item.link));
            req.setHeader(QNetworkRequest::UserAgentHeader, "BATorrent/1.9");
            auto *reply = m_net->get(req);
            QString sp = savePath;

            connect(reply, &QNetworkReply::finished, this, [this, reply, sp]() {
                reply->deleteLater();
                if (reply->error() != QNetworkReply::NoError) return;

                // Save to temp file and add
                QString tmpPath = QDir::tempPath() + "/batorrent_rss_" +
                    QString::number(QDateTime::currentMSecsSinceEpoch()) + ".torrent";
                QFile f(tmpPath);
                if (f.open(QIODevice::WriteOnly)) {
                    f.write(reply->readAll());
                    f.close();
                    m_session->addTorrent(tmpPath, sp);
                    QFile::remove(tmpPath);
                }
            });
        }

        item.downloaded = true;
        m_downloadedLinks.insert(item.link);
        emit itemAutoDownloaded(feed.name, item.title);
    }

    saveFeeds();
}

// --- Manual download ---

QList<RssItem> RssManager::itemsForFeed(int index) const
{
    return m_items.value(index);
}

void RssManager::downloadItem(int feedIndex, int itemIndex)
{
    if (!m_session) return;
    if (feedIndex < 0 || feedIndex >= m_feeds.size()) return;

    auto &items = m_items[feedIndex];
    if (itemIndex < 0 || itemIndex >= items.size()) return;

    auto &item = items[itemIndex];
    const auto &feed = m_feeds[feedIndex];
    QString savePath = feed.savePath.isEmpty() ? m_defaultSavePath : feed.savePath;

    if (item.link.startsWith("magnet:"))
        m_session->addMagnet(item.link, savePath);
    else
        m_session->addTorrent(item.link, savePath);

    item.downloaded = true;
    m_downloadedLinks.insert(item.link);
    saveFeeds();
}

void RssManager::markDownloaded(const QString &, const QString &link)
{
    m_downloadedLinks.insert(link);
}

// --- Persistence ---

void RssManager::loadFeeds()
{
    QSettings settings("BATorrent", "BATorrent");

    int count = settings.beginReadArray("rssFeeds");
    m_feeds.clear();
    for (int i = 0; i < count; ++i) {
        settings.setArrayIndex(i);
        RssFeed f;
        f.url = settings.value("url").toString();
        f.name = settings.value("name").toString();
        f.savePath = settings.value("savePath").toString();
        f.filterPattern = settings.value("filterPattern").toString();
        f.enabled = settings.value("enabled", true).toBool();
        f.autoDownload = settings.value("autoDownload", false).toBool();
        f.checkIntervalMin = settings.value("checkInterval", 30).toInt();
        f.lastChecked = settings.value("lastChecked").toDateTime();
        m_feeds.append(f);
    }
    settings.endArray();

    // Load downloaded links set
    QStringList downloaded = settings.value("rssDownloadedLinks").toStringList();
    m_downloadedLinks = QSet<QString>(downloaded.begin(), downloaded.end());
}

void RssManager::saveFeeds()
{
    QSettings settings("BATorrent", "BATorrent");

    settings.beginWriteArray("rssFeeds", m_feeds.size());
    for (int i = 0; i < m_feeds.size(); ++i) {
        settings.setArrayIndex(i);
        settings.setValue("url", m_feeds[i].url);
        settings.setValue("name", m_feeds[i].name);
        settings.setValue("savePath", m_feeds[i].savePath);
        settings.setValue("filterPattern", m_feeds[i].filterPattern);
        settings.setValue("enabled", m_feeds[i].enabled);
        settings.setValue("autoDownload", m_feeds[i].autoDownload);
        settings.setValue("checkInterval", m_feeds[i].checkIntervalMin);
        settings.setValue("lastChecked", m_feeds[i].lastChecked);
    }
    settings.endArray();

    // Save downloaded links (keep last 5000 to avoid bloat)
    QStringList downloaded = m_downloadedLinks.values();
    if (downloaded.size() > 5000)
        downloaded = downloaded.mid(downloaded.size() - 5000);
    settings.setValue("rssDownloadedLinks", downloaded);
}
