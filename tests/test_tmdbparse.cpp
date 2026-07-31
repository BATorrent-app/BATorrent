#include <catch2/catch_test_macros.hpp>
#include "services/discovery/tmdbparse.h"

#include <QByteArray>

static QByteArray sampleImagesJson()
{
    return QByteArrayLiteral(R"({
      "backdrops": [
        { "file_path": "/tagged.jpg", "iso_639_1": "en" },
        { "file_path": "/clean1.jpg", "iso_639_1": null },
        { "file_path": "/clean2.jpg", "iso_639_1": null },
        { "file_path": "/tagged2.jpg", "iso_639_1": "pt" },
        { "file_path": "/clean1.jpg", "iso_639_1": null }
      ]
    })");
}

TEST_CASE("backdropUrls prefers untagged then tagged, dedupes, respects limit", "[tmdbparse]") {
    const auto urls = TmdbParse::backdropUrls(
        sampleImagesJson(), QStringLiteral("https://img"), 3);
    REQUIRE(urls.size() == 3);
    REQUIRE(urls.at(0) == QLatin1String("https://img/clean1.jpg"));
    REQUIRE(urls.at(1) == QLatin1String("https://img/clean2.jpg"));
    REQUIRE(urls.at(2) == QLatin1String("https://img/tagged.jpg"));
}

TEST_CASE("backdropUrls empty on bad input", "[tmdbparse]") {
    REQUIRE(TmdbParse::backdropUrls({}, QStringLiteral("https://img")).isEmpty());
    REQUIRE(TmdbParse::backdropUrls(sampleImagesJson(), QStringLiteral("https://img"), 0).isEmpty());
}

TEST_CASE("youtubeTrailerKey prefers Trailer over Teaser", "[tmdbparse]") {
    const QByteArray json = QByteArrayLiteral(R"({
      "results": [
        { "site": "Vimeo", "type": "Trailer", "key": "vimeo1" },
        { "site": "YouTube", "type": "Teaser", "key": "teaser1" },
        { "site": "YouTube", "type": "Trailer", "key": "trailer1" }
      ]
    })");
    REQUIRE(TmdbParse::youtubeTrailerKey(json) == QStringLiteral("trailer1"));
}

TEST_CASE("youtubeTrailerKey falls back to Teaser", "[tmdbparse]") {
    const QByteArray json = QByteArrayLiteral(R"({
      "results": [
        { "site": "YouTube", "type": "Featurette", "key": "feat" },
        { "site": "YouTube", "type": "Teaser", "key": "teaser1" }
      ]
    })");
    REQUIRE(TmdbParse::youtubeTrailerKey(json) == QStringLiteral("teaser1"));
    REQUIRE(TmdbParse::youtubeTrailerKey({}).isEmpty());
}

TEST_CASE("episodeRows maps season payload fields", "[tmdbparse]") {
    const QByteArray json = QByteArrayLiteral(R"({
      "episodes": [
        { "episode_number": 1, "name": "Pilot", "air_date": "2020-01-01" },
        { "episode_number": 2, "name": "Next", "air_date": "2020-01-08" }
      ]
    })");
    const auto eps = TmdbParse::episodeRows(json);
    REQUIRE(eps.size() == 2);
    REQUIRE(eps.at(0).toMap().value(QStringLiteral("episode")).toInt() == 1);
    REQUIRE(eps.at(0).toMap().value(QStringLiteral("name")).toString() == QLatin1String("Pilot"));
    REQUIRE(eps.at(1).toMap().value(QStringLiteral("air_date")).toString()
            == QLatin1String("2020-01-08"));
    REQUIRE(TmdbParse::episodeRows({}).isEmpty());
}

TEST_CASE("recommendationRows skips missing posters and respects limit", "[tmdbparse]") {
    const QByteArray json = QByteArrayLiteral(R"({
      "results": [
        { "title": "A", "poster_path": "/a.jpg", "release_date": "2021-05-01",
          "vote_average": 7.5, "overview": "oa" },
        { "title": "B", "poster_path": "", "release_date": "2022-01-01",
          "vote_average": 1.0, "overview": "ob" },
        { "name": "C", "poster_path": "/c.jpg", "first_air_date": "2019-02-03",
          "vote_average": 8.0, "overview": "oc" }
      ]
    })");
    const auto rows = TmdbParse::recommendationRows(
        json, /*isTv=*/false, QStringLiteral("https://p"), 1);
    REQUIRE(rows.size() == 1);
    REQUIRE(rows.at(0).toMap().value(QStringLiteral("title")).toString() == QLatin1String("A"));
    REQUIRE(rows.at(0).toMap().value(QStringLiteral("poster")).toString()
            == QLatin1String("https://p/a.jpg"));
    REQUIRE(rows.at(0).toMap().value(QStringLiteral("year")).toString() == QLatin1String("2021"));
    REQUIRE(!rows.at(0).toMap().contains(QStringLiteral("tmdbId")));
}

TEST_CASE("shelfRows includes backdrop and tmdbId", "[tmdbparse]") {
    const QByteArray json = QByteArrayLiteral(R"({
      "results": [
        { "id": 42, "name": "Show", "poster_path": "/s.jpg", "backdrop_path": "/b.jpg",
          "first_air_date": "2018-11-11", "vote_average": 9.1, "overview": "os" }
      ]
    })");
    const auto rows = TmdbParse::shelfRows(
        json, /*isTv=*/true, QStringLiteral("https://p"), QStringLiteral("https://b"));
    REQUIRE(rows.size() == 1);
    const auto m = rows.at(0).toMap();
    REQUIRE(m.value(QStringLiteral("type")).toString() == QLatin1String("series"));
    REQUIRE(m.value(QStringLiteral("tmdbId")).toInt() == 42);
    REQUIRE(m.value(QStringLiteral("backdrop")).toString() == QLatin1String("https://b/b.jpg"));
}

TEST_CASE("multiSearchRows keeps originalTitle for tracker queries", "[tmdbparse]") {
    const QByteArray json = QByteArrayLiteral(R"({
      "results": [
        { "media_type": "person", "name": "X", "poster_path": "/x.jpg" },
        { "media_type": "movie", "id": 7, "title": "Local", "original_title": "Original",
          "poster_path": "/m.jpg", "release_date": "2020-01-01",
          "vote_average": 6.0, "overview": "om" }
      ]
    })");
    const auto rows = TmdbParse::multiSearchRows(json, QStringLiteral("https://p"));
    REQUIRE(rows.size() == 1);
    const auto m = rows.at(0).toMap();
    REQUIRE(m.value(QStringLiteral("title")).toString() == QLatin1String("Local"));
    REQUIRE(m.value(QStringLiteral("originalTitle")).toString() == QLatin1String("Original"));
    REQUIRE(m.value(QStringLiteral("tmdbId")).toInt() == 7);
}
