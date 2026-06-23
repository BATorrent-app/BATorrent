// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "releasepick.h"

namespace {

// Desirability order of quality tiers given a preference: the preferred tier
// first, then better tiers, then lower ones. Unknown quality always ranks last.
QStringList preferenceOrder(const QString &pref)
{
    if (pref == QLatin1String("4K"))   return {"4K", "1080p", "720p", "480p"};
    if (pref == QLatin1String("720p")) return {"720p", "1080p", "4K", "480p"};
    // "1080p", "Auto", or empty → 1080p is the sweet spot, then up, then down.
    return {"1080p", "4K", "720p", "480p"};
}

// Higher = more desirable. Unknown/unlisted quality scores 0.
int qualityScore(const QString &quality, const QStringList &order)
{
    const int idx = order.indexOf(quality);
    return idx < 0 ? 0 : (order.size() - idx);
}

}

namespace ReleasePick {

int best(const QList<Candidate> &cands, const QString &preferredQuality, qint64 maxSizeBytes)
{
    const QStringList order = preferenceOrder(preferredQuality);

    // Respect the size cap only if at least one alive release fits under it;
    // otherwise the cap is ignored rather than leaving the user with nothing.
    bool anyUnderCap = false;
    if (maxSizeBytes > 0) {
        for (const auto &c : cands)
            if (c.seeders > 0 && c.sizeBytes > 0 && c.sizeBytes <= maxSizeBytes) { anyUnderCap = true; break; }
    }

    int bestIdx = -1;
    int bestQ = -1; bool bestNative = false; int bestSeeds = -1;
    for (int i = 0; i < cands.size(); ++i) {
        const Candidate &c = cands[i];
        if (c.seeders <= 0) continue;                                  // dead
        if (anyUnderCap && c.sizeBytes > 0 && c.sizeBytes > maxSizeBytes) continue;

        const int q = qualityScore(c.quality, order);
        // lexicographic: quality, then native, then seeders
        if (q > bestQ
            || (q == bestQ && !bestNative && c.native)
            || (q == bestQ && bestNative == c.native && c.seeders > bestSeeds)) {
            bestIdx = i; bestQ = q; bestNative = c.native; bestSeeds = c.seeders;
        }
    }
    return bestIdx;
}

}
