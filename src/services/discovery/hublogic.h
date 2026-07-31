// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#pragma once

#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QSet>

// Pure Hub library helpers (genre matching, shelf filtering, search/sort).
// Kept out of QML so the recommendation heuristics stay characterization-tested.
namespace HubLogic {

// Normalize a free-text genre label (EN/PT) into a stable discovery key, or "".
QString genreKey(const QString &name);

// Most frequent genreKey across a flat list of genre name strings.
QString topGenre(const QStringList &genreNames);

// Drop candidates whose title (case-insensitive) is already in `ownedLower`.
QVariantList excludeOwnedTitles(const QVariantList &candidates,
                                const QSet<QString> &ownedLower,
                                int limit = 12);

// Filter by title substring + optional name sort. `sort` is "name" or anything else
// (caller sorts by recency for non-name).
QVariantList applyView(const QVariantList &list,
                       const QString &search,
                       const QString &sort);

} // namespace HubLogic
