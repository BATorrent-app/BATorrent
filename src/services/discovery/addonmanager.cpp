// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "services/discovery/addonmanager.h"
#include "services/platform/translator.h"
#include "services/platform/utils.h"
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QRegularExpression>
#include <QSettings>
#include <QUrl>
#include <algorithm>

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

bool AddonManager::hasMetaAddon() const
{
    for (const auto &a : m_addons)
        if (a.enabled && a.resources.contains("meta")) return true;
    return false;
}

// Stremio meta: GET {url}/meta/{type}/{id}.json → meta.videos = the episode list
void AddonManager::fetchMeta(const QString &type, const QString &id)
{
    const quint32 gen = ++m_metaGen;
    const AddonManifest *chosen = nullptr;
    for (const auto &addon : m_addons) {
        if (!addon.enabled || !addon.resources.contains("meta")) continue;
        if (!addon.types.contains(type)) continue;
        chosen = &addon;
        break;
    }
    if (!chosen) { emit metaVideos(id, {}); return; }

    QNetworkRequest req{QUrl(QString("%1/meta/%2/%3.json").arg(chosen->url, type, id))};
    req.setHeader(QNetworkRequest::UserAgentHeader, "BATorrent/1.9");
    req.setTransferTimeout(12000);
    auto *reply = m_net->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, gen, id]() {
        reply->deleteLater();
        if (gen != m_metaGen) return; // stale reply, ignore

        QVariantList videos;
        if (reply->error() == QNetworkReply::NoError) {
            const QJsonArray arr = QJsonDocument::fromJson(reply->readAll())
                                       .object().value("meta").toObject()
                                       .value("videos").toArray();
            for (const auto &val : arr) {
                const QJsonObject v = val.toObject();
                const QString videoId = v.value("id").toString();
                if (videoId.isEmpty()) continue;
                QVariantMap ep;
                ep["videoId"] = videoId;
                ep["season"] = v.value("season").toInt();
                // Cinemeta uses "episode"; some addons use "number"
                ep["episode"] = v.contains("episode") ? v.value("episode").toInt()
                                                      : v.value("number").toInt();
                ep["name"] = decodeHtmlEntities(v.value("name").toString(v.value("title").toString()));
                ep["released"] = v.value("released").toString().left(10);
                videos << ep;
            }
            std::sort(videos.begin(), videos.end(), [](const QVariant &a, const QVariant &b) {
                const QVariantMap ma = a.toMap(), mb = b.toMap();
                const int sa = ma.value("season").toInt(), sb = mb.value("season").toInt();
                if (sa != sb) return sa < sb;
                return ma.value("episode").toInt() < mb.value("episode").toInt();
            });
        }
        emit metaVideos(id, videos);
    });
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
                        item.name = decodeHtmlEntities(m.value("name").toString());
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

QString AddonManager::streamBaseUrl(const QString &addonUrl, const QString &torrentioLang)
{
    if (torrentioLang.isEmpty()) return addonUrl;
    if (!QUrl(addonUrl).host().contains(QLatin1String("torrentio"))) return addonUrl;
    if (addonUrl.contains(QLatin1Char('='))) return addonUrl;   // user already configured it
    return addonUrl + QLatin1String("/language=") + torrentioLang;
}

namespace {
// Torrentio's `language=` values for the app UI languages. English stays
// unconfigured — the global default already is English-first.
QString torrentioLanguageForApp()
{
    if (!QSettings().value("preferNativeLang", true).toBool()) return {};
    switch (Translator::instance().language()) {
    case Translator::Portuguese: return QStringLiteral("portuguese");
    case Translator::Spanish:    return QStringLiteral("spanish");
    case Translator::Russian:    return QStringLiteral("russian");
    case Translator::Japanese:   return QStringLiteral("japanese");
    case Translator::Chinese:    return QStringLiteral("chinese");
    case Translator::German:     return QStringLiteral("german");
    case Translator::Ukrainian:  return QStringLiteral("ukrainian");
    case Translator::Turkish:    return QStringLiteral("turkish");
    case Translator::English:    break;
    }
    return {};
}
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
            .arg(streamBaseUrl(addon.url, torrentioLanguageForApp()), type, id);

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
                    r.title = decodeHtmlEntities(s.value("title").toString());
                    if (r.title.isEmpty())
                        r.title = decodeHtmlEntities(s.value("name").toString());

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
            QString name = decodeHtmlEntities(obj.value("name").toString());
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

void AddonManager::summarizeTorrents(const QString &query, int category)
{
    // Prefer the default search providers (apibay etc.) — enabled out of the box,
    // so the hero summary works without flipping the legacy "torrent search" on.
    // Falls back to the legacy single-URL endpoint. Stays off the search UI signals.
    int provIdx = -1;
    for (int i = 0; i < m_searchProviders.size(); ++i)
        if (m_searchProviders[i].enabled && !m_searchProviders[i].urlTemplate.isEmpty()) { provIdx = i; break; }

    QString url;
    SearchProvider prov;
    bool useProvider = false;
    if (provIdx >= 0) {
        prov = m_searchProviders[provIdx];
        url = prov.urlTemplate;
        url.replace("{query}", QUrl::toPercentEncoding(query));
        url.replace("{category}", QString::number(category));
        useProvider = true;
    } else if (m_torrentSearchEnabled && !m_torrentSearchUrl.isEmpty()) {
        QString baseUrl = m_torrentSearchUrl;
        if (baseUrl.endsWith('/')) baseUrl.chop(1);
        url = QString("%1/q.php?q=%2&cat=%3").arg(baseUrl, QUrl::toPercentEncoding(query), QString::number(category));
    } else {
        emit torrentSummaryReady(query, 0, 0, 0);
        return;
    }

    QNetworkRequest req{QUrl(url)};
    req.setHeader(QNetworkRequest::UserAgentHeader, "BATorrent/2.0");
    req.setTransferTimeout(12000);
    auto *reply = m_net->get(req);

    connect(reply, &QNetworkReply::finished, this, [this, reply, query, prov, useProvider]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) { emit torrentSummaryReady(query, 0, 0, 0); return; }
        const QByteArray data = reply->readAll();

        QList<TorrentSearchResult> results;
        if (useProvider) {
            results = parseProviderResponse(prov, data);   // handles each provider's JSON shape
        } else {
            const QJsonDocument doc = QJsonDocument::fromJson(data);
            if (doc.isArray())
                for (const auto &val : doc.array()) {
                    const QJsonObject obj = val.toObject();
                    const QString ih = obj.value("info_hash").toString();
                    if (ih.isEmpty() || ih == "0") continue;
                    TorrentSearchResult r;
                    r.size = obj.value("size").toVariant().toLongLong();
                    r.seeders = obj.value("seeders").toVariant().toInt();
                    results.append(r);
                }
        }

        int count = 0, maxSeeds = -1;
        qint64 bestSize = 0;   // size of the healthiest (most-seeded) release
        for (const auto &r : results) {
            ++count;
            if (r.seeders > maxSeeds) { maxSeeds = r.seeders; bestSize = r.size; }
        }
        emit torrentSummaryReady(query, count, bestSize, qMax(0, maxSeeds));
    });
}

// --- Search Providers ---

void AddonManager::installDefaultProviders()
{
    QSettings s("BATorrent", "BATorrent");

    // Only the safe public-JSON, global sources ship enabled by default. Every
    // localized source lives in providerCatalog() so the user opts in per region.
    struct Def { QString id, name, url, arr, nm, hash, sz, seed, leech, region; bool enabled; };
    const QList<Def> defaults = {
        {"apibay", "The Pirate Bay (apibay)",
         "https://apibay.org/q.php?q={query}&cat={category}",
         "", "name", "info_hash", "size", "seeders", "leechers", "global", true},
        {"nyaa_api", "Nyaa.si",
         "https://nyaa.si/api/v2?q={query}&limit=50",
         "torrents", "title", "info_hash", "total_size", "seeders", "leechers", "anime", true},
        // Open torrent database (no login), returns raw-byte sizes + real swarm
        // counts. Global content; safe to enable like apibay/nyaa.
        {"torrents_csv", "Torrents-CSV",
         "https://torrents-csv.com/service/search?q={query}&size=50",
         "torrents", "name", "infohash", "size_bytes", "seeders", "leechers", "global", true},
        // BitSearch: login-free multi-tracker aggregator (TPB/1337x/YTS/nyaa across
        // languages). results[] with infohash → magnet built like apibay. ~200
        // req/day per IP anon; that's per-search, generous for normal use.
        {"bitsearch", "BitSearch (multi-idioma)",
         "https://bitsearch.eu/api/v1/search?q={query}&limit=50",
         "results", "title", "infohash", "size", "seeders", "leechers", "global", true},
    };

    // Seed each built-in once (tracked by id) so a new provider reaches existing
    // users on update, while one the user deleted never resurrects.
    QStringList seeded = s.value("seededProviderIds").toStringList();
    if (seeded.isEmpty() && s.value("searchProvidersInitialized", false).toBool())
        seeded << QStringLiteral("apibay") << QStringLiteral("nyaa_api");  // pre-seed-flag users
    // Migration: the CIS/Jackett presets used to seed here (off). They now live in
    // the catalog only — mark them seeded so they don't resurrect as dead rows,
    // but keep any the user actually enabled.
    for (const char *legacy : { "rutor_torapi", "rutracker_torapi", "jackett_local" })
        if (!seeded.contains(QLatin1String(legacy))) seeded << QLatin1String(legacy);
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
            p.enabled = d.enabled; p.builtIn = true; p.region = d.region;
            m_searchProviders.append(p);
        }
    }
    s.setValue("searchProvidersInitialized", true);
    s.setValue("seededProviderIds", seeded);
    if (changed) saveSearchProviders();
}

QList<ProviderPreset> AddonManager::providerCatalog()
{
    auto torApi = [](const QString &id, const QString &name, const QString &tracker,
                     const QString &region, const QString &note, bool selfHost = false) {
        ProviderPreset ps;
        SearchProvider &p = ps.provider;
        p.id = id;
        p.name = name;
        // Lifailon/TorAPI exposes each tracker under /api/search/title/<tracker>;
        // it scrapes server-side and returns the info_hash, so no login is needed
        // for the public trackers. The default instance is editable to self-host.
        p.urlTemplate = QStringLiteral("https://torapi.vercel.app/api/search/title/%1?query={query}").arg(tracker);
        p.arrayPath = QString();
        p.namePath = QStringLiteral("Name");
        p.hashPath = QStringLiteral("Hash");
        p.sizePath = QStringLiteral("Size");
        p.seedersPath = QStringLiteral("Seeds");
        p.leechersPath = QStringLiteral("Peers");
        p.builtIn = true;
        p.region = region;
        ps.note = note;
        ps.needsConfig = selfHost;
        return ps;
    };
    auto json = [](const QString &id, const QString &name, const QString &url,
                   const QString &arr, const QString &nm, const QString &hash,
                   const QString &sz, const QString &seed, const QString &leech,
                   const QString &region, const QString &note, bool needsConfig = false) {
        ProviderPreset ps;
        SearchProvider &p = ps.provider;
        p.id = id; p.name = name; p.urlTemplate = url;
        p.arrayPath = arr; p.namePath = nm; p.hashPath = hash;
        p.sizePath = sz; p.seedersPath = seed; p.leechersPath = leech;
        p.builtIn = true; p.region = region;
        ps.note = note; ps.needsConfig = needsConfig;
        return ps;
    };

    QList<ProviderPreset> cat;

    // Only GET endpoints with a {query} template fit searchWithProvider — POST-body
    // aggregators (Knaben v1) can't be presets here; Jackett covers that ground.

    // ── PT-BR (filmes/séries — via torrent-indexer self-host) ────────────────
    // felipemarinho97/torrent-indexer scrapes Comando/BluDV/Torrent dos Filmes/etc.
    // and returns info_hash + magnet as JSON. No public instance → self-host (Docker).
    {
        ProviderPreset ps;
        SearchProvider &p = ps.provider;
        p.id = QStringLiteral("torrentindexer_ptbr");
        p.name = QStringLiteral("Comando/BluDV… (torrent-indexer)");
        p.urlTemplate = QStringLiteral("http://127.0.0.1:7006/search?q={query}");
        p.arrayPath = QStringLiteral("results");
        p.namePath = QStringLiteral("title");
        p.hashPath = QStringLiteral("info_hash");
        p.sizePath = QStringLiteral("size");
        p.seedersPath = QStringLiteral("seed_count");
        p.leechersPath = QStringLiteral("leech_count");
        p.builtIn = true;
        p.region = QStringLiteral("ptbr");
        ps.note = QStringLiteral("Filmes/séries PT-BR. Exige rodar o torrent-indexer localmente (Docker) — edite a URL.");
        ps.needsConfig = true;
        cat << ps;
    }

    // ── CIS / Russo (via TorAPI) ─────────────────────────────────────────────
    // Only RuTor returns the info_hash in *title* search (verified); Kinozal/
    // NNM-Club expose it only in per-ID detail lookups, which searchWithProvider
    // doesn't do — so they'd yield magnet-less rows and are deliberately omitted.
    cat << torApi(QStringLiteral("rutor_torapi"), QStringLiteral("RuTor"), QStringLiteral("rutor"),
                  QStringLiteral("cis"),
                  QStringLiteral("Tracker russo público. Consulta via instância TorAPI de terceiros (editável)."));
    cat << torApi(QStringLiteral("rutracker_torapi"), QStringLiteral("RuTracker (self-host)"),
                  QStringLiteral("rutracker"), QStringLiteral("cis"),
                  QStringLiteral("Exige login → só funciona apontando para uma TorAPI própria com sua conta."),
                  /*selfHost*/ true);

    // ── Localizado via Jackett (cobre BluDV/Comando PT-BR, ES, etc.) ─────────
    cat << json(QStringLiteral("jackett_local"), QStringLiteral("Jackett (todos os seus indexers)"),
                QStringLiteral("http://127.0.0.1:9117/api/v2.0/indexers/all/results?apikey=API_KEY&Query={query}"),
                QStringLiteral("Results"), QStringLiteral("Title"), QStringLiteral("InfoHash"),
                QStringLiteral("Size"), QStringLiteral("Seeders"), QStringLiteral("Peers"),
                QStringLiteral("self"),
                QStringLiteral("Uma fonte cobre TODOS os indexers localizados do seu Jackett (PT-BR, ES…). Troque API_KEY."),
                /*needsConfig*/ true);

    return cat;
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
        p.region = s.value("region", "global").toString();
        p.note = s.value("note").toString();
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
        s.setValue("region", p.region);
        s.setValue("note", p.note);
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

void AddonManager::setSearchProviderUrl(int index, const QString &urlTemplate)
{
    if (index < 0 || index >= m_searchProviders.size() || urlTemplate.trimmed().isEmpty()) return;
    m_searchProviders[index].urlTemplate = urlTemplate.trimmed();
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
        QString name = decodeHtmlEntities(obj.value(p.namePath).toString());
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
