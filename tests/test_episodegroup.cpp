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

TEST_CASE("no series marker -> unknown, not a pack", "[episodegroup]")
{
    auto m = T("Oppenheimer.2023.2160p.UHD.BluRay.x265");
    CHECK(m.season == -1);
    CHECK(m.episode == -1);
    CHECK_FALSE(m.pack);
}

TEST_CASE("Reacher S01 season packs around 27 GB are packs, not episode 1", "[episodegroup]")
{
    // Real names from a search: one of them sat on episode 1 as a 27 GB
    // "episode", while episodes 2-8 read "in a season pack".
    const char *names[] = {
        "Reacher.S01.1080p.AMZN.WEBRip.DDP5.1.x264-NTb[rartv]",
        "Reacher.S01.1080p.AMZN.WEB-DL.DDP5.1.H.264-NTb",
        "Jack Reacher (S01)(2022)(Complete)(FHD)(1080p)(x264)(WebDL)(Multi 11 Lang)(MultiSUB) PHDTeam",
        "Reacher.S01.1080p.AMZN.WEB-DL.DDP5.1.x264-NTb_RUTOR",
        "Reacher.2022.S01.WEB-DL.1080p",
        "Reacher.S01.WEBDL.1080p.Rus.Eng.HDRezka",
        "[ 高清剧集网 www.BTHDTV.com ]侠探杰克[第01-08集][中文字幕].Reacher.S01.1080p.AMZN.WEB-DL.DDP5.1.H.264-CatWEB",
        "Reacher.S01.PL.1080p.AMZN.WEB-DL.x264-666",
        "Reacher.S01.MULTi.1080p.AMZN.WEB-DL.DDP5.1.H264-TV4TG",
        "Reacher.S01.MULTi.VF2.1080p.WEB.H264-FW",
        "Reacher.S01.MULTi.1080p.AMZN.WEB-DL.DDP5.1.H.264.PACK-PSiG",
        "Reacher.2022.S01.1080p",
        "Reacher.S01.iTALiAN.MULTi.1080p.AMZN.WEB-DL.DDP5.1.H.264-MeM.GP",
        "Reacher.S01.1080p.Rus.Eng",
        "Reacher.S01E01-E08.1080p.WEB",
        "Reacher.S01E01-08.1080p.WEB",
    };
    for (const char *n : names) {
        const EpisodeTag t = EpisodeGroup::classify(QString::fromUtf8(n));
        INFO(n);
        CHECK(t.episode == -1);
        CHECK(t.pack);
        CHECK(t.season == 1);
    }
}
