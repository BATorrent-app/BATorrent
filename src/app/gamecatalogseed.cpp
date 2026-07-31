// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "app/gamecatalogseed.h"

#include "services/discovery/gamesourcemanager.h"

#include <QSettings>

namespace GameCatalogSeed {

void apply()
{
    static const int kCatalogSeedGen = 5;
    static const char *kCatalogName = "BATorrent Games";
    static const char *kCatalogUrl =
        "https://gist.githubusercontent.com/Mateuscruz19/038beb9fef8681e191e3053b8a79c29b/raw/feed.json";
    static const char *kLegacyUrl =
        "https://raw.githubusercontent.com/Jdjsjjqq/rutracker-hydra/main/combined_torrents.json";
    static const char *kLegacyGistGames =
        "https://gist.githubusercontent.com/Mateuscruz19/038beb9fef8681e191e3053b8a79c29b/raw/games.json";
    static const char *kLegacyGistBat =
        "https://gist.githubusercontent.com/Mateuscruz19/038beb9fef8681e191e3053b8a79c29b/raw/batorrent-games.json";

    QSettings gs;
    const int gen = gs.value(QStringLiteral("gameCatalogSeedGen"), 0).toInt();
    auto &gsm = GameSourceManager::instance();
    if (gen < kCatalogSeedGen) {
        gs.setValue(QStringLiteral("gameCatalogSeedGen"), kCatalogSeedGen);
        gs.setValue(QStringLiteral("gameSourcesSeeded"), true);
        gsm.removeSource(QString::fromUtf8(kLegacyUrl));
        gsm.removeSource(QString::fromUtf8(kLegacyGistGames));
        gsm.removeSource(QString::fromUtf8(kLegacyGistBat));
        bool has = false;
        for (const auto &s : gsm.sources())
            if (s.second == QLatin1String(kCatalogUrl)) { has = true; break; }
        if (!has)
            gsm.addSource(QString::fromUtf8(kCatalogName), QString::fromUtf8(kCatalogUrl));
    }
}

} // namespace GameCatalogSeed
