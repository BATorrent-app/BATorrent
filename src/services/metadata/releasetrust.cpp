// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "services/metadata/releasetrust.h"

#include <QRegularExpression>
#include <algorithm>

namespace {

constexpr qint64 kMiB = 1024LL * 1024LL;
constexpr qint64 kGiB = 1024LL * kMiB;

bool matches(const QString &name, const QRegularExpression &re)
{
    return name.contains(re);
}

// Season packs and single episodes are legitimately a fraction of a movie's
// size at the same resolution, so the "too small for the resolution" rule can't
// apply to them. Detected from the name so callers don't have to pass it in.
bool isEpisodic(const QString &name)
{
    static const QRegularExpression re(
        QStringLiteral("\\bS\\d{1,2}(E\\d{1,3})?\\b|\\b\\d{1,2}x\\d{2}\\b|"
                       "\\bseason\\b|\\btemporada\\b|\\bcomplete series\\b|\\bepis[oó]dio\\b"),
        QRegularExpression::CaseInsensitiveOption);
    return matches(name, re);
}

// A release that hands out a password is asking the user to visit a site to get
// it — the classic bait for adware/credential pages. Bounded by separators so a
// film actually called "Password" doesn't trip it.
bool isPasswordBait(const QString &name)
{
    static const QRegularExpression re(
        QStringLiteral("(^|[\\s._\\-\\[(])(password(ed)?|passwd|senha)([\\s._\\-\\])]|$)"),
        QRegularExpression::CaseInsensitiveOption);
    return matches(name, re);
}

// Uploader spam prefixes ("www.Site.com - Movie", "[Site.io] Movie") mark the
// re-uploads that carry bundled "installers" far more often than scene releases.
bool hasSpamPrefix(const QString &name)
{
    static const QRegularExpression re(
        QStringLiteral("^\\s*[\\[(]?\\s*(www\\.)?[a-z0-9-]{3,}\\.(com|net|org|io|me|to|info|biz|xyz|ru|tv)\\b"),
        QRegularExpression::CaseInsensitiveOption);
    return matches(name, re);
}

// Floor below which the claimed resolution can't be a real movie encode — set
// well under even aggressive x265 releases so a legitimate small encode is
// never flagged. 0 = no floor for that quality.
qint64 sizeFloor(const QString &quality)
{
    if (quality == QLatin1String("4K"))    return 1500 * kMiB;
    if (quality == QLatin1String("1080p")) return 400 * kMiB;
    if (quality == QLatin1String("720p"))  return 150 * kMiB;
    return 0;
}

}

namespace ReleaseTrust {

Verdict assess(const Release &r)
{
    Verdict v;
    int score = 60;
    bool risky = false;

    if (isPasswordBait(r.name)) {
        v.reasons << QStringLiteral("trust_password");
        score -= 50;
        risky = true;
    }

    const qint64 floor = sizeFloor(r.quality);
    if (r.sizeBytes > 0 && floor > 0 && r.sizeBytes < floor && !isEpisodic(r.name)) {
        v.reasons << QStringLiteral("trust_fake_size");
        score -= 40;
        risky = true;
    }

    if (r.source == QLatin1String("CAM")) {
        v.reasons << QStringLiteral("trust_cam");
        score -= 25;
    }

    if (hasSpamPrefix(r.name)) {
        v.reasons << QStringLiteral("trust_spam_name");
        score -= 15;
    }

    // Swarm health feeds the score (for ranking) but never produces a reason —
    // the row already draws seeders as a coloured bar, and a second badge saying
    // the same thing is noise.
    if (r.seeders <= 0)       score -= 30;
    else                      score += std::min(30, r.seeders / 4);

    v.score = std::clamp(score, 0, 100);
    v.tier = risky ? Tier::Risky : (v.reasons.isEmpty() ? Tier::Ok : Tier::Caution);
    return v;
}

QString tierKey(Tier t)
{
    switch (t) {
    case Tier::Risky:   return QStringLiteral("risky");
    case Tier::Caution: return QStringLiteral("caution");
    case Tier::Ok:      break;
    }
    return QStringLiteral("ok");
}

}
