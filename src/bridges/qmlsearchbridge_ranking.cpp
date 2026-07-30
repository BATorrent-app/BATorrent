// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details
//
// QmlSearchBridge — relevance ranking and best-release pick.

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

QStringList QmlSearchBridge::queryWords() const
{
    return SearchRanker::significantWords(m_activeQuery);
}

int QmlSearchBridge::relevance(const QString &name, const QStringList &words) const
{
    return SearchRanker::relevanceScore(name, words);
}

QVariantList QmlSearchBridge::queryWordSets() const
{
    QVariantList out;
    // the names this drill-down was actually searched under; falls back to the
    // typed query for a flat/raw search, where there is no picked work
    const QStringList titles = m_activeTitles.isEmpty() ? QStringList{ m_activeQuery }
                                                        : m_activeTitles;
    for (const QString &t : titles) {
        const QStringList w = SearchRanker::significantWords(t);
        if (!w.isEmpty()) out << QVariant(w);
    }
    return out;
}

int QmlSearchBridge::relevanceMulti(const QString &name, const QVariantList &sets) const
{
    QList<QStringList> ws;
    ws.reserve(sets.size());
    for (const QVariant &v : sets) ws << v.toStringList();
    return SearchRanker::bestRelevance(name, ws);
}

int QmlSearchBridge::pickBestResult() const
{
    if (m_isGameSearch || m_workType == QLatin1String("game")) {
        QList<GameReleasePick::Candidate> cands;
        cands.reserve(m_results.size());
        for (int i = 0; i < m_results.size(); ++i) {
            const bool hasUri = (i < m_resultMagnets.size() && !m_resultMagnets[i].isEmpty())
                             || (i < m_resultHttp.size() && !m_resultHttp[i].isEmpty());
            cands.append(SearchBridgeUtil::gameCandFromRow(m_results[i].toMap(), hasUri));
        }
        return GameReleasePick::best(cands);
    }

    QList<ReleasePick::Candidate> cands;
    cands.reserve(m_results.size());
    for (const QVariant &v : m_results) {
        const QVariantMap m = v.toMap();
        const QString mode = m.value(QStringLiteral("audioMode")).toString();
        const int audioRank = mode == QLatin1String("dub") ? 2
                            : mode == QLatin1String("sub") ? 1 : 0;
        cands.append({ m.value(QStringLiteral("quality")).toString(),
                       m.value(QStringLiteral("native")).toBool() || audioRank > 0,
                       audioRank,
                       m.value(QStringLiteral("seedsN")).toInt(),
                       m.value(QStringLiteral("sizeBytes")).toLongLong() });
    }
    const QSettings s(QStringLiteral("BATorrent"), QStringLiteral("BATorrent"));
    // select index → quality token (matches the SettingsWindow "Reprodução" options)
    static const char *qmap[] = { "Auto", "1080p", "720p", "4K" };
    const int qi = s.value(QStringLiteral("preferredQuality"), 1).toInt();
    const QString prefQ = QString::fromLatin1((qi >= 0 && qi < 4) ? qmap[qi] : "1080p");
    const qint64 maxBytes = s.value(QStringLiteral("preferMaxSize"), 0).toLongLong() * 1024 * 1024;
    const bool preferNative = s.value(QStringLiteral("preferNativeLang"), true).toBool();
    return ReleasePick::best(cands, prefQ, maxBytes, preferNative);
}

int QmlSearchBridge::compareBuildVersions(const QString &a, const QString &b) const
{
    return GameReleasePick::compareVersions(a, b);
}

int QmlSearchBridge::compareGameReleases(const QVariantMap &a, const QVariantMap &b) const
{
    // strcmp-style: positive means a ranks above b (matches GameReleasePick::compareCandidates).
    return GameReleasePick::compareCandidates(
        SearchBridgeUtil::gameCandFromRow(a, a.value(QStringLiteral("hasUri"), true).toBool()),
        SearchBridgeUtil::gameCandFromRow(b, b.value(QStringLiteral("hasUri"), true).toBool()));
}

