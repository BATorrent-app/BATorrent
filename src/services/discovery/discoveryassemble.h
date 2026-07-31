// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#pragma once

#include <QMap>
#include <QVariantList>
#include <QVariantMap>

// Pure Discover page assembly: shelf merge, cross-row de-dupe + genre tags,
// round-robin hero. DiscoveryService owns network + cache + emit.
namespace DiscoveryAssemble {

// Append incoming shelf page items, de-duped by poster URL.
QVariantList mergeShelfByPoster(const QVariantList &existing,
                                const QVariantList &incoming);

// Ordered rows from accum (order → {label, items}), de-duped across shelves,
// with stable genre keys for Hub taste matching.
QVariantList rowsFromAccum(const QMap<int, QVariantMap> &accum);

// Round-robin one item per row per pass (backdrop + overview required).
QVariantList heroFromAccum(const QMap<int, QVariantMap> &accum, int limit = 6);

} // namespace DiscoveryAssemble
