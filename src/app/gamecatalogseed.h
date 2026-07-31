// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef BATORRENT_GAMECATALOGSEED_H
#define BATORRENT_GAMECATALOGSEED_H

// One-shot migration: remove legacy game feed URLs and seed the BATorrent catalog.
namespace GameCatalogSeed {

void apply();

} // namespace GameCatalogSeed

#endif
