// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef SERVICES_DISCOVERY_CINEMETA_H
#define SERVICES_DISCOVERY_CINEMETA_H

#include <QByteArray>
#include <QDate>
#include <QList>
#include <QString>
#include <QUrl>

// Episode numbering as IMDb (and so Torrentio, and Stremio) count it, from
// Stremio's own Cinemeta addon. TMDB often splits a show differently: it may
// list Bleach as one long season where IMDb has sixteen. The two agree on when
// an episode first aired, so that date carries a TMDB episode over to the
// numbers the stream addons expect.
namespace Cinemeta {

struct Video {
    int season = 0;
    int episode = 0;
    QDate released;
};

QUrl metaUrl(const QString &imdbId);
QList<Video> videos(const QByteArray &json);

struct Number { int season; int episode; };
// The IMDb season and episode for a TMDB one that aired on `airDate`. Same day
// first, then a day either side (the two count in different time zones); among
// several that day, the one with the same episode number, else the first. No
// match, no date or no list: TMDB's own numbers, which is what was asked before.
Number mapByAirDate(const QList<Video> &videos, const QDate &airDate, int tmdbSeason, int tmdbEpisode);

// The episode after (season, episode) in IMDb's airing order, specials left
// out. A file numbered past its first season's length is read as absolute
// ("Bleach - 110" is the 110th episode). {-1, -1} when it is the last one or
// cannot be placed.
Number nextAfter(const QList<Video> &videos, int season, int episode);

} // namespace Cinemeta

#endif
