// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#pragma once

#include "services/games/achievementformats.h"

#include <QHash>
#include <QString>

// BATorrent's own record of every achievement it has seen unlocked, by Steam
// appid. The emulators keep unlocks where they please, often inside the game's
// folder (gbe's steam_settings, 3DM, ALI213), so deleting or reinstalling the
// game, or switching to another release of it, used to lose them. Merged with
// what the emulator files say, the earliest time wins. One JSON file in the
// app's data folder, exportable to keep across a reinstall of Windows.
class AchievementLedger
{
public:
    struct Game {
        QString title;                        // for a human reading an export
        AchievementFormats::Unlocks unlocks;  // apiName → unlock time (secs, 0 = unknown)
    };
    using Games = QHash<int, Game>;

    explicit AchievementLedger(const QString &path);

    // What the files say plus what was seen before. Anything the files show
    // for the first time is written down.
    AchievementFormats::Unlocks merged(int appId, const QString &title,
                                       const AchievementFormats::Unlocks &fromFiles);

    // A copy of the whole record, anywhere: a backup before formatting.
    bool exportTo(const QString &path) const;
    // Another record (an export, another PC's) folded in; nothing is removed.
    // How many unlocks were new, or -1 when the file is not a record.
    int importFrom(const QString &path);

    // Pure, for the tests: the file format both ways, and one record folded
    // into another (earliest time wins). fold returns how many unlocks it added.
    static QByteArray toJson(const Games &games);
    static bool fromJson(const QByteArray &json, Games *out);
    static int fold(Games &into, const Games &from);

private:
    void load() const;   // on first use: the file is only read when a game asks
    bool save() const;

    QString m_path;
    mutable bool m_loaded = false;
    mutable Games m_games;
};
