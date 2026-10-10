// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#pragma once

#include <QByteArray>
#include <QString>

// Finding a game on Steam by name, and the public art Steam keeps per appid.
namespace SteamStore {

// The title without an edition tail ("… - Complete Edition", "GOTY"): Steam's
// search finds nothing for the long form, and renames editions freely.
QString searchTerm(const QString &title);

// store.steampowered.com/api/storesearch → the app whose name is the title
// once both drop their edition words. Never the first hit blindly: that is
// as often a soundtrack or a DLC. 0 when nothing matches.
int bestAppId(const QByteArray &searchJson, const QString &title);

QString heroUrl(int appId);   // library_hero.jpg, composed to sit behind the logo
QString logoUrl(int appId);

} // namespace SteamStore
