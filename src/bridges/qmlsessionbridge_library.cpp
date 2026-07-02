// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details
//
// QmlSessionBridge — library slice. The movie-library poster projection (folds
// the torrent list into resolved cover cards, games filtered out) and the
// watchlist (persisted in QSettings as JSON). Split out of qmlsessionbridge.cpp
// verbatim; no behaviour change.

#include "bridges/qmlsessionbridge.h"
#include "torrent/sessionmanager.h"   // full IEngine + TorrentInfo
#include "services/metadata/metadataresolver.h"
#include "services/metadata/nameparser.h"

#include <QSettings>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QVariantMap>
#include <QVariantList>

QVariantList QmlSessionBridge::movieLibrary() const
{
    static const QStringList videoExts = {".mp4",".mkv",".avi",".mov",".wmv",".flv",".webm",".m4v",".ts"};
    auto stripBt = [](const QString &p){ return p.endsWith(QStringLiteral(".!bt")) ? p.chopped(4) : p; };
    QSettings s;
    QVariantList out;
    const int n = m_session->torrentCount();
    for (int row = 0; row < n; ++row) {
        if (isGameTorrent(row)) continue;                // a game's bundled .mp4 isn't a movie
        auto files = m_session->filesAt(row);
        int bestIdx = -1; qint64 bestSize = 0;
        struct Vid { int idx; QString name; int season; int episode; };
        std::vector<Vid> vids;                           // all video files (for episode picking)
        for (int i = 0; i < int(files.size()); ++i) {
            const QString mp = stripBt(files[i].path);
            for (const auto &ext : videoExts)
                if (mp.endsWith(ext, Qt::CaseInsensitive)) {
                    if (files[i].size > bestSize) { bestSize = files[i].size; bestIdx = i; }
                    const QString fname = QFileInfo(mp).fileName();
                    const ParsedName pn = NameParser::parse(fname);
                    vids.push_back({ i, fname, pn.season, pn.episode });
                    break;
                }
        }
        if (bestIdx < 0) continue;                       // no video file

        const TorrentInfo info = m_session->torrentAt(row);
        const float fprog = files[bestIdx].progress;
        const qint64 done = qint64(double(fprog) * files[bestIdx].size);
        const bool watchable = info.completed || fprog >= 0.02f || done > 5 * 1024 * 1024;
        if (!watchable) continue;                        // not enough buffered to start

        const QString hash = m_session->torrentHashAt(row);
        if (hash.isEmpty()) continue;

        // Order episodes by season then episode (loose files first stay by index),
        // and flag each as watched from its resume record.
        std::sort(vids.begin(), vids.end(), [](const Vid &a, const Vid &b) {
            if (a.season != b.season) return a.season < b.season;
            if (a.episode != b.episode) return a.episode < b.episode;
            return a.idx < b.idx;
        });
        QVariantList videos;
        bool isSeries = false;
        for (const Vid &v : vids) {
            const QString vrk = QStringLiteral("resume_%1_%2").arg(hash).arg(v.idx);
            QVariantMap vf;
            vf["idx"] = v.idx;
            vf["name"] = v.name;
            vf["season"] = v.season;
            vf["episode"] = v.episode;
            vf["watched"] = s.value(vrk + QStringLiteral("_watched"), false).toBool();
            videos << vf;
            if (v.season >= 0 && v.episode >= 0) isSeries = true;
        }

        QString poster, title = info.name, description;
        int year = 0, tmdbId = 0;
        QStringList genres;
        if (m_resolver && m_resolver->hasCached(hash)) {
            const auto meta = m_resolver->cached(hash);
            if (meta.valid) {
                if (!meta.posterPath.isEmpty()) poster = QUrl::fromLocalFile(meta.posterPath).toString();
                if (!meta.title.isEmpty()) title = meta.title;
                year = meta.year;
                tmdbId = meta.tmdbId;
                genres = meta.genres;
                description = meta.description;
            }
        }

        const QString rk = QStringLiteral("resume_%1_%2").arg(hash).arg(bestIdx);
        const qint64 resumeMs = s.value(rk, 0).toLongLong();
        const qint64 durMs    = s.value(rk + QStringLiteral("_dur"), 0).toLongLong();
        const qint64 resumeAt = s.value(rk + QStringLiteral("_at"), 0).toLongLong();

        QVariantMap m;
        m["infoHash"]   = hash;
        m["title"]      = title;
        m["year"]       = year > 0 ? QString::number(year) : QString();
        m["poster"]     = poster;
        m["fileIndex"]  = bestIdx;
        m["videos"]     = videos;                         // [{idx,name,season,episode,watched}] sorted
        m["isSeries"]   = isSeries;                       // has SxxExx markers → group by season
        m["tmdbId"]     = tmdbId;                          // for TMDB episode-title lookup
        m["genres"]     = genres;                          // taste signal for HUB recommendations
        m["description"] = description;                    // synopsis for the HUB detail drawer
        m["progress"]   = double(fprog);                 // download progress 0..1
        m["completed"]  = info.completed;
        m["size"]       = info.totalSize;
        m["addedTime"]  = qint64(info.addedTime);
        m["resumeMs"]   = resumeMs;
        m["durMs"]      = durMs;
        m["resumeAt"]   = resumeAt;                       // last-watched timestamp (ms)
        m["watchedPct"] = (durMs > 0 && resumeMs > 0) ? double(resumeMs) / double(durMs) : 0.0;
        out << m;
    }
    return out;
}

QVariantList QmlSessionBridge::watchlist() const
{
    const QByteArray raw = QSettings().value(QStringLiteral("watchlist")).toString().toUtf8();
    return QJsonDocument::fromJson(raw).array().toVariantList();
}

bool QmlSessionBridge::inWatchlist(const QString &title, const QString &type) const
{
    const QVariantList list = watchlist();
    for (const QVariant &v : list) {
        const QVariantMap m = v.toMap();
        if (m.value(QStringLiteral("title")).toString() == title
            && m.value(QStringLiteral("type")).toString() == type)
            return true;
    }
    return false;
}

void QmlSessionBridge::toggleWatchlist(const QVariantMap &item)
{
    const QString title = item.value(QStringLiteral("title")).toString();
    const QString type = item.value(QStringLiteral("type")).toString();
    if (title.isEmpty()) return;
    QVariantList list = watchlist();
    bool removed = false;
    for (int i = 0; i < list.size(); ++i) {
        const QVariantMap m = list[i].toMap();
        if (m.value(QStringLiteral("title")).toString() == title
            && m.value(QStringLiteral("type")).toString() == type) {
            list.removeAt(i); removed = true; break;
        }
    }
    if (!removed) list.prepend(item);
    QSettings().setValue(QStringLiteral("watchlist"),
                         QString::fromUtf8(QJsonDocument(QJsonArray::fromVariantList(list)).toJson(QJsonDocument::Compact)));
    emit watchlistChanged();
}

