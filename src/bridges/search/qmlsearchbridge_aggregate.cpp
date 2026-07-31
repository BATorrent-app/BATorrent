// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details
//
// QmlSearchBridge — aggregate row append + catalog seed enrichment.

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
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QRegularExpression>
#include <QSettings>
#include <QStorageInfo>
#include <QUrl>
#include <QSet>
#include <algorithm>

QSet<QString> QmlSearchBridge::currentResultKeys() const
{
    QSet<QString> seen;
    seen.reserve(m_results.size());
    for (int i = 0; i < m_results.size() && i < m_resultMagnets.size(); ++i) {
        const auto rm = m_results[i].toMap();
        seen.insert(SearchBridgeUtil::resultDedupeKey(
            m_resultMagnets[i], rm.value(QStringLiteral("name")).toString(),
            rm.value(QStringLiteral("sizeBytes")).toLongLong()));
    }
    return seen;
}

void QmlSearchBridge::appendGameRows(const QList<GameDownload> &games)
{
    QSet<QString> seen = currentResultKeys();
    for (const auto &g : games) {
        const QString key = SearchBridgeUtil::resultDedupeKey(g.magnet, g.cleanTitle.isEmpty() ? g.title : g.cleanTitle, 0);
        if (seen.contains(key)) continue;
        seen.insert(key);
        QVariantMap m;
        m["name"] = g.cleanTitle.isEmpty() ? g.title : g.cleanTitle;
        m["sub"] = g.source;
        m["provider"] = g.source;
        m["sizeStr"] = g.fileSize;
        m["seeds"] = ""; m["leech"] = ""; m["hasSeeds"] = false;
        m["releaseGroup"] = detectReleaseGroup(g.title);
        const QString ih = infoHashFromMagnet(g.magnet);
        m["poster"] = ""; m["coverHash"] = ih;
        m["seedsN"] = 0;
        m["sizeBytes"] = parseSizeToBytes(g.fileSize);
        m["fromCatalog"] = true;
        m["uploadDate"] = g.uploadDate;
        m["hasUri"] = !g.magnet.isEmpty() || !g.httpUrl.isEmpty();
        fillMediaAttrs(m, g.title);
        fillTrust(m, g.title);
        m_results << m;
        m_resultMagnets << g.magnet;
        m_resultHttp << g.httpUrl;
        m_resultTitles << (g.cleanTitle.isEmpty() ? g.title : g.cleanTitle);
    }
    emit resultsChanged();
}

void QmlSearchBridge::appendTorrentRows(const QList<TorrentSearchResult> &results)
{
    auto sorted = results;
    std::sort(sorted.begin(), sorted.end(),
              [](const TorrentSearchResult &a, const TorrentSearchResult &b) { return a.seeders > b.seeders; });
    // Season/episode grouping only makes sense inside one picked series' releases.
    const bool groupEpisodes = m_titleSources && m_workType == QLatin1String("series");
    QSet<QString> seen = currentResultKeys();
    for (const auto &r : sorted) {
        const QString key = SearchBridgeUtil::resultDedupeKey(r.magnet, r.name, static_cast<qlonglong>(r.size));
        if (seen.contains(key)) continue;
        seen.insert(key);
        QVariantMap m;
        m["name"] = r.name;
        if (groupEpisodes) {
            const EpisodeTag tag = EpisodeGroup::classify(r.name);
            m["season"] = tag.season;
            m["episode"] = tag.episode;
            m["pack"] = tag.pack;
        }
        m["sub"] = r.provider;
        m["provider"] = r.provider;
        m["sizeStr"] = r.size > 0 ? formatSize(r.size) : QString();
        m["seeds"] = QString::number(r.seeders);
        m["leech"] = QString::number(r.leechers);
        m["hasSeeds"] = r.seeders > 0;
        m["releaseGroup"] = detectReleaseGroup(r.name);
        QString ih = r.infoHash.toLower();
        if (ih.size() != 40)
            ih = infoHashFromMagnet(r.magnet);
        m["poster"] = ""; m["coverHash"] = ih;
        m["seedsN"] = r.seeders; m["sizeBytes"] = static_cast<qlonglong>(r.size);
        m["fromCatalog"] = false;
        m["uploadDate"] = QString();
        m["hasUri"] = !r.magnet.isEmpty();
        fillMediaAttrs(m, r.name);
        fillTrust(m, r.name);
        m_results << m;
        m_resultMagnets << r.magnet;
        m_resultHttp << QString();          // torrent rows download via magnet
        m_resultTitles << QString();        // torrent rows have no game cover hint
    }
    mergeCatalogSwarms();
    emit resultsChanged();
}

void QmlSearchBridge::finishAggregateSource()
{
    if (--m_pendingSources > 0) return;
    setSearching(false);
    mergeCatalogSwarms();
    setStatus(tr_("search_results_n").arg(m_results.size()));
    requestCatalogSeedEnrichment();
    if (m_gwActive) gwResolve();
}

QString QmlSearchBridge::infoHashFromMagnet(const QString &magnet)
{
    static const QRegularExpression re(QStringLiteral("btih:([0-9A-Fa-f]{40})"),
                                       QRegularExpression::CaseInsensitiveOption);
    const auto m = re.match(magnet);
    return m.hasMatch() ? m.captured(1).toLower() : QString();
}

qint64 QmlSearchBridge::parseSizeToBytes(const QString &s)
{
    const QString t = s.trimmed();
    if (t.isEmpty()) return 0;
    bool ok = false;
    // raw integer bytes
    if (!t.contains(QLatin1Char(' ')) && !t.contains(QLatin1Char('.')) && t.at(0).isDigit()) {
        const qint64 n = t.toLongLong(&ok);
        if (ok && n > 0) return n;
    }
    static const QRegularExpression re(
        QStringLiteral(R"(^\s*([\d]+(?:[.,]\d+)?)\s*(B|KB|MB|GB|TB)\s*$)"),
        QRegularExpression::CaseInsensitiveOption);
    const auto m = re.match(t);
    if (!m.hasMatch()) return 0;
    QString num = m.captured(1);
    num.replace(QLatin1Char(','), QLatin1Char('.'));
    const double v = num.toDouble(&ok);
    if (!ok || v < 0) return 0;
    const QString u = m.captured(2).toUpper();
    double mul = 1;
    if (u == QLatin1String("KB")) mul = 1024.0;
    else if (u == QLatin1String("MB")) mul = 1024.0 * 1024.0;
    else if (u == QLatin1String("GB")) mul = 1024.0 * 1024.0 * 1024.0;
    else if (u == QLatin1String("TB")) mul = 1024.0 * 1024.0 * 1024.0 * 1024.0;
    return static_cast<qint64>(v * mul + 0.5);
}

void QmlSearchBridge::applySeedHits(const QHash<QString, QPair<int, int>> &byHash)
{
    if (byHash.isEmpty()) return;
    bool changed = false;
    for (int i = 0; i < m_results.size(); ++i) {
        QVariantMap m = m_results[i].toMap();
        if (!m.value(QStringLiteral("fromCatalog")).toBool()) continue;
        QString h = m.value(QStringLiteral("coverHash")).toString().toLower();
        if (h.size() != 40 && i < m_resultMagnets.size())
            h = infoHashFromMagnet(m_resultMagnets[i]);
        if (!byHash.contains(h)) continue;
        const auto hit = byHash.value(h);
        if (hit.first <= m.value(QStringLiteral("seedsN")).toInt()) continue;
        m[QStringLiteral("seedsN")] = hit.first;
        m[QStringLiteral("seeds")] = QString::number(hit.first);
        m[QStringLiteral("leech")] = QString::number(hit.second);
        m[QStringLiteral("hasSeeds")] = hit.first > 0;
        m_results[i] = m;
        changed = true;
    }
    if (changed) emit resultsChanged();
}

void QmlSearchBridge::mergeCatalogSwarms()
{
    QSet<QString> catalogHashes;
    QHash<QString, QPair<int, int>> swarm;   // hash → (seeds, leech)
    QHash<QString, qint64> sizes;

    for (int i = 0; i < m_results.size(); ++i) {
        const QVariantMap m = m_results[i].toMap();
        QString h = m.value(QStringLiteral("coverHash")).toString().toLower();
        if (h.size() != 40 && i < m_resultMagnets.size())
            h = infoHashFromMagnet(m_resultMagnets[i]);
        if (h.size() != 40) continue;

        if (m.value(QStringLiteral("fromCatalog")).toBool()) {
            catalogHashes.insert(h);
            continue;
        }
        const int seeds = m.value(QStringLiteral("seedsN")).toInt();
        const int leech = m.value(QStringLiteral("leech")).toString().toInt();
        if (!swarm.contains(h) || seeds > swarm.value(h).first)
            swarm.insert(h, {seeds, leech});
        const qint64 sz = m.value(QStringLiteral("sizeBytes")).toLongLong();
        if (sz > 0) sizes.insert(h, sz);
    }
    if (catalogHashes.isEmpty()) return;

    applySeedHits(swarm);

    // Prefer the catalog row: drop indexer duplicates of the same magnet.
    QList<int> drop;
    for (int i = 0; i < m_results.size(); ++i) {
        const QVariantMap m = m_results[i].toMap();
        if (m.value(QStringLiteral("fromCatalog")).toBool()) {
            QString h = m.value(QStringLiteral("coverHash")).toString().toLower();
            if (h.size() != 40 && i < m_resultMagnets.size())
                h = infoHashFromMagnet(m_resultMagnets[i]);
            if (sizes.contains(h) && m.value(QStringLiteral("sizeBytes")).toLongLong() <= 0) {
                QVariantMap mm = m;
                mm[QStringLiteral("sizeBytes")] = sizes.value(h);
                mm[QStringLiteral("sizeStr")] = formatSize(sizes.value(h));
                m_results[i] = mm;
            }
            continue;
        }
        QString h = m.value(QStringLiteral("coverHash")).toString().toLower();
        if (h.size() != 40 && i < m_resultMagnets.size())
            h = infoHashFromMagnet(m_resultMagnets[i]);
        if (catalogHashes.contains(h))
            drop.append(i);
    }
    if (drop.isEmpty()) {
        emit resultsChanged();
        return;
    }
    std::sort(drop.begin(), drop.end(), std::greater<int>());
    for (int i : drop) {
        m_results.removeAt(i);
        if (i < m_resultMagnets.size()) m_resultMagnets.removeAt(i);
        if (i < m_resultHttp.size()) m_resultHttp.removeAt(i);
        if (i < m_resultTitles.size()) m_resultTitles.removeAt(i);
    }
    emit resultsChanged();
}

void QmlSearchBridge::requestCatalogSeedEnrichment()
{
    QStringList queries;
    const QString titled = !m_workTitle.trimmed().isEmpty() ? m_workTitle.trimmed()
                         : m_activeQuery.trimmed();
    if (!titled.isEmpty()) {
        queries << titled;
    } else {
        // Catalog browse page: budget a few title lookups (BitSearch rate limits).
        int budget = 8;
        for (const QVariant &v : m_results) {
            if (budget <= 0) break;
            const QVariantMap m = v.toMap();
            if (!m.value(QStringLiteral("fromCatalog")).toBool()) continue;
            if (m.value(QStringLiteral("seedsN")).toInt() > 0) continue;
            if (m.value(QStringLiteral("coverHash")).toString().size() != 40) continue;
            const QString name = m.value(QStringLiteral("name")).toString().trimmed();
            if (name.isEmpty() || queries.contains(name)) continue;
            queries << name;
            --budget;
        }
    }
    if (queries.isEmpty()) return;

    bool need = false;
    for (const QVariant &v : m_results) {
        const QVariantMap m = v.toMap();
        if (m.value(QStringLiteral("fromCatalog")).toBool()
            && m.value(QStringLiteral("seedsN")).toInt() <= 0
            && m.value(QStringLiteral("coverHash")).toString().size() == 40) {
            need = true;
            break;
        }
    }
    if (!need) return;

    const int gen = ++m_seedEnrichGen;
    auto *nam = new QNetworkAccessManager(this);
    int *pending = new int(queries.size());
    for (const QString &q : queries) {
        const QUrl url(QStringLiteral("https://bitsearch.eu/api/v1/search?q=%1&limit=50")
                           .arg(QString::fromUtf8(QUrl::toPercentEncoding(q))));
        QNetworkRequest req(url);
        req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("BATorrent/2.0"));
        req.setTransferTimeout(12000);
        QNetworkReply *reply = nam->get(req);
        connect(reply, &QNetworkReply::finished, this, [this, reply, nam, pending, gen]() {
            reply->deleteLater();
            if (gen == m_seedEnrichGen && reply->error() == QNetworkReply::NoError) {
                const QJsonObject root = QJsonDocument::fromJson(reply->readAll()).object();
                const QJsonArray arr = root.value(QStringLiteral("results")).toArray();
                QHash<QString, QPair<int, int>> byHash;
                for (const QJsonValue &v : arr) {
                    const QJsonObject o = v.toObject();
                    QString h = o.value(QStringLiteral("infohash")).toString().toLower();
                    if (h.size() != 40)
                        h = o.value(QStringLiteral("info_hash")).toString().toLower();
                    if (h.size() != 40) continue;
                    const int seeds = o.value(QStringLiteral("seeders")).toInt();
                    const int leech = o.value(QStringLiteral("leechers")).toInt();
                    if (!byHash.contains(h) || seeds > byHash.value(h).first)
                        byHash.insert(h, {seeds, leech});
                }
                applySeedHits(byHash);
            }
            if (--(*pending) == 0) {
                delete pending;
                nam->deleteLater();
            }
        });
    }
}

