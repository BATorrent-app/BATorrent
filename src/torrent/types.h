// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef TORRENT_TYPES_H
#define TORRENT_TYPES_H

#include <QString>
#include <QStringList>
#include <libtorrent/torrent_handle.hpp>

struct TorrentInfo {
    lt::torrent_handle handle;
    QString name;
    QString savePath;
    qint64 totalSize;
    qint64 totalDone;
    float progress;
    int downloadRate;
    int uploadRate;
    int numPeers;
    int numSeeds;
    QString stateString;
    QString stateDetail;   // why a downloading torrent isn't moving ("" when fine)
    bool paused;
    bool completed = false;
    // Straight from libtorrent, which counts pieces instead of comparing a
    // float: finished = every piece with priority > 0 is on disk (so it
    // respects deselected files); seeding = every piece, wanted or not.
    bool finished = false;
    bool seeding = false;
    bool filesMissing = false;   // data was deleted/moved off disk under a live torrent
    bool queued = false;   // paused by the download-queue cap, not the user
    int queuePos = 0;      // 1-based position among queued torrents
    float ratio;
    float availability = 0;     // distributed copies (swarm health)
    qint64 totalUploaded = 0;
    qint64 addedTime = 0;       // unix seconds
    QString category;
    QStringList tags;
};

// Stable classification key for a torrent's state — the token every UI surface
// keys its colour off (grid tile, detail bar, status dot). Distinct from
// TorrentInfo::stateString, which is a *translated label* and therefore useless
// for comparison. Lives here so the poster model and the selection bridge can't
// drift apart.
inline QString torrentStateKey(const TorrentInfo &info)
{
    if (info.filesMissing) return QStringLiteral("missing");
    if (info.completed)    return QStringLiteral("completed");
    if (info.queued)       return QStringLiteral("queued");
    if (info.paused)       return QStringLiteral("paused");
    // Ask libtorrent, don't compare a float. progress is total_wanted_done over
    // total_wanted in floating point, so a finished torrent can sit at
    // 0.99999994 and never satisfy >= 1.0f — that is a torrent stuck on
    // DOWNLOADING for months over a few bytes it already has. is_finished
    // counts pieces, and already excludes priority-0 files, which is what the
    // old totalDone > 0 guard was reaching for.
    if (info.finished || info.seeding) return QStringLiteral("seeding");
    return QStringLiteral("downloading");
}

struct PeerInfo {
    QString ip;
    int port;
    int downloadRate;
    int uploadRate;
    float progress;
    QString client;
};

struct FileInfo {
    QString path;
    qint64 size;
    float progress;
    int priority;
};

struct TrackerInfo {
    QString url;
    int tier;
    QString status;
};

// Shared so the IEngine interface (and the IPC layer) can name them — moved out
// of SessionManager's body for the engine/UI split.
struct RemovedEntry {
    QString hash;
    QString name;
    qint64 totalSize;
    qint64 removedAt;     // unix seconds
    QString resumePath;   // absolute path to the snapshot file
};

struct DetailedStats {
    int dhtNodes = 0;
    int peersCount = 0;
    qint64 totalWasted = 0;       // redundant + failed bytes
    int diskReadQueue = 0;
    int diskWriteQueue = 0;
    bool hasIncomingConnections = false;
};

// Advanced libtorrent tuning — exposed in Settings → Advanced. Lives here (not
// nested in SessionManager) so IEngine and the IPC layer can name it.
struct AdvancedSettings {
    int aioThreads = 10;
    int hashingThreads = 2;
    int filePoolSize = 100;
    int checkingMemUsage = 512;   // × 16KB = 8MB default
    int diskIOReadMode = 0;       // 0=EnableOSCache, 1=DisableOSCache
    int diskIOWriteMode = 0;      // 0=EnableOSCache, 1=DisableOSCache, 2=WriteThrough
    int connectionsLimit = 500;
    int connectionSpeed = 30;
    int maxUploadsPerTorrent = 4;
    int maxConnectionsPerTorrent = 100;
    int unchokeSlotsLimit = 20;
    int chokingAlgorithm = 0;     // 0=FixedSlots, 1=RateBased
    int seedChokingAlgorithm = 0; // 0=RoundRobin, 1=FastestUpload, 2=AntiLeech
    int sendBufferWatermark = 500; // KB
    int outgoingPortMin = 0;
    int outgoingPortMax = 0;
    bool rateLimitIpOverhead = false;
    bool ignoreLimitsOnLAN = true;
};

#endif
