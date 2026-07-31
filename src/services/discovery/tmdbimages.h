// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#pragma once

#include <QByteArray>
#include <QString>
#include <QStringList>

// Pure TMDB response parsers (no network). DiscoveryService stays fetch + emit.
namespace TmdbImages {

// Prefer language-untagged backdrops (no burned-in title text), then the rest.
QStringList backdropUrls(const QByteArray &imagesJson,
                         const QString &imageBaseUrl,
                         int limit = 10);

// YouTube key: official Trailer first, else first Teaser. Empty if none.
QString youtubeTrailerKey(const QByteArray &videosJson);

} // namespace TmdbImages
