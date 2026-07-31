// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details
//
// QmlSearchBridge — core: ctor, search entry, sources/results getters.

#include "bridges/search/qmlsearchbridge.h"
#include "bridges/search/qmlsearchbridge_util.h"
#include "torrent/iengine.h"
#include "services/metadata/audiomode.h"
#include "services/metadata/episodegroup.h"
#include "services/metadata/metadataresolver.h"
#include "services/discovery/discoveryservice.h"
#include "services/downloads/httpdownloadmanager.h"
#include "services/downloads/filehostresolver.h"
#include "services/metadata/nameparser.h"
#include "services/metadata/releasegroup.h"
#include "services/metadata/releasepick.h"
#include "services/metadata/gamereleasepick.h"
#include "services/metadata/searchranker.h"
#include "services/metadata/releasetrust.h"
#include "services/integrations/rssmanager.h"
#include "services/discovery/addonmanager.h"
#include "services/platform/utils.h"
#include "services/platform/contentlanguage.h"
#include "services/platform/translator.h"

#include <QClipboard>
#include <QGuiApplication>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QRegularExpression>
#include <QSettings>
#include <QStandardPaths>
#include <QStorageInfo>
#include <QDir>
#include <QUrl>
#include <algorithm>

QmlSearchBridge::QmlSearchBridge(IEngine *session, QObject *parent)
    : QObject(parent), m_session(session), m_mode("torrent")
{
    auto &mgr = AddonManager::instance();
    connect(&mgr, &AddonManager::catalogResults, this, [this](const QList<CatalogItem> &items) {
        m_catalogCache = items;
        if (m_mode != "catalog") return;
        rebuildCatalogRows();
    });
    connect(&mgr, &AddonManager::catalogFinished, this, [this]() {
        setSearching(false);
        setStatus(tr_("search_results_n").arg(m_catalogCache.size()));
    });
    connect(&mgr, &AddonManager::streamResults, this, [this](const QList<StreamResult> &streams) {
        m_streamCache = streams;
        if (m_mode != "streams") return;
        m_results.clear();
        for (const auto &s : streams) {
            QVariantMap m;
            m["name"] = s.title;
            m["sub"] = s.addonName;
            m["provider"] = s.addonName;
            m["sizeStr"] = s.size > 0 ? formatSize(s.size) : QString();
            m["seeds"] = ""; m["leech"] = ""; m["releaseGroup"] = "";
            m["poster"] = m_streamHintPoster; m["coverHash"] = "";
            m["quality"] = s.quality;
            m["seedsN"] = 0; m["sizeBytes"] = s.size;
            fillMediaAttrs(m, s.title);
            fillTrust(m, s.title);
            m_results << m;
        }
        emit resultsChanged();
    });
    connect(&mgr, &AddonManager::streamFinished, this, [this]() {
        setSearching(false);
        setStatus(tr_("search_streams_n").arg(m_streamCache.size()));
    });
    connect(&mgr, &AddonManager::metaVideos, this, [this](const QString &id, const QVariantList &videos) {
        if (m_mode != "episodes" || id != m_epId) return;   // stale or user moved on
        setSearching(false);
        if (videos.isEmpty()) {   // no episode meta → old bare-id lookup is better than nothing
            setMode("streams");
            auto &am = AddonManager::instance();
            if (!am.hasStreamAddon()) { setStatus(tr_("search_no_stream_addon")); return; }
            setSearching(true);
            setStatus(tr_("search_loading_streams_from").arg(m_streamHintTitle));
            am.getStreams(m_epType, m_epId);
            return;
        }
        m_episodeCache = videos;
        showEpisodeRows();
    });
    connect(&mgr, &AddonManager::torrentSearchResults, this,
            [this](const QList<TorrentSearchResult> &results) {
        if (m_mode != "torrent" && m_mode != "games" && m_mode != "all") return;
        if (!m_aggregate) {   // single source replaces; aggregate appends each batch
            m_results.clear(); m_resultMagnets.clear(); m_resultHttp.clear(); m_torrentCache.clear();
        }
        appendTorrentRows(results);
    });
    connect(&mgr, &AddonManager::torrentSearchFinished, this, [this]() {
        if (m_aggregate) { finishAggregateSource(); return; }
        setSearching(false);
        setStatus(tr_("search_results_n").arg(m_results.size()));
    });
    connect(&mgr, &AddonManager::torrentSearchError, this, [this](const QString &err) {
        if (m_aggregate) { finishAggregateSource(); return; }   // one provider failing ≠ whole search
        setSearching(false);
        setStatus(err);
    });

    connect(&mgr, &AddonManager::torrentSummaryReady, this,
            [this](const QString &query, int count, qint64 bestSize, int maxSeeds) {
        const QString key = query.toLower().trimmed();
        m_srcSummaryInFlight.remove(key);
        QVariantList v; v << count << QVariant::fromValue(bestSize) << maxSeeds;
        m_srcSummaryCache.insert(key, v);
        emit sourceSummary(query, count, bestSize, maxSeeds);
    });

    connect(&GameSourceManager::instance(), &GameSourceManager::refreshed, this, [this](int count) {
        emit gameSourcesChanged();
        if (m_pendingGameQuery.isEmpty()) return;
        const QString q = m_pendingGameQuery;
        m_pendingGameQuery.clear();
        if (m_aggregate) {
            if (count > 0) appendGameRows(GameSourceManager::instance().search(q));
            finishAggregateSource();
            return;
        }
        if (m_mode != "games") return;
        if (count > 0) runGameSearch(q);
        else { setSearching(false); setStatus(tr_("search_no_games")); }
    });

    QSettings s;
    m_savePath = s.value(QStringLiteral("lastSavePath")).toString();
    if (m_savePath.isEmpty() || !QDir(m_savePath).exists())
        m_savePath = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
}

void QmlSearchBridge::copyMagnet(int index)
{
    if (index < 0 || index >= m_resultMagnets.size()) return;
    const QString magnet = m_resultMagnets[index];
    if (magnet.isEmpty()) return;
    QGuiApplication::clipboard()->setText(magnet);
    setStatus(tr_("search_magnet_copied"));
}

QString QmlSearchBridge::magnetAt(int index) const
{
    if (index < 0 || index >= m_resultMagnets.size()) return {};
    return m_resultMagnets[index];
}

void QmlSearchBridge::searchRaw()
{
    if (m_titleQuery.isEmpty()) return;
    // keep the title context so "back" still returns to the titles grid
    m_fromTitles = !m_titleCache.isEmpty();
    rawAggregateSearch(m_titleQuery, 0);
}

void QmlSearchBridge::rawAggregateSearch(const QString &q, int categoryCode)
{
    m_results.clear();
    m_resultMagnets.clear();
    m_resultTitles.clear();
    m_resultHttp.clear();
    m_torrentCache.clear();
    m_gameCache.clear();
    m_pendingGameQuery.clear();
    emit resultsChanged();

    m_activeQuery = q;
    clearWorkContext();
    auto &mgr = AddonManager::instance();
    m_aggregate = true;
    m_titleSources = false;         // raw mixed list → keep per-row covers
    m_isGameSearch = false;
    setMode("all");
    setSearching(true);
    setStatus(tr_("search_searching2"));
    m_pendingSources = 0;
    auto &gsm = GameSourceManager::instance();
    if (gsm.gameCount() > 0) {
        appendGameRows(gsm.search(q));
    } else if (!gsm.sources().isEmpty()) {
        m_pendingGameQuery = q;          // catalogs load async; counts as a pending source
        ++m_pendingSources;
        gsm.refresh();
    }
    const auto providers = mgr.searchProviders();
    for (int i = 0; i < providers.size(); ++i)
        if (providers[i].enabled) { ++m_pendingSources; mgr.searchWithProvider(i, q); }
    if (mgr.torrentSearchEnabled()) { ++m_pendingSources; mgr.searchTorrents(q, categoryCode); }
    if (m_pendingSources == 0) {
        setSearching(false);
        setStatus(tr_("search_results_n").arg(m_results.size()));
    }
}

QVariantList QmlSearchBridge::sources() const
{
    QVariantList out;
    auto add = [&out](const QString &key, const QString &label) {
        QVariantMap m; m["key"] = key; m["label"] = label; out << m;
    };
    add("all", tr_("search_source_all"));                  // default: search every source at once
    add("stremio", tr_("search_source_stremio"));
    auto &mgr = AddonManager::instance();
    if (mgr.torrentSearchEnabled())
        add("legacy", tr_("search_source_torrents"));
    // Games search is independent of the torrent provider: show it whenever a
    // game catalog is configured (a default is seeded on first run), or as a
    // fallback when the torrent provider is on (TPB Games category).
    if (!GameSourceManager::instance().sources().isEmpty() || mgr.torrentSearchEnabled())
        add("games", tr_("search_source_games"));
    const auto providers = mgr.searchProviders();
    for (int i = 0; i < providers.size(); ++i)
        if (providers[i].enabled)
            add(QString("provider:%1").arg(i), providers[i].name);
    return out;
}

QVariantList QmlSearchBridge::categories() const
{
    QVariantList out;
    auto add = [&out](int code, const QString &label) {
        QVariantMap m; m["code"] = code; m["label"] = label; out << m;
    };
    add(0, tr_("search_cat_all")); add(100, tr_("search_cat_audio")); add(200, tr_("search_cat_video"));
    add(300, tr_("search_cat_apps")); add(400, tr_("search_cat_games")); add(500, tr_("search_cat_other"));
    return out;
}

QVariantList QmlSearchBridge::results() const
{
    // Stamp each row's index into the data itself. QML used to add `_idx` by
    // mutating the map (o._idx = i), but a QVariantMap handed to QML is a copy —
    // the mutation didn't always stick, leaving srcIndex undefined and breaking
    // activateResult()/openDetail() ("no source" on every pick).
    QVariantList out;
    out.reserve(m_results.size());
    for (int i = 0; i < m_results.size(); ++i) {
        QVariantMap m = m_results.at(i).toMap();
        m[QStringLiteral("_idx")] = i;
        out.append(m);
    }
    return out;
}
QString QmlSearchBridge::activeQuery() const { return m_activeQuery; }
QString QmlSearchBridge::mode() const { return m_mode; }
bool QmlSearchBridge::inStreams() const { return m_mode == "streams"; }
bool QmlSearchBridge::canGoBack() const { return m_mode == "streams" || m_mode == "episodes" || m_fromTitles; }
bool QmlSearchBridge::singleTitleView() const { return m_titleSources || m_mode == "streams" || m_mode == "episodes"; }
bool QmlSearchBridge::searching() const { return m_searching; }
QString QmlSearchBridge::statusText() const { return m_status; }

void QmlSearchBridge::setSearching(bool on) { if (m_searching == on) return; m_searching = on; emit searchingChanged(); }
void QmlSearchBridge::setStatus(const QString &s) { if (m_status == s) return; m_status = s; emit statusChanged(); }
void QmlSearchBridge::setMode(const QString &m) { if (m_mode == m) return; m_mode = m; emit modeChanged(); }

void QmlSearchBridge::refreshSources() { emit sourcesChanged(); }

void QmlSearchBridge::search(const QString &sourceKey, const QString &query, int categoryCode)
{
    const QString q = query.trimmed();
    if (q.isEmpty()) return;
    m_lastQuery = q;
    m_activeQuery = q;
    m_aggregate = false;
    m_titleSources = false;
    m_fromEpisodes = false;
    clearWorkContext();
    m_pendingGameQuery.clear();
    m_results.clear();
    m_resultMagnets.clear();
    m_resultTitles.clear();
    m_resultHttp.clear();
    m_torrentCache.clear();
    m_gameCache.clear();
    emit resultsChanged();

    auto &mgr = AddonManager::instance();
    if (sourceKey == "all") {
        // Title-first: resolve the query to real works (TMDB/IGDB), then let the
        // user drill into one title's torrents. Only when a metadata service with
        // keys is available — otherwise go straight to the flat aggregate.
        if (!m_discovery || !m_discovery->hasMetadataKeys()) {
            rawAggregateSearch(q, categoryCode);
            return;
        }
        m_fromTitles = false;
        m_titleCache.clear();
        m_titleQuery = q;
        setMode("titles");
        setSearching(true);
        setStatus(tr_("search_searching_titles"));
        m_discovery->searchTitles(q);
        return;
    } else if (sourceKey == "games") {
        m_isGameSearch = true;
        setMode("games");
        auto &gsm = GameSourceManager::instance();
        if (gsm.gameCount() == 0 && !gsm.sources().isEmpty()) {
            m_pendingGameQuery = q;          // search once the catalogs finish loading
            setSearching(true);
            setStatus(tr_("search_loading_game_catalogs"));
            gsm.refresh();
            return;
        }
        if (gsm.gameCount() > 0) { runGameSearch(q); return; }
        // No game catalogs configured → fall back to the bundled torrent provider's
        // Games category so the search isn't empty out of the box.
        m_gameCache.clear();
        setSearching(true);
        setStatus(tr_("search_searching2"));
        mgr.searchTorrents(q, 400);
    } else if (sourceKey.startsWith("provider:")) {
        m_isGameSearch = false;
        setMode("torrent");
        setSearching(true);
        setStatus(tr_("search_searching2"));
        mgr.searchWithProvider(sourceKey.mid(9).toInt(), q);
    } else if (sourceKey == "legacy") {
        m_isGameSearch = false;
        setMode("torrent");
        setSearching(true);
        setStatus(tr_("search_searching2"));
        mgr.searchTorrents(q, categoryCode);
    } else {
        if (!mgr.hasCatalogAddon()) { setStatus(tr_("search_no_catalog_addon")); return; }
        setMode("catalog");
        setSearching(true);
        setStatus(tr_("search_searching2"));
        mgr.searchCatalog(q);
    }
}

