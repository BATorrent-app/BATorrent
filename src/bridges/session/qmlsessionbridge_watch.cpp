// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details
//
// QmlSessionBridge: watch-when-ready buffering gate for Get & Watch.

#include "bridges/session/qmlsessionbridge.h"
#include "torrent/sessionmanager.h"
#include "torrent/streamgate.h"

#include <QDebug>

void QmlSessionBridge::watchWhenReady(const QString &infoHash, const QString &title,
                                      const QVariantMap &target)
{
    if (infoHash.isEmpty()) return;
    m_pendingWatch.insert(infoHash, qMakePair(title, QDateTime::currentSecsSinceEpoch()));
    if (target.isEmpty()) m_watchTargets.remove(infoHash);
    else m_watchTargets.insert(infoHash, target);
    m_watchClock[infoHash].start();
    if (!m_watchFast.isActive()) m_watchFast.start();
    emit watchBuffering(title);
}

void QmlSessionBridge::cancelWatch(const QString &infoHash)
{
    m_pendingWatch.remove(infoHash);
    m_watchPrepped.remove(infoHash);
    m_watchClock.remove(infoHash);
    m_watchTargets.remove(infoHash);
    m_watchNoPeersSince.remove(infoHash);
}

void QmlSessionBridge::abandonWatch(const QString &infoHash)
{
    cancelWatch(infoHash);
    removeTorrentByHash(infoHash, true, false);
}

void QmlSessionBridge::onWatchTick()
{
    pollRunningGames();
    pollInstallWatch();
    pollPendingInstall();
    pollPendingFilePick();
    pollPendingWatch(true);
}

// Open the player for any pending Get&Watch hash that has become playable;
// give up after ~2 min of no metadata/seeds. Runs on the engine's ~1 s tick
// (verbose) and on m_watchFast in between, so a ready file isn't left waiting.
void QmlSessionBridge::pollPendingWatch(bool verbose)
{
    if (m_pendingWatch.isEmpty()) { m_watchFast.stop(); return; }
    const qint64 now = QDateTime::currentSecsSinceEpoch();
    for (const QString &hash : m_pendingWatch.keys()) {
        const int idx = m_session->torrentIndexByInfoHash(hash);
        const int file = idx >= 0 ? watchFile(idx, hash) : -1;
        const qint64 waited = now - m_pendingWatch.value(hash).second;
        QVariantMap st;
        st[QStringLiteral("meta")] = file >= 0;
        st[QStringLiteral("elapsed")] = waited;
        if (idx >= 0) {
            const TorrentInfo info = m_session->torrentAt(idx);
            st[QStringLiteral("down")] = info.downloadRate;
            st[QStringLiteral("peers")] = info.numPeers;
            st[QStringLiteral("seeds")] = info.numSeeds;
            // nobody to get it from for 45 s: stop pretending, keep the download
            if (info.numPeers > 0) m_watchNoPeersSince.remove(hash);
            else if (!m_watchNoPeersSince.contains(hash)) m_watchNoPeersSince.insert(hash, now);
            if (m_watchNoPeersSince.contains(hash) && now - m_watchNoPeersSince.value(hash) >= 45) {
                st[QStringLiteral("noSource")] = true;
                emit waitState(hash, st);
                cancelWatch(hash);
                continue;
            }
        }
        if (file >= 0) {
            // Until the player opens the torrent is rarest-first: fetch it the player's way now.
            if (!m_watchPrepped.contains(hash)) {
                prepStream(idx, file);
                m_watchPrepped.insert(hash);
                qInfo().noquote() << "[watch]" << hash.left(8) << "metadata after"
                                  << m_watchClock.value(hash).elapsed() << "ms";
            }
            const qint64 size = m_session->streamFileSize(idx, file);
            const qint64 head = m_session->streamContiguousAvailableBytes(idx, file, 0,
                                                                          StreamGate::headBytes(size));
            const bool tail = size > 0 && m_session->streamContiguousAvailableBytes(idx, file, size - 1, 1) > 0;
            const bool tailNeeded = StreamGate::needsTail(m_session->streamFilePath(idx, file));
            if (verbose) {
                const TorrentInfo now = m_session->torrentAt(idx);
                qInfo().noquote() << "[watch]" << hash.left(8) << "head" << head << "/"
                                  << StreamGate::headBytes(size) << "tail" << tail << "needed" << tailNeeded
                                  << "down" << now.downloadRate / 1024 << "KB/s peers" << now.numPeers
                                  << "seeds" << now.numSeeds;
            }
            const TorrentInfo info = m_session->torrentAt(idx);
            // what has arrived of the start, partial pieces included: the bar
            // moves while the first pieces are in flight (the gate still
            // waits for the start to be contiguous)
            const qint64 received = m_session->streamFileStats(idx, file).value(QStringLiteral("headReceived")).toLongLong();
            st[QStringLiteral("have")] = qMax(head, received);
            st[QStringLiteral("need")] = StreamGate::headBytes(size);
            emit waitState(hash, st);
            emit watchProgress(hash, StreamGate::progress(head, size, tail, tailNeeded));
            emit watchHealth(hash, info.downloadRate, info.numPeers,
                             int(now - m_pendingWatch.value(hash).second));
            if (info.completed || StreamGate::ready(head, size, tail, tailNeeded)) {
                qInfo().noquote() << "[watch]" << hash.left(8) << "ready to play after"
                                  << m_watchClock.take(hash).elapsed() << "ms, head" << head;
                m_pendingWatch.remove(hash);
                m_watchPrepped.remove(hash);
                m_watchNoPeersSince.remove(hash);   // a later watch of it starts its own clock
                playByHashFile(hash, file);
                continue;
            }
            // Priority alone loses to a slow peer holding the first piece; a
            // deadline is what jumps the queue (the tail already has one).
            m_session->streamSetDeadlineWindow(idx, file, head);
        } else {
            emit waitState(hash, st);
        }
        // Peers but no file list for this long, or the torrent never showed
        // up: the waiting screen has nothing true left to say.
        if ((file < 0 && waited > 120) || (idx < 0 && waited > 20)) {
            // the player waiting for this hash says so itself
            st[QStringLiteral("noSource")] = true;
            emit waitState(hash, st);
            m_watchNoPeersSince.remove(hash);
            m_watchPrepped.remove(hash);
            m_watchClock.remove(hash);
            emit watchFailed(m_pendingWatch.take(hash).first);
        }
    }
}

// The game library + launch + install pipeline lives in
// qmlsessionbridge_games.cpp.

