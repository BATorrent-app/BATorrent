// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details
//
// QmlSearchBridge — activateResult / back / catalog+episode row rebuild.

#include "bridges/qmlsearchbridge.h"
#include "bridges/qmlsearchbridge_util.h"
#include "torrent/iengine.h"
#include "services/metadata/metadataresolver.h"
#include "services/discovery/addonmanager.h"
#include "services/downloads/httpdownloadmanager.h"
#include "services/downloads/filehostresolver.h"
#include "services/platform/contentlanguage.h"
#include "services/platform/translator.h"
#include "services/platform/utils.h"

#include <QStorageInfo>
#include <QUrl>

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

