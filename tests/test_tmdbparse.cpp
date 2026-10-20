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

TEST_CASE("episodeRows carries what a series screen needs", "[tmdbparse]") {
    const QByteArray json = QByteArrayLiteral(R"({
      "episodes": [
        { "episode_number": 1, "name": "Chapter 1", "overview": "A bounty hunter.",
          "still_path": "/abc.jpg", "runtime": 39, "vote_average": 7.9 },
        { "episode_number": 2, "name": "No image", "overview": "", "still_path": null }
      ]
    })");
    const auto eps = TmdbParse::episodeRows(json, QStringLiteral("https://img/w300"));
    REQUIRE(eps.size() == 2);

    const QVariantMap first = eps.at(0).toMap();
    REQUIRE(first.value(QStringLiteral("overview")).toString() == QLatin1String("A bounty hunter."));
    REQUIRE(first.value(QStringLiteral("runtime")).toInt() == 39);
    REQUIRE(first.value(QStringLiteral("rating")).toDouble() == 7.9);
    REQUIRE(first.value(QStringLiteral("still")).toString() == QLatin1String("https://img/w300/abc.jpg"));

    // An episode with no image must come back with nothing, not with a base
    // URL that resolves to a 404 and draws a broken thumbnail.
    REQUIRE(eps.at(1).toMap().value(QStringLiteral("still")).toString().isEmpty());
}

TEST_CASE("recommendationRows skips missing posters and respects limit", "[tmdbparse]") {
    const QByteArray json = QByteArrayLiteral(R"({
      "results": [
        { "id": 354912, "title": "A", "original_title": "Orig A", "poster_path": "/a.jpg", "release_date": "2021-05-01",
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
    // The tile asks for the title's logo by its id, and opening it hunts
    // releases under the original name too: a "Because you watched" row
    // without them showed plain names and found no torrents for a film
    // whose translated title differs (Coco as "Viva: A Vida é Uma Festa").
    REQUIRE(rows.at(0).toMap().value(QStringLiteral("tmdbId")).toInt() == 354912);
    REQUIRE(rows.at(0).toMap().value(QStringLiteral("originalTitle")).toString() == QLatin1String("Orig A"));
}

TEST_CASE("shelfRows includes backdrop and tmdbId", "[tmdbparse]") {
    const QByteArray json = QByteArrayLiteral(R"({
      "results": [
        { "id": 42, "name": "Show", "original_name": "Shou", "poster_path": "/s.jpg", "backdrop_path": "/b.jpg",
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
    // opened from a shelf, the release search needs the original name too
    REQUIRE(m.value(QStringLiteral("originalTitle")).toString() == QLatin1String("Shou"));
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

TEST_CASE("logoUrl picks the lettering a hero can use", "[tmdbparse]") {
    const QByteArray json = QByteArrayLiteral(R"({
      "logos": [
        { "file_path": "/pt.png", "iso_639_1": "pt", "vote_average": 9.9 },
        { "file_path": "/en.png", "iso_639_1": "en", "vote_average": 1.0 },
        { "file_path": "/none.png", "iso_639_1": null, "vote_average": 5.0 }
      ]
    })");
    // English beats a higher-scoring Portuguese one: the app is read in nine
    // languages and the lettering is usually the show's own wordmark anyway.
    REQUIRE(TmdbParse::logoUrl(json, QStringLiteral("https://img")) == "https://img/en.png");
}

TEST_CASE("logoUrl falls back and gives up cleanly", "[tmdbparse]") {
    SECTION("no English one, so the language-less wordmark wins") {
        const QByteArray json = QByteArrayLiteral(R"({
          "logos": [ { "file_path": "/a.png", "iso_639_1": "de", "vote_average": 9.0 },
                     { "file_path": "/b.png", "iso_639_1": "", "vote_average": 1.0 } ]
        })");
        REQUIRE(TmdbParse::logoUrl(json, QStringLiteral("x")) == "x/b.png");
    }
    SECTION("an SVG is skipped: Qt will not draw every one TMDB holds") {
        const QByteArray json = QByteArrayLiteral(R"({
          "logos": [ { "file_path": "/a.svg", "iso_639_1": "en", "vote_average": 9.0 } ]
        })");
        REQUIRE(TmdbParse::logoUrl(json, QStringLiteral("x")).isEmpty());
    }
    SECTION("a title with no lettering at all") {
        REQUIRE(TmdbParse::logoUrl(QByteArrayLiteral(R"({"backdrops":[]})"),
                                   QStringLiteral("x")).isEmpty());
    }
    SECTION("a malformed body") {
        REQUIRE(TmdbParse::logoUrl(QByteArrayLiteral("nope"), QStringLiteral("x")).isEmpty());
    }
}

TEST_CASE("workDetails reads genres, billed cast and season count", "[tmdbparse]") {
    const QByteArray json = QByteArrayLiteral(R"({
      "genres": [ { "id": 80, "name": "Crime" }, { "id": 18, "name": "Drama" }, { "id": 9648, "name": "" } ],
      "number_of_seasons": 4,
      "credits": { "cast": [
        { "name": "Michelle Monaghan", "order": 2 },
        { "name": "Matthew McConaughey", "order": 0 },
        { "name": "", "order": 1 },
        { "name": "Woody Harrelson", "order": 1 },
        { "name": "Extra", "order": 9 }
      ] }
    })");
    const QVariantMap d = TmdbParse::workDetails(json);
    REQUIRE(d.value("genres").toStringList() == QStringList{ "Crime", "Drama" });
    REQUIRE(d.value("cast").toStringList()
            == QStringList{ "Matthew McConaughey", "Woody Harrelson", "Michelle Monaghan" });
    REQUIRE(d.value("seasons").toInt() == 4);
    REQUIRE(d.value("imdbId").toString().isEmpty());
}

TEST_CASE("workDetails reads the IMDb id of a show and of a film", "[tmdbparse]") {
    // a show only has it under external_ids (append_to_response=external_ids)
    const QVariantMap show = TmdbParse::workDetails(QByteArrayLiteral(
        R"({"number_of_seasons":2,"external_ids":{"imdb_id":"tt0434665","tvdb_id":74796}})"));
    REQUIRE(show.value("imdbId").toString() == QLatin1String("tt0434665"));
    const QVariantMap film = TmdbParse::workDetails(QByteArrayLiteral(R"({"imdb_id":"tt0111161"})"));
    REQUIRE(film.value("imdbId").toString() == QLatin1String("tt0111161"));
}

TEST_CASE("workDetails survives a film and garbage", "[tmdbparse]") {
    SECTION("a film has no seasons and may have no credits block") {
        const QVariantMap d = TmdbParse::workDetails(
            QByteArrayLiteral(R"({"genres":[{"name":"Action"}],"runtime":120})"));
        REQUIRE(d.value("genres").toStringList() == QStringList{ "Action" });
        REQUIRE(d.value("cast").toStringList().isEmpty());
        REQUIRE(d.value("seasons").toInt() == 0);
    }
    SECTION("an unparseable body is an empty map, not a crash") {
        REQUIRE(TmdbParse::workDetails(QByteArrayLiteral("<html>")).isEmpty());
        REQUIRE(TmdbParse::workDetails(QByteArray()).isEmpty());
    }
}

TEST_CASE("workDetails reads a season payload's own cast", "[tmdbparse]") {
    // The season endpoint carries credits in the same shape, and no genres.
    const QVariantMap d = TmdbParse::workDetails(QByteArrayLiteral(R"({
      "episodes": [], "credits": { "cast": [ { "name": "Woody Harrelson", "order": 9 },
                                             { "name": "Matthew McConaughey", "order": 0 } ] } })"));
    REQUIRE(d.value("cast").toStringList()
            == QStringList{ "Matthew McConaughey", "Woody Harrelson" });
}

// A film's page lists the rest of its TMDB collection, so the details must say
// which one, and the collection's films come back in release order.
TEST_CASE("workDetails names a film's collection; collectionParts orders its films", "[tmdbparse][collection]") {
    const QByteArray details = R"({"id":671,"title":"Harry Potter and the Philosopher's Stone",
        "belongs_to_collection":{"id":1241,"name":"Harry Potter Collection","poster_path":"/c.jpg"}})";
    const QVariantMap d = TmdbParse::workDetails(details);
    CHECK(d.value("collectionId").toInt() == 1241);
    CHECK(d.value("collectionName").toString() == QStringLiteral("Harry Potter Collection"));
    CHECK(TmdbParse::workDetails(R"({"id":550,"belongs_to_collection":null})").value("collectionId").toInt() == 0);

    const QByteArray col = R"({"id":1241,"name":"Harry Potter Collection","parts":[
        {"id":672,"title":"Chamber of Secrets","release_date":"2002-11-13","poster_path":"/b.jpg","backdrop_path":"/bb.jpg"},
        {"id":999,"title":"Not Out Yet","release_date":"","poster_path":"/z.jpg"},
        {"id":671,"title":"Philosopher's Stone","release_date":"2001-11-16","poster_path":"/a.jpg"},
        {"id":998,"title":"No Poster","release_date":"2003-01-01"}]})";
    const QVariantList parts = TmdbParse::collectionParts(col, "P", "B");
    REQUIRE(parts.size() == 3);   // the one without a poster is no card
    CHECK(parts[0].toMap().value("tmdbId").toInt() == 671);
    CHECK(parts[1].toMap().value("tmdbId").toInt() == 672);
    CHECK(parts[1].toMap().value("backdrop").toString() == QStringLiteral("B/bb.jpg"));
    CHECK(parts[1].toMap().value("type").toString() == QStringLiteral("movie"));
    CHECK(parts[2].toMap().value("tmdbId").toInt() == 999);   // undated last
}

// The release search hunts under the names uploaders use, whatever list the
// title was opened from: the details carry the original name and, from the
// appended translations, the English one ("Harry Potter e a Pedra Filosofal"
// was searched alone and found eight Portuguese releases).
TEST_CASE("workDetails reads the original and the English title", "[tmdbparse]") {
    const QVariantMap film = TmdbParse::workDetails(QByteArrayLiteral(R"({
        "title": "Harry Potter e a Pedra Filosofal",
        "original_title": "Harry Potter and the Philosopher's Stone",
        "original_language": "en",
        "translations": { "translations": [
            { "iso_3166_1": "BR", "iso_639_1": "pt", "data": { "title": "Harry Potter e a Pedra Filosofal" } },
            { "iso_3166_1": "US", "iso_639_1": "en", "data": { "title": "" } } ] } })"));
    CHECK(film.value("originalTitle").toString() == QStringLiteral("Harry Potter and the Philosopher's Stone"));
    // an empty English translation of an English film means its own title
    CHECK(film.value("englishTitle").toString() == QStringLiteral("Harry Potter and the Philosopher's Stone"));

    const QVariantMap show = TmdbParse::workDetails(QByteArrayLiteral(R"({
        "name": "Jujutsu Kaisen", "original_name": "呪術廻戦", "original_language": "ja",
        "translations": { "translations": [
            { "iso_3166_1": "GB", "iso_639_1": "en", "data": { "name": "Sorcery Fight" } },
            { "iso_3166_1": "US", "iso_639_1": "en", "data": { "name": "JUJUTSU KAISEN" } } ] } })"));
    CHECK(show.value("originalTitle").toString() == QStringLiteral("呪術廻戦"));
    CHECK(show.value("englishTitle").toString() == QStringLiteral("JUJUTSU KAISEN"));   // US first

    const QVariantMap bare = TmdbParse::workDetails(QByteArrayLiteral(R"({"title":"Amélie","original_title":"Le Fabuleux Destin d'Amélie Poulain","original_language":"fr"})"));
    CHECK(bare.value("englishTitle").toString().isEmpty());
}
