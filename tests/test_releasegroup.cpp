// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include <catch2/catch_test_macros.hpp>

#include "services/metadata/releasegroup.h"

// The names below are verbatim from a real "meccha" search: they are the reason
// this module exists. Every one of them showed the user "BitSearch" and nothing
// about who released it.
TEST_CASE("release names from the wild report their group", "[releasegroup]")
{
    SECTION("online-fix publishes as OFME, and the acronym is decoded") {
        REQUIRE(ReleaseGroup::detect("MECCHA.CHAMELEON.v2.3.0-OFME.zip") == "Online-Fix");
        REQUIRE(ReleaseGroup::detect("MECCHA.CHAMELEON.v1.2.2-OFME.rar") == "Online-Fix");
        REQUIRE(ReleaseGroup::detect("MECCHA.CHAMELEON.v1.0.2-OFME") == "Online-Fix");
        REQUIRE(ReleaseGroup::detect("Some.Game-Online-Fix") == "Online-Fix");
        REQUIRE(ReleaseGroup::detect("Some.Game.OnlineFix.v2") == "Online-Fix");
    }

    SECTION("the browser's copy index and the domain tail don't hide the group") {
        REQUIRE(ReleaseGroup::detect("MECCHA.CHAMELEON-SteamRIP.com(1).zip") == "SteamRIP");
        REQUIRE(ReleaseGroup::detect("MECCHA.CHAMELEON-SteamRIP.com.zip") == "SteamRIP");
    }

    SECTION("uploader credit games carry") {
        REQUIRE(ReleaseGroup::detect("MECCHA CHAMELEON v1.0.2 by Pioneer") == "Pioneer");
        REQUIRE(ReleaseGroup::detect("The Witcher 3 Wild Hunt By Igruha") == "Igruha");
    }

    SECTION("movie scene tail — an unknown group is still shown, not swallowed") {
        REQUIRE(ReleaseGroup::detect("Obsessao.2026.WEB-DL.1080p.x264.DUAL.5.1-STARCKFILMES")
                == "STARCKFILMES");
    }
}

// The failure mode that matters most: reporting a codec as a release group would
// be worse than reporting nothing, because the user would trust it.
TEST_CASE("format tokens are never mistaken for a group", "[releasegroup]")
{
    REQUIRE(ReleaseGroup::detect("Movie.2024.WEB-DL").isEmpty());
    REQUIRE(ReleaseGroup::detect("Movie.2024.1080p.BluRay.x264").isEmpty());
    REQUIRE(ReleaseGroup::detect("Show.S01E01.2160p-HDR").isEmpty());
    REQUIRE(ReleaseGroup::detect("Movie.2024.Dual.Audio-DUBLADO").isEmpty());
    REQUIRE(ReleaseGroup::detect("Some.Movie-2160").isEmpty());   // bare number
    REQUIRE(ReleaseGroup::detect("").isEmpty());
    REQUIRE(ReleaseGroup::detect("-").isEmpty());
    REQUIRE(ReleaseGroup::detect("A Plain Movie Title 2024").isEmpty());
}

TEST_CASE("canonical() normalises the spellings people actually type", "[releasegroup]")
{
    REQUIRE(ReleaseGroup::canonical("ofme") == "Online-Fix");
    REQUIRE(ReleaseGroup::canonical("OFME") == "Online-Fix");
    REQUIRE(ReleaseGroup::canonical("online_fix") == "Online-Fix");
    REQUIRE(ReleaseGroup::canonical("steamrip.com") == "SteamRIP");
    REQUIRE(ReleaseGroup::canonical("fitgirl") == "FitGirl");
    REQUIRE(ReleaseGroup::canonical("0xdeadcode") == "0xdeadc0de");
    REQUIRE(ReleaseGroup::canonical("  -DODI- ") == "DODI");

    SECTION("an unknown but plausible tag passes through with its own casing") {
        REQUIRE(ReleaseGroup::canonical("STARCKFILMES") == "STARCKFILMES");
    }
    SECTION("a format token is rejected outright") {
        REQUIRE(ReleaseGroup::canonical("x264").isEmpty());
        REQUIRE(ReleaseGroup::canonical("1080p").isEmpty());
        REQUIRE(ReleaseGroup::canonical("9").isEmpty());
    }
}

// Two known groups in one name must resolve the same way on every run — the
// reason the alias table is an ordered list and not a QHash.
TEST_CASE("detection is deterministic when a name carries two known groups", "[releasegroup]")
{
    const QString name = QStringLiteral("Game.Repack-FitGirl.Online-Fix.zip");
    const QString first = ReleaseGroup::detect(name);
    REQUIRE_FALSE(first.isEmpty());
    for (int i = 0; i < 20; ++i)
        REQUIRE(ReleaseGroup::detect(name) == first);
}
