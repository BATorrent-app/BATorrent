// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "addonmanager.h"
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QRegularExpression>
#include <QSettings>
#include <QUrl>

namespace {
// Public trackers appended to magnets built from a bare info_hash so freshly
// added results can find peers before any tracker/DHT bootstrap.
QString magnetTrackerParams()
{
    static const QStringList trackers = {
        "udp://tracker.opentrackr.org:1337/announce",
        "udp://open.stealth.si:80/announce",
        "udp://tracker.openbittorrent.com:6969/announce",
        "udp://exodus.desync.com:6969/announce",
    };
    QString params;
    for (const auto &t : trackers)
        params += "&tr=" + QUrl::toPercentEncoding(t);
    return params;
}
}

AddonManager &AddonManager::instance()
{
    static AddonManager mgr;
    return mgr;
}

AddonManager::AddonManager()
    : m_net(new QNetworkAccessManager(this))
{
    loadAddons();
    installDefaults();
    loadSearchProviders();
    installDefaultProviders();
}

void AddonManager::loadAddons()
{
    QSettings settings("BATorrent", "BATorrent");
    m_autoTrackers = settings.value("autoTrackers", true).toBool();
    m_torrentSearchEnabled = settings.value("torrentSearchEnabled", false).toBool();
    m_torrentSearchUrl = settings.value("torrentSearchUrl").toString();

    int count = settings.beginReadArray("addons");
    m_addons.clear();
    for (int i = 0; i < count; ++i) {
        settings.setArrayIndex(i);
        AddonManifest m;
        m.id = settings.value("id").toString();
        m.name = settings.value("name").toString();
        m.description = settings.value("description").toString();
        m.url = settings.value("url").toString();
        m.types = settings.value("types").toStringList();
        m.resources = settings.value("resources").toStringList();
        m.enabled = settings.value("enabled", true).toBool();
        m_addons.append(m);
    }
    settings.endArray();

    // Load cached tracker list
    m_trackerList = settings.value("trackerList").toStringList();
}

void AddonManager::saveAddons()
{
    QSettings settings("BATorrent", "BATorrent");

    settings.beginWriteArray("addons", m_addons.size());
    for (int i = 0; i < m_addons.size(); ++i) {
        settings.setArrayIndex(i);
        settings.setValue("id", m_addons[i].id);
        settings.setValue("name", m_addons[i].name);
        settings.setValue("description", m_addons[i].description);
        settings.setValue("url", m_addons[i].url);
        settings.setValue("types", m_addons[i].types);
        settings.setValue("resources", m_addons[i].resources);
        settings.setValue("enabled", m_addons[i].enabled);
    }
    settings.endArray();
}

void AddonManager::installDefaults()
{
    QSettings settings("BATorrent", "BATorrent");
    if (settings.value("addonsInitialized", false).toBool())
        return;

    settings.setValue("addonsInitialized", true);

    // Pre-install Cinemeta (catalog) and Torrentio (streams)
    struct DefaultAddon {
        QString id, name, desc, url;
        QStringList types, resources;
    };
    QList<DefaultAddon> defaults = {
        {"com.linvo.cinemeta", "Cinemeta",
         "The official addon for movie and series catalogs",
         "https://v3-cinemeta.strem.io",
         {"movie", "series"}, {"catalog", "meta"}},
        {"com.stremio.torrentio.addon", "Torrentio",
         "Torrent streams from multiple providers",
         "https://torrentio.strem.fun",
         {"movie", "series"}, {"stream"}},
    };

    for (const auto &d : defaults) {
        bool exists = false;
        for (const auto &a : m_addons) {
            if (a.url == d.url || a.id == d.id) { exists = true; break; }
        }
        if (!exists) {
            AddonManifest m;
            m.id = d.id;
            m.name = d.name;
            m.description = d.desc;
            m.url = d.url;
            m.types = d.types;
            m.resources = d.resources;
            m.enabled = true;
            m_addons.append(m);
        }
    }
    saveAddons();
}

bool AddonManager::hasCatalogAddon() const
{
    for (const auto &a : m_addons)
        if (a.enabled && a.resources.contains("catalog")) return true;
    return false;
}

bool AddonManager::hasStreamAddon() const
{
    for (const auto &a : m_addons)
        if (a.enabled && a.resources.contains("stream")) return true;
    return false;
}

void AddonManager::addAddon(const QString &url)
{
    QString baseUrl = url;
    if (baseUrl.endsWith('/'))
        baseUrl.chop(1);

    // Check duplicates
    for (const auto &a : m_addons) {
        if (a.url == baseUrl) {
            emit addonError("Addon already installed.");
            return;
        }
    }

    fetchManifest(baseUrl);
}

void AddonManager::fetchManifest(const QString &url)
{
    QNetworkRequest req(QUrl(url + "/manifest.json"));
    req.setHeader(QNetworkRequest::UserAgentHeader, "BATorrent/1.9");
    auto *reply = m_net->get(req);

    connect(reply, &QNetworkReply::finished, this, [this, reply, url]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit addonError(reply->errorString());
            return;
        }

        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        if (!doc.isObject()) {
            emit addonError("Invalid manifest format.");
            return;
        }

        // Parse Stremio manifest
        QJsonObject obj = doc.object();
        AddonManifest m;
        m.id = obj.value("id").toString();
        m.name = obj.value("name").toString("Unknown Addon");
        m.description = obj.value("description").toString();
        m.url = url;

        // Parse types
        QJsonArray types = obj.value("types").toArray();
        for (const auto &t : types)
            m.types.append(t.toString());

        // Parse resources
        QJsonArray resources = obj.value("resources").toArray();
        for (const auto &r : resources) {
            if (r.isString())
                m.resources.append(r.toString());
            else if (r.isObject())
                m.resources.append(r.toObject().value("name").toString());
        }

        m.enabled = true;
        m_addons.append(m);
        saveAddons();
        emit addonAdded(m);
    });
}

void AddonManager::removeAddon(int index)
{
    if (index < 0 || index >= m_addons.size()) return;
    m_addons.removeAt(index);
    saveAddons();
}

void AddonManager::setAddonEnabled(int index, bool enabled)
{
    if (index < 0 || index >= m_addons.size()) return;
    m_addons[index].enabled = enabled;
    saveAddons();
}

QList<AddonManifest> AddonManager::addons() const
{
    return m_addons;
}

// Stremio catalog search: GET {url}/catalog/{type}/{catalogId}/search={query}.json
void AddonManager::searchCatalog(const QString &query)
{
    m_catalogResults.clear();
    m_pendingCatalog = 0;
    const quint32 gen = ++m_catalogGen;

    for (const auto &addon : m_addons) {
        if (!addon.enabled || !addon.resources.contains("catalog"))
            continue;

        // Search each type the addon supports
        for (const auto &type : addon.types) {
            m_pendingCatalog++;
            // Stremio search URL format
            QString searchUrl = QString("%1/catalog/%2/top/search=%3.json")
                .arg(addon.url, type, QUrl::toPercentEncoding(query));

            QNetworkRequest req{QUrl(searchUrl)};
            req.setHeader(QNetworkRequest::UserAgentHeader, "BATorrent/1.9");
            auto *reply = m_net->get(req);

            connect(reply, &QNetworkReply::finished, this, [this, reply, gen]() {
                reply->deleteLater();
                if (gen != m_catalogGen) return; // stale reply, ignore
                m_pendingCatalog--;

                if (reply->error() == QNetworkReply::NoError) {
                    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
                    QJsonObject obj = doc.object();
                    QJsonArray metas = obj.value("metas").toArray();

                    for (const auto &val : metas) {
                        QJsonObject m = val.toObject();
                        CatalogItem item;
                        item.id = m.value("id").toString();
                        item.type = m.value("type").toString();
                        item.name = m.value("name").toString();
                        item.poster = m.value("poster").toString();
                        // Year from releaseInfo or year field
                        QString release = m.value("releaseInfo").toString();
                        if (!release.isEmpty())
                            item.year = release.left(4).toInt();

                        // Avoid duplicates by IMDB ID
                        bool dup = false;
                        for (const auto &existing : m_catalogResults) {
                            if (existing.id == item.id) { dup = true; break; }
                        }
                        if (!dup && !item.id.isEmpty())
                            m_catalogResults.append(item);
                    }
                }

                emit catalogResults(m_catalogResults);
                if (m_pendingCatalog <= 0)
                    emit catalogFinished();
            });
        }
    }

    if (m_pendingCatalog == 0)
        emit catalogFinished();
}

// Stremio stream: GET {url}/stream/{type}/{id}.json
void AddonManager::getStreams(const QString &type, const QString &id)
{
    m_streamResults.clear();
    m_pendingStreams = 0;
    const quint32 gen = ++m_streamGen;

    for (const auto &addon : m_addons) {
        if (!addon.enabled || !addon.resources.contains("stream"))
            continue;
        if (!addon.types.contains(type))
            continue;

        m_pendingStreams++;
        QString streamUrl = QString("%1/stream/%2/%3.json")
            .arg(addon.url, type, id);

        QNetworkRequest req{QUrl(streamUrl)};
        req.setHeader(QNetworkRequest::UserAgentHeader, "BATorrent/1.9");
        auto *reply = m_net->get(req);

        connect(reply, &QNetworkReply::finished, this, [this, reply, gen, addonName = addon.name]() {
            reply->deleteLater();
            if (gen != m_streamGen) return; // stale reply, ignore
            m_pendingStreams--;

            if (reply->error() == QNetworkReply::NoError) {
                QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
                QJsonObject obj = doc.object();
                QJsonArray streams = obj.value("streams").toArray();

                for (const auto &val : streams) {
                    QJsonObject s = val.toObject();
                    StreamResult r;
                    r.addonName = addonName;

                    // Stremio streams can have infoHash+fileIdx or direct magnet
                    QString infoHash = s.value("infoHash").toString();
                    if (!infoHash.isEmpty()) {
                        r.magnet = QString("magnet:?xt=urn:btih:%1").arg(infoHash);
                        // Add trackers from behaviorHints or sources
                        QJsonArray sources = s.value("sources").toArray();
                        for (const auto &src : sources) {
                            QString tracker = src.toString();
                            if (tracker.startsWith("tracker:"))
                                r.magnet += "&tr=" + QUrl::toPercentEncoding(tracker.mid(8));
                        }
                    } else {
                        // Direct URL (magnet or HTTP)
                        r.magnet = s.value("url").toString();
                    }

                    // Parse title/name for quality info
                    r.title = s.value("title").toString();
                    if (r.title.isEmpty())
                        r.title = s.value("name").toString();

                    // Extract size from behaviorHints
                    QJsonObject hints = s.value("behaviorHints").toObject();
                    r.size = hints.value("videoSize").toVariant().toLongLong();

                    if (!r.magnet.isEmpty() && r.magnet.startsWith("magnet:"))
                        m_streamResults.append(r);
                }
            }

            emit streamResults(m_streamResults);
            if (m_pendingStreams <= 0)
                emit streamFinished();
        });
    }

    if (m_pendingStreams == 0)
        emit streamFinished();
}

// --- Auto tracker list ---

void AddonManager::fetchTrackerList()
{
    QUrl url("https://raw.githubusercontent.com/ngosang/trackerslist/master/trackers_best.txt");
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::UserAgentHeader, "BATorrent/1.9");
    auto *reply = m_net->get(req);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) return;

        QString data = QString::fromUtf8(reply->readAll());
        QStringList trackers;
        for (const auto &line : data.split('\n')) {
            QString trimmed = line.trimmed();
            if (!trimmed.isEmpty())
                trackers.append(trimmed);
        }

        if (!trackers.isEmpty()) {
            m_trackerList = trackers;
            QSettings settings("BATorrent", "BATorrent");
            settings.setValue("trackerList", m_trackerList);
            emit trackerListUpdated();
        }
    });
}

QStringList AddonManager::trackerList() const
{
    return m_trackerList;
}

bool AddonManager::autoTrackersEnabled() const
{
    return m_autoTrackers;
}

void AddonManager::setAutoTrackersEnabled(bool enabled)
{
    m_autoTrackers = enabled;
    QSettings settings("BATorrent", "BATorrent");
    settings.setValue("autoTrackers", enabled);
}

// --- Torrent Search ---

bool AddonManager::torrentSearchEnabled() const
{
    return m_torrentSearchEnabled;
}

void AddonManager::setTorrentSearchEnabled(bool enabled)
{
    m_torrentSearchEnabled = enabled;
    QSettings settings("BATorrent", "BATorrent");
    settings.setValue("torrentSearchEnabled", enabled);
}

QString AddonManager::torrentSearchUrl() const
{
    return m_torrentSearchUrl;
}

void AddonManager::setTorrentSearchUrl(const QString &url)
{
    m_torrentSearchUrl = url;
    QSettings settings("BATorrent", "BATorrent");
    settings.setValue("torrentSearchUrl", url);
}

void AddonManager::searchTorrents(const QString &query, int category)
{
    if (!m_torrentSearchEnabled || m_torrentSearchUrl.isEmpty()) {
        emit torrentSearchError(tr("Torrent search is not configured."));
        emit torrentSearchFinished();
        return;
    }

    QString baseUrl = m_torrentSearchUrl;
    if (baseUrl.endsWith('/'))
        baseUrl.chop(1);

    QString searchUrl = QString("%1/q.php?q=%2&cat=%3")
        .arg(baseUrl, QUrl::toPercentEncoding(query), QString::number(category));

    QNetworkRequest req{QUrl(searchUrl)};
    req.setHeader(QNetworkRequest::UserAgentHeader, "BATorrent/2.0");
    req.setTransferTimeout(15000);   // don't let a slow provider hang the search UI
    auto *reply = m_net->get(req);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit torrentSearchError(reply->errorString());
            emit torrentSearchFinished();
            return;
        }

        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        if (!doc.isArray()) {
            emit torrentSearchError(tr("Invalid response format."));
            emit torrentSearchFinished();
            return;
        }

        const QString trackerParams = magnetTrackerParams();

        QList<TorrentSearchResult> results;
        QJsonArray arr = doc.array();
        for (const auto &val : arr) {
            QJsonObject obj = val.toObject();
            QString name = obj.value("name").toString();
            QString infoHash = obj.value("info_hash").toString();
            if (infoHash.isEmpty() || infoHash == "0")
                continue;

            TorrentSearchResult r;
            r.name = name;
            r.infoHash = infoHash;
            r.size = obj.value("size").toVariant().toLongLong();
            r.seeders = obj.value("seeders").toVariant().toInt();
            r.leechers = obj.value("leechers").toVariant().toInt();
            r.category = obj.value("category").toString();
            r.provider = QStringLiteral("Torrents");
            r.magnet = QString("magnet:?xt=urn:btih:%1&dn=%2%3")
                .arg(infoHash, QUrl::toPercentEncoding(name), trackerParams);

            results.append(r);
        }

        emit torrentSearchResults(results);
        emit torrentSearchFinished();
    });
}

// --- Search Providers ---

void AddonManager::installDefaultProviders()
{
    QSettings s("BATorrent", "BATorrent");

    struct Def { QString id, name, url, arr, nm, hash, sz, seed, leech; bool enabled; };
    const QList<Def> defaults = {
        {"apibay", "The Pirate Bay (apibay)",
         "https://apibay.org/q.php?q={query}&cat={category}",
         "", "name", "info_hash", "size", "seeders", "leechers", true},
        {"nyaa_api", "Nyaa.si",
         "https://nyaa.si/api/v2?q={query}&limit=50",
         "torrents", "title", "info_hash", "total_size", "seeders", "leechers", true},
        // Open torrent database (no login), returns raw-byte sizes + real swarm
        // counts. Global content; safe to enable like apibay/nyaa.
        {"torrents_csv", "Torrents-CSV",
         "https://torrents-csv.com/service/search?q={query}&size=50",
         "torrents", "name", "infohash", "size_bytes", "seeders", "leechers", true},
        // RuTor via TorAPI scrapes the (registration-walled) Russian/CIS trackers
        // server-side and returns the info_hash, so the magnet downloads over DHT
        // without the user ever logging in. Off by default — it routes the query
        // through a third-party instance; the URL is editable to self-host TorAPI.
        {"rutor_torapi", "RuTor (TorAPI · CIS)",
         "https://torapi.vercel.app/api/search/title/rutor?query={query}",
         "", "Name", "Hash", "Size", "Seeds", "Peers", false},
        // RuTracker needs a login, so the public TorAPI can't search it. Off by
        // default and only useful pointed at a self-hosted TorAPI that holds your
        // RuTracker account — edit the URL to your instance, then enable.
        {"rutracker_torapi", "RuTracker (TorAPI — self-host login)",
         "https://torapi.vercel.app/api/search/title/rutracker?query={query}",
         "", "Name", "Hash", "Size", "Seeds", "Peers", false},
    };

    // Seed each built-in once (tracked by id) so a new provider reaches existing
    // users on update, while one the user deleted never resurrects.
    QStringList seeded = s.value("seededProviderIds").toStringList();
    if (seeded.isEmpty() && s.value("searchProvidersInitialized", false).toBool())
        seeded << QStringLiteral("apibay") << QStringLiteral("nyaa_api");  // pre-seed-flag users
    bool changed = false;
    for (const auto &d : defaults) {
        if (seeded.contains(d.id)) continue;
        seeded << d.id;
        changed = true;
        bool exists = false;
        for (const auto &p : m_searchProviders)
            if (p.id == d.id) { exists = true; break; }
        if (!exists) {
            SearchProvider p;
            p.id = d.id; p.name = d.name; p.urlTemplate = d.url;
            p.arrayPath = d.arr; p.namePath = d.nm; p.hashPath = d.hash;
            p.sizePath = d.sz; p.seedersPath = d.seed; p.leechersPath = d.leech;
            p.enabled = d.enabled; p.builtIn = true;
            m_searchProviders.append(p);
        }
    }
    s.setValue("searchProvidersInitialized", true);
    s.setValue("seededProviderIds", seeded);
    if (changed) saveSearchProviders();
}

void AddonManager::loadSearchProviders()
{
    QSettings s("BATorrent", "BATorrent");
    int count = s.beginReadArray("searchProviders");
    m_searchProviders.clear();
    for (int i = 0; i < count; ++i) {
        s.setArrayIndex(i);
        SearchProvider p;
        p.id = s.value("id").toString();
        p.name = s.value("name").toString();
        p.urlTemplate = s.value("urlTemplate").toString();
        p.arrayPath = s.value("arrayPath").toString();
        p.namePath = s.value("namePath", "name").toString();
        p.hashPath = s.value("hashPath", "info_hash").toString();
        p.sizePath = s.value("sizePath", "size").toString();
        p.seedersPath = s.value("seedersPath", "seeders").toString();
        p.leechersPath = s.value("leechersPath", "leechers").toString();
        p.enabled = s.value("enabled", true).toBool();
        p.builtIn = s.value("builtIn", false).toBool();
        m_searchProviders.append(p);
    }
    s.endArray();
}

void AddonManager::saveSearchProviders()
{
    QSettings s("BATorrent", "BATorrent");
    s.beginWriteArray("searchProviders", m_searchProviders.size());
    for (int i = 0; i < m_searchProviders.size(); ++i) {
        s.setArrayIndex(i);
        const auto &p = m_searchProviders[i];
        s.setValue("id", p.id);
        s.setValue("name", p.name);
        s.setValue("urlTemplate", p.urlTemplate);
        s.setValue("arrayPath", p.arrayPath);
        s.setValue("namePath", p.namePath);
        s.setValue("hashPath", p.hashPath);
        s.setValue("sizePath", p.sizePath);
        s.setValue("seedersPath", p.seedersPath);
        s.setValue("leechersPath", p.leechersPath);
        s.setValue("enabled", p.enabled);
        s.setValue("builtIn", p.builtIn);
    }
    s.endArray();
}

QList<SearchProvider> AddonManager::searchProviders() const
{
    return m_searchProviders;
}

void AddonManager::addSearchProvider(const SearchProvider &p)
{
    m_searchProviders.append(p);
    saveSearchProviders();
}

void AddonManager::removeSearchProvider(int index)
{
    if (index < 0 || index >= m_searchProviders.size()) return;
    m_searchProviders.removeAt(index);
    saveSearchProviders();
}

void AddonManager::setSearchProviderEnabled(int index, bool enabled)
{
    if (index < 0 || index >= m_searchProviders.size()) return;
    m_searchProviders[index].enabled = enabled;
    saveSearchProviders();
}

// Size can arrive as raw bytes (apibay/nyaa) or a human string like "28.47 GB"
// (the TorAPI providers scrape display text). Accept both.
static qint64 parseSizeValue(const QJsonValue &v)
{
    if (v.isDouble()) return static_cast<qint64>(v.toDouble());
    QString s = v.toString().trimmed();
    s.replace(QChar(0x00A0), QLatin1Char(' '));   // TorAPI separates number/unit with NBSP
    if (s.isEmpty()) return 0;
    bool ok = false;
    const qint64 plain = s.toLongLong(&ok);
    if (ok) return plain;
    static const QRegularExpression re(
        QStringLiteral("([\\d.,]+)\\s*([KMGT]?)i?B"), QRegularExpression::CaseInsensitiveOption);
    const auto m = re.match(s);
    if (!m.hasMatch()) return 0;
    const double num = m.captured(1).replace(QLatin1Char(','), QLatin1Char('.')).toDouble();
    const QString u = m.captured(2).toUpper();
    const double mult = u == QLatin1String("K") ? 1024.0
                      : u == QLatin1String("M") ? 1024.0 * 1024
                      : u == QLatin1String("G") ? 1024.0 * 1024 * 1024
                      : u == QLatin1String("T") ? 1024.0 * 1024 * 1024 * 1024 : 1.0;
    return static_cast<qint64>(num * mult);
}

QList<TorrentSearchResult> AddonManager::parseProviderResponse(
    const SearchProvider &p, const QByteArray &data)
{
    QJsonDocument doc = QJsonDocument::fromJson(data);

    QJsonArray arr;
    if (p.arrayPath.isEmpty()) {
        if (doc.isArray()) arr = doc.array();
    } else {
        QJsonObject root = doc.object();
        QJsonValue v = root.value(p.arrayPath);
        if (v.isArray()) arr = v.toArray();
    }

    const QString trackerParams = magnetTrackerParams();

    QList<TorrentSearchResult> results;
    for (const auto &val : arr) {
        QJsonObject obj = val.toObject();
        QString name = obj.value(p.namePath).toString();
        QString infoHash = obj.value(p.hashPath).toString();
        if (infoHash.isEmpty() || infoHash == "0") continue;

        TorrentSearchResult r;
        r.name = name;
        r.infoHash = infoHash;
        r.size = parseSizeValue(obj.value(p.sizePath));
        r.seeders = obj.value(p.seedersPath).toVariant().toInt();
        r.leechers = obj.value(p.leechersPath).toVariant().toInt();
        r.category = obj.value("category").toString();
        r.provider = p.name;
        r.magnet = QString("magnet:?xt=urn:btih:%1&dn=%2%3")
            .arg(infoHash, QUrl::toPercentEncoding(name), trackerParams);
        results.append(r);
    }
    return results;
}

void AddonManager::searchWithProvider(int providerIndex, const QString &query, int category)
{
    if (providerIndex < 0 || providerIndex >= m_searchProviders.size()) {
        emit torrentSearchFinished();
        return;
    }
    const SearchProvider &p = m_searchProviders[providerIndex];
    if (!p.enabled || p.urlTemplate.isEmpty()) {
        emit torrentSearchFinished();
        return;
    }

    QString url = p.urlTemplate;
    url.replace("{query}", QUrl::toPercentEncoding(query));
    url.replace("{category}", QString::number(category));

    QNetworkRequest req{QUrl(url)};
    req.setHeader(QNetworkRequest::UserAgentHeader, "BATorrent/" APP_VERSION);
    req.setTransferTimeout(15000);   // a slow/dead provider must not hang the search UI
    auto *reply = m_net->get(req);

    connect(reply, &QNetworkReply::finished, this, [this, reply, p]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit torrentSearchError(reply->errorString());
            emit torrentSearchFinished();
            return;
        }
        auto results = parseProviderResponse(p, reply->readAll());
        emit torrentSearchResults(results);
        emit torrentSearchFinished();
    });
}
