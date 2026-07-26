// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "services/integrations/rssmanager.h"
#include "torrent/iengine.h"
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QXmlStreamReader>
#include <QSettings>
#include <QUrl>
#include <QRegularExpression>
#include <QDir>
#include <QFile>
#include <QTimer>

RssManager &RssManager::instance()
{
    static RssManager mgr;
    return mgr;
}

RssManager::RssManager()
    : m_net(new QNetworkAccessManager(this))
    , m_checkTimer(new QTimer(this))
{
    m_net->setTransferTimeout(30000);
    loadFeeds();
    connect(m_checkTimer, &QTimer::timeout, this, &RssManager::checkAllFeeds);
    setupTimers();
}

void RssManager::setSession(IEngine *session, const QString &savePath)
{
    m_session = session;
    m_defaultSavePath = savePath;

    // Items are NOT persisted — only the feed definitions are. So every launch
    // starts with an empty list, and checkAllFeeds() won't refill it because it
    // honours checkIntervalMin and lastChecked was minutes ago: the feed sat at
    // "0 items" for up to half an hour, looking broken. Refetch exactly the feeds
    // we have nothing cached for, once, shortly after startup (delayed so the UI
    // is listening for feedUpdated by the time it lands).
    QTimer::singleShot(1500, this, [this]() {
        for (int i = 0; i < m_feeds.size(); ++i) {
            if (!m_feeds[i].enabled) continue;
            if (!m_items.value(i).isEmpty()) continue;
            checkFeed(i);
        }
    });
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

    // Removing a feed can change the minimum check interval; rebuild the
    // timer so we don't keep polling at the old (faster or slower) rate.
    setupTimers();
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
    // The single timer ticks at the smallest configured interval; honor each
    // feed's own checkIntervalMin here so a feed set to "every 6 h" isn't
    // actually polled every 30 min (which can get the user banned from
    // private trackers).
    const QDateTime now = QDateTime::currentDateTime();
    for (int i = 0; i < m_feeds.size(); ++i) {
        const auto &f = m_feeds[i];
        if (!f.enabled) continue;
        if (f.lastChecked.isValid()
            && f.lastChecked.secsTo(now) < f.checkIntervalMin * 60)
            continue;
        checkFeed(i);
    }
}

void RssManager::checkFeed(int index)
{
    if (index < 0 || index >= m_feeds.size()) return;

    QNetworkRequest req(QUrl(m_feeds[index].url));
    req.setHeader(QNetworkRequest::UserAgentHeader, "BATorrent/1.9");
    auto *reply = m_net->get(req);

    // Capture by URL, not index — if the user removes a feed below this one
    // while the request is in flight, the index would shift and the reply
    // would parse the wrong feed.
    const QString feedUrl = m_feeds[index].url;

    connect(reply, &QNetworkReply::finished, this, [this, reply, feedUrl]() {
        reply->deleteLater();

        int idx = -1;
        for (int i = 0; i < m_feeds.size(); ++i) {
            if (m_feeds[i].url == feedUrl) { idx = i; break; }
        }
        if (idx < 0) return; // feed removed while request was in flight

        if (reply->error() != QNetworkReply::NoError) {
            emit feedError(QString("%1: %2").arg(m_feeds[idx].name, reply->errorString()));
            return;
        }

        m_feeds[idx].lastChecked = QDateTime::currentDateTime();
        parseFeedXml(idx, reply->readAll());
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

        addFromLink(item.link, savePath);

        item.downloaded = true;
        m_downloadedLinks.insert(item.link);
        emit itemAutoDownloaded(feed.name, item.title);
    }

    saveFeeds();
}

// A feed item's link is one of three things: a magnet, a remote .torrent, or (rarely)
// a local path. Only magnets can go straight to the engine — SessionManager::addTorrent
// opens its argument as a FILE, so handing it an https URL just throws. The auto-download
// rules already fetched-then-added; the manual click didn't, so clicking an item on any
// feed that publishes .torrent links (Nyaa, most trackers) silently did nothing.
void RssManager::addFromLink(const QString &link, const QString &savePath)
{
    if (!m_session || link.isEmpty()) return;

    if (link.startsWith(QLatin1String("magnet:"))) {
        m_session->addMagnet(link, savePath);
        return;
    }
    if (!link.startsWith(QLatin1String("http"))) {   // already a local .torrent
        m_session->addTorrent(link, savePath);
        return;
    }

    QNetworkRequest req{QUrl(link)};
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("BATorrent/1.9"));
    // trackers routinely 302 from /download/<id> to the real file
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                     QNetworkRequest::NoLessSafeRedirectPolicy);
    auto *reply = m_net->get(req);

    connect(reply, &QNetworkReply::finished, this, [this, reply, savePath]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            qWarning() << "[rss] .torrent fetch failed:" << reply->errorString();
            return;
        }
        const QByteArray body = reply->readAll();
        // A bencoded torrent starts with 'd'. An error page or a login redirect
        // would otherwise be written to disk and fail deep inside libtorrent with
        // a useless message.
        if (body.isEmpty() || body.front() != 'd') {
            qWarning() << "[rss] response is not a .torrent (" << body.size() << "bytes )";
            return;
        }
        const QString tmpPath = QDir::tempPath() + QStringLiteral("/batorrent_rss_")
                              + QString::number(QDateTime::currentMSecsSinceEpoch())
                              + QStringLiteral(".torrent");
        QFile f(tmpPath);
        if (!f.open(QIODevice::WriteOnly)) {
            qWarning() << "[rss] cannot write" << tmpPath;
            return;
        }
        f.write(body);
        f.close();
        m_session->addTorrent(tmpPath, savePath);
        QFile::remove(tmpPath);   // libtorrent has parsed it by now
    });
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

    addFromLink(item.link, savePath);

    item.downloaded = true;
    m_downloadedLinks.insert(item.link);
    saveFeeds();
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
