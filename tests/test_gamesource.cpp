// SPDX-License-Identifier: MIT
// GameSourceManager suite (Catch2 v3).
//
// Locks in the catalog parsing that powers game search: Hydra-format JSON
// (downloads[].title/uris/fileSize) → indexed entries, repacker/version tag
// stripping for cover matching, and local substring search. No network — the
// fetch path is exercised separately.

#include <catch2/catch_test_macros.hpp>

#include "services/discovery/gamesourcemanager.h"

static QString clean(const char *s) { return GameSourceManager::cleanGameTitle(QString::fromUtf8(s)); }

TEST_CASE("cleanGameTitle strips repacker / version / language tags", "[gamesource]")
{
    CHECK(clean("Cyberpunk 2077 v2.1 [FitGirl Repack]") == "Cyberpunk 2077");
    CHECK(clean("Elden Ring [DODI Repack]")             == "Elden Ring");
    CHECK(clean("The Witcher 3 (GOTY Edition)")         == "The Witcher 3");
    CHECK(clean("DOOM Eternal MULTi13")                 == "DOOM Eternal");
    CHECK(clean("Hades")                                == "Hades");   // no tags → unchanged
}

TEST_CASE("cleanGameTitle handles RuTracker-style bracket prefixes", "[gamesource]")
{
    CHECK(clean("[DL] Cyberpunk 2077 [P] [RUS + ENG + 12 / ENG] (2022, RPG) (1.16.0 + 6 DLC) [Portable]") == "Cyberpunk 2077");
    CHECK(clean("[DL] Hogwarts Legacy [P] [RUS + ENG / ENG] (2023, TPS) (1117238 + 14 DLC)") == "Hogwarts Legacy");
    CHECK(clean("[DL] God of War [P] [RUS + ENG + 16 / RUS + ENG] (2022, TPS) (1.0.13) [P2P]") == "God of War");
    CHECK_FALSE(clean("[DL] [В разработке] Cyberpunk SFX [L]").isEmpty());   // never empty
}

TEST_CASE("indexCatalog parses Hydra format; keeps http-only entries as a fallback", "[gamesource]")
{
    const QByteArray fixture = R"({
        "name": "Test Catalog",
        "downloads": [
            { "title": "Cyberpunk 2077 v2.1 [FitGirl Repack]",
              "uris": ["magnet:?xt=urn:btih:aaaa"], "fileSize": "58 GB", "uploadDate": "2024-01-01T00:00:00Z" },
            { "title": "Elden Ring [DODI Repack]",
              "uris": ["magnet:?xt=urn:btih:bbbb"], "fileSize": "45 GB", "uploadDate": "2024-02-01T00:00:00Z" },
            { "title": "Host Only Game",
              "uris": ["https://pixeldrain.com/u/abc123"], "fileSize": "1 GB", "uploadDate": "2024-03-01T00:00:00Z" },
            { "title": "Nothing Fetchable",
              "uris": [], "fileSize": "1 GB", "uploadDate": "2024-04-01T00:00:00Z" }
        ]
    })";

    auto &gsm = GameSourceManager::instance();
    const int added = gsm.indexCatalog("Test Catalog", fixture);
    REQUIRE(added == 3);   // 2 magnet + 1 http-only kept; the no-uri entry dropped

    // Both searches live in one SECTION: indexCatalog accumulates into the
    // GameSourceManager singleton (no per-source clear), so only the first-run
    // leaf is guaranteed a single index pass — later leaves would see duplicates.
    SECTION("resolves magnet rows and keeps http-only rows as a fallback") {
        const auto magnetHit = gsm.search("cyberpunk");
        REQUIRE(magnetHit.size() == 1);
        CHECK(magnetHit[0].cleanTitle == "Cyberpunk 2077");
        CHECK(magnetHit[0].magnet.startsWith("magnet:"));
        CHECK(magnetHit[0].httpUrl.isEmpty());   // magnet entries carry no http fallback
        CHECK(magnetHit[0].fileSize == "58 GB");
        CHECK(magnetHit[0].source == "Test Catalog");

        const auto httpHit = gsm.search("host only");
        REQUIRE(httpHit.size() == 1);
        CHECK(httpHit[0].magnet.isEmpty());
        CHECK(httpHit[0].httpUrl == "https://pixeldrain.com/u/abc123");
    }

    SECTION("search returns nothing for a miss") {
        CHECK(gsm.search("half-life").isEmpty());
    }

    SECTION("malformed JSON adds nothing") {
        CHECK(gsm.indexCatalog("Bad", QByteArray("not json")) == 0);
    }
}
