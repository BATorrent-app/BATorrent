// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details
//
// QmlSessionBridge — playback/streaming slice. Everything behind "watch": the
// local stream-server URLs, external-player handoff, sidecar/loaded subtitles,
// play-by-selection / play-by-hash, next-episode resolution, and the
// watch-when-ready polling. Split out of qmlsessionbridge.cpp verbatim.

#include "bridges/qmlsessionbridge.h"
#include "torrent/sessionmanager.h"   // full IEngine + TorrentInfo
#include "services/subtitles/subtitleparser.h"
#include "services/metadata/metadataresolver.h"
#include "services/metadata/nameparser.h"
#include "services/metadata/mkvchapters.h"
#include "services/platform/translator.h"
#include "services/platform/utils.h"

#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QUrl>
#include <QSettings>
#include <QDesktopServices>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

void QmlSessionBridge::streamSelected()
{
    if (!hasSelection()) return;
    static const QStringList videoExts = {".mp4",".mkv",".avi",".mov",".wmv",".flv",".webm",".m4v",".ts"};
    const int row = m_selectedIndex;
    auto files = m_session->filesAt(row);
    TorrentInfo info = m_session->torrentAt(row);
    int bestIdx = -1; qint64 bestSize = 0;
    auto stripBt = [](const QString &p){ return p.endsWith(QStringLiteral(".!bt")) ? p.chopped(4) : p; };
    for (int i = 0; i < int(files.size()); ++i) {
        const QString mp = stripBt(files[i].path);
        for (const auto &ext : videoExts)
            if (mp.endsWith(ext, Qt::CaseInsensitive)) {
                if (files[i].size > bestSize) { bestSize = files[i].size; bestIdx = i; }
                break;
            }
    }
    if (bestIdx < 0) { emit toast(tr_("ctx_stream"), tr_("stream_no_video")); return; }

    m_session->resumeTorrent(row);   // a paused torrent would never buffer
    m_session->setSequentialDownload(row, true);
    for (int i = 0; i < int(files.size()); ++i)
        if (i != bestIdx) m_session->setFilePriority(row, i, 0);
    m_session->setFilePriority(row, bestIdx, 7);
    m_session->prioritizeFilePieceBoundaries(row, bestIdx);

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
        // Row may have shifted to a different torrent (list reordered/removed) — bail.
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
            const bool opened = launchMediaPlayer(actual);
            emit toast(tr_("ctx_stream"), opened ? tr_("stream_started").arg(cur.name)
                                                 : tr_("stream_no_player"));
        } else if (++m_streamTries > 300) {   // ~10 min with no buffer (dead torrent) — give up
            m_streamTimer->stop();
        }
    });
    m_streamTimer->start();
    emit toast(tr_("ctx_stream"), tr_("stream_started").arg(info.name));
}

QString QmlSessionBridge::streamUrl(int row)
{
    if (m_streamPort == 0) return {};
    if (row < 0 || row >= m_session->torrentCount()) return {};
    static const QStringList videoExts = {".mp4",".mkv",".avi",".mov",".wmv",".flv",".webm",".m4v",".ts"};
    auto files = m_session->filesAt(row);
    auto stripBt = [](const QString &p){ return p.endsWith(QStringLiteral(".!bt")) ? p.chopped(4) : p; };
    int bestIdx = -1; qint64 bestSize = 0;
    for (int i = 0; i < int(files.size()); ++i) {
        const QString mp = stripBt(files[i].path);
        for (const auto &ext : videoExts)
            if (mp.endsWith(ext, Qt::CaseInsensitive)) {
                if (files[i].size > bestSize) { bestSize = files[i].size; bestIdx = i; }
                break;
            }
    }
    if (bestIdx < 0) return {};
    const QString hash = m_session->torrentHashAt(row);
    if (hash.isEmpty()) return {};

    // same prep as the external stream: resume, sequential, only the video
    // file at full priority, and boost the header/index pieces.
    m_session->resumeTorrent(row);
    m_session->setSequentialDownload(row, true);
    for (int i = 0; i < int(files.size()); ++i)
        if (i != bestIdx) m_session->setFilePriority(row, i, 0);
    m_session->setFilePriority(row, bestIdx, 7);
    m_session->prioritizeFilePieceBoundaries(row, bestIdx);

    return QStringLiteral("http://127.0.0.1:%1/stream/%2/%3").arg(m_streamPort).arg(hash).arg(bestIdx);
}

void QmlSessionBridge::openExternalForHash(const QString &infoHash, int fileIndex)
{
    const int row = m_session->torrentIndexByInfoHash(infoHash);
    if (row < 0) return;
    const QString path = m_session->streamFilePath(row, fileIndex);
    if (path.isEmpty()) return;
    if (!launchMediaPlayer(path))
        emit toast(tr_("ctx_stream"), tr_("stream_no_player"));
}

// External subtitles for the built-in player: parse to plain cue maps the
// QML overlay can binary-search.
QVariantList QmlSessionBridge::loadSubtitleFile(const QString &path)
{
    QString p = path;
    if (p.startsWith(QLatin1String("file://"))) p = QUrl(p).toLocalFile();
    QVariantList out;
    const auto cues = SubtitleParser::parseFile(p);
    out.reserve(cues.size());
    for (const auto &c : cues) {
        QVariantMap m;
        m["start"] = c.startMs;
        m["end"] = c.endMs;
        m["text"] = c.text;
        out << m;
    }
    return out;
}

QString QmlSessionBridge::findSidecarSubtitle(const QString &infoHash, int fileIndex)
{
    const int row = m_session->torrentIndexByInfoHash(infoHash);
    if (row < 0) return {};
    QString video = m_session->streamFilePath(row, fileIndex);
    if (video.isEmpty()) return {};
    if (video.endsWith(QLatin1String(".!bt"))) video.chop(4);
    const QFileInfo vi(video);
    const QString base = vi.completeBaseName();
    QDir dir = vi.dir();
    for (const char *ext : {"srt", "vtt"}) {
        const QString exact = dir.filePath(base + QLatin1Char('.') + QLatin1String(ext));
        if (QFileInfo::exists(exact)) return exact;
    }
    // language-suffixed sidecars ("Movie.pt-BR.srt") sort first by name
    const QStringList matches = dir.entryList({base + QStringLiteral("*.srt"), base + QStringLiteral("*.vtt")},
                                              QDir::Files, QDir::Name);
    return matches.isEmpty() ? QString() : dir.filePath(matches.first());
}

void QmlSessionBridge::playSelected()
{
    if (!hasSelection()) return;
    const int row = m_selectedIndex;
    const QString url = streamUrl(row);
    if (url.isEmpty()) { emit toast(tr_("ctx_stream"), tr_("stream_no_video")); return; }
    const TorrentInfo info = m_session->torrentAt(row);
    const QString hash = m_session->torrentHashAt(row);
    const int fileIdx = url.section('/', -1).toInt();
    emit openPlayer(url, info.name, hash, fileIdx);
}

void QmlSessionBridge::playFile(const QString &infoHash, int fileIndex)
{
    const int row = m_session->torrentIndexByInfoHash(infoHash);
    if (row < 0 || m_streamPort == 0) return;
    auto files = m_session->filesAt(row);
    if (fileIndex < 0 || fileIndex >= int(files.size())) return;
    m_session->resumeTorrent(row);
    m_session->setSequentialDownload(row, true);
    for (int i = 0; i < int(files.size()); ++i)
        m_session->setFilePriority(row, i, i == fileIndex ? 7 : 0);
    m_session->prioritizeFilePieceBoundaries(row, fileIndex);
    const QString hash = m_session->torrentHashAt(row);
    const TorrentInfo info = m_session->torrentAt(row);
    emit openPlayer(QStringLiteral("http://127.0.0.1:%1/stream/%2/%3").arg(m_streamPort).arg(hash).arg(fileIndex),
                    info.name, hash, fileIndex);
}

void QmlSessionBridge::clearResume(const QString &infoHash, int fileIndex)
{
    const QString rk = QStringLiteral("resume_%1_%2").arg(infoHash).arg(fileIndex);
    QSettings s;
    s.remove(rk);
    s.remove(rk + QStringLiteral("_dur"));
    s.remove(rk + QStringLiteral("_at"));
}

QVariantMap QmlSessionBridge::streamFileStats(const QString &infoHash, int fileIndex) const
{
    const int row = m_session->torrentIndexByInfoHash(infoHash);
    if (row < 0) return {};
    return m_session->streamFileStats(row, fileIndex);
}

QString QmlSessionBridge::streamLocalPath(const QString &infoHash, int fileIndex) const
{
    // Absolute on-disk path (possibly still ".!bt") — the seek-preview decoder
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
    return names.at(fileIndex).section('/', -1);   // leaf name
}

void QmlSessionBridge::playByHash(const QString &infoHash)
{
    const int row = m_session->torrentIndexByInfoHash(infoHash);
    if (row < 0) return;
    const QString url = streamUrl(row);                  // preps priorities + picks the best video
    if (url.isEmpty()) { emit toast(tr_("ctx_stream"), tr_("stream_no_video")); return; }
    const TorrentInfo info = m_session->torrentAt(row);
    const int fileIdx = url.section('/', -1).toInt();
    emit openPlayer(url, info.name, infoHash, fileIdx);
}

// Resume a *specific* file (the episode you were watching) — streamUrl() picks the
// largest video, which is wrong for a series. The HUB hero passes the resume file.
void QmlSessionBridge::playByHashFile(const QString &infoHash, int fileIndex)
{
    const int row = m_session->torrentIndexByInfoHash(infoHash);
    if (row < 0 || m_streamPort == 0) { playByHash(infoHash); return; }
    auto files = m_session->filesAt(row);
    if (fileIndex < 0 || fileIndex >= int(files.size())) { playByHash(infoHash); return; }
    m_session->resumeTorrent(row);
    m_session->setSequentialDownload(row, true);
    for (int i = 0; i < int(files.size()); ++i)
        m_session->setFilePriority(row, i, i == fileIndex ? 7 : 0);
    m_session->prioritizeFilePieceBoundaries(row, fileIndex);
    const QString hash = m_session->torrentHashAt(row);
    const QString url = QStringLiteral("http://127.0.0.1:%1/stream/%2/%3").arg(m_streamPort).arg(hash).arg(fileIndex);
    const TorrentInfo info = m_session->torrentAt(row);
    emit openPlayer(url, info.name, infoHash, fileIndex);
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
                const ParsedName pn = NameParser::parse(QFileInfo(mp).fileName());
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

    // Raw leaf name of the playing file — the honest source, kept for the
    // info tooltip so nothing is hidden, just de-emphasised.
    const QStringList names = m_session->torrentFileNames(row);
    QString raw = (fileIndex >= 0 && fileIndex < names.size())
        ? names.at(fileIndex).section('/', -1)
        : QString::fromStdString(m_session->torrentAt(row).name.toStdString());
    if (raw.endsWith(QLatin1String(".!bt"))) raw.chop(4);
    out["raw"] = raw;

    const ParsedName pn = NameParser::parse(raw);

    // Prefer the resolver's clean title (TMDB/IGDB) when we have it; fall back
    // to the parsed title, then the raw name.
    QString title;
    if (m_resolver && m_resolver->hasCached(infoHash)) {
        const MetadataResult meta = m_resolver->cached(infoHash);
        if (meta.valid && !meta.title.isEmpty()) title = meta.title;
    }
    if (title.isEmpty()) title = pn.cleanTitle.trimmed();
    if (title.isEmpty()) title = raw;

    // Subtitle: series → "S4 · E10"; movie → the year. Season/episode come
    // from the per-file parse (a season pack shares one resolved title).
    QString subtitle;
    if (pn.season >= 0 && pn.episode >= 0)
        subtitle = QStringLiteral("S%1 · E%2").arg(pn.season).arg(pn.episode);
    else if (pn.year > 0)
        subtitle = QString::number(pn.year);

    out["title"] = title;
    out["subtitle"] = subtitle;
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

QString QmlSessionBridge::posterForHash(const QString &infoHash) const
{
    if (!m_resolver || !m_resolver->hasCached(infoHash)) return {};
    const MetadataResult meta = m_resolver->cached(infoHash);
    if (!meta.valid || meta.posterPath.isEmpty()) return {};
    return meta.posterPath;
}

void QmlSessionBridge::watchWhenReady(const QString &infoHash, const QString &title)
{
    if (infoHash.isEmpty()) return;
    m_pendingWatch.insert(infoHash, qMakePair(title, QDateTime::currentSecsSinceEpoch()));
    emit watchBuffering(title);
}

void QmlSessionBridge::cancelWatch(const QString &infoHash)
{
    m_pendingWatch.remove(infoHash);
}

// Runs each ~1s tick: open the player for any pending Get&Watch hash that has
// become playable; give up after ~2 min of no metadata/seeds.
void QmlSessionBridge::onWatchTick()
{
    pollRunningGames();
    pollInstallWatch();
    if (m_pendingWatch.isEmpty()) return;
    const qint64 now = QDateTime::currentSecsSinceEpoch();
    for (const QString &hash : m_pendingWatch.keys()) {
        const int idx = m_session->torrentIndexByInfoHash(hash);
        if (idx >= 0) {
            const TorrentInfo info = m_session->torrentAt(idx);
            emit watchProgress(hash, info.progress);
            if (m_session->torrentHasVideo(idx)) {
                const bool ready = info.completed || info.progress >= 0.02f
                                 || info.totalDone > 5LL * 1024 * 1024;
                if (ready) {
                    m_pendingWatch.remove(hash);
                    playByHash(hash);
                    continue;
                }
            }
        }
        if (now - m_pendingWatch.value(hash).second > 120)
            emit watchFailed(m_pendingWatch.take(hash).first);
    }
}

// The game library + launch + install pipeline lives in
// qmlsessionbridge_games.cpp.

