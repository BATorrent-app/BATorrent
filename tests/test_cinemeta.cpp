#include <catch2/catch_test_macros.hpp>
#include "services/discovery/cinemeta.h"

// Trimmed from Cinemeta's reply for Bleach (tt0434665): IMDb splits it into
// sixteen seasons where TMDB has one long one.
static const QByteArray kBleach = R"({"meta":{"videos":[
  {"season":1,"episode":1,"number":1,"released":"2004-10-05T15:00:00.000Z","name":"The Day I Became a Shinigami"},
  {"season":1,"episode":2,"number":2,"released":"2004-10-12T15:00:00.000Z"},
  {"season":5,"episode":16,"number":16,"released":"2006-12-14T00:00:00.000Z"},
  {"season":5,"episode":17,"number":17,"released":"2006-12-21T00:00:00.000Z"},
  {"season":0,"episode":1,"number":1,"released":"2006-12-21T00:00:00.000Z"},
  {"season":16,"episode":24,"number":24,"released":"2012-03-28T00:00:00.000Z"}
]}})";

TEST_CASE("Cinemeta: a TMDB episode is carried to IMDb numbers by its air date", "[cinemeta]")
{
    const auto v = Cinemeta::videos(kBleach);
    REQUIRE(v.size() == 6);
    CHECK(v[0].released == QDate(2004, 10, 5));

    // TMDB's single season, episode 109, aired the day IMDb's S05E17 did
    auto n = Cinemeta::mapByAirDate(v, QDate(2006, 12, 21), 1, 109);
    CHECK(n.season == 5);
    CHECK(n.episode == 17);

    // a day apart: TMDB dates by Japan, Cinemeta by UTC
    n = Cinemeta::mapByAirDate(v, QDate(2004, 10, 6), 1, 1);
    CHECK(n.season == 1);
    CHECK(n.episode == 1);

    // no match, no date or no list: the numbers that were asked for
    n = Cinemeta::mapByAirDate(v, QDate(1999, 1, 1), 2, 3);
    CHECK((n.season == 2 && n.episode == 3));
    n = Cinemeta::mapByAirDate(v, QDate(), 2, 3);
    CHECK((n.season == 2 && n.episode == 3));
    n = Cinemeta::mapByAirDate({}, QDate(2006, 12, 21), 1, 109);
    CHECK((n.season == 1 && n.episode == 109));

    // a special stays a special
    n = Cinemeta::mapByAirDate(v, QDate(2006, 12, 21), 0, 1);
    CHECK((n.season == 0 && n.episode == 1));

    // the next one, across a season break, by absolute number, and none after the last
    n = Cinemeta::nextAfter(v, 5, 16);
    CHECK((n.season == 5 && n.episode == 17));
    n = Cinemeta::nextAfter(v, 1, 2);
    CHECK((n.season == 5 && n.episode == 16));
    n = Cinemeta::nextAfter(v, 1, 4);                 // "Bleach - 004": the fourth aired
    CHECK((n.season == 16 && n.episode == 24));
    n = Cinemeta::nextAfter(v, 16, 24);
    CHECK(n.season == -1);
    n = Cinemeta::nextAfter(v, 9, 9);                 // a season IMDb doesn't have
    CHECK(n.season == -1);

    CHECK(Cinemeta::videos("not json").isEmpty());
    CHECK(Cinemeta::metaUrl(QStringLiteral("tt0434665")).path() == QStringLiteral("/meta/series/tt0434665.json"));
}
