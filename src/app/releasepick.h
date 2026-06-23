// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef RELEASEPICK_H
#define RELEASEPICK_H

#include <QList>
#include <QString>

// Pure "best release" ranking for the one-click Get & Watch flow. Given the
// search results for a title, pick the release a sensible person would: honor
// the user's quality preference (with graceful fallback), respect a max-size
// cap when one is set, then prefer the user's own language, then seeders.
// Dead releases (0 seeders) are never chosen.
namespace ReleasePick {

struct Candidate {
    QString quality;     // "4K" | "1080p" | "720p" | "480p" | "" (unknown)
    bool native = false; // release is in the user's language
    int seeders = 0;
    qint64 sizeBytes = 0;
};

// Returns the index of the best candidate, or -1 if none is usable.
// preferredQuality: "4K" | "1080p" | "720p" | "Auto"/"" (Auto = 1080p sweet spot).
// maxSizeBytes: 0 = no cap.
// preferNative: when true, a release in the user's language wins over a
// higher-quality one (what most movie/series viewers actually want — Torrentio
// behaviour). When false, quality leads and native is only a tiebreaker.
int best(const QList<Candidate> &cands, const QString &preferredQuality,
         qint64 maxSizeBytes, bool preferNative = true);

}

#endif
