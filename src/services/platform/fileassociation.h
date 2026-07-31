// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#pragma once

#include <QString>

// Register / unregister BATorrent as the handler for .torrent / magnet /
// bittorrent:. Platform-specific; returns false when the helper is missing
// or the registry write fails.
namespace FileAssociation {

// kind: "torrent" | "magnet" | "bittorrent"
bool apply(const QString &kind, bool on);

// Register all three kinds (Windows) or the platform one-shot (Linux/macOS).
bool setAsDefaultApp();

} // namespace FileAssociation
