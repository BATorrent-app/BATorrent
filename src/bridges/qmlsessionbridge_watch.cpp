// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details
//
// QmlSessionBridge — watch-when-ready buffering gate for Get & Watch.

#include "bridges/qmlsessionbridge.h"
#include "torrent/sessionmanager.h"

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
    pollPendingInstall();
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

