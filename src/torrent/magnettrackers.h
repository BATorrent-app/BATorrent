// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef TORRENT_MAGNETTRACKERS_H
#define TORRENT_MAGNETTRACKERS_H

// Pure decision for the magnet tracker top-up: which open trackers get
// appended to a magnet's own list so the metadata fetch never hinges on DHT
// alone. Kept libtorrent-free so tests can pin it down.

#include <algorithm>
#include <string>
#include <vector>

namespace bat {

// Long-lived open trackers. Appended to magnets at add time (addMagnet) and
// stripped again in onMetadataReceived if the torrent turns out private.
inline const std::vector<std::string> &publicTrackers()
{
    static const std::vector<std::string> list = {
        "udp://tracker.opentrackr.org:1337/announce",
        "udp://open.demonii.com:1337/announce",
        "udp://open.stealth.si:80/announce",
        "udp://tracker.torrent.eu.org:451/announce",
        "udp://exodus.desync.com:6969/announce",
    };
    return list;
}

// parse_magnet_uri puts each URI tracker on its own tier, so the appended ones
// continue the tier sequence — the magnet's own trackers keep priority.
// Dedup'd and idempotent; repairs a tiers vector that got out of sync.
inline void appendPublicTrackers(std::vector<std::string> &trackers,
                                 std::vector<int> &tiers)
{
    if (tiers.size() != trackers.size())
        tiers.resize(trackers.size(), 0);
    int tier = static_cast<int>(trackers.size());
    for (const std::string &t : publicTrackers()) {
        if (std::find(trackers.begin(), trackers.end(), t) != trackers.end())
            continue;
        trackers.push_back(t);
        tiers.push_back(tier++);
    }
}

} // namespace bat

#endif
