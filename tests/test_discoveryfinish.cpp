#include <catch2/catch_test_macros.hpp>
#include "services/discovery/discoveryfinish.h"

#include <QByteArray>
#include <QMap>
#include <QVariantList>
#include <QVariantMap>

namespace {

const QString Poster = QStringLiteral("https://p");
const QString Backdrop = QStringLiteral("https://b");

QByteArray shelfJson(int id, const char *name, const char *poster, const char *backdrop,
                     const char *overview)
{
    return QByteArrayLiteral(R"({
      "results": [
        { "id": )")
           + QByteArray::number(id)
           + QByteArrayLiteral(R"(, "title": ")")
           + QByteArray(name)
           + QByteArrayLiteral(R"(", "poster_path": ")")
           + QByteArray(poster)
           + QByteArrayLiteral(R"(", "backdrop_path": ")")
           + QByteArray(backdrop)
           + QByteArrayLiteral(R"(", "release_date": "2021-01-01",
          "vote_average": 8.0, "overview": ")")
           + QByteArray(overview)
           + QByteArrayLiteral(R"(" }
      ]
    })");
}

QByteArray multiSearchJson()
{
    return QByteArrayLiteral(R"({
      "results": [
        { "media_type": "movie", "id": 7, "title": "Dune", "original_title": "Dune",
          "poster_path": "/d.jpg", "release_date": "2021-01-01",
          "vote_average": 8.0, "overview": "sand" }
      ]
    })");
}

QByteArray igdbSearchJson()
{
    return QByteArrayLiteral(R"([
      { "name": "Dune", "total_rating": 90.0, "first_release_date": 1609459200,
        "summary": "game", "cover": { "image_id": "dune" },
        "screenshots": [ { "image_id": "s1" } ] }
    ])");
}

} // namespace

TEST_CASE("ingestTmdbShelf merges pages and skips error bodies", "[discoveryfinish]") {
    QMap<int, QVariantMap> accum;
    DiscoveryFinish::ingestTmdbShelf(
        accum, 1, QStringLiteral("Popular"), /*isTv=*/false, /*ok=*/true,
        shelfJson(1, "Alpha", "/a.jpg", "/ab.jpg", "oa"), Poster, Backdrop);
    REQUIRE(accum.contains(1));
    REQUIRE(accum.value(1).value(QStringLiteral("items")).toList().size() == 1);

    DiscoveryFinish::ingestTmdbShelf(
        accum, 1, QStringLiteral("Popular"), /*isTv=*/false, /*ok=*/true,
        shelfJson(2, "Beta", "/b.jpg", "/bb.jpg", "ob"), Poster, Backdrop);
    REQUIRE(accum.value(1).value(QStringLiteral("items")).toList().size() == 2);

    DiscoveryFinish::ingestTmdbShelf(
        accum, 2, QStringLiteral("Broken"), /*isTv=*/false, /*ok=*/false,
        shelfJson(3, "Gamma", "/g.jpg", "/gb.jpg", "og"), Poster, Backdrop);
    REQUIRE_FALSE(accum.contains(2));
}

TEST_CASE("tryFinishShelves waits on pending then assembles", "[discoveryfinish]") {
    QMap<int, QVariantMap> accum;
    DiscoveryFinish::ingestTmdbShelf(
        accum, 1, QStringLiteral("Row"), /*isTv=*/false, /*ok=*/true,
        shelfJson(1, "Hero", "/h.jpg", "/hb.jpg", "overview"), Poster, Backdrop);

    int pending = 2;
    QVariantList rows;
    QVariantList hero;
    REQUIRE_FALSE(DiscoveryFinish::tryFinishShelves(pending, accum, &rows, &hero));
    REQUIRE(pending == 1);
    REQUIRE(rows.isEmpty());

    REQUIRE(DiscoveryFinish::tryFinishShelves(pending, accum, &rows, &hero));
    REQUIRE(pending == 0);
    REQUIRE(rows.size() == 1);
    REQUIRE(hero.size() == 1);
    REQUIRE(hero.at(0).toMap().value(QStringLiteral("title")).toString()
            == QLatin1String("Hero"));
}

TEST_CASE("tryFinishShelves empty accum yields empty rows", "[discoveryfinish]") {
    QMap<int, QVariantMap> accum;
    int pending = 1;
    QVariantList rows;
    QVariantList hero;
    REQUIRE(DiscoveryFinish::tryFinishShelves(pending, accum, &rows, &hero));
    REQUIRE(rows.isEmpty());
    REQUIRE(hero.isEmpty());
}

TEST_CASE("search finish ranks TMDB+IGDB and ignores error bodies", "[discoveryfinish]") {
    QVariantList works;
    DiscoveryFinish::ingestTmdbSearch(works, /*ok=*/true, multiSearchJson(), Poster);
    DiscoveryFinish::ingestIgdbSearch(works, /*ok=*/false, igdbSearchJson());
    REQUIRE(works.size() == 1);

    DiscoveryFinish::ingestIgdbSearch(works, /*ok=*/true, igdbSearchJson());
    REQUIRE(works.size() == 2);

    int pending = 2;
    QVariantList ranked;
    REQUIRE_FALSE(DiscoveryFinish::tryFinishSearch(
        pending, QStringLiteral("Dune"), works, &ranked));
    REQUIRE(ranked.isEmpty());

    REQUIRE(DiscoveryFinish::tryFinishSearch(
        pending, QStringLiteral("Dune"), works, &ranked));
    REQUIRE(ranked.size() == 2);
    // Game rates higher (9.0) than movie (8.0) at equal name score.
    REQUIRE(ranked.at(0).toMap().value(QStringLiteral("type")).toString()
            == QLatin1String("game"));
}

TEST_CASE("search finish empty bodies still completes", "[discoveryfinish]") {
    QVariantList works;
    DiscoveryFinish::ingestTmdbSearch(works, /*ok=*/true, QByteArrayLiteral("{}"), Poster);
    DiscoveryFinish::ingestIgdbSearch(works, /*ok=*/true, QByteArrayLiteral("[]"));
    REQUIRE(works.isEmpty());

    int pending = 1;
    QVariantList ranked;
    REQUIRE(DiscoveryFinish::tryFinishSearch(
        pending, QStringLiteral("x"), works, &ranked));
    REQUIRE(ranked.isEmpty());
}
