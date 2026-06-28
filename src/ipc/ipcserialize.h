// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// QDataStream wire operators for the engine/UI split. TorrentInfo deliberately
// skips its lt::torrent_handle — it's not serialisable and means nothing in the
// UI process, which only ever reads the display fields. See ipcprotocol.h.
#ifndef BATORRENT_IPCSERIALIZE_H
#define BATORRENT_IPCSERIALIZE_H

#include <QDataStream>
#include <QList>
#include <vector>
#include "../torrent/types.h"

inline QDataStream &operator<<(QDataStream &s, const TorrentInfo &t)
{
    s << t.name << t.savePath << qint64(t.totalSize) << qint64(t.totalDone)
      << t.progress << qint32(t.downloadRate) << qint32(t.uploadRate)
      << qint32(t.numPeers) << qint32(t.numSeeds) << t.stateString << t.stateDetail
      << t.paused << t.completed << t.ratio << t.availability
      << qint64(t.totalUploaded) << qint64(t.addedTime) << t.category << t.tags;
    return s;
}
inline QDataStream &operator>>(QDataStream &s, TorrentInfo &t)
{
    qint32 dr, ur, np, ns;
    s >> t.name >> t.savePath >> t.totalSize >> t.totalDone
      >> t.progress >> dr >> ur >> np >> ns >> t.stateString >> t.stateDetail
      >> t.paused >> t.completed >> t.ratio >> t.availability
      >> t.totalUploaded >> t.addedTime >> t.category >> t.tags;
    t.downloadRate = dr; t.uploadRate = ur; t.numPeers = np; t.numSeeds = ns;
    return s;
}

inline QDataStream &operator<<(QDataStream &s, const FileInfo &f)
{ s << f.path << qint64(f.size) << f.progress << qint32(f.priority); return s; }
inline QDataStream &operator>>(QDataStream &s, FileInfo &f)
{ qint32 pr; s >> f.path >> f.size >> f.progress >> pr; f.priority = pr; return s; }

inline QDataStream &operator<<(QDataStream &s, const PeerInfo &p)
{ s << p.ip << qint32(p.port) << qint32(p.downloadRate) << qint32(p.uploadRate) << p.progress << p.client; return s; }
inline QDataStream &operator>>(QDataStream &s, PeerInfo &p)
{ qint32 port, dr, ur; s >> p.ip >> port >> dr >> ur >> p.progress >> p.client; p.port = port; p.downloadRate = dr; p.uploadRate = ur; return s; }

inline QDataStream &operator<<(QDataStream &s, const TrackerInfo &t)
{ s << t.url << qint32(t.tier) << t.status; return s; }
inline QDataStream &operator>>(QDataStream &s, TrackerInfo &t)
{ qint32 tier; s >> t.url >> tier >> t.status; t.tier = tier; return s; }

inline QDataStream &operator<<(QDataStream &s, const RemovedEntry &r)
{ s << r.hash << r.name << qint64(r.totalSize) << qint64(r.removedAt) << r.resumePath; return s; }
inline QDataStream &operator>>(QDataStream &s, RemovedEntry &r)
{ s >> r.hash >> r.name >> r.totalSize >> r.removedAt >> r.resumePath; return s; }

inline QDataStream &operator<<(QDataStream &s, const DetailedStats &d)
{ s << qint32(d.dhtNodes) << qint32(d.peersCount) << qint64(d.totalWasted)
    << qint32(d.diskReadQueue) << qint32(d.diskWriteQueue) << d.hasIncomingConnections; return s; }
inline QDataStream &operator>>(QDataStream &s, DetailedStats &d)
{ qint32 dht, peers, rq, wq; s >> dht >> peers >> d.totalWasted >> rq >> wq >> d.hasIncomingConnections;
  d.dhtNodes = dht; d.peersCount = peers; d.diskReadQueue = rq; d.diskWriteQueue = wq; return s; }

inline QDataStream &operator<<(QDataStream &s, const AdvancedSettings &a)
{
    s << qint32(a.aioThreads) << qint32(a.hashingThreads) << qint32(a.filePoolSize)
      << qint32(a.checkingMemUsage) << qint32(a.diskIOReadMode) << qint32(a.diskIOWriteMode)
      << qint32(a.connectionsLimit) << qint32(a.connectionSpeed) << qint32(a.maxUploadsPerTorrent)
      << qint32(a.maxConnectionsPerTorrent) << qint32(a.unchokeSlotsLimit) << qint32(a.chokingAlgorithm)
      << qint32(a.seedChokingAlgorithm) << qint32(a.sendBufferWatermark) << qint32(a.outgoingPortMin)
      << qint32(a.outgoingPortMax) << a.rateLimitIpOverhead << a.ignoreLimitsOnLAN;
    return s;
}
inline QDataStream &operator>>(QDataStream &s, AdvancedSettings &a)
{
    qint32 v[16];
    for (int i = 0; i < 16; ++i) s >> v[i];
    s >> a.rateLimitIpOverhead >> a.ignoreLimitsOnLAN;
    a.aioThreads = v[0]; a.hashingThreads = v[1]; a.filePoolSize = v[2]; a.checkingMemUsage = v[3];
    a.diskIOReadMode = v[4]; a.diskIOWriteMode = v[5]; a.connectionsLimit = v[6]; a.connectionSpeed = v[7];
    a.maxUploadsPerTorrent = v[8]; a.maxConnectionsPerTorrent = v[9]; a.unchokeSlotsLimit = v[10];
    a.chokingAlgorithm = v[11]; a.seedChokingAlgorithm = v[12]; a.sendBufferWatermark = v[13];
    a.outgoingPortMin = v[14]; a.outgoingPortMax = v[15];
    return s;
}

// std::vector<T> helpers (QDataStream only ships QList/QVector operators).
template <class T> inline void writeVec(QDataStream &s, const std::vector<T> &v)
{ s << qint32(v.size()); for (const T &e : v) s << e; }
template <class T> inline void readVec(QDataStream &s, std::vector<T> &v)
{ qint32 n = 0; s >> n; v.clear(); v.reserve(n); for (qint32 i = 0; i < n; ++i) { T e; s >> e; v.push_back(e); } }

inline void writeBoolVec(QDataStream &s, const std::vector<bool> &v)
{ s << qint32(v.size()); for (bool b : v) s << quint8(b ? 1 : 0); }
inline void readBoolVec(QDataStream &s, std::vector<bool> &v)
{ qint32 n = 0; s >> n; v.assign(n, false); for (qint32 i = 0; i < n; ++i) { quint8 b; s >> b; v[i] = b != 0; } }

// The per-tick state the engine pushes to the UI. Everything the main torrent
// list, stats bar and nav card read comes from here — no blocking round-trips.
struct EngineSnapshot {
    QStringList hashes;          // parallel to rows
    QList<TorrentInfo> rows;
    qint64 globalUploaded = 0, globalDownloaded = 0;
    qint64 sessionUploaded = 0, sessionDownloaded = 0;
    float globalRatio = 0;
    qint32 totalTorrentsAdded = 0, portStatus = 0, listenPort = 0;
    bool dhtEnabled = false, altSpeedsActive = false;
    DetailedStats detailed;
};

inline QDataStream &operator<<(QDataStream &s, const EngineSnapshot &k)
{
    s << k.hashes << qint32(k.rows.size());
    for (const TorrentInfo &t : k.rows) s << t;
    s << k.globalUploaded << k.globalDownloaded << k.sessionUploaded << k.sessionDownloaded
      << k.globalRatio << k.totalTorrentsAdded << k.portStatus << k.listenPort
      << k.dhtEnabled << k.altSpeedsActive << k.detailed;
    return s;
}
inline QDataStream &operator>>(QDataStream &s, EngineSnapshot &k)
{
    qint32 n = 0; s >> k.hashes >> n; k.rows.clear(); k.rows.reserve(n);
    for (qint32 i = 0; i < n; ++i) { TorrentInfo t; s >> t; k.rows.push_back(t); }
    s >> k.globalUploaded >> k.globalDownloaded >> k.sessionUploaded >> k.sessionDownloaded
      >> k.globalRatio >> k.totalTorrentsAdded >> k.portStatus >> k.listenPort
      >> k.dhtEnabled >> k.altSpeedsActive >> k.detailed;
    return s;
}

#endif
