// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details
//
// QmlSearchBridge: the next episode for the player when it is not in the
// torrent being watched. The show comes from the resolver, its IMDb numbering
// from Cinemeta, and the release from the stream addons; none of it needs the
// series page to be open.

#include "bridges/search/qmlsearchbridge.h"
#include "bridges/search/qmlsearchbridge_util.h"
#include "torrent/iengine.h"
#include "services/metadata/episodegroup.h"
#include "services/metadata/metadataresolver.h"
#include "services/metadata/nameparser.h"
#include "services/discovery/discoveryservice.h"
#include "services/discovery/addonmanager.h"

#include <QDebug>
#include <QRegularExpression>
#include <QUrl>
#include <algorithm>

namespace {

QString qualityOf(const QString &name)
{
    static const QRegularExpression re(QStringLiteral("(2160p|4k|1080p|720p|480p)"),
                                       QRegularExpression::CaseInsensitiveOption);
    const auto m = re.match(name);
    QString q = m.hasMatch() ? m.captured(1).toLower() : QString();
    return q == QLatin1String("4k") ? QStringLiteral("2160p") : q;
}

}

void QmlSearchBridge::findNextEpisode(const QString &infoHash, int fileIndex)
{
    const int row = m_session->torrentIndexByInfoHash(infoHash);
    if (row < 0 || !m_resolver || !m_resolver->hasCached(infoHash) || !m_discovery) return;
    const MetadataResult meta = m_resolver->cached(infoHash);
    if (!meta.valid || meta.tmdbId <= 0) return;
    const auto files = m_session->filesAt(row);
    if (fileIndex < 0 || fileIndex >= int(files.size())) return;
    const QString leaf = QString(files[size_t(fileIndex)].path).replace(QLatin1Char('\\'), QLatin1Char('/'))
                             .section(QLatin1Char('/'), -1);
    const EpisodeTag tag = EpisodeGroup::classify(leaf);
    if (tag.episode <= 0) return;   // a film, or a file that names no episode

    NextAsk ask;
    ask.infoHash = infoHash;
    ask.fileIndex = fileIndex;
    ask.tmdbId = meta.tmdbId;
    ask.show = meta.title;
    ask.season = tag.season;
    ask.episode = tag.episode;
    const QString torrentName = m_session->torrentAt(row).name;
    ask.quality = qualityOf(leaf + QLatin1Char(' ') + torrentName);
    ask.group = detectReleaseGroup(torrentName);
    m_nextAsks.removeIf([&](const NextAsk &a) { return a.infoHash == infoHash && a.fileIndex == fileIndex; });
    m_nextAsks << ask;

    if (m_imdbByTmdb.contains(meta.tmdbId)) { resumeNextAsks(); return; }
    // the IMDb id comes with the show's details
    m_discovery->fetchWorkDetails(meta.tmdbId, QStringLiteral("series"));
}

void QmlSearchBridge::resumeNextAsks()
{
    for (NextAsk &ask : m_nextAsks) {
        if (ask.asked) continue;
        const QString imdb = m_imdbByTmdb.value(ask.tmdbId);
        if (imdb.isEmpty()) continue;
        if (!m_cinemetaVideos.contains(imdb)) { fetchCinemeta(imdb); continue; }

        ask.asked = true;
        const Cinemeta::Number next = Cinemeta::nextAfter(m_cinemetaVideos.value(imdb), ask.season, ask.episode);
        if (next.season < 0) continue;   // the last one, or a numbering we can't place
        ask.nextSeason = next.season;
        ask.nextEpisode = next.episode;
        const QString videoId = QStringLiteral("%1:%2:%3").arg(imdb).arg(next.season).arg(next.episode);
        m_nextByVideoId.insert(videoId, ask);
        qInfo().noquote() << "[next]" << ask.infoHash.left(8) << "after" << ask.season << ask.episode
                          << "asks" << videoId;
        AddonManager::instance().getStreamsFor(QStringLiteral("series"), videoId);
    }
    m_nextAsks.removeIf([](const NextAsk &a) { return a.asked; });
}

void QmlSearchBridge::takeNextStreams(const QString &videoId, const QList<StreamResult> &streams)
{
    if (!m_nextByVideoId.contains(videoId)) return;
    const NextAsk ask = m_nextByVideoId.value(videoId);
    // The same group and quality as the episode playing reads like the same
    // series of releases; within that, the best seeded.
    const StreamResult *best = nullptr;
    int bestScore = -1;
    for (const StreamResult &s : streams) {
        if (!s.magnet.startsWith(QLatin1String("magnet:"))) continue;
        const QString name = s.torrentName.isEmpty() ? s.title : s.torrentName;
        int score = qMax(0, s.seeders);
        if (!ask.group.isEmpty() && detectReleaseGroup(name).compare(ask.group, Qt::CaseInsensitive) == 0) score += 100000;
        if (!ask.quality.isEmpty() && qualityOf(name + QLatin1Char(' ') + s.fileName) == ask.quality) score += 10000;
        if (s.seeders == 0) score = qMin(score, 0);   // nobody to download it from
        if (score > bestScore) { bestScore = score; best = &s; }
    }
    if (!best) return;   // another addon may still answer
    m_nextByVideoId.remove(videoId);

    const QString name = best->torrentName.isEmpty() ? best->title : best->torrentName;
    QString magnet = best->magnet;
    if (!magnet.contains(QStringLiteral("&dn=")))
        magnet += QStringLiteral("&dn=") + QString::fromLatin1(QUrl::toPercentEncoding(name));
    QVariantMap next;
    next[QStringLiteral("show")] = ask.show;
    next[QStringLiteral("season")] = ask.nextSeason;
    next[QStringLiteral("episode")] = ask.nextEpisode;
    next[QStringLiteral("label")] = QStringLiteral("S%1E%2").arg(ask.nextSeason, 2, 10, QLatin1Char('0'))
                                        .arg(ask.nextEpisode, 2, 10, QLatin1Char('0'));
    next[QStringLiteral("magnet")] = magnet;
    next[QStringLiteral("fileIdx")] = best->fileIdx;
    next[QStringLiteral("fileName")] = best->fileName;
    next[QStringLiteral("name")] = name;
    next[QStringLiteral("seeds")] = best->seeders;
    qInfo().noquote() << "[next]" << ask.infoHash.left(8) << "found" << next.value(QStringLiteral("label")).toString()
                      << name << best->seeders << "seeds";
    emit nextEpisodeFound(ask.infoHash, ask.fileIndex, next);
}

void QmlSearchBridge::playFoundEpisode(const QVariantMap &next)
{
    const QString magnet = next.value(QStringLiteral("magnet")).toString();
    const QString hash = SearchBridgeUtil::btihFromMagnet(magnet);
    if (hash.isEmpty()) return;
    m_session->addMagnet(magnet, m_savePath, next.value(QStringLiteral("show")).toString(),
                         static_cast<int>(ContentType::Series));
    emit addedTorrent(hash);
    // only that episode's file is fetched, as when it is picked on the page
    QVariantMap target;
    target[QStringLiteral("fileIdx")] = next.value(QStringLiteral("fileIdx"));
    target[QStringLiteral("fileName")] = next.value(QStringLiteral("fileName"));
    target[QStringLiteral("season")] = next.value(QStringLiteral("season"));
    target[QStringLiteral("episode")] = next.value(QStringLiteral("episode"));
    emit prepareAndWatch(hash, next.value(QStringLiteral("show")).toString(), target);
}
