// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details
//
// QmlSearchBridge — work context, covers, drill-down search, activate/back.

#include "bridges/qmlsearchbridge.h"
#include "bridges/qmlsearchbridge_util.h"
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
#include <QStorageInfo>
#include <QUrl>
#include <algorithm>

void QmlSearchBridge::setResolver(MetadataResolver *r)
{
    m_resolver = r;
    if (!m_resolver) return;
    // Poster fills mutate m_results WITHOUT resultsChanged() on purpose: QML
    // treats that signal as "new result set" (closes the detail panel, resets
    // the view); delegates repaint targeted via coverReady instead.
    connect(m_resolver, &MetadataResolver::metadataReady, this,
            [this](const QString &infoHash, const MetadataResult &meta) {
        if (!meta.valid || meta.posterPath.isEmpty()) return;
        for (auto &v : m_results) {
            QVariantMap m = v.toMap();
            if (m.value(QStringLiteral("coverHash")).toString() == infoHash
                && m.value(QStringLiteral("poster")).toString().isEmpty()) {
                m["poster"] = meta.posterPath;
                v = m;
            }
        }
        emit coverReady(infoHash, meta.posterPath);
    });
}

void QmlSearchBridge::resolveCover(int index)
{
    if (!m_resolver || index < 0 || index >= m_results.size()) return;
    const QVariantMap m = m_results[index].toMap();
    if (!m.value(QStringLiteral("poster")).toString().isEmpty()) return;
    const QString hash = m.value(QStringLiteral("coverHash")).toString();
    if (hash.isEmpty()) return;
    if (m_resolver->hasCached(hash)) {
        const auto meta = m_resolver->cached(hash);
        if (meta.valid && !meta.posterPath.isEmpty()) {
            QVariantMap mm = m;
            mm["poster"] = meta.posterPath;
            m_results[index] = mm;
            emit coverReady(hash, meta.posterPath);
        }
        return;
    }
    m_resolver->resolve(hash, m.value(QStringLiteral("name")).toString());
}

void QmlSearchBridge::setWorkContext(const QVariantMap &work)
{
    m_workType = work.value(QStringLiteral("type")).toString();
    m_workTitle = work.value(QStringLiteral("title")).toString();
    if (m_workTitle.isEmpty()) m_workTitle = work.value(QStringLiteral("name")).toString();
    m_workPoster = work.value(QStringLiteral("poster")).toString();
    m_workYear = work.value(QStringLiteral("year")).toString();
    m_workTmdbId = work.value(QStringLiteral("tmdbId")).toInt();
    m_workStills = work.value(QStringLiteral("stills")).toStringList();
    m_workStillsRequested = false;
    emit workChanged();
    emit workStillsChanged();
}

void QmlSearchBridge::clearWorkContext()
{
    if (m_workType.isEmpty() && m_workTitle.isEmpty() && m_workTmdbId == 0 && m_workStills.isEmpty()) return;
    m_workType.clear();
    m_workTitle.clear();
    m_workPoster.clear();
    m_workYear.clear();
    m_workTmdbId = 0;
    m_workStills.clear();
    m_workStillsRequested = false;
    emit workChanged();
    emit workStillsChanged();
}

void QmlSearchBridge::fetchWorkStills()
{
    if (m_workStillsRequested || !m_workStills.isEmpty()) return;   // inline (games) or already asked
    if (m_workTmdbId <= 0 || !m_discovery) return;
    m_workStillsRequested = true;
    m_discovery->fetchBackdrops(m_workTmdbId, m_workType);
}

void QmlSearchBridge::setDiscovery(DiscoveryService *d)
{
    m_discovery = d;
    if (!m_discovery) return;
    connect(m_discovery, &DiscoveryService::backdropsReady, this,
            [this](int tmdbId, const QStringList &urls) {
        if (tmdbId != m_workTmdbId || urls.isEmpty()) return;   // stale reply for a former title
        m_workStills = urls;
        emit workStillsChanged();
    });
    connect(m_discovery, &DiscoveryService::titleResults, this,
            [this](const QString &query, const QVariantList &works) {
        if (query != m_titleQuery || m_mode != QLatin1String("titles")) return;   // stale
        m_results.clear();
        m_resultMagnets.clear();
        m_resultTitles.clear();
    m_resultHttp.clear();
        for (const QVariant &v : works) {
            const QVariantMap w = v.toMap();
            QVariantMap row;
            row["name"]    = w.value(QStringLiteral("title"));
            row["title"]   = w.value(QStringLiteral("title"));
            row["originalTitle"] = w.value(QStringLiteral("originalTitle"));
            row["sub"]     = w.value(QStringLiteral("type"));
            row["sizeStr"] = w.value(QStringLiteral("year"));
            row["year"]    = w.value(QStringLiteral("year"));
            row["type"]    = w.value(QStringLiteral("type"));
            row["poster"]  = w.value(QStringLiteral("poster"));
            row["rating"]  = w.value(QStringLiteral("rating"));
            row["overview"] = w.value(QStringLiteral("overview"));
            row["tmdbId"]  = w.value(QStringLiteral("tmdbId"));
            row["stills"]  = w.value(QStringLiteral("stills"));
            row["coverHash"] = QString();
            row["isTitle"] = true;
            m_results << row;
        }
        m_titleCache = m_results;
        setSearching(false);
        // Stay in the grid even when empty — the page shows an empty state with a
        // "raw results" escape, so the flow is consistent (never silently flips).
        setStatus(m_results.isEmpty() ? tr_("search_no_titles")
                                      : tr_("search_titles_n").arg(m_results.size()));
        emit resultsChanged();
    });
}

void QmlSearchBridge::searchSourcesForWork(const QString &title, const QString &year,
                                           const QString &type, const QString &originalTitle)
{
    m_results.clear();
    m_resultMagnets.clear();
    m_resultTitles.clear();
    m_resultHttp.clear();
    m_torrentCache.clear();
    m_gameCache.clear();
    m_pendingGameQuery.clear();
    emit resultsChanged();

    m_activeQuery = title;
    auto &mgr = AddonManager::instance();
    const bool isGame = (type == QLatin1String("game"));
    m_aggregate = true;
    m_titleSources = true;          // rows are one picked title → page drops per-row covers
    m_isGameSearch = isGame;
    setMode("all");
    setSearching(true);
    setStatus(tr_("search_searching2"));
    m_pendingSources = 0;

    if (isGame) {
        auto &gsm = GameSourceManager::instance();
        if (gsm.gameCount() > 0) appendGameRows(gsm.search(title));
        else if (!gsm.sources().isEmpty()) { m_pendingGameQuery = title; ++m_pendingSources; gsm.refresh(); }
    }
    // A work is released under different names per language, and the uploader
    // picks one: a Portuguese dub is "Shang-Chi e a Lenda dos Dez Anéis", the
    // original is "Shang-Chi and the Legend of the Ten Rings". Since our TMDB
    // requests carry language=, `title` is already localised — so searching it
    // alone found the dubs and missed everything published under the original
    // name (usually the majority, and the better-seeded half). Hunt under both.
    //
    // Capped at two on purpose. Adding TMDB's alternative_titles here would
    // multiply requests per provider, and on a private tracker that is how an
    // account gets banned.
    QStringList titles{ title };
    if (!originalTitle.isEmpty() && !SearchBridgeUtil::sameTitle(originalTitle, title))
        titles << originalTitle;
    m_activeTitles = titles;

    const int cat = isGame ? 400 : 200;   // 400 = games, 200 = video
    const auto providers = mgr.searchProviders();
    for (const QString &t : titles) {
        // movies disambiguate well with a year; games/series search cleaner by title
        const QString q = (type == QLatin1String("movie") && !year.isEmpty())
                        ? t + QLatin1Char(' ') + year : t;
        for (int i = 0; i < providers.size(); ++i)
            if (providers[i].enabled) { ++m_pendingSources; mgr.searchWithProvider(i, q, cat); }
        if (mgr.torrentSearchEnabled()) { ++m_pendingSources; mgr.searchTorrents(q, cat); }
    }
    if (m_pendingSources == 0) {
        setSearching(false);
        setStatus(tr_("search_results_n").arg(m_results.size()));
    }
}


void QmlSearchBridge::activateResult(int index, bool force)
{
    auto &mgr = AddonManager::instance();
    if (m_mode == "titles") {
        if (index < 0 || index >= m_results.size()) return;
        const QVariantMap w = m_results[index].toMap();
        m_fromTitles = true;
        setWorkContext(w);
        searchSourcesForWork(w.value(QStringLiteral("name")).toString(),
                             w.value(QStringLiteral("year")).toString(),
                             w.value(QStringLiteral("type")).toString(),
                             w.value(QStringLiteral("originalTitle")).toString());
        return;
    }
    if (m_mode == "catalog") {
        if (index < 0 || index >= m_catalogCache.size()) return;
        const auto &it = m_catalogCache[index];
        // Carry the catalog item's clean title + type into the stream add, so the
        // cover resolves from Stremio's metadata, not the messy torrent title.
        m_streamHintTitle = it.year > 0 ? QString("%1 %2").arg(it.name).arg(it.year) : it.name;
        m_streamHintType = it.type == QLatin1String("series") ? static_cast<int>(ContentType::Series)
                         : it.type == QLatin1String("movie")  ? static_cast<int>(ContentType::Movie) : -1;
        m_streamHintPoster = it.poster;
        // Series streams need an "id:season:episode" — a bare series id returns
        // nothing from most addons. Route through the episode picker when the
        // addon exposes meta; otherwise keep the old direct lookup.
        if (it.type == QLatin1String("series") && mgr.hasMetaAddon()) {
            m_epType = it.type;
            m_epId = it.id;
            m_fromEpisodes = false;
            setMode("episodes");
            m_results.clear();
            m_episodeCache.clear();
            emit resultsChanged();
            setSearching(true);
            setStatus(tr_("search_loading_episodes"));
            mgr.fetchMeta(it.type, it.id);
            return;
        }
        setMode("streams");
        m_results.clear();
        emit resultsChanged();
        if (!mgr.hasStreamAddon()) { setStatus(tr_("search_no_stream_addon")); return; }
        setSearching(true);
        setStatus(tr_("search_loading_streams_from").arg(it.name));
        mgr.getStreams(it.type, it.id);
    } else if (m_mode == "episodes") {
        if (index < 0 || index >= m_episodeCache.size()) return;
        const QVariantMap ep = m_episodeCache[index].toMap();
        const QString videoId = ep.value(QStringLiteral("videoId")).toString();
        if (videoId.isEmpty()) return;
        m_fromEpisodes = true;
        setMode("streams");
        m_results.clear();
        emit resultsChanged();
        if (!mgr.hasStreamAddon()) { setStatus(tr_("search_no_stream_addon")); return; }
        setSearching(true);
        setStatus(tr_("search_loading_streams_from").arg(ep.value(QStringLiteral("name")).toString()));
        mgr.getStreams(m_epType, videoId);
    } else if (m_mode == "streams") {
        if (index < 0 || index >= m_streamCache.size()) return;
        const auto &s = m_streamCache[index];
        if (s.magnet.startsWith("magnet:")) {
            m_session->addMagnet(s.magnet, m_savePath, m_streamHintTitle, m_streamHintType);
            setStatus(tr_("search_added_name").arg(s.title));
            emit addedTorrent(SearchBridgeUtil::btihFromMagnet(s.magnet));
        }
    } else {   // torrent / games / all → each flat row carries a magnet OR an http url
        if (index < 0 || index >= m_resultMagnets.size()) return;
        const QString magnet = m_resultMagnets[index];
        const QString httpUrl = index < m_resultHttp.size() ? m_resultHttp[index] : QString();
        if (magnet.isEmpty() && httpUrl.isEmpty()) return;
        const QVariantMap rm = index < m_results.size() ? m_results[index].toMap() : QVariantMap();
        const QString name = rm.value(QStringLiteral("name")).toString();
        const qint64 needed = rm.value(QStringLiteral("sizeBytes")).toLongLong();
        if (!force && !fitsOnSaveVolume(needed)) {
            const QStorageInfo si(m_savePath);
            emit addWontFit(index, name, needed, si.isValid() ? si.bytesAvailable() : 0);
            return;   // QML asks the user, then re-calls with force = true
        }
        const QString hint = index < m_resultTitles.size() ? m_resultTitles[index] : QString();
        const int type = hint.isEmpty() ? -1 : static_cast<int>(ContentType::Game);

        // A file-host-only source (no magnet) downloads directly over HTTP and
        // shows up in the Downloads list via the engine decorator.
        if (magnet.isEmpty()) {
            if (!m_httpDownloads) { setStatus(tr_("add_url_failed")); return; }
            const QString id = m_httpDownloads->add(bat::directDownloadUrl(QUrl(httpUrl)), m_savePath);
            // Resolve an IGDB cover keyed by the row's pseudo-hash, same as a
            // magnet game gets one via addMagnet's cover hint.
            if (!hint.isEmpty() && m_resolver)
                m_resolver->resolveManual(bat::httpRowHash(id), hint, ContentType::Game);
            setStatus(name.isEmpty() ? tr_("search_added") : tr_("search_added_name").arg(name));
            return;
        }
        m_session->addMagnet(magnet, m_savePath, hint, type);   // hint = clean game title, "" for torrents
        setStatus(name.isEmpty() ? tr_("search_added") : tr_("search_added_name").arg(name));
        QString hash = rm.value(QStringLiteral("coverHash")).toString();   // torrent rows carry the hash
        if (hash.isEmpty()) hash = SearchBridgeUtil::btihFromMagnet(magnet);
        emit addedTorrent(hash);
    }
}

void QmlSearchBridge::back()
{
    if (m_fromTitles && m_mode != "streams" && m_mode != "episodes") {   // sources view → back to the titles grid
        m_fromTitles = false;
        m_aggregate = false;
        clearWorkContext();
        m_results = m_titleCache;
        m_resultMagnets.clear();
        m_resultTitles.clear();
    m_resultHttp.clear();
        setMode("titles");
        setSearching(false);
        setStatus(tr_("search_titles_n").arg(m_results.size()));
        emit resultsChanged();
        return;
    }
    if (m_mode == "streams" && m_fromEpisodes) {   // streams → episode picker
        m_fromEpisodes = false;
        showEpisodeRows();
        return;
    }
    if (m_mode != "streams" && m_mode != "episodes") return;
    m_fromEpisodes = false;
    setMode("catalog");
    rebuildCatalogRows();
    setStatus(tr_("search_results_n").arg(m_catalogCache.size()));
}

void QmlSearchBridge::rebuildCatalogRows()
{
    m_results.clear();
    for (const auto &it : std::as_const(m_catalogCache)) {
        QVariantMap m;
        m["name"] = it.name;
        m["sub"] = it.type;
        m["sizeStr"] = it.year > 0 ? QString::number(it.year) : QString();
        m["seeds"] = ""; m["leech"] = ""; m["releaseGroup"] = "";
        m["poster"] = it.poster; m["coverHash"] = "";
        m["seedsN"] = 0; m["sizeBytes"] = 0;
        fillMediaAttrs(m, it.name);
        m_results << m;
    }
    emit resultsChanged();
}

void QmlSearchBridge::showEpisodeRows()
{
    setMode("episodes");
    m_results.clear();
    m_resultMagnets.clear();
    m_resultTitles.clear();
    m_resultHttp.clear();
    for (const QVariant &v : std::as_const(m_episodeCache)) {
        const QVariantMap ep = v.toMap();
        QVariantMap m;
        m["name"] = ep.value(QStringLiteral("name"));
        m["sub"] = ""; m["provider"] = "";
        m["sizeStr"] = ep.value(QStringLiteral("released")).toString();
        m["seeds"] = ""; m["leech"] = ""; m["releaseGroup"] = "";
        m["poster"] = m_streamHintPoster; m["coverHash"] = "";
        m["seedsN"] = 0; m["sizeBytes"] = 0;
        m["season"] = ep.value(QStringLiteral("season"));
        m["episode"] = ep.value(QStringLiteral("episode"));
        m_results << m;
    }
    setStatus(tr_("search_episodes_n").arg(m_results.size()));
    emit resultsChanged();
}

