// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef GAMERELEASEPICK_H
#define GAMERELEASEPICK_H

#include <QList>
#include <QString>

// Game release ranking: prefer curated catalog hits and newer builds over
// stale indexer torrents that still show a few seeders.
namespace GameReleasePick {

struct Candidate {
    bool fromCatalog = false;
    QString version;      // "3.2.2" or ""
    QString uploadDate;   // ISO-8601 or ""
    int seeders = 0;
    bool hasUri = true;   // magnet or direct http
};

// Parse a game build version out of a release title ("v3.2.2", "3.2.2").
QString parseVersion(const QString &title);

// strcmp-style on build strings: <0 if a older, 0 equal, >0 if a newer. Empty is oldest.
int compareVersions(const QString &a, const QString &b);

// strcmp-style on full candidates: <0 if a ranks worse, 0 equal, >0 if a ranks better.
int compareCandidates(const Candidate &a, const Candidate &b);

// Index of the best candidate, or -1 if none is usable.
int best(const QList<Candidate> &cands);

}

#endif
