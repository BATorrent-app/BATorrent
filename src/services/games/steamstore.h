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

// A game's library art, as an image://steamart/<kind>/<appid> URL that the
// steamart provider resolves. Steam moved store art to hashed paths
// (store_item_assets/steam/apps/<id>/<hash>/library_hero.jpg); the bare
// /steam/apps/<id>/library_hero.jpg only exists for games from before, so
// any newer game showed no art at all.
QString heroUrl(int appId);   // library_hero.jpg, composed to sit behind the logo
QString logoUrl(int appId);

// IStoreBrowseService/GetItems for one app, with its asset paths. No key needed.
QString storeItemsUrl(int appId);
// The full URL of an asset from that answer: kind "hero" or "logo". Empty when
// the game has no such art (many have no logo) or the answer is not one.
QString assetUrl(const QByteArray &storeItemsJson, const QString &kind);
// The old unhashed path, for when the store answer could not be had at all.
QString legacyArtUrl(int appId, const QString &kind);

} // namespace SteamStore
