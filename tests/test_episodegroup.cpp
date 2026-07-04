// SPDX-License-Identifier: MIT
// EpisodeGroup regression suite (Catch2 v3).
//
// Locks in the release→episode mapping behind the series drill-down grouping:
// single episodes (SxxExx and friends), season packs, and complete/batch packs.

#include <catch2/catch_test_macros.hpp>

#include "services/metadata/episodegroup.h"

static EpisodeTag T(const char *s) { return EpisodeGroup::classify(QString::fromUtf8(s)); }

TEST_CASE("single episodes: SxxExx and NxNN forms", "[episodegroup]")
{
    auto e = T("Breaking Bad S05E14 Ozymandias 1080p WEB-DL");
    CHECK(e.season == 5);
    CHECK(e.episode == 14);
    CHECK_FALSE(e.pack);

    auto x = T("The Mandalorian 2x05 1080p");
    CHECK(x.season == 2);
    CHECK(x.episode == 5);
    CHECK_FALSE(x.pack);

    auto sp = T("The.Last.of.Us.S01.E05.720p");   // separated form
    CHECK(sp.season == 1);
    CHECK(sp.episode == 5);
    CHECK_FALSE(sp.pack);
}

TEST_CASE("anime absolute numbering pins season 1", "[episodegroup]")
{
    auto a = T("[SubsPlease] One Piece - 1071 (1080p) [ABCD1234]");
    CHECK(a.season == 1);
    CHECK(a.episode == 1071);
    CHECK_FALSE(a.pack);
}

TEST_CASE("season packs: bare Sxx and localized words", "[episodegroup]")
{
    auto s = T("The.Bear.S02.1080p.WEB-DL.DDP5.1");
    CHECK(s.season == 2);
    CHECK(s.episode == -1);
    CHECK(s.pack);

    auto w = T("Stranger Things Season 4 Complete 2160p");
    CHECK(w.season == 4);
    CHECK(w.pack);

    auto pt = T("Round 6 Temporada 2 Dublado WEB-DL");
    CHECK(pt.season == 2);
    CHECK(pt.pack);

    auto ord = T("Dark 3ª Temporada Completa Dual Audio");
    CHECK(ord.season == 3);
    CHECK(ord.pack);
}

TEST_CASE("multi-season ranges and complete keywords are whole-series packs", "[episodegroup]")
{
    auto r = T("Game of Thrones S01-S08 Complete BluRay");
    CHECK(r.pack);
    CHECK(r.season == -1);

    auto sr = T("Friends Seasons 1-10 720p");
    CHECK(sr.pack);
    CHECK(sr.season == -1);

    auto c = T("Avatar The Last Airbender COMPLETE Series 1080p");
    CHECK(c.pack);
    CHECK(c.season == -1);

    auto b = T("[Judas] Naruto Shippuden (Batch) (001-500)");
    CHECK(b.pack);

    auto todas = T("Chaves Todas as Temporadas DVDRip Nacional");
    CHECK(todas.pack);
}

TEST_CASE("no series marker → unknown, not a pack", "[episodegroup]")
{
    auto m = T("Oppenheimer.2023.2160p.UHD.BluRay.x265");
    CHECK(m.season == -1);
    CHECK(m.episode == -1);
    CHECK_FALSE(m.pack);
}
