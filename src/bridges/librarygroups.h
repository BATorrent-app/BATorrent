// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef LIBRARYGROUPS_H
#define LIBRARYGROUPS_H

#include <QSet>
#include <QString>
#include <QStringList>
#include <QVector>

// Which library rows belong to the same series, and the order the list shows
// them in. Watching a show one episode at a time adds one torrent per episode;
// this folds them back into one entry. No Qt models here, so it can be tested
// on plain data.
namespace LibraryGroups {

// One torrent as the grouping sees it, in the order the list currently sorts it.
struct Member {
    int tmdbId = 0;     // the resolved show, 0 while unknown
    QString nameKey;    // nameKey() of the show's title; empty for anything not an episode
    int season = -1;    // -1 for a complete pack or when the name says nothing
    int episode = -1;   // -1 for a season pack
    // A film's TMDB collection ("The Lord of the Rings Collection"), 0 for
    // none. Only a film that is no episode groups by it.
    int collectionId = 0;
    int year = 0;       // a collection lists its films in release order
    QString name;       // last tie-break, so the order never depends on the sort
};

// One line of the list: a lone torrent (members holds its index), a group
// header (all of its members, in episode order), or an episode shown under an
// open group (child set, members holds its index).
struct Row {
    QString groupKey;   // empty for a torrent that belongs to no group
    QVector<int> members;
    bool group = false;
    bool child = false;
};

// Lowercase letters and digits of a show title, words split by single spaces.
// "The.Last.of.Us" and "The Last Of Us" land on the same key.
QString nameKey(const QString &title);

// The show each member belongs to, or empty when it belongs to none. The TMDB
// id wins where it is known; an episode the resolver has not reached yet joins
// the resolved show with the same name rather than starting a group of its own.
// A film of a TMDB collection gets the collection's key ("col:119").
QStringList keys(const QVector<Member> &members);

// The rows to show. A show with a single torrent stays a plain row. A group
// takes the place of its first member in the current order, and an open group
// lists its episodes right under it by season and episode.
QVector<Row> layout(const QVector<Member> &members, const QSet<QString> &expanded);

// A key from keys() that groups the films of a collection, not a show.
inline bool isCollectionKey(const QString &key) { return key.startsWith(QLatin1String("col:")); }

} // namespace LibraryGroups

#endif // LIBRARYGROUPS_H
