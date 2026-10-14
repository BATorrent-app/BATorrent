// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details
//
// QmlSearchBridge: work context and title→sources drill-down.

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
#include <QDebug>
#include <QGuiApplication>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QRegularExpression>
#include <QSettings>
#include <QStorageInfo>
#include <QElapsedTimer>
#include <QTimer>
#include <QUrl>
#include <algorithm>

void QmlSearchBridge::setWorkContext(const QVariantMap &work)
{
    m_workType = work.value(QStringLiteral("type")).toString();
    m_workTitle = work.value(QStringLiteral("title")).toString();
    if (m_workTitle.isEmpty()) m_workTitle = work.value(QStringLiteral("name")).toString();
    m_workPoster = work.value(QStringLiteral("poster")).toString();
    m_workYear = work.value(QStringLiteral("year")).toString();
    m_workTmdbId = work.value(QStringLiteral("tmdbId")).toInt();
    m_workStills = work.value(QStringLiteral("stills")).toStringList();
    m_workBackdrop = work.value(QStringLiteral("backdrop")).toString();
    m_workOverview = work.value(QStringLiteral("overview")).toString();
    m_workRating = work.value(QStringLiteral("rating")).toDouble();
    m_workStillsRequested = false;
    m_workLogo.clear();
    // IGDB answers these inline with the title; TMDB fills them on fetchWorkStills.
    m_workGenres = work.value(QStringLiteral("genres")).toStringList();
    m_workCast.clear();
    m_workSeasonCount = 0;
    m_workMaker = work.value(QStringLiteral("maker")).toString();
    m_workImdbId.clear();
    m_addonEpisodesWaiting.clear();
    emit workChanged();
    emit workStillsChanged();
    emit workLogoChanged();
    emit workDetailsChanged();
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
    m_workGenres.clear();
    m_workCast.clear();
    m_workSeasonCount = 0;
    m_workMaker.clear();
    m_workImdbId.clear();
    m_addonEpisodesWaiting.clear();
    emit workChanged();
    emit workStillsChanged();
    emit workDetailsChanged();
}

void QmlSearchBridge::fetchWorkStills()
{
    if (m_workStillsRequested || !m_workStills.isEmpty()) return;   // inline (games) or already asked
    if (m_workTmdbId <= 0 || !m_discovery) return;
    m_workStillsRequested = true;
    m_discovery->fetchBackdrops(m_workTmdbId, m_workType);
    m_discovery->fetchWorkDetails(m_workTmdbId, m_workType);
}

void QmlSearchBridge::setDiscovery(DiscoveryService *d)
{
    m_discovery = d;
    if (!m_discovery) return;
    connect(m_discovery, &DiscoveryService::logoReady, this,
            [this](int tmdbId, const QString &url) {
        if (tmdbId != m_workTmdbId) return;        // stale reply for a former title
        m_workLogo = url;
        emit workLogoChanged();
    });
    connect(m_discovery, &DiscoveryService::workDetailsReady, this,
            [this](int tmdbId, const QVariantMap &d) {
        if (tmdbId != m_workTmdbId || d.isEmpty()) return;   // stale reply for a former title
        m_workGenres = d.value(QStringLiteral("genres")).toStringList();
        m_workCast = d.value(QStringLiteral("cast")).toStringList();
        m_workSeasonCount = d.value(QStringLiteral("seasons")).toInt();
        m_workImdbId = d.value(QStringLiteral("imdbId")).toString();
        qInfo().noquote() << "[addon] tmdb" << tmdbId << "imdb" << (m_workImdbId.isEmpty() ? QStringLiteral("none") : m_workImdbId);
        emit workDetailsChanged();
        const auto waiting = std::exchange(m_addonEpisodesWaiting, {});
        for (const auto &se : waiting) fetchEpisodeStreams(se.first, se.second);
    });
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
        for (const QVariant &v : works)
            m_results << SearchBridgeUtil::titleRowFromWork(v.toMap());
        m_titleCache = m_results;
        setSearching(false);
        // Stay in the grid even when empty: the page shows an empty state with a
        // "raw results" escape, so the flow is consistent (never silently flips).
        setStatus(m_results.isEmpty() ? tr_("search_no_titles")
                                      : tr_("search_titles_n").arg(m_results.size()));
        emit resultsChanged();
    });
}

void QmlSearchBridge::openTitle(const QVariantMap &work)
{
    const QString title = work.value(QStringLiteral("title")).toString();
    if (title.isEmpty()) return;
    // No grid to return to: back() lands on an empty titles stage and the
    // page takes the user home from there.
    m_titleCache.clear();
    m_titleQuery.clear();
    m_fromTitles = true;
    setWorkContext(work);
    searchSourcesForWork(title,
                         work.value(QStringLiteral("year")).toString(),
                         work.value(QStringLiteral("type")).toString(),
                         work.value(QStringLiteral("originalTitle")).toString());
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
    m_addonEpisodesAsked.clear();
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
    // requests carry language=, `title` is already localised: so searching it
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


// Stremio asks Torrentio for "tt0434665:1:3" and gets every torrent that holds
// that episode, season and series packs included, each with the file inside it.
// The indexers above only know release names, so a 366-episode batch with 561
// seeders never reached an episode page. Ask the addons too.
void QmlSearchBridge::fetchEpisodeStreams(int season, int episode)
{
    if (m_workType != QLatin1String("series") || !m_titleSources || season < 0 || episode <= 0) {
        qInfo().noquote() << "[addon] episode streams skipped: type" << m_workType
                          << "titleSources" << m_titleSources << "S" << season << "E" << episode;
        return;
    }
    auto &mgr = AddonManager::instance();
    if (!mgr.hasStreamAddon()) { qInfo() << "[addon] episode streams skipped: no stream addon enabled"; return; }
    if (m_workImdbId.isEmpty()) {
        qInfo().noquote() << "[addon] episode streams wait for the IMDb id: tmdb" << m_workTmdbId
                          << "S" << season << "E" << episode;
        // the title's details (and its IMDb id) are still on the way
        const QPair<int, int> se(season, episode);
        if (!m_addonEpisodesWaiting.contains(se)) m_addonEpisodesWaiting << se;
        return;
    }
    const QString key = QStringLiteral("%1:%2").arg(season).arg(episode);
    if (m_addonEpisodesAsked.contains(key)) return;
    m_addonEpisodesAsked.insert(key);
    mgr.getStreamsFor(QStringLiteral("series"), m_workImdbId + QLatin1Char(':') + key);
}

void QmlSearchBridge::notifyResults(const char *why)
{
    QElapsedTimer t;
    t.start();
    emit resultsChanged();
    qDebug().noquote() << "[find]" << why << "update:" << m_results.size() << "rows,"
                       << t.elapsed() << "ms in bindings";
}

void QmlSearchBridge::appendAddonEpisodeRows(const QString &videoId, const QList<StreamResult> &streams)
{
    QElapsedTimer merge;
    merge.start();
    const QStringList parts = videoId.split(QLatin1Char(':'));
    if (parts.size() != 3 || parts.at(0) != m_workImdbId) return;   // another title
    if (m_mode != QLatin1String("all") || !m_titleSources) return;
    if (!m_addonEpisodesAsked.contains(parts.at(1) + QLatin1Char(':') + parts.at(2))) return;   // list was reset since
    const int season = parts.at(1).toInt();
    const int episode = parts.at(2).toInt();

    // One row per torrent per episode: two addons often answer with the same one.
    QSet<QString> seen;
    for (const QVariant &v : std::as_const(m_results)) {
        const QVariantMap r = v.toMap();
        if (!r.value(QStringLiteral("fromAddon")).toBool()) continue;
        seen.insert(r.value(QStringLiteral("coverHash")).toString() + QLatin1Char(':')
                    + r.value(QStringLiteral("season")).toString() + QLatin1Char(':')
                    + r.value(QStringLiteral("episode")).toString());
    }
    int added = 0;
    for (const StreamResult &s : streams) {
        const QString hash = SearchBridgeUtil::btihFromMagnet(s.magnet).toLower();
        if (hash.isEmpty()) continue;
        const QString key = hash + QLatin1Char(':') + QString::number(season) + QLatin1Char(':')
                          + QString::number(episode);
        if (seen.contains(key)) continue;
        seen.insert(key);
        const QString name = s.torrentName.isEmpty() ? s.title : s.torrentName;
        // Torrentio maps by IMDb, which files Thousand-Year Blood War as Bleach
        // season 17: its "S17E03" came back for 1:3. A file that names another
        // season, or another show's numbering, is not this episode.
        const QString file = (s.fileName.isEmpty() ? name : s.fileName).section(QLatin1Char('/'), -1);
        const EpisodeTag fileTag = EpisodeGroup::classify(file, m_activeTitles);
        if (fileTag.season > 1 && fileTag.season != season) continue;
        if (fileTag.episode < 0 && EpisodeGroup::classify(file).episode > 0) continue;
        const int seeds = qMax(0, s.seeders);
        QVariantMap m;
        m["name"] = name;
        // Listed as the episode it was asked for, whatever the torrent's size:
        // only that file is fetched when it is played.
        m["season"] = season;
        m["episode"] = episode;
        m["pack"] = false;
        m["inPack"] = s.inPack;
        m["fileIdx"] = s.fileIdx;
        m["fileName"] = s.fileName;
        m["fromAddon"] = true;
        m["sub"] = s.source.isEmpty() ? s.addonName : s.addonName + QStringLiteral(" / ") + s.source;
        m["provider"] = s.addonName;
        m["sizeStr"] = s.size > 0 ? formatSize(s.size) : QString();
        m["seeds"] = s.seeders >= 0 ? QString::number(s.seeders) : QString();
        m["leech"] = QString();
        m["hasSeeds"] = seeds > 0;
        m["releaseGroup"] = detectReleaseGroup(name);
        m["poster"] = ""; m["coverHash"] = hash;
        m["seedsN"] = seeds; m["sizeBytes"] = static_cast<qlonglong>(s.size);
        m["fromCatalog"] = false;
        m["uploadDate"] = QString();
        m["hasUri"] = true;
        fillMediaAttrs(m, name);
        fillTrust(m, name);
        QString magnet = s.magnet;
        if (!magnet.contains(QStringLiteral("&dn=")))
            magnet += QStringLiteral("&dn=") + QString::fromLatin1(QUrl::toPercentEncoding(name));
        m_results << m;
        m_resultMagnets << magnet;
        m_resultHttp << QString();
        m_resultTitles << QString();
        ++added;
    }
    qInfo().noquote() << "[addon]" << videoId << "rows added" << added << "of" << streams.size()
                      << "in" << merge.elapsed() << "ms";
    if (added == 0) return;
    if (!m_addonRowsFlush) {
        m_addonRowsFlush = new QTimer(this);
        m_addonRowsFlush->setSingleShot(true);
        m_addonRowsFlush->setInterval(400);
        connect(m_addonRowsFlush, &QTimer::timeout, this, [this]() { notifyResults("addon rows"); });
    }
    if (!m_addonRowsFlush->isActive()) m_addonRowsFlush->start();
}

// What to play out of the torrent a row adds: the addon's file, else the
// episode the page is on.
QVariantMap QmlSearchBridge::fileTargetFor(const QVariantMap &row) const
{
    int season = m_watchSeason, episode = m_watchEpisode;
    if (row.value(QStringLiteral("episode")).toInt() > 0) {
        season = row.value(QStringLiteral("season")).toInt();
        episode = row.value(QStringLiteral("episode")).toInt();
    }
    const int fileIdx = row.value(QStringLiteral("fileIdx"), -1).toInt();
    const QString fileName = row.value(QStringLiteral("fileName")).toString();
    if (episode <= 0 && fileIdx < 0 && fileName.isEmpty()) return {};
    return { { QStringLiteral("season"), season }, { QStringLiteral("episode"), episode },
             { QStringLiteral("fileIdx"), fileIdx }, { QStringLiteral("fileName"), fileName } };
}
