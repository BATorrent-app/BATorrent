#include <catch2/catch_test_macros.hpp>
#include "services/metadata/aniskip.h"

#include <QUrlQuery>

static QByteArray page(const char *media)
{
    return QByteArray(R"({"data":{"Page":{"media":)") + media + "}}}";
}

TEST_CASE("AniSkip: an AniList search gives the entry whose name matches", "[aniskip]")
{
    const QStringList bleach{ QStringLiteral("Bleach") };
    // the reply AniList gave for "Bleach", trimmed
    CHECK(AniSkip::malIdFromSearch(page(R"([{"idMal":269,"title":{"romaji":"BLEACH","english":"Bleach","native":"BLEACH"},"synonyms":[]},
        {"idMal":41467,"title":{"romaji":"BLEACH: Sennen Kessen-hen","english":"BLEACH: Thousand-Year Blood War"},"synonyms":["BLEACH TYBW"]}])"),
        bleach) == 269);
    // matched through a synonym, ignoring case and punctuation
    CHECK(AniSkip::malIdFromSearch(page(R"([{"idMal":16498,"title":{"romaji":"Shingeki no Kyojin"},"synonyms":["Attack on Titan"]}])"),
                                   { QStringLiteral("attack on titan!") }) == 16498);
    // a search for a show that isn't anime still returns anime: none of it counts
    CHECK(AniSkip::malIdFromSearch(page(R"([{"idMal":1,"title":{"romaji":"Cowboy Bebop"},"synonyms":[]}])"),
                                   { QStringLiteral("Breaking Bad") }) == 0);
    // AniList-only entries have no MyAnimeList id to ask AniSkip with
    CHECK(AniSkip::malIdFromSearch(page(R"([{"idMal":null,"title":{"romaji":"Bleach"},"synonyms":[]}])"), bleach) == 0);
    CHECK(AniSkip::malIdFromSearch(page("[]"), bleach) == 0);
    CHECK(AniSkip::malIdFromSearch("not json", bleach) == 0);
    CHECK(AniSkip::malIdFromSearch(page(R"([{"idMal":269,"title":{"romaji":"Bleach"}}])"), {}) == 0);

    CHECK(AniSkip::malSearchUrl().host() == QStringLiteral("graphql.anilist.co"));
    CHECK(AniSkip::malSearchBody(QStringLiteral("Bleach")).contains(R"("q":"Bleach")"));
}

TEST_CASE("AniSkip: opening and ending times become skip chapters", "[aniskip]")
{
    const QByteArray json = R"({"found":true,"results":[
        {"interval":{"startTime":84.5,"endTime":174.5},"skipType":"op","episodeLength":1420},
        {"interval":{"startTime":1290,"endTime":1380},"skipType":"ed","episodeLength":1420},
        {"interval":{"startTime":10,"endTime":20},"skipType":"recap","episodeLength":1420},
        {"interval":{"startTime":50,"endTime":40},"skipType":"op","episodeLength":1420}
    ]})";
    const auto ch = AniSkip::chaptersFromSkipTimes(json);
    REQUIRE(ch.size() == 2);
    CHECK(ch[0].kind == QStringLiteral("intro"));
    CHECK(ch[0].startMs == 84500);
    CHECK(ch[0].endMs == 174500);
    CHECK(ch[1].kind == QStringLiteral("credits"));
    CHECK(ch[1].startMs == 1290000);

    CHECK(AniSkip::chaptersFromSkipTimes(R"({"found":false,"results":[]})").isEmpty());

    const QUrl url = AniSkip::skipTimesUrl(269, 4);
    CHECK(url.path() == QStringLiteral("/v2/skip-times/269/4"));
    CHECK(QUrlQuery(url).allQueryItemValues(QStringLiteral("types")) == QStringList{ "op", "ed" });
}
