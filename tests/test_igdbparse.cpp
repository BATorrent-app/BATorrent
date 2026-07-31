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
