// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "services/discovery/addonmanager.h"
#include "services/discovery/addoncatalog.h"
#include "services/discovery/addonparse.h"
#include "services/platform/contentlanguage.h"
#include "services/platform/translator.h"
#include "services/platform/utils.h"
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QSettings>
#include <QUrl>
#include <algorithm>

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
    // Seed gen bumps install missing curated free addons for existing users.
    // 4 = broad out-of-box: core + Brazuca + Anime Kitsu + Nyaa pack + all lang packs
    //     (regional packs installed; enabled intelligently via syncCuratedAddons).
    constexpr int kAddonSeedGen = 4;
    QSettings settings("BATorrent", "BATorrent");
    const int have = settings.value(QStringLiteral("addonCatalogSeed"), 0).toInt();
    if (have < kAddonSeedGen) {
        settings.setValue(QStringLiteral("addonsInitialized"), true);
        syncCuratedAddons();
        settings.setValue(QStringLiteral("addonCatalogSeed"), kAddonSeedGen);
    }
}

void AddonManager::syncCuratedAddons()
{
    const QString contentLang = []() -> QString {
        switch (ContentLanguage::current()) {
        case Translator::Portuguese: return QStringLiteral("pt");
        case Translator::Spanish:    return QStringLiteral("es");
        case Translator::Russian:    return QStringLiteral("ru");
        case Translator::Chinese:    return QStringLiteral("zh");
        case Translator::Japanese:   return QStringLiteral("ja");
        case Translator::German:     return QStringLiteral("de");
        case Translator::Turkish:    return QStringLiteral("tr");
        case Translator::Ukrainian:  return QStringLiteral("uk");
        case Translator::English:    break;
        }
        return QStringLiteral("en");
    }();

    bool changed = false;
    for (const auto &d : curatedCatalog()) {
        if (d.needsConfig || d.url.isEmpty() || !d.seedDefault)
            continue;

        int idx = -1;
        for (int i = 0; i < m_addons.size(); ++i) {
            if (m_addons[i].url == d.url || (!d.id.isEmpty() && m_addons[i].id == d.id)) {
                idx = i;
                break;
            }
        }
        if (idx < 0) {
            AddonManifest m;
            m.id = d.id;
            m.name = d.name;
            m.description = d.description;
            m.url = d.url;
            m.types = d.types;
            m.resources = d.resources;
            m.enabled = true;
            m_addons.append(m);
            idx = m_addons.size() - 1;
            changed = true;
        }

        const bool wantOn = d.alwaysOn
                            || d.lang.isEmpty()
                            || d.lang == QLatin1String("anime")
                            || d.lang == contentLang;
        if (m_addons[idx].enabled != wantOn) {
            m_addons[idx].enabled = wantOn;
            changed = true;
        }
    }
    if (changed)
        saveAddons();
}

QList<CuratedAddon> AddonManager::curatedCatalog()
{
    return AddonCatalog::curatedCatalog();
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
    QString baseUrl = AddonParse::normalizeAddonBaseUrl(url);
    if (baseUrl.isEmpty()) {
        emit addonError(QStringLiteral("Invalid addon URL."));
        return;
    }

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

        AddonManifest m;
        if (!AddonParse::parseManifestJson(reply->readAll(), url, &m)) {
            emit addonError("Invalid manifest format.");
            return;
        }

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

void AddonManager::searchCatalog(const QString &query)
{
    m_catalogResults.clear();
    m_pendingCatalog = 0;
    const quint32 gen = ++m_catalogGen;

    for (const auto &addon : m_addons) {
        if (!addon.enabled || !addon.resources.contains("catalog"))
            continue;

        for (const auto &type : addon.types) {
            m_pendingCatalog++;
            QString searchUrl = QString("%1/catalog/%2/top/search=%3.json")
                .arg(addon.url, type, QUrl::toPercentEncoding(query));

            QNetworkRequest req{QUrl(searchUrl)};
            req.setHeader(QNetworkRequest::UserAgentHeader, "BATorrent/1.9");
            auto *reply = m_net->get(req);

            connect(reply, &QNetworkReply::finished, this, [this, reply, gen]() {
                reply->deleteLater();
                if (gen != m_catalogGen) return;
                m_pendingCatalog--;

                if (reply->error() == QNetworkReply::NoError) {
                    for (const auto &item : AddonParse::parseCatalogMetas(reply->readAll())) {
                        bool dup = false;
                        for (const auto &existing : m_catalogResults) {
                            if (existing.id == item.id) { dup = true; break; }
                        }
                        if (!dup)
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
    return AddonParse::streamBaseUrl(addonUrl, torrentioLang);
}

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
            .arg(streamBaseUrl(addon.url, AddonParse::torrentioLanguageForApp()), type, id);

        QNetworkRequest req{QUrl(streamUrl)};
        req.setHeader(QNetworkRequest::UserAgentHeader, "BATorrent/1.9");
        auto *reply = m_net->get(req);

        connect(reply, &QNetworkReply::finished, this, [this, reply, gen, addonName = addon.name]() {
            reply->deleteLater();
            if (gen != m_streamGen) return;
            m_pendingStreams--;

            if (reply->error() == QNetworkReply::NoError) {
                for (const auto &r : AddonParse::parseStreamResults(reply->readAll(), addonName))
                    m_streamResults.append(r);
            }

            emit streamResults(m_streamResults);
            if (m_pendingStreams <= 0)
                emit streamFinished();
        });
    }

    if (m_pendingStreams == 0)
        emit streamFinished();
}

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
    req.setTransferTimeout(15000);
    auto *reply = m_net->get(req);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit torrentSearchError(reply->errorString());
            emit torrentSearchFinished();
            return;
        }

        const QByteArray data = reply->readAll();
        if (!QJsonDocument::fromJson(data).isArray()) {
            emit torrentSearchError(tr("Invalid response format."));
            emit torrentSearchFinished();
            return;
        }

        emit torrentSearchResults(AddonParse::parseApibayArray(data));
        emit torrentSearchFinished();
    });
}

void AddonManager::summarizeTorrents(const QString &query, int category)
{
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
            results = AddonParse::parseProviderResponse(prov, data);
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
        qint64 bestSize = 0;
        for (const auto &r : results) {
            ++count;
            if (r.seeders > maxSeeds) { maxSeeds = r.seeders; bestSize = r.size; }
        }
        emit torrentSummaryReady(query, count, bestSize, qMax(0, maxSeeds));
    });
}

void AddonManager::installDefaultProviders()
{
    QSettings s("BATorrent", "BATorrent");

    QStringList seeded = s.value("seededProviderIds").toStringList();
    if (seeded.isEmpty() && s.value("searchProvidersInitialized", false).toBool())
        seeded << QStringLiteral("apibay") << QStringLiteral("nyaa_api");
    for (const char *legacy : { "rutor_torapi", "rutracker_torapi", "jackett_local" })
        if (!seeded.contains(QLatin1String(legacy))) seeded << QLatin1String(legacy);
    bool changed = false;
    for (const auto &d : AddonCatalog::defaultProviders()) {
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
    return AddonCatalog::providerCatalog();
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
    req.setTransferTimeout(15000);
    auto *reply = m_net->get(req);

    connect(reply, &QNetworkReply::finished, this, [this, reply, p]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit torrentSearchError(reply->errorString());
            emit torrentSearchFinished();
            return;
        }
        auto results = AddonParse::parseProviderResponse(p, reply->readAll());
        emit torrentSearchResults(results);
        emit torrentSearchFinished();
    });
}
