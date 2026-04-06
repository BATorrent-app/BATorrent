// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef TORRENT_TYPES_H
#define TORRENT_TYPES_H

#include <QString>
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
    bool paused;
    float ratio;
    QString category;
};

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

#endif
