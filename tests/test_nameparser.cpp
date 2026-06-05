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

TEST_CASE("audio channel layouts don't leak into the title", "[nameparser]")
{
    CHECK(P("The.Batman.2022.2160p.WEB-DL.DDP5.1.Atmos.DV.HDR.H.265-FLUX").cleanTitle == "The Batman");
    CHECK(P("Shogun.2024.S01E01.MULTi.1080p.WEB.H264-FW").cleanTitle == "Shogun");
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

TEST_CASE("type scoring: game tokens win over a stray video tag", "[nameparser]")
{
    // No repacker group, but a Denuvo marker → still a game.
    CHECK(P("Hogwarts Legacy Denuvo Bypass").contentType == ContentType::Game);
    CHECK(P("Elden Ring [Goldberg]").contentType == ContentType::Game);
    // A movie REPACK release must stay a movie (REPACK is ambiguous, not a game token).
    CHECK(P("Dune Part Two 2024 1080p REPACK x264").contentType == ContentType::Movie);
}

TEST_CASE("edition/release qualifiers are stripped from the title", "[nameparser]")
{
    CHECK(P("Hades II Early Access").cleanTitle == "Hades II");
    CHECK(P("The Witcher 3 Wild Hunt Complete Edition").cleanTitle == "The Witcher 3 Wild Hunt");
    CHECK(P("Hogwarts Legacy Deluxe Edition").cleanTitle == "Hogwarts Legacy");
    CHECK(P("Cyberpunk 2077 Ultimate Edition").cleanTitle == "Cyberpunk 2077");
    CHECK(P("Skyrim Special Edition").cleanTitle == "Skyrim");
}

TEST_CASE("a lone [Repack] tag doesn't misclassify a game as a movie", "[nameparser]")
{
    // No resolution/codec/year — "Repack" alone must not score video.
    CHECK(P("Red Dead Redemption 2 [Repack]").contentType != ContentType::Movie);
    // A real movie repack still has resolution+codec, so it stays a movie.
    CHECK(P("Dune Part Two 2024 1080p REPACK x264").contentType == ContentType::Movie);
}

TEST_CASE("bilingual RuTracker titles drop the Cyrillic half", "[nameparser]")
{
    CHECK(P("Ведьмак 3 / The Witcher 3").cleanTitle == "The Witcher 3");
    // Pure-Latin titles are untouched.
    CHECK(P("The Witcher 3").cleanTitle == "The Witcher 3");
}

TEST_CASE("classifyByFiles: payload extensions decide the type", "[nameparser]")
{
    CHECK(NameParser::classifyByFiles({"setup.exe", "data1.bin", "data2.bin"}) == ContentType::Game);
    CHECK(NameParser::classifyByFiles({"Movie.2024.1080p.mkv"}) == ContentType::Movie);
    CHECK(NameParser::classifyByFiles({"Show.S01E01.mkv", "Show.S01E02.mkv", "Show.S01E03.mkv"}) == ContentType::Series);
    CHECK(NameParser::classifyByFiles({"readme.txt"}) == ContentType::Unknown);
    // A game with a bundled intro video still reads as a game (payload dominates).
    CHECK(NameParser::classifyByFiles({"game.exe", "d3d.dll", "intro.mkv"}) == ContentType::Game);
}
