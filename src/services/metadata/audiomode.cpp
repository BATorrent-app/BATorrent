// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "services/metadata/audiomode.h"

#include <QHash>
#include <QRegularExpression>

namespace {

struct Tokens { const char *dub; const char *sub; };

// Per-language scene/release vocabulary. `dub` = audio in that language;
// `sub` = original audio + subtitles in that language. Kept deliberately
// conservative — a false Dubbed is worse than a missed one (it hands an
// anti-dub viewer exactly what they filtered out).
const QHash<QString, Tokens> &tables()
{
    static const QHash<QString, Tokens> t = {
        {"PT", {"dublad[oa]|dublagem|nacional|dual[ ._-]?[aá]udio|\\bdub\\b|dublado",
                "legendad[oa]|legenda|\\bleg\\b|leg[ ._-]?pt|vost[ ._-]?pt|sub[ ._-]?pt"}},
        {"ES", {"castellano|latino|doblad[oa]|espa[nñ]ol|\\bdual\\b|\\bcast\\b",
                "subtitulad[oa]|\\bvose\\b|sub[ ._-]?es|\\bsubs?\\b"}},
        {"FR", {"truefrench|\\bvff\\b|\\bvf[fiq]?\\b|\\bfrench\\b|\\bfra\\b|multi",
                "vostfr|\\bvost\\b|sub[ ._-]?fr|st[ ._-]?fr"}},
        {"DE", {"\\bgerman\\b|deutsch|\\bger\\b|\\bdl\\b|synchro",
                "gersub|german[ ._-]?sub|untertitel|sub[ ._-]?de"}},
        {"IT", {"\\bita\\b|italian|\\bml\\b",
                "sub[ ._-]?ita|italian[ ._-]?sub"}},
        {"RU", {"дубляж|многоголос|\\brus\\b|\\brus(sian)?\\b|русск",
                "субтитр|sub[ ._-]?rus|rus[ ._-]?sub"}},
        {"UK", {"дубляж[ ._-]?укр|\\bukr\\b|ukrainian",
                "суб[а-я]*[ ._-]?укр|ukr[ ._-]?sub"}},
        {"ZH", {"国语|普通话|mandarin|cantonese",
                "中字|字幕|\\bchs\\b|\\bcht\\b|chi[ ._-]?sub"}},
        {"JA", {"\\bjpn?[ ._-]?dub\\b|japanese[ ._-]?dub",
                "\\bjp?[ ._-]?sub\\b|japanese[ ._-]?sub"}},
    };
    return t;
}

bool matches(const QString &name, const char *pattern)
{
    static QHash<const char *, QRegularExpression> cache;
    auto it = cache.find(pattern);
    if (it == cache.end())
        it = cache.insert(pattern, QRegularExpression(
            QLatin1String(pattern), QRegularExpression::CaseInsensitiveOption));
    return name.contains(*it);
}

// A dual/multi-audio release plausibly carries the user's dub — matches the
// existing `native` heuristic so we don't classify inconsistently.
bool multiAudio(const QString &name)
{
    return matches(name, "\\bmulti\\b|dual[ ._-]?(a|á)udio|dual[ ._-]?lat");
}

}

namespace AudioMode {

Mode classify(const QString &releaseName, const QString &userLang)
{
    const QString lang = userLang.toUpper();
    if (lang == QLatin1String("EN") || lang.isEmpty()) return Original;
    auto it = tables().constFind(lang);
    if (it == tables().constEnd()) return Original;

    // Dub wins ties: a "Dual Áudio + Legendado" release still gives you the dub.
    if (matches(releaseName, it->dub) || multiAudio(releaseName)) return Dubbed;
    if (matches(releaseName, it->sub)) return Subbed;
    return Original;
}

QString key(Mode m)
{
    switch (m) {
    case Dubbed:  return QStringLiteral("dub");
    case Subbed:  return QStringLiteral("sub");
    case Original: break;
    }
    return QStringLiteral("original");
}

}
