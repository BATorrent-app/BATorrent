// SPDX-License-Identifier: MIT
// NameParser regression suite (Catch2 v3).
//
// Locks in the title parsing that drives cover-art lookup: release-site prefix
// stripping, episode (SxxExx / NxNN) detection with the show title taken from
// before the marker, movie year extraction, and game repacker detection.

#include <catch2/catch_test_macros.hpp>

#include "app/nameparser.h"

static ParsedName P(const char *s) { return NameParser::parse(QString::fromUtf8(s)); }

TEST_CASE("leading release-site prefixes are stripped", "[nameparser]")
{
    CHECK(P("www.UIndex.org - Euphoria US S03E08 in God We trust").cleanTitle == "Euphoria Us");
    CHECK(P("[ www.Torrenting.com ] - The Last of Us S01E05 1080p WEB-DL x265").cleanTitle == "The Last Of Us");
    CHECK(P("www.1337x.to - Breaking Bad S05E14 Ozymandias 720p").cleanTitle == "Breaking Bad");
    CHECK(P("[www.site.org]The.Bear.S02E03").cleanTitle == "The Bear");
    CHECK(P("www.sitedotorrent.com - euphoria s3e7").cleanTitle == "Euphoria");
}

TEST_CASE("a bare domain-looking name is not eaten (no trailing separator)", "[nameparser]")
{
    // "Doom.com" has no separator after the TLD, so it must NOT be treated as a
    // site prefix and stripped to nothing.
    CHECK_FALSE(P("Doom.com").cleanTitle.isEmpty());
}

TEST_CASE("episodes: season/episode parsed, title is the part before the marker", "[nameparser]")
{
    auto e = P("www.UIndex.org - Euphoria US S03E08 in God We trust");
    CHECK(e.contentType == ContentType::Series);
    CHECK(e.season == 3);
    CHECK(e.episode == 8);

    auto x = P("The Mandalorian 2x05 1080p");      // NxNN form
    CHECK(x.contentType == ContentType::Series);
    CHECK(x.season == 2);
    CHECK(x.episode == 5);
    CHECK(x.cleanTitle == "The Mandalorian");
}

TEST_CASE("movies: year extracted, no season", "[nameparser]")
{
    auto d = P("Dune.Part.Two.2024.2160p.UHD.BluRay");
    CHECK(d.cleanTitle == "Dune Part Two");
    CHECK(d.year == 2024);
    CHECK(d.season == -1);
    CHECK(d.episode == -1);
    CHECK(d.contentType == ContentType::Movie);
}

TEST_CASE("games: repacker tag detected", "[nameparser]")
{
    auto g = P("FitGirl - Cyberpunk 2077 v2.1");
    CHECK(g.cleanTitle == "Cyberpunk 2077");
    CHECK(g.contentType == ContentType::Game);
    CHECK(g.repackerTag == "FitGirl");
}

TEST_CASE("roman numerals stay upper-case", "[nameparser]")
{
    CHECK(P("Final Fantasy VII Remake").cleanTitle == "Final Fantasy VII Remake");
}

TEST_CASE("anime fansub naming: [Group] Title - NN is an episode", "[nameparser]")
{
    auto f = P("[SubsPlease] Sousou no Frieren - 28 (1080p) [9C4F2E3A].mkv");
    CHECK(f.cleanTitle == "Sousou No Frieren");
    CHECK(f.contentType == ContentType::Series);
    CHECK(f.episode == 28);

    auto o = P("[HorribleSubs] One Piece - 1085 [720p].mkv");
    CHECK(o.cleanTitle == "One Piece");
    CHECK(o.episode == 1085);

    // An album "Artist - NN" without a leading [group] must NOT become an episode.
    auto a = P("Adele - 30 (2021) [FLAC]");
    CHECK(a.contentType != ContentType::Series);
    CHECK(a.episode == -1);
}
