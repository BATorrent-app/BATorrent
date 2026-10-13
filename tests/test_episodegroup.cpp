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

static EpisodeTag W(const char *s, QStringList works) { return EpisodeGroup::classify(QString::fromUtf8(s), works); }

TEST_CASE("an anime sequel numbered from 1 stays off the show's season 1", "[episodegroup]")
{
    const QStringList bleach{ QStringLiteral("Bleach"), QStringLiteral("BLEACH") };
    // Thousand-Year Blood War counts its own episodes from 1; as "Bleach - 04"
    // it ranked first on Bleach T1 E4 with 979 seeds.
    auto tybw = W("[Erai-raws] Bleach - Sennen Kessen-hen - Soukoku-tan - 04 [1080p CR WEB-DL AVC AAC][MultiSub]", bleach);
    CHECK(tybw.season == -1);
    CHECK(tybw.episode == -1);
    CHECK_FALSE(tybw.pack);

    auto own = W("[Erai-raws] Bleach - 004 [1080p][Multiple Subtitle]", bleach);
    CHECK(own.season == 1);
    CHECK(own.episode == 4);

    auto shippuden = W("[Judas] Naruto Shippuden - 120 [1080p]", { QStringLiteral("Naruto") });
    CHECK(shippuden.season == -1);
    CHECK(shippuden.episode == -1);

    // a year or brackets after the name are not another show
    auto year = W("[SubsPlease] Bleach (2004) - 12 (1080p)", bleach);
    CHECK(year.season == 1);
    CHECK(year.episode == 12);

    // without the show's names nothing changes
    CHECK(T("[Erai-raws] Bleach - Sennen Kessen-hen - 04 [1080p]").season == 1);

    // an explicit SxxExx is the season whatever the title says
    auto explicitSe = W("Bleach Sennen Kessen-hen S02E04 1080p", bleach);
    CHECK(explicitSe.season == 2);
    CHECK(explicitSe.episode == 4);
}

TEST_CASE("an anime season named in the title is that season", "[episodegroup]")
{
    const QStringList jjk{ QStringLiteral("Jujutsu Kaisen") };
    auto s2 = W("[SubsPlease] Jujutsu Kaisen S2 - 05 (1080p) [ABCD1234]", jjk);
    CHECK(s2.season == 2);
    CHECK(s2.episode == 5);

    auto nd = W("[Erai-raws] Jujutsu Kaisen 2nd Season - 05 [1080p]", jjk);
    CHECK(nd.season == 2);
    CHECK(nd.episode == 5);
}

TEST_CASE("pickFile: the episode's file out of a pack", "[episodegroup]")
{
    const QStringList batch{
        QStringLiteral("[Erai-raws] Bleach - 001 ~ 366/[Erai-raws] Bleach - 001 [1080p][Multiple Subtitle].mkv"),
        QStringLiteral("[Erai-raws] Bleach - 001 ~ 366/[Erai-raws] Bleach - 002 [1080p][Multiple Subtitle].mkv"),
        QStringLiteral("[Erai-raws] Bleach - 001 ~ 366/[Erai-raws] Bleach - 003 [1080p][Multiple Subtitle].mkv.!bt"),
        QStringLiteral("[Erai-raws] Bleach - 001 ~ 366/[Erai-raws] Bleach - 004 [1080p][Multiple Subtitle].mkv"),
        QStringLiteral("[Erai-raws] Bleach - 001 ~ 366/Bleach - Movie 1 [1080p].mkv"),
        QStringLiteral("[Erai-raws] Bleach - 001 ~ 366/info.txt"),
    };
    const QList<qint64> sizes{ 300, 310, 320, 330, 4000, 1 };

    // the addon's file name, even with a wrong index
    auto byName = EpisodeGroup::pickFile(batch, sizes, 0,
        QStringLiteral("[Erai-raws] Bleach - 003 [1080p][Multiple Subtitle].mkv"), 1, 3);
    CHECK(byName.index == 2);
    CHECK(byName.how == QLatin1String("filename"));

    auto byIdx = EpisodeGroup::pickFile(batch, sizes, 3, QString(), 1, 4);
    CHECK(byIdx.index == 3);
    CHECK(byIdx.how == QLatin1String("fileIdx"));

    // an index on a non-video falls through to the episode
    auto byEp = EpisodeGroup::pickFile(batch, sizes, 5, QString(), 1, 4);
    CHECK(byEp.index == 3);
    CHECK(byEp.how == QLatin1String("episode"));

    // nothing known: the largest video, as before (here the film)
    auto largest = EpisodeGroup::pickFile(batch, sizes, -1, QString(), -1, -1);
    CHECK(largest.index == 4);
    CHECK(largest.how == QLatin1String("largest"));

    const QStringList season{ QStringLiteral("Show.S02E01.mkv"), QStringLiteral("Show.S02E02.mkv"),
                              QStringLiteral("Show.S01E02.mkv") };
    auto s2e2 = EpisodeGroup::pickFile(season, { 10, 10, 50 }, -1, QString(), 2, 2);
    CHECK(s2e2.index == 1);

    CHECK(EpisodeGroup::pickFile({}, {}, 0, QString(), 1, 1).index == -1);
}
