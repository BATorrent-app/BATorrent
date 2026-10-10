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

} // namespace SearchBridgeUtil
