// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#pragma once

#include "services/discovery/addonmanager.h"

#include <QList>

// Curated Stremio addons + search-provider presets (data tables only).
namespace AddonCatalog {

QList<CuratedAddon> curatedCatalog();
QList<ProviderPreset> providerCatalog();

// Built-in search providers seeded on first run / update (not opt-in presets).
struct DefaultProvider {
    QString id, name, url, arr, nm, hash, sz, seed, leech, region;
    bool enabled = true;
};
QList<DefaultProvider> defaultProviders();

} // namespace AddonCatalog
