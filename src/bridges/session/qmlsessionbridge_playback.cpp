// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details
//
// QmlSessionBridge: playback/streaming slice. Everything behind "watch": the
// local stream-server URLs, external-player handoff, sidecar/loaded subtitles,
// play-by-selection / play-by-hash, next-episode resolution, and the
// watch-when-ready polling. Split out of qmlsessionbridge.cpp verbatim.

#include "bridges/session/qmlsessionbridge.h"
#include "torrent/sessionmanager.h"   // full IEngine + TorrentInfo
#include "services/subtitles/subtitleparser.h"
#include "services/metadata/metadataresolver.h"
#include "services/metadata/episodegroup.h"
#include "services/metadata/nameparser.h"
#include "services/metadata/mkvchapters.h"
#include "services/metadata/aniskip.h"
#include "services/platform/translator.h"
#include "services/platform/utils.h"

#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QUrl>
#include <QSettings>
#include <QRegularExpression>
#include <QDesktopServices>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDebug>
#include <QLocale>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <algorithm>

void QmlSessionBridge::streamSelected()
{
    if (!hasSelection()) return;
    const int row = m_selectedIndex;
    auto files = m_session->filesAt(row);
    TorrentInfo info = m_session->torrentAt(row);
    auto stripBt = [](const QString &p){ return p.endsWith(QStringLiteral(".!bt")) ? p.chopped(4) : p; };
    const int bestIdx = bestVideoFile(row);
    if (guardRefuses(m_session->torrentHashAt(row), bestIdx)) return;
    if (bestIdx < 0) { emit toast(tr_("ctx_stream"), tr_("stream_no_video")); return; }
    prepStream(row, bestIdx);

    m_streamIndex = row; m_streamFileIdx = bestIdx;
    m_streamFilePath = stripBt(info.savePath + "/" + files[bestIdx].path);   // nice name, no ".!bt"
    m_streamTries = 0;

    if (!m_streamTimer) { m_streamTimer = new QTimer(this); m_streamTimer->setInterval(2000); }
    QObject::disconnect(m_streamTimer, nullptr, nullptr, nullptr);
    connect(m_streamTimer, &QTimer::timeout, this, [this, stripBt]() {
        if (m_streamIndex < 0 || m_streamIndex >= m_session->torrentCount()) { m_streamTimer->stop(); return; }
        auto files = m_session->filesAt(m_streamIndex);
        if (m_streamFileIdx >= int(files.size())) { m_streamTimer->stop(); return; }
        TorrentInfo cur = m_session->torrentAt(m_streamIndex);
        // Row may have shifted to a different torrent (list reordered/removed): bail.
        if (stripBt(cur.savePath + "/" + files[m_streamFileIdx].path) != m_streamFilePath) {
            m_streamTimer->stop(); return;
        }
        // Open the finished file, or the in-progress ".!bt" if that's what's on disk.
        QString actual = m_streamFilePath;
        if (!QFile::exists(actual) && QFile::exists(actual + QStringLiteral(".!bt")))
            actual += QStringLiteral(".!bt");
        const float prog = files[m_streamFileIdx].progress;
        const qint64 done = qint64(prog * files[m_streamFileIdx].size);
        if (QFile::exists(actual) && (prog >= 0.02f || done > 5*1024*1024)) {
            m_streamTimer->stop();
            if (guardRefuses(m_session->torrentHashAt(m_streamIndex), m_streamFileIdx)) return;
            if (!launchMediaPlayer(actual)) emit toast(tr_("ctx_stream"), tr_("stream_no_player"));
        } else if (++m_streamTries > 300) {   // ~10 min with no buffer (dead torrent): give up
            m_streamTimer->stop();
        }
    });
    m_streamTimer->start();
}

int QmlSessionBridge::bestVideoFile(int row) const
{
    if (row < 0 || row >= m_session->torrentCount()) return -1;
    static const QStringList videoExts = {".mp4",".mkv",".avi",".mov",".wmv",".flv",".webm",".m4v",".ts"};
    const auto files = m_session->filesAt(row);
    int bestIdx = -1; qint64 bestSize = 0;
    for (int i = 0; i < int(files.size()); ++i) {
        QString mp = files[i].path;
        if (mp.endsWith(QStringLiteral(".!bt"))) mp.chop(4);
        for (const auto &ext : videoExts)
            if (mp.endsWith(ext, Qt::CaseInsensitive)) {
                if (files[i].size > bestSize) { bestSize = files[i].size; bestIdx = i; }
                break;
            }
    }
    return bestIdx;
}

namespace {

EpisodeFile pickTargetFile(IEngine *session, int row, const QVariantMap &target)
{
    QStringList paths;
    QList<qint64> sizes;
    for (const auto &f : session->filesAt(row)) { paths << f.path; sizes << f.size; }
    return EpisodeGroup::pickFile(paths, sizes,
                                  target.value(QStringLiteral("fileIdx"), -1).toInt(),
                                  target.value(QStringLiteral("fileName")).toString(),
                                  target.value(QStringLiteral("season"), -1).toInt(),
                                  target.value(QStringLiteral("episode"), -1).toInt());
}

} // namespace

// A season or series pack holds many videos and the largest is rarely the
// episode asked for. The first answer is kept: the file list never changes.
int QmlSessionBridge::watchFile(int row, const QString &infoHash)
{
    auto it = m_watchTargets.find(infoHash);
    if (it == m_watchTargets.end()) return bestVideoFile(row);
    if (it->contains(QStringLiteral("picked"))) return it->value(QStringLiteral("picked")).toInt();
    if (m_session->filesAt(row).empty()) return -1;   // no metadata yet
    const EpisodeFile pick = pickTargetFile(m_session, row, *it);
    qInfo().noquote() << "[watch]" << infoHash.left(8) << "file" << pick.index << "by" << pick.how
                      << "(want S" << it->value(QStringLiteral("season")).toInt()
                      << "E" << it->value(QStringLiteral("episode")).toInt()
                      << "idx" << it->value(QStringLiteral("fileIdx"), -1).toInt() << ")"
                      << streamFileName(infoHash, pick.index);
    it->insert(QStringLiteral("picked"), pick.index);
    return pick.index;
}

void QmlSessionBridge::pickFileWhenReady(const QString &infoHash, const QVariantMap &target)
{
    if (infoHash.isEmpty() || target.isEmpty()) return;
    // already here in full (the whole pack was added before): leave it whole
    if (target.value(QStringLiteral("hadTorrent")).toBool() && !m_pickedFiles.contains(infoHash)) return;
    m_pendingFilePick.insert(infoHash, qMakePair(target, QDateTime::currentSecsSinceEpoch()));
}

void QmlSessionBridge::pollPendingFilePick()
{
    if (m_pendingFilePick.isEmpty()) return;
    const qint64 now = QDateTime::currentSecsSinceEpoch();
    for (const QString &hash : m_pendingFilePick.keys()) {
        const int row = m_session->torrentIndexByInfoHash(hash);
        const std::vector<FileInfo> files = row >= 0 ? m_session->filesAt(row) : std::vector<FileInfo>{};
        if (files.empty()) {
            if (now - m_pendingFilePick.value(hash).second > 600) m_pendingFilePick.remove(hash);
            continue;
        }
        const EpisodeFile pick = pickTargetFile(m_session, row, m_pendingFilePick.take(hash).first);
        if (pick.index < 0) continue;
        // E4 taken from the same pack after E3 keeps E3 coming
        QSet<int> &picked = m_pickedFiles[hash];
        picked.insert(pick.index);
        for (int i = 0; i < int(files.size()); ++i)
            m_session->setFilePriority(row, i, picked.contains(i) ? 4 : 0);
        qInfo().noquote() << "[add]" << hash.left(8) << "only file" << pick.index << "by" << pick.how
                          << streamFileName(hash, pick.index);
    }
}

void QmlSessionBridge::prepStream(int row, int fileIndex)
{
    const auto files = m_session->filesAt(row);
    if (fileIndex < 0 || fileIndex >= int(files.size())) return;
    m_session->resumeTorrent(row);   // a paused torrent would never buffer
    // The deadline window ahead of the reader keeps playback fed; outside it
    // rarest-first gets more out of the swarm than strict order does.
    m_session->setSequentialDownload(row, false);
    for (int i = 0; i < int(files.size()); ++i)
        m_session->setFilePriority(row, i, i == fileIndex ? 7 : 0);
    m_session->prioritizeFilePieceBoundaries(row, fileIndex);
}

QString QmlSessionBridge::streamUrl(int row)
{
    if (m_streamPort == 0) return {};
    const int bestIdx = bestVideoFile(row);
    if (bestIdx < 0) return {};
    const QString hash = m_session->torrentHashAt(row);
    if (hash.isEmpty()) return {};
    prepStream(row, bestIdx);
    return QStringLiteral("http://127.0.0.1:%1/stream/%2/%3").arg(m_streamPort).arg(hash).arg(bestIdx);
}

void QmlSessionBridge::openExternalForHash(const QString &infoHash, int fileIndex)
{
    const int row = m_session->torrentIndexByInfoHash(infoHash);
    if (row < 0) return;
    const QString path = m_session->streamFilePath(row, fileIndex);
    if (path.isEmpty() || guardRefuses(infoHash, fileIndex)) return;
    if (!launchMediaPlayer(path))
        emit toast(tr_("ctx_stream"), tr_("stream_no_player"));
}

// External subtitles for the built-in player: parse to plain cue maps the
// QML overlay can binary-search.
void QmlSessionBridge::playSelected()
{
    if (!hasSelection()) return;
    const int row = m_selectedIndex;
    const QString hash = m_session->torrentHashAt(row);
    if (guardRefuses(hash, bestVideoFile(row))) return;   // before streamUrl() resumes the torrent
    const QString url = streamUrl(row);
    if (url.isEmpty()) { emit toast(tr_("ctx_stream"), tr_("stream_no_video")); return; }
    const TorrentInfo info = m_session->torrentAt(row);
    const int fileIdx = url.section('/', -1).toInt();
    emit openPlayer(url, info.name, hash, fileIdx);
}

void QmlSessionBridge::playFile(const QString &infoHash, int fileIndex)
{
    const QString url = fileStreamUrl(infoHash, fileIndex);
    if (url.isEmpty()) return;
    const int row = m_session->torrentIndexByInfoHash(infoHash);
    qInfo().noquote() << "[play] playFile" << infoHash.left(8) << "file" << fileIndex;
    emit openPlayer(url, m_session->torrentAt(row).name, m_session->torrentHashAt(row), fileIndex);
}

QString QmlSessionBridge::fileStreamUrl(const QString &infoHash, int fileIndex)
{
    const int row = m_session->torrentIndexByInfoHash(infoHash);
    if (row < 0 || m_streamPort == 0) return {};
    auto files = m_session->filesAt(row);
    if (fileIndex < 0 || fileIndex >= int(files.size()) || guardRefuses(infoHash, fileIndex)) return {};
    prepStream(row, fileIndex);
    return QStringLiteral("http://127.0.0.1:%1/stream/%2/%3")
        .arg(m_streamPort).arg(m_session->torrentHashAt(row)).arg(fileIndex);
}

void QmlSessionBridge::clearResume(const QString &infoHash, int fileIndex)
{
    const QString rk = QStringLiteral("resume_%1_%2").arg(infoHash).arg(fileIndex);
    QSettings s;
    s.remove(rk);
    s.remove(rk + QStringLiteral("_dur"));
    s.remove(rk + QStringLiteral("_at"));
}

QString QmlSessionBridge::langCode(int qlocaleLanguage) const
{
    const auto lang = static_cast<QLocale::Language>(qlocaleLanguage);
    if (lang == QLocale::AnyLanguage || lang == QLocale::C) return {};
    return QLocale::languageToCode(lang, QLocale::ISO639Part1);
}

QVariantMap QmlSessionBridge::streamFileStats(const QString &infoHash, int fileIndex, double from) const
{
    const int row = m_session->torrentIndexByInfoHash(infoHash);
    if (row < 0) return {};
    QVariantMap stats = m_session->streamFileStats(row, fileIndex);
    const qint64 size = stats.value(QStringLiteral("totalBytes")).toLongLong();
    if (from >= 0 && size > 0) {
        // Bytes map to time only roughly (VBR), but the run is exact in bytes.
        const qint64 fromByte = std::clamp<qint64>(qint64(from * double(size)), 0, size - 1);
        const qint64 run = m_session->streamContiguousAvailableBytes(row, fileIndex, fromByte, size - fromByte);
        stats[QStringLiteral("bufferedFrom")] = double(fromByte) / double(size);
        stats[QStringLiteral("bufferedTo")] = double(fromByte + run) / double(size);
    }
    return stats;
}

QString QmlSessionBridge::streamLocalPath(const QString &infoHash, int fileIndex) const
{
    // Absolute on-disk path (possibly still ".!bt"): the seek-preview decoder
    // reads the file directly instead of opening a second HTTP stream session.
    const int row = m_session->torrentIndexByInfoHash(infoHash);
    if (row < 0) return {};
    return m_session->streamFilePath(row, fileIndex);
}

QString QmlSessionBridge::streamFileName(const QString &infoHash, int fileIndex) const
{
    const int row = m_session->torrentIndexByInfoHash(infoHash);
    if (row < 0) return {};
    const QStringList names = m_session->torrentFileNames(row);
    if (fileIndex < 0 || fileIndex >= names.size()) return {};
    return QString(names.at(fileIndex)).replace(QLatin1Char('\\'), QLatin1Char('/'))
        .section('/', -1);   // leaf name, whichever separator the engine used
}

void QmlSessionBridge::playByHash(const QString &infoHash)
{
    const int row = m_session->torrentIndexByInfoHash(infoHash);
    if (row < 0 || guardRefuses(infoHash, bestVideoFile(row))) return;
    const QString url = streamUrl(row);                  // preps priorities + picks the best video
    if (url.isEmpty()) { emit toast(tr_("ctx_stream"), tr_("stream_no_video")); return; }
    const TorrentInfo info = m_session->torrentAt(row);
    const int fileIdx = url.section('/', -1).toInt();
    qInfo().noquote() << "[play] playByHash" << infoHash.left(8) << "file" << fileIdx;
    emit openPlayer(url, info.name, infoHash, fileIdx);
}

// Resume a *specific* file (the episode you were watching): streamUrl() picks the
// largest video, which is wrong for a series. The HUB hero passes the resume file.
void QmlSessionBridge::playByHashFile(const QString &infoHash, int fileIndex)
{
    const int row = m_session->torrentIndexByInfoHash(infoHash);
    if (row < 0 || m_streamPort == 0) { playByHash(infoHash); return; }
    auto files = m_session->filesAt(row);
    if (fileIndex < 0 || fileIndex >= int(files.size())) { playByHash(infoHash); return; }
    if (guardRefuses(infoHash, fileIndex)) return;
    prepStream(row, fileIndex);
    const QString hash = m_session->torrentHashAt(row);
    const QString url = QStringLiteral("http://127.0.0.1:%1/stream/%2/%3").arg(m_streamPort).arg(hash).arg(fileIndex);
    const TorrentInfo info = m_session->torrentAt(row);
    qInfo().noquote() << "[play] playByHashFile" << infoHash.left(8) << "file" << fileIndex;
    emit openPlayer(url, info.name, infoHash, fileIndex);
}

QVariantMap QmlSessionBridge::episodeOf(const QString &infoHash, int fileIndex) const
{
    QVariantMap out{ { QStringLiteral("season"), -1 }, { QStringLiteral("episode"), -1 } };
    const int row = m_session->torrentIndexByInfoHash(infoHash);
    if (row < 0) return out;
    const auto files = m_session->filesAt(row);
    if (fileIndex < 0 || fileIndex >= int(files.size())) return out;
    QString name = QFileInfo(QDir::fromNativeSeparators(files[fileIndex].path)).fileName();
    if (name.endsWith(QStringLiteral(".!bt"))) name.chop(4);
    const ParsedName pn = NameParser::parse(name);
    out[QStringLiteral("season")] = pn.season;
    out[QStringLiteral("episode")] = pn.episode;
    return out;
}

QVariantMap QmlSessionBridge::watchState(const QString &infoHash) const
{
    QVariantMap out;
    const int row = m_session->torrentIndexByInfoHash(infoHash);
    if (row < 0) return out;
    static const QStringList videoExts = {".mp4",".mkv",".avi",".mov",".wmv",".flv",".webm",".m4v",".ts"};
    const auto files = m_session->filesAt(row);
    int best = -1, videos = 0;
    qint64 bestSize = 0;
    for (int i = 0; i < int(files.size()); ++i) {
        QString mp = files[i].path;
        if (mp.endsWith(QStringLiteral(".!bt"))) mp.chop(4);
        for (const auto &ext : videoExts)
            if (mp.endsWith(ext, Qt::CaseInsensitive)) {
                ++videos;
                if (files[i].size > bestSize) { bestSize = files[i].size; best = i; }
                break;
            }
    }
    out[QStringLiteral("videos")] = videos;
    if (best < 0) return out;
    QSettings s;
    const QString rk = QStringLiteral("resume_%1_%2").arg(infoHash).arg(best);
    out[QStringLiteral("fileIndex")] = best;
    out[QStringLiteral("watched")] = s.value(rk + QStringLiteral("_watched"), false).toBool();
    out[QStringLiteral("resumeMs")] = s.value(rk, 0).toLongLong();
    out[QStringLiteral("durMs")] = s.value(rk + QStringLiteral("_dur"), 0).toLongLong();
    return out;
}

int QmlSessionBridge::nextEpisode(const QString &infoHash, int fileIndex) const
{
    const int row = m_session->torrentIndexByInfoHash(infoHash);
    if (row < 0) return -1;
    static const QStringList videoExts = {".mp4",".mkv",".avi",".mov",".wmv",".flv",".webm",".m4v",".ts"};
    auto files = m_session->filesAt(row);
    struct V { int idx; int season; int episode; };
    std::vector<V> vids;
    for (int i = 0; i < int(files.size()); ++i) {
        QString mp = files[i].path;
        if (mp.endsWith(QStringLiteral(".!bt"))) mp.chop(4);
        for (const auto &ext : videoExts)
            if (mp.endsWith(ext, Qt::CaseInsensitive)) {
                const ParsedName pn = NameParser::parse(QFileInfo(QDir::fromNativeSeparators(mp)).fileName());
                vids.push_back({ i, pn.season, pn.episode });
                break;
            }
    }
    std::sort(vids.begin(), vids.end(), [](const V &a, const V &b) {
        if (a.season != b.season) return a.season < b.season;
        if (a.episode != b.episode) return a.episode < b.episode;
        return a.idx < b.idx;
    });
    for (size_t i = 0; i + 1 < vids.size(); ++i)
        if (vids[i].idx == fileIndex) return vids[i + 1].idx;
    return -1;
}

QVariantMap QmlSessionBridge::playerTitle(const QString &infoHash, int fileIndex) const
{
    QVariantMap out;
    const int row = m_session->torrentIndexByInfoHash(infoHash);
    if (row < 0) return out;

    // Raw leaf name of the playing file: the honest source, kept for the
    // info tooltip so nothing is hidden, just de-emphasised.
    const QStringList names = m_session->torrentFileNames(row);
    // libtorrent joins a pack's folder and file with '\' on Windows; cut at
    // either, or the folder ("Bleach - 001 ~ 366") is parsed as the episode
    QString raw = (fileIndex >= 0 && fileIndex < names.size())
        ? QString(names.at(fileIndex)).replace(QLatin1Char('\\'), QLatin1Char('/')).section('/', -1)
        : QString::fromStdString(m_session->torrentAt(row).name.toStdString());
    if (raw.endsWith(QLatin1String(".!bt"))) raw.chop(4);
    out["raw"] = raw;

    const ParsedName pn = NameParser::parse(raw);

    // Prefer the resolver's clean title (TMDB/IGDB) when we have it; fall back
    // to the parsed title, then the raw name.
    QString title;
    int showTmdb = 0;
    if (m_resolver && m_resolver->hasCached(infoHash)) {
        const MetadataResult meta = m_resolver->cached(infoHash);
        if (meta.valid && !meta.title.isEmpty()) title = meta.title;
        if (meta.valid) showTmdb = meta.tmdbId;
    }
    if (title.isEmpty()) title = pn.cleanTitle.trimmed();
    if (title.isEmpty()) title = raw;

    // Subtitle: series → "S4 · E10"; movie → the year. Season/episode come
    // from the per-file parse (a season pack shares one resolved title).
    // A fansub's "[Group] Show - 353" is the absolute episode, not season 1:
    // the player maps it to the real season from TMDB (PlayerWindow) and
    // shows "Ep 353" until then, never "S1 · E353".
    bool absolute = false;
    QString subtitle = episodeCode(raw, pn.season, pn.episode, &absolute);
    if (subtitle.isEmpty() && pn.year > 0)
        subtitle = QString::number(pn.year);

    out["title"] = title;
    out["subtitle"] = subtitle;
    out["season"] = pn.season;
    out["episode"] = pn.episode;
    out["absolute"] = absolute;
    out["showTmdb"] = (pn.episode > 0) ? showTmdb : 0;
    return out;
}

QVariantList QmlSessionBridge::mkvChapters(const QString &infoHash, int fileIndex) const
{
    QVariantList out;
    const int row = m_session->torrentIndexByInfoHash(infoHash);
    if (row < 0) return out;
    QString path = m_session->streamFilePath(row, fileIndex);
    if (path.isEmpty() || !path.endsWith(QLatin1String(".mkv"), Qt::CaseInsensitive)) {
        // may still be mid-download under the .!bt suffix
        if (!path.endsWith(QLatin1String(".mkv.!bt"), Qt::CaseInsensitive)) return out;
    }
    for (const MkvChapter &c : readMkvChapters(path)) {
        QVariantMap m;
        m["startMs"] = c.startMs;
        m["endMs"] = c.endMs;
        m["name"] = c.name;
        m["kind"] = c.kind;
        out << m;
    }
    return out;
}

void QmlSessionBridge::fetchSkipTimes(const QString &infoHash, int fileIndex)
{
    const int row = m_session->torrentIndexByInfoHash(infoHash);
    if (row < 0 || !m_resolver || !m_resolver->hasCached(infoHash)) return;
    const MetadataResult meta = m_resolver->cached(infoHash);
    if (!meta.valid || meta.title.isEmpty()) return;
    // TMDB names the genre in the content language
    bool animated = false;
    for (const QString &g : meta.genres)
        if (g.startsWith(QLatin1String("Anim"), Qt::CaseInsensitive) || g.contains(QStringLiteral("アニメ"))
            || g.contains(QStringLiteral("动画")) || g.contains(QStringLiteral("ульт")))
            animated = true;
    if (!animated) return;
    const QVariantMap ep = episodeOf(infoHash, fileIndex);
    const int season = ep.value(QStringLiteral("season")).toInt();
    const int episode = ep.value(QStringLiteral("episode")).toInt();
    // MyAnimeList gives each season an entry of its own, so episode N of a
    // later season is not episode N of the show: only the first lines up.
    if (episode <= 0 || season > 1) return;

    if (!m_skipNet) m_skipNet = new QNetworkAccessManager(this);
    auto askTimes = [this, infoHash, fileIndex, episode](int malId) {
        QNetworkRequest req(AniSkip::skipTimesUrl(malId, episode));
        req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("BATorrent"));
        QNetworkReply *r = m_skipNet->get(req);
        connect(r, &QNetworkReply::finished, this, [this, r, infoHash, fileIndex, malId, episode] {
            r->deleteLater();
            const auto chapters = AniSkip::chaptersFromSkipTimes(r->readAll());
            qInfo().noquote() << "[skip] AniSkip" << malId << "ep" << episode << ":" << chapters.size() << "times";
            if (chapters.isEmpty()) return;
            QVariantList out;
            for (const MkvChapter &c : chapters)
                out << QVariantMap{ { QStringLiteral("startMs"), c.startMs }, { QStringLiteral("endMs"), c.endMs },
                                    { QStringLiteral("name"), c.name }, { QStringLiteral("kind"), c.kind } };
            emit skipTimesReady(infoHash, fileIndex, out);
        });
    };

    const QString key = QStringLiteral("malId/") + (meta.tmdbId > 0 ? QString::number(meta.tmdbId) : meta.title);
    const int known = QSettings().value(key, 0).toInt();
    if (known > 0) { askTimes(known); return; }
    QStringList names{ meta.title };
    const QString parsed = NameParser::parse(m_session->torrentAt(row).name).cleanTitle;
    if (!parsed.isEmpty()) names << parsed;
    QNetworkRequest req(AniSkip::malSearchUrl());
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("BATorrent"));
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    QNetworkReply *r = m_skipNet->post(req, AniSkip::malSearchBody(parsed.isEmpty() ? meta.title : parsed));
    connect(r, &QNetworkReply::finished, this, [r, names, key, askTimes] {
        r->deleteLater();
        const int malId = AniSkip::malIdFromSearch(r->readAll(), names);
        if (malId <= 0) return;
        QSettings().setValue(key, malId);
        askTimes(malId);
    });
}

QString QmlSessionBridge::posterForHash(const QString &infoHash) const
{
    if (!m_resolver || !m_resolver->hasCached(infoHash)) return {};
    const MetadataResult meta = m_resolver->cached(infoHash);
    if (!meta.valid || meta.posterPath.isEmpty()) return {};
    return meta.posterPath;
}
