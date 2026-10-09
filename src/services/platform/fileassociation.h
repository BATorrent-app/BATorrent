// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#pragma once

#include <QMap>
#include <QString>
#include <QStringList>

// Register / unregister BATorrent as the handler for .torrent / magnet /
// bittorrent:. Platform-specific; returns false when the helper is missing
// or the registry write fails.
namespace FileAssociation {

// kind: "torrent" | "magnet" | "bittorrent"
bool apply(const QString &kind, bool on);

// Register all three kinds (Windows) or the platform one-shot (Linux/macOS).
bool setAsDefaultApp();

// Which of the kinds the user asked for are no longer actually registered to
// this executable. Pure, so the policy can be tested without a registry.
//
// `registered` maps kind -> the open command currently recorded for it, empty
// when nothing is. A kind drifts when it is wanted and that command does not
// name our exe: the installer rewrites nothing on a repeat install, an
// uninstall takes the keys with it, and moving the install directory leaves
// every command pointing at a path that is gone.
//
// Never the other way round: a kind the user turned off is left alone, because
// by then the command names whichever application took over and removing it
// would break that one.
QStringList driftedKinds(const QMap<QString, bool> &wanted,
                         const QMap<QString, QString> &registered,
                         const QString &exePath);

// Re-applies what driftedKinds() finds, reading both sides from the system.
// Cheap enough for startup: three registry reads when nothing is wrong.
void reconcile();

} // namespace FileAssociation
