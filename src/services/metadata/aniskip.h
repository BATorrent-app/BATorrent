// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef SERVICES_METADATA_ANISKIP_H
#define SERVICES_METADATA_ANISKIP_H

#include "services/metadata/mkvchapters.h"

#include <QByteArray>
#include <QList>
#include <QString>
#include <QStringList>
#include <QUrl>

// Opening and ending times for anime episodes from AniSkip, a community
// database keyed by MyAnimeList id and episode number. The MyAnimeList id
// comes from an AniList title search. Only requests and parsing live here; the
// session bridge does the fetching. The times come back as chapters so the
// player's skip button treats them like a file's own "Opening" chapter.
namespace AniSkip {

// AniList's GraphQL endpoint and the POST body that searches it by title.
QUrl malSearchUrl();
QByteArray malSearchBody(const QString &title);
// The first TV entry of an AniList search whose name matches one of `names`
// (any of its titles, ignoring case and punctuation), 0 when none does. A
// search for a show that isn't anime still returns some anime; taking it
// would skip the wrong minutes of the wrong show.
int malIdFromSearch(const QByteArray &json, const QStringList &names);

QUrl skipTimesUrl(int malId, int episode);
// "op" becomes an intro chapter and "ed" a credits one; anything else, or
// an interval that doesn't make sense, is dropped.
QList<MkvChapter> chaptersFromSkipTimes(const QByteArray &json);

} // namespace AniSkip

#endif
