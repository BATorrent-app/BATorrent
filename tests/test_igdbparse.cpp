#include <catch2/catch_test_macros.hpp>
#include "services/discovery/igdbparse.h"

#include <QByteArray>
#include <QJsonObject>

TEST_CASE("isFreeLiveService matches substrings without wiping paid franchises", "[igdbparse]") {
    REQUIRE(IgdbParse::isFreeLiveService(QStringLiteral("Fortnite")));
    REQUIRE(IgdbParse::isFreeLiveService(QStringLiteral("Call of Duty: Warzone")));
    REQUIRE_FALSE(IgdbParse::isFreeLiveService(QStringLiteral("Call of Duty: Modern Warfare")));
    REQUIRE_FALSE(IgdbParse::isFreeLiveService(QStringLiteral("Elden Ring")));
}

TEST_CASE("gameCards requires cover, dedupes, filters F2P, caps", "[igdbparse]") {
    const QByteArray json = QByteArrayLiteral(R"([
      { "name": "Elden Ring", "rating": 95.0, "first_release_date": 1647302400,
        "summary": "souls", "cover": { "image_id": "elden" },
        "artworks": [ { "image_id": "elden_art" } ] },
      { "name": "Fortnite", "rating": 70.0, "first_release_date": 1500000000,
        "summary": "br", "cover": { "image_id": "fn" } },
      { "name": "No Cover", "rating": 50.0, "summary": "x" },
      { "name": "Elden Ring", "rating": 95.0, "cover": { "image_id": "dup" } },
      { "name": "Hades", "rating": 88.0, "first_release_date": 1596240000,
        "summary": "roguelike", "cover": { "image_id": "hades" },
        "screenshots": [ { "image_id": "hades_shot" } ] }
    ])");
    const auto cards = IgdbParse::gameCardsFromJson(json, 10);
    REQUIRE(cards.size() == 2);
    REQUIRE(cards.at(0).toMap().value(QStringLiteral("title")).toString() == QLatin1String("Elden Ring"));
    REQUIRE(cards.at(0).toMap().value(QStringLiteral("poster")).toString()
            .contains(QLatin1String("elden.jpg")));
    REQUIRE(cards.at(0).toMap().value(QStringLiteral("backdrop")).toString()
            .contains(QLatin1String("elden_art.jpg")));
    REQUIRE(cards.at(0).toMap().value(QStringLiteral("rating")).toDouble() == 9.5);
    REQUIRE(cards.at(1).toMap().value(QStringLiteral("title")).toString() == QLatin1String("Hades"));
    REQUIRE(cards.at(1).toMap().value(QStringLiteral("backdrop")).toString()
            .contains(QLatin1String("hades_shot.jpg")));
    REQUIRE(IgdbParse::gameCardsFromJson(json, 1).size() == 1);
}

TEST_CASE("titleSearchRows maps cover stills and total_rating", "[igdbparse]") {
    const QByteArray json = QByteArrayLiteral(R"([
      { "name": "Hades", "total_rating": 93.0, "first_release_date": 1596240000,
        "summary": "roguelike", "cover": { "image_id": "hades" },
        "screenshots": [ { "image_id": "s1" }, { "image_id": "s2" } ] },
      { "name": "No Cover", "total_rating": 50.0 }
    ])");
    const auto rows = IgdbParse::titleSearchRows(json);
    REQUIRE(rows.size() == 1);
    const auto m = rows[0].toMap();
    REQUIRE(m.value(QStringLiteral("title")).toString() == QLatin1String("Hades"));
    REQUIRE(m.value(QStringLiteral("type")).toString() == QLatin1String("game"));
    REQUIRE(m.value(QStringLiteral("rating")).toDouble() == 9.3);
    REQUIRE(m.value(QStringLiteral("stills")).toStringList().size() == 2);
    REQUIRE(IgdbParse::titleSearchRows(QByteArray()).isEmpty());
}

TEST_CASE("pickHypeTypeId prefers top sellers then want-to-play", "[igdbparse]") {
    const QByteArray json = QByteArrayLiteral(R"([
      { "id": 1, "name": "Visits" },
      { "id": 9, "name": "Global Top Sellers" },
      { "id": 3, "name": "Want to Play" },
      { "id": 4, "name": "Playing" }
    ])");
    REQUIRE(IgdbParse::pickHypeTypeId(json) == 9);
    REQUIRE(IgdbParse::pickHypeTypeId(QByteArrayLiteral(
        R"([{ "id": 3, "name": "Want to Play" }, { "id": 4, "name": "Playing" }])")) == 3);
    REQUIRE(IgdbParse::pickHypeTypeId(QByteArray()) == 9);
}

TEST_CASE("orderedGameIds dedupes and sortObjectsByIdRank reorders", "[igdbparse]") {
    const QByteArray prim = QByteArrayLiteral(R"([
      { "game_id": 10, "value": 100 },
      { "game_id": 20, "value": 90 },
      { "game_id": 10, "value": 80 }
    ])");
    const auto ids = IgdbParse::orderedGameIds(prim);
    REQUIRE(ids == QList<qint64>({10, 20}));

    QList<QJsonObject> objs;
    QJsonObject a; a.insert(QStringLiteral("id"), 20); a.insert(QStringLiteral("name"), QStringLiteral("B"));
    QJsonObject b; b.insert(QStringLiteral("id"), 10); b.insert(QStringLiteral("name"), QStringLiteral("A"));
    objs << a << b;
    const auto sorted = IgdbParse::sortObjectsByIdRank(objs, ids);
    REQUIRE(sorted.size() == 2);
    REQUIRE(sorted[0].value(QLatin1String("id")).toInt() == 10);
    REQUIRE(sorted[1].value(QLatin1String("id")).toInt() == 20);
}
