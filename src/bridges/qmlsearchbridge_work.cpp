// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details
//
// QmlSearchBridge — work context and title→sources drill-down.

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


