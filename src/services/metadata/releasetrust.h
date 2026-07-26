// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef RELEASETRUST_H
#define RELEASETRUST_H

#include <QString>
#include <QStringList>

// Pre-download quality/trust heuristic for a search result — the counterpart to
// SuspiciousScan, which can only run once the file list exists (i.e. after the
// torrent is added). Everything here is derived from the release name, its size
// and its swarm, so it can warn *before* the user commits to a download.
//
// Same framing rule as the rest of the safety work: warn only on what we
// actually found, stay silent otherwise. A clean release gets Tier::Ok and no
// UI at all — we never claim a release is "safe".
namespace ReleaseTrust {

enum class Tier { Ok, Caution, Risky };

struct Release {
    QString name;
    QString quality;    // "4K" | "1080p" | "720p" | "480p" | "" (unknown)
    QString source;     // "Remux" | "BluRay" | "WEB" | "HDTV" | "DVD" | "CAM" | ""
    int seeders = 0;
    qint64 sizeBytes = 0;   // 0 = unknown (size rules are skipped)
};

struct Verdict {
    Tier tier = Tier::Ok;
    int score = 0;          // 0..100, higher is better — for ranking, not display
    QStringList reasons;    // i18n keys, worst first; empty when Tier::Ok
};

Verdict assess(const Release &r);

// "ok" | "caution" | "risky" — the string the QML layer switches on.
QString tierKey(Tier t);

}

#endif // RELEASETRUST_H
