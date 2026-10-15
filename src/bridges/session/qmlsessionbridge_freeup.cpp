// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details
//
// QmlSessionBridge: what the Free up space window lists. Every item on one
// disk with the facts its reason is made of (played, watched, paused for how
// long, an episode a season pack already holds), and removing several at once.

#include "bridges/session/qmlsessionbridge.h"
#include "bridges/librarygroups.h"
#include "torrent/iengine.h"
#include "services/games/playlog.h"
#include "services/metadata/episodegroup.h"
#include "services/metadata/metadataresolver.h"
#include "services/metadata/nameparser.h"
#include "services/platform/translator.h"
#include "services/platform/utils.h"

#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QSettings>
#include <QStorageInfo>
#include <QUrl>

namespace {
QString rootOf(const QString &path)
{
    QDir d(path.isEmpty() ? QDir::homePath() : path);
    while (!d.exists() && !d.isRoot())
        if (!d.cdUp()) break;
    return QStorageInfo(d.absolutePath()).rootPath();
}
}

// When each item stopped, kept across runs. libtorrent does not say how long
// something has been paused, and "paused for months" is what makes it a
// candidate. One first seen already paused at 0% has sat there since it was
// added; any other one counts from now.
void QmlSessionBridge::trackStillSince()
{
    if (!m_stillLoaded) {
        const QVariantMap saved = QSettings().value(QStringLiteral("pausedSince")).toMap();
        for (auto it = saved.constBegin(); it != saved.constEnd(); ++it)
            m_pausedSince.insert(it.key(), it.value().toLongLong());
        m_stillLoaded = true;
    }
    const qint64 now = QDateTime::currentSecsSinceEpoch();
    bool changed = false;
    QSet<QString> live;
    for (int i = 0; i < m_session->torrentCount(); ++i) {
        const QString h = m_session->torrentHashAt(i);
        if (h.isEmpty()) continue;
        live.insert(h);
        const TorrentInfo info = m_session->torrentAt(i);
        const bool still = info.paused && !info.queued;
        if (still && !m_pausedSince.contains(h)) {
            m_pausedSince.insert(h, info.progress <= 0.0f && info.addedTime > 0 ? info.addedTime : now);
            changed = true;
        } else if (!still && m_pausedSince.remove(h)) {
            changed = true;
        }
    }
    for (auto it = m_pausedSince.begin(); it != m_pausedSince.end(); ) {
        if (!live.contains(it.key()) && m_session->torrentCount() > 0) { it = m_pausedSince.erase(it); changed = true; }
        else ++it;
    }
    if (changed) {
        QVariantMap out;
        for (auto it = m_pausedSince.constBegin(); it != m_pausedSince.constEnd(); ++it) out.insert(it.key(), it.value());
        QSettings().setValue(QStringLiteral("pausedSince"), out);
    }
}

QVariantMap QmlSessionBridge::freeUpSpace(const QString &path) const
{
    QString p = path;
    if (p.startsWith(QLatin1String("file://"))) p = QUrl(p).toLocalFile();
    if (p.isEmpty()) p = defaultSavePath();
    const QString root = rootOf(p);
    const QStorageInfo si(root);

    QVariantMap out;
    out["disk"] = si.displayName().isEmpty() ? root : si.displayName();
    out["freeBytes"] = double(si.bytesAvailable());

    // season packs by show and season, for "also in the pack"
    QHash<QString, QString> packs;   // nameKey + season -> the pack's name
    const int n = m_session->torrentCount();
    QVector<ParsedName> parsed(n);
    for (int i = 0; i < n; ++i) {
        parsed[i] = NameParser::parse(m_session->torrentAt(i).name);
        // the library's reading: a season pack has no episode, whatever the name parser guessed
        const EpisodeTag tag = EpisodeGroup::classify(m_session->torrentAt(i).name);
        if (tag.season >= 0) parsed[i].season = tag.season;
        parsed[i].episode = tag.episode >= 0 ? tag.episode : (tag.pack ? -1 : parsed[i].episode);
        if (parsed[i].season > 0 && parsed[i].episode < 0)
            packs.insert(LibraryGroups::nameKey(parsed[i].cleanTitle) + QLatin1Char('#') + QString::number(parsed[i].season),
                         m_session->torrentAt(i).name);
    }

    QSettings st;
    const QStringList keys = st.allKeys();
    const qint64 now = QDateTime::currentSecsSinceEpoch();
    QVariantList rows;
    for (int i = 0; i < n; ++i) {
        const TorrentInfo info = m_session->torrentAt(i);
        if (rootOf(info.savePath) != root) continue;
        const QString hash = m_session->torrentHashAt(i);
        QVariantMap m;
        m["infoHash"] = hash;
        QString title = parsed[i].cleanTitle.isEmpty() ? info.name : parsed[i].cleanTitle;
        if (m_resolver && m_resolver->hasCached(hash)) {
            const auto meta = m_resolver->cached(hash);
            if (meta.valid) {
                if (!meta.title.isEmpty()) title = meta.title;
                if (QFileInfo::exists(meta.posterPath)) m["poster"] = meta.posterPath;
            }
        }
        if (parsed[i].season > 0 && parsed[i].episode > 0)
            title += QStringLiteral(" · S%1E%2").arg(parsed[i].season, 2, 10, QLatin1Char('0'))
                                                 .arg(parsed[i].episode, 2, 10, QLatin1Char('0'));
        else if (parsed[i].season > 0)
            title += QStringLiteral(" · S%1").arg(parsed[i].season, 2, 10, QLatin1Char('0'));
        m["name"] = title;
        m["sizeBytes"] = double(info.totalSize);
        m["addedTime"] = info.addedTime;
        m["paused"] = info.paused && !info.queued;
        m["completed"] = info.completed || info.finished;
        m["progress"] = info.progress;
        m["upRate"] = info.uploadRate;
        m["pausedSince"] = m_pausedSince.value(hash, 0);
        m["lastPlayed"] = m_playLog ? double(m_playLog->summary(hash).lastPlayedMs / 1000) : 0.0;
        // watched: the player marks a file watched; the newest mark counts
        qint64 watchedAt = 0;
        const QString pre = QStringLiteral("resume_") + hash + QLatin1Char('_');
        for (const QString &k : keys) {
            if (!k.startsWith(pre) || !k.endsWith(QLatin1String("_watched"))) continue;
            if (!st.value(k).toBool()) continue;
            const qint64 at = st.value(k.left(k.size() - 8) + QStringLiteral("_at"), 0).toLongLong();
            watchedAt = qMax(watchedAt, at > 1e12 ? at / 1000 : (at > 0 ? at : now));
        }
        m["watchedAt"] = double(watchedAt);
        if (parsed[i].episode > 0 && parsed[i].season > 0)
            m["inPack"] = packs.value(LibraryGroups::nameKey(parsed[i].cleanTitle) + QLatin1Char('#')
                                      + QString::number(parsed[i].season));
        rows << m;
    }
    out["rows"] = rows;
    return out;
}

void QmlSessionBridge::removeTorrentsByHash(const QStringList &hashes, bool deleteFiles)
{
    const QString selected = m_selectedIndex >= 0 ? m_session->torrentHashAt(m_selectedIndex) : QString();
    int done = 0;
    for (const QString &h : hashes) {
        const int i = m_session->torrentIndexByInfoHash(h);
        if (i < 0) continue;
        m_session->removeTorrent(i, deleteFiles, false);
        ++done;
    }
    if (done > 0 && m_selectedIndex >= 0) {
        const int now = hashes.contains(selected) ? -1 : m_session->torrentIndexByInfoHash(selected);
        if (now != m_selectedIndex) { m_selectedIndex = now; emit selectionChanged(); emit selectionListsChanged(); }
    }
}

QString QmlSessionBridge::shownTitle(const QString &infoHash, const QString &rawName, QString *poster) const
{
    QString title = NameParser::parse(rawName).cleanTitle;
    if (title.isEmpty()) title = rawName;
    if (m_resolver && m_resolver->hasCached(infoHash)) {
        const auto meta = m_resolver->cached(infoHash);
        if (meta.valid) {
            if (!meta.title.isEmpty()) title = meta.title;
            if (poster && QFileInfo::exists(meta.posterPath)) *poster = QUrl::fromLocalFile(meta.posterPath).toString();
        }
    }
    return title;
}

void QmlSessionBridge::keepTrying(const QString &infoHash)
{
    const int i = m_session->torrentIndexByInfoHash(infoHash);
    if (i < 0) return;
    m_fetchSnooze.insert(infoHash, qMax<qint64>(0, m_session->torrentAt(i).fetchingSecs) + 600);
    m_needsSeen.remove(infoHash + QStringLiteral("/fetching"));
}

// What only the person can fix, said once per item and reason: a magnet that
// has found no one to send its file list for ten minutes, a download the disk
// has no room for (with the disk that does have it), files gone from disk.
void QmlSessionBridge::watchNeeds()
{
    QSet<QString> live;
    for (int i = 0; i < m_session->torrentCount(); ++i) {
        const QString hash = m_session->torrentHashAt(i);
        if (hash.isEmpty()) continue;
        const TorrentInfo info = m_session->torrentAt(i);
        QString reason;
        if (info.fetchingMetadata && fetchingTooLong(hash, info.fetchingSecs)) reason = QStringLiteral("fetching");
        else if (info.filesMissing) reason = QStringLiteral("missing");
        else if (info.hasError && info.diskFull)
            reason = QStringLiteral("nospace");
        if (reason.isEmpty()) continue;
        const QString key = hash + QLatin1Char('/') + reason;
        live.insert(key);
        if (m_needsSeen.contains(key)) continue;
        m_needsSeen.insert(key);

        QVariantMap n;
        n[QStringLiteral("infoHash")] = hash;
        n[QStringLiteral("reason")] = reason;
        QString poster;
        n[QStringLiteral("title")] = shownTitle(hash, info.name, &poster);
        if (!poster.isEmpty()) n[QStringLiteral("poster")] = poster;
        n[QStringLiteral("minutes")] = int(qMax<qint64>(0, info.fetchingSecs) / 60);
        if (reason == QLatin1String("nospace")) {
            const qint64 need = qMax<qint64>(0, info.totalSize - info.totalDone);
            n[QStringLiteral("need")] = formatSize(need);
            // the roomiest other disk that holds what is left
            QStorageInfo best;
            const QString here = rootOf(info.savePath);
            for (const QStorageInfo &v : QStorageInfo::mountedVolumes()) {
                if (!v.isValid() || !v.isReady() || v.isReadOnly() || v.rootPath() == here) continue;
                if (v.bytesAvailable() < need) continue;
                if (!best.isValid() || v.bytesAvailable() > best.bytesAvailable()) best = v;
            }
            if (best.isValid()) {
                n[QStringLiteral("disk")] = QDir::toNativeSeparators(best.rootPath()).remove(QLatin1Char('\\'));
                n[QStringLiteral("diskPath")] = QDir(best.rootPath()).filePath(QFileInfo(info.savePath).fileName());
                n[QStringLiteral("diskFree")] = formatSize(best.bytesAvailable());
            }
        }
        emit needsYou(n);
    }
    // a fixed problem may come back later and be said again
    for (auto it = m_needsSeen.begin(); it != m_needsSeen.end(); )
        it = live.contains(*it) ? std::next(it) : m_needsSeen.erase(it);
}
