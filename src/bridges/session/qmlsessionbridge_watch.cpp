// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details
//
// QmlSessionBridge: watch-when-ready buffering gate for Get & Watch.

#include "bridges/session/qmlsessionbridge.h"
#include "torrent/sessionmanager.h"
#include "torrent/streamgate.h"

#include <QDebug>

void QmlSessionBridge::watchWhenReady(const QString &infoHash, const QString &title)
{
    if (infoHash.isEmpty()) return;
    m_pendingWatch.insert(infoHash, qMakePair(title, QDateTime::currentSecsSinceEpoch()));
    emit watchBuffering(title);
}

void QmlSessionBridge::cancelWatch(const QString &infoHash)
{
    m_pendingWatch.remove(infoHash);
    m_watchPrepped.remove(infoHash);
}

void QmlSessionBridge::abandonWatch(const QString &infoHash)
{
    cancelWatch(infoHash);
    removeTorrentByHash(infoHash, true, false);
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
        const int file = idx >= 0 ? bestVideoFile(idx) : -1;
        if (file >= 0) {
            // Until the player opens the torrent is rarest-first: fetch it the player's way now.
            if (!m_watchPrepped.contains(hash)) { prepStream(idx, file); m_watchPrepped.insert(hash); }
            const qint64 size = m_session->streamFileSize(idx, file);
            const qint64 head = m_session->streamContiguousAvailableBytes(idx, file, 0,
                                                                          StreamGate::headBytes(size));
            const bool tail = size > 0 && m_session->streamContiguousAvailableBytes(idx, file, size - 1, 1) > 0;
            const bool tailNeeded = StreamGate::needsTail(m_session->streamFilePath(idx, file));
            qInfo().noquote() << "[watch]" << hash.left(8) << "head" << head << "/"
                              << StreamGate::headBytes(size) << "tail" << tail << "needed" << tailNeeded;
            const TorrentInfo info = m_session->torrentAt(idx);
            emit watchProgress(hash, StreamGate::progress(head, size, tail, tailNeeded));
            emit watchHealth(hash, info.downloadRate, info.numPeers,
                             int(now - m_pendingWatch.value(hash).second));
            if (info.completed || StreamGate::ready(head, size, tail, tailNeeded)) {
                m_pendingWatch.remove(hash);
                m_watchPrepped.remove(hash);
                playByHashFile(hash, file);
                continue;
            }
            // Priority alone loses to a slow peer holding the first piece; a
            // deadline is what jumps the queue (the tail already has one).
            m_session->streamSetDeadlineWindow(idx, file, head);
        }
        if (now - m_pendingWatch.value(hash).second > 120) {
            m_watchPrepped.remove(hash);
            emit watchFailed(m_pendingWatch.take(hash).first);
        }
    }
}

// The game library + launch + install pipeline lives in
// qmlsessionbridge_games.cpp.

