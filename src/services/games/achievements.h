// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#pragma once

#include <QByteArray>
#include <QHash>
#include <QPair>
#include <QList>
#include <QString>
#include <QVariantList>

// A game's achievements as Steam describes them, crossed with what the
// game's emulator wrote to disk. Pure: no network, no files.
// At or below this share of Steam players an unlock is rare.
constexpr double kRarePercent = 10.0;

struct Achievement {
    QString apiName;      // the id Steam and every emulator key on
    QString name;
    QString description;
    QString icon;
    QString iconGray;
    bool hidden = false;
    double percent = -1;  // share of Steam players who have it, -1 unknown
    bool unlocked = false;
    qint64 unlockedAt = 0;   // unix seconds, 0 when the emulator kept no time
    qint64 progress = 0;     // toward progressMax, for an achievement that counts
    qint64 progressMax = 0;  // 0 = not a counting achievement
    bool rare() const { return percent >= 0 && percent <= kRarePercent; }
};

namespace Achievements {

// ISteamUserStats/GetSchemaForGame/v2 → game.availableGameStats.achievements.
QList<Achievement> parseSchema(const QByteArray &json);

// ISteamUserStats/GetGlobalAchievementPercentagesForApp/v2 → apiName → percent.
// Steam has sent the percent both as a number and as a string.
QHash<QString, double> parsePercentages(const QByteArray &json);

// Unlocked come first, newest first; then the locked ones in schema order,
// with the locked hidden ones at the very end.
// `unlocked` maps apiName → unlock time (0 = unlocked, time unknown).
QList<Achievement> merge(QList<Achievement> schema, const QHash<QString, double> &percent,
                         const QHash<QString, qint64> &unlocked,
                         const QHash<QString, QPair<qint64, qint64>> &progress = {});

int unlockedCount(const QList<Achievement> &list);
QVariantList toVariant(const QList<Achievement> &list);

} // namespace Achievements
