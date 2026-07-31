// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details
//
// QmlSearchBridge — game catalog sources and browse.

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
#include <QStorageInfo>
#include <QUrl>
#include <algorithm>

void QmlSearchBridge::runGameSearch(const QString &query)
{
    m_results.clear();
    m_resultMagnets.clear();
    m_resultTitles.clear();
    m_resultHttp.clear();
    m_gameCache = GameSourceManager::instance().search(query);
    appendGameRows(m_gameCache);
    setSearching(false);
    setStatus(tr_("search_results_n").arg(m_results.size()));
    requestCatalogSeedEnrichment();
}

QVariantList QmlSearchBridge::gameSources() const
{
    QVariantList out;
    for (const auto &s : GameSourceManager::instance().sources()) {
        QVariantMap m; m["name"] = s.first; m["url"] = s.second; out << m;
    }
    return out;
}

void QmlSearchBridge::addGameSource(const QString &name, const QString &url)
{
    auto &gsm = GameSourceManager::instance();
    gsm.addSource(name, url);
    emit gameSourcesChanged();
    if (!gsm.sources().isEmpty()) { setStatus(tr_("search_loading_game_catalogs")); gsm.refresh(false); }
}

void QmlSearchBridge::removeGameSource(const QString &url)
{
    auto &gsm = GameSourceManager::instance();
    gsm.removeSource(url);
    emit gameSourcesChanged();
    gsm.refresh(false);   // re-index remaining from cache
}

void QmlSearchBridge::refreshGames()
{
    if (GameSourceManager::instance().sources().isEmpty()) { emit gameSourcesChanged(); return; }
    setStatus(tr_("search_loading_game_catalogs"));
    GameSourceManager::instance().refresh(true);   // manual refresh → bypass cache
}

void QmlSearchBridge::ensureGamesIndexed()
{
    auto &gsm = GameSourceManager::instance();
    if (gsm.gameCount() > 0 || gsm.sources().isEmpty())
        return;
    setStatus(tr_("search_loading_game_catalogs"));
    gsm.refresh(false);
}

void QmlSearchBridge::browseGames(const QString &group, int page, int pageSize)
{
    auto &gsm = GameSourceManager::instance();
    if (gsm.gameCount() == 0 && !gsm.sources().isEmpty()) {
        ensureGamesIndexed();
        return;   // refreshed signal → QML retries browse
    }

    const int size = pageSize > 0 ? pageSize : 48;
    const int p = page < 0 ? 0 : page;
    const int total = gsm.countByGroup(group);
    const int offset = p * size;

    clearWorkContext();
    m_fromTitles = false;
    m_titleSources = false;
    m_isGameSearch = true;
    m_aggregate = false;
    m_activeQuery.clear();
    m_lastQuery.clear();
    setMode(QStringLiteral("games"));

    m_results.clear();
    m_resultMagnets.clear();
    m_resultTitles.clear();
    m_resultHttp.clear();
    m_gameCache = gsm.browse(group, offset, size);
    appendGameRows(m_gameCache);
    setSearching(false);

    if (total <= 0) {
        setStatus(tr_("find_catalog_empty"));
        return;
    }
    const int from = offset + 1;
    const int to = qMin(offset + m_results.size(), total);
    setStatus(tr_("find_catalog_showing").arg(from).arg(to).arg(total));
    requestCatalogSeedEnrichment();
}

int QmlSearchBridge::gameBrowseTotal(const QString &group) const
{
    return GameSourceManager::instance().countByGroup(group);
}

QVariantList QmlSearchBridge::gameRepackTabs() const
{
    return GameSourceManager::instance().groupCounts();
}

bool QmlSearchBridge::fitsOnSaveVolume(qint64 needed) const
{
    if (needed <= 0) return true;   // unknown size — don't block
    const QStorageInfo si(m_savePath);
    return !si.isValid() || needed <= si.bytesAvailable();
}

