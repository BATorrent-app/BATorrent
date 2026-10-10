// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#pragma once

#include "services/platform/contentlanguage.h"

#include <QSettings>
#include <QString>

// Keys and image bases shared by the DiscoveryService TUs.
namespace DiscoveryKeys {


inline QString tmdbApiKey()
{
    QString key = QSettings("BATorrent", "BATorrent").value("tmdbApiKey").toString();
#ifdef BAT_TMDB_KEY
    if (key.isEmpty()) key = QStringLiteral(BAT_TMDB_KEY);
#endif
    return key;
}
inline QString igdbClientId()
{
    QString id = QSettings("BATorrent", "BATorrent").value("igdbClientId").toString();
#ifdef BAT_IGDB_CLIENT_ID
    if (id.isEmpty()) id = QStringLiteral(BAT_IGDB_CLIENT_ID);
#endif
    return id;
}
inline QString igdbClientSecret()
{
    QString s = QSettings("BATorrent", "BATorrent").value("igdbClientSecret").toString();
#ifdef BAT_IGDB_CLIENT_SECRET
    if (s.isEmpty()) s = QStringLiteral(BAT_IGDB_CLIENT_SECRET);
#endif
    return s;
}

inline const QString TmdbBaseUrl    = QStringLiteral("https://api.themoviedb.org/3");
inline const QString TmdbPosterBase = QStringLiteral("https://image.tmdb.org/t/p/w342");
inline const QString TmdbBackdrop   = QStringLiteral("https://image.tmdb.org/t/p/w1280");
// Episode thumbnails sit at list size, not hero size: w300 is the smallest
// TMDB still that does not look soft at the width a row gives them.
inline const QString TmdbStillBase  = QStringLiteral("https://image.tmdb.org/t/p/w300");
// Wide enough for a title treatment at hero size, and transparent PNGs
// stay transparent at any width.
inline const QString TmdbLogoBase   = QStringLiteral("https://image.tmdb.org/t/p/w500");

inline QString tmdbLang() { return ContentLanguage::tmdb(); }

} // namespace DiscoveryKeys
