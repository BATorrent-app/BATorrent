// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#pragma once

#include <QString>
#include <QVariantList>

// Pure title-search finish: relevance-sort TMDB + IGDB works, interleave by
// (name-match score, rating), then de-dupe. DiscoveryService stays fetch + emit.
namespace DiscoverySearch {

QVariantList rankAndMerge(const QString &query, const QVariantList &works);

} // namespace DiscoverySearch
