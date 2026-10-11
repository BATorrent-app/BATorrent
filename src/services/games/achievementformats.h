// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#pragma once

#include <QByteArray>
#include <QHash>
#include <QPair>
#include <QString>

// What each Steam emulator writes when an achievement unlocks, read back as
// apiName → unlock time in unix seconds (0 = unlocked, time unknown). Formats
// follow Hydra's and Achievement Watcher's readers. Pure: bytes in, map out.
namespace AchievementFormats {

using Unlocks = QHash<QString, qint64>;
// apiName → {current, maximum} for the achievements that count toward a goal.
using Progress = QHash<QString, QPair<qint64, qint64>>;
using Ini = QHash<QString, QHash<QString, QString>>;   // section → key → value

enum class Format {
    GoldbergJson,   // Goldberg, gbe_fork, EMPRESS
    CodexIni,       // CODEX, RUNE, ALI213, user stats with progress
    OnlineFixIni,
    SkidrowIni,
    ThreeDmIni,
    RldIni,
    CreamApiIni,
};

// [section] / key=value, BOM stripped, ';' and '#' comments skipped.
Ini parseIni(const QByteArray &bytes);

Unlocks parse(Format format, const QByteArray &bytes);
// Only Goldberg/gbe/EMPRESS, CODEX-style and RLD! files keep progress.
Progress parseProgress(Format format, const QByteArray &bytes);

// Some emulators write milliseconds where others write seconds.
qint64 normalizeTime(qint64 t);

} // namespace AchievementFormats
