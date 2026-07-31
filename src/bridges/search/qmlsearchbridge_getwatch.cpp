// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details
//
// QmlSearchBridge — Get & Watch / Install + source summary.

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

void QmlSearchBridge::getAndWatch(const QString &title, const QString &year, const QString &type)
{
    m_gwActive = true;
    m_gwCancelled = false;
    m_gwTitle = title;
    m_gwType = type.isEmpty() ? QStringLiteral("movie") : type;
    emit getFlowChanged();
    emit watchSearching(title);
    setWorkContext({ { QStringLiteral("title"), title },
                     { QStringLiteral("type"), type },
                     { QStringLiteral("year"), year } });
    searchSourcesForWork(title, year, type);      // gwResolve() runs when this settles
    // If the search had nothing to wait on (no providers), it finished inline.
    if (m_gwActive && !m_searching) gwResolve();
}

void QmlSearchBridge::cancelGetAndWatch()
{
    m_gwActive = false;
    m_gwCancelled = true;
    emit getFlowChanged();
}

void QmlSearchBridge::summarizeSources(const QString &title)
{
    const QString key = title.toLower().trimmed();
    if (key.isEmpty()) return;
    if (m_srcSummaryCache.contains(key)) {
        const QVariantList v = m_srcSummaryCache.value(key);
        emit sourceSummary(title, v.value(0).toInt(), v.value(1).toLongLong(), v.value(2).toInt());
        return;
    }
    if (m_srcSummaryInFlight.contains(key)) return;
    m_srcSummaryInFlight.insert(key);
    AddonManager::instance().summarizeTorrents(title, 0);
}

void QmlSearchBridge::gwResolve()
{
    m_gwActive = false;
    emit getFlowChanged();
    if (m_gwCancelled) { m_gwCancelled = false; return; }   // user backed out during the search

    auto hasMagnet = [this](int i) {
        return i >= 0 && i < m_resultMagnets.size() && !m_resultMagnets[i].isEmpty();
    };

    // Prefer the ranked pick; if it's HTTP-only, re-rank among magnet rows so
    // Get & Install / Get & Watch always get an info-hash they can poll.
    int idx = pickBestResult();
    if (!hasMagnet(idx)
        && (m_gwType == QLatin1String("game") || m_isGameSearch
            || m_workType == QLatin1String("game"))) {
        QList<GameReleasePick::Candidate> cands;
        QList<int> idxs;
        for (int i = 0; i < m_results.size(); ++i) {
            if (!hasMagnet(i)) continue;
            idxs.append(i);
            cands.append(SearchBridgeUtil::gameCandFromRow(m_results[i].toMap(), true));
        }
        const int local = GameReleasePick::best(cands);
        idx = (local >= 0 && local < idxs.size()) ? idxs[local] : -1;
    }
    if (!hasMagnet(idx)) {
        emit watchNoRelease(m_gwTitle);
        return;
    }
    const QString magnet = m_resultMagnets[idx];
    const QVariantMap rm = m_results[idx].toMap();
    // Prefer Get&Watch's known title/type as the cover hint, so the player and
    // library show the real movie/series poster instead of the raw torrent name
    // (which is empty until the magnet's metadata resolves → placeholder). Fall
    // back to the per-row game title for the game flow.
    QString hint = idx < m_resultTitles.size() ? m_resultTitles[idx] : QString();
    int type = hint.isEmpty() ? -1 : static_cast<int>(ContentType::Game);
    if (hint.isEmpty() && !m_gwTitle.isEmpty()) {
        hint = m_gwTitle;
        type = m_gwType == QLatin1String("series") ? static_cast<int>(ContentType::Series)
             : m_gwType == QLatin1String("game")   ? static_cast<int>(ContentType::Game)
             : static_cast<int>(ContentType::Movie);
    } else if (m_gwType == QLatin1String("game")) {
        type = static_cast<int>(ContentType::Game);
        if (hint.isEmpty()) hint = m_gwTitle;
    }
    m_session->addMagnet(magnet, m_savePath, hint, type);
    QString hash = rm.value(QStringLiteral("coverHash")).toString();
    if (hash.isEmpty()) hash = SearchBridgeUtil::btihFromMagnet(magnet);
    if (m_gwType == QLatin1String("game"))
        emit prepareAndInstall(hash, m_gwTitle);
    else
        emit prepareAndWatch(hash, m_gwTitle);
}

