// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#pragma once

#include <QByteArray>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

// Pure TMDB response parsers (no network). DiscoveryService stays fetch + emit.
namespace TmdbParse {

// Prefer language-untagged backdrops (no burned-in title text), then the rest.
// The title treatment: the show's own lettering, which TMDB returns in the
// same /images payload the backdrops come from. English first, then one with
// no language at all (most logos are wordmarks and carry none), then whatever
// scored best. Empty when the title has none, which is common and fine.
QString logoUrl(const QByteArray &imagesJson, const QString &imageBaseUrl);

QStringList backdropUrls(const QByteArray &imagesJson,
                         const QString &imageBaseUrl,
                         int limit = 10);

// YouTube key: official Trailer first, else first Teaser. Empty if none.
QString youtubeTrailerKey(const QByteArray &videosJson);

// Season payload → [{episode, name, air_date}, ...]
// One row per episode of a season. stillBase is prefixed onto still_path, so
// an episode with no image comes back with an empty "still" rather than a URL
// that 404s. The synopsis and the thumbnail ride in the same response as the
// title did — reading only the title was leaving a screen's worth of material
// on the floor.
QVariantList episodeRows(const QByteArray &seasonJson, const QString &stillBase = QString());

// /{movie|tv}/{id}?append_to_response=credits → {genres, cast, seasons}.
// Cast is billing order, capped: the hero names three people, not thirty.
QVariantMap workDetails(const QByteArray &detailsJson, int castLimit = 3);

// /recommendations results → poster cards (cap applies).
QVariantList recommendationRows(const QByteArray &json,
                                bool isTv,
                                const QString &posterBase,
                                int limit = 16);

// Discover/list shelf results → poster cards with backdrop + tmdbId.
QVariantList shelfRows(const QByteArray &json,
                       bool isTv,
                       const QString &posterBase,
                       const QString &backdropBase);

// /search/multi → movie/tv works including originalTitle (for tracker queries).
QVariantList multiSearchRows(const QByteArray &json, const QString &posterBase,
                             const QString &backdropBase = QString());

} // namespace TmdbParse
