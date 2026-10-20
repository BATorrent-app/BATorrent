// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#pragma once

#include "services/metadata/gamereleasepick.h"

#include <QString>
#include <QVariantMap>

// Shared free helpers for QmlSearchBridge TUs (magnet parse / title fold / dedupe / title rows).
namespace SearchBridgeUtil {

bool sameTitle(const QString &a, const QString &b);
QString btihFromMagnet(const QString &magnet);
QString resultDedupeKey(const QString &magnet, const QString &name, qlonglong size);
GameReleasePick::Candidate gameCandFromRow(const QVariantMap &m, bool hasUri);
// A discovery work (TMDB/IGDB) as a row of the titles grid. Activating the row
// hands this same map to setWorkContext, so anything the hero reads must ride here.
QVariantMap titleRowFromWork(const QVariantMap &work);
// The film after tmdbId among a collection's films (in release order, as
// TmdbParse::collectionParts gives them); empty when it is the last or absent.
QVariantMap filmAfter(const QVariantList &films, int tmdbId);
// The names from `candidates` a picked title should still be hunted under:
// not empty, not one already in `searched` (or picked), and three names in all
// at most, since each costs a request per provider.
QStringList huntTitles(const QStringList &searched, const QStringList &candidates);
// What an indexer is asked for a title: its words without the punctuation
// release names leave out, and a film's year after them.
QString trackerQuery(const QString &title, const QString &year, const QString &type);

} // namespace SearchBridgeUtil
