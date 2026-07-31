// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#pragma once

#include <QByteArray>
#include <QString>
#include <QStringList>
#include <QVariantList>

// Pure TMDB response parsers (no network). DiscoveryService stays fetch + emit.
namespace TmdbParse {

// Prefer language-untagged backdrops (no burned-in title text), then the rest.
QStringList backdropUrls(const QByteArray &imagesJson,
                         const QString &imageBaseUrl,
                         int limit = 10);

// YouTube key: official Trailer first, else first Teaser. Empty if none.
QString youtubeTrailerKey(const QByteArray &videosJson);

// Season payload → [{episode, name, air_date}, ...]
QVariantList episodeRows(const QByteArray &seasonJson);

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
QVariantList multiSearchRows(const QByteArray &json, const QString &posterBase);

} // namespace TmdbParse
