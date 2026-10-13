// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef EPISODEGROUP_H
#define EPISODEGROUP_H

#include <QList>
#include <QString>
#include <QStringList>

// How one release of a series maps onto its episodes: a single episode
// (season+episode), a season pack (season, pack), or a complete/batch pack
// (pack, no season). Unknown fields stay -1.
struct EpisodeTag {
    int season = -1;
    int episode = -1;
    bool pack = false;   // covers more than one episode
};

// The file of a torrent to play for one episode, and what decided it.
struct EpisodeFile {
    int index = -1;
    QString how;   // "filename" | "fileIdx" | "episode" | "largest" | ""
};

class EpisodeGroup
{
public:
    // workTitles: the names the show was searched under. With them, an anime
    // release numbered from 1 under a longer name ("Bleach - Sennen Kessen-hen
    // - 04" for "Bleach") is another series or arc, and stays off season 1.
    static EpisodeTag classify(const QString &releaseName, const QStringList &workTitles = {});

    // Which file of a torrent is the episode. paths/sizes are the torrent's
    // files in order. Tried in turn: the addon's file name, the addon's file
    // index, the season+episode read from each video's name, the largest video.
    static EpisodeFile pickFile(const QStringList &paths, const QList<qint64> &sizes,
                                int fileIdx, const QString &fileName, int season, int episode);
};

#endif
