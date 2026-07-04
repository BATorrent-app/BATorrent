// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef EPISODEGROUP_H
#define EPISODEGROUP_H

#include <QString>

// How one release of a series maps onto its episodes: a single episode
// (season+episode), a season pack (season, pack), or a complete/batch pack
// (pack, no season). Unknown fields stay -1.
struct EpisodeTag {
    int season = -1;
    int episode = -1;
    bool pack = false;   // covers more than one episode
};

class EpisodeGroup
{
public:
    static EpisodeTag classify(const QString &releaseName);
};

#endif
