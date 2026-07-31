#include <catch2/catch_test_macros.hpp>
#include "services/discovery/discoverysearch.h"

#include <QVariantMap>

static QVariantMap work(const QString &title, const QString &type,
                        double rating = 0.0, const QString &year = {})
{
    QVariantMap m;
    m.insert(QStringLiteral("title"), title);
    m.insert(QStringLiteral("type"), type);
    m.insert(QStringLiteral("rating"), rating);
    m.insert(QStringLiteral("year"), year);
    return m;
}

TEST_CASE("rankAndMerge prefers exact name match then prefix", "[discoverysearch]") {
    QVariantList works{
        work(QStringLiteral("The Witcher 3"), QStringLiteral("game"), 9.5),
        work(QStringLiteral("Witcher Tales"), QStringLiteral("movie"), 6.0),
        work(QStringLiteral("The Witcher"), QStringLiteral("series"), 8.5),
        work(QStringLiteral("Unrelated"), QStringLiteral("movie"), 9.0),
    };
    const auto out = DiscoverySearch::rankAndMerge(QStringLiteral("The Witcher"), works);
    REQUIRE(out.size() == 4);
    REQUIRE(out[0].toMap().value(QStringLiteral("title")).toString()
            == QStringLiteral("The Witcher"));
    REQUIRE(out[0].toMap().value(QStringLiteral("type")).toString()
            == QStringLiteral("series"));
}

TEST_CASE("rankAndMerge rating tiebreak when scores match", "[discoverysearch]") {
    QVariantList works{
        work(QStringLiteral("Dune"), QStringLiteral("movie"), 8.0),
        work(QStringLiteral("Dune"), QStringLiteral("game"), 9.2),
    };
    const auto out = DiscoverySearch::rankAndMerge(QStringLiteral("Dune"), works);
    REQUIRE(out.size() == 2);
    REQUIRE(out[0].toMap().value(QStringLiteral("type")).toString()
            == QStringLiteral("game"));
    REQUIRE(out[0].toMap().value(QStringLiteral("rating")).toDouble() == 9.2);
}

TEST_CASE("rankAndMerge dedupes title|year|type", "[discoverysearch]") {
    QVariantList works{
        work(QStringLiteral("Alpha"), QStringLiteral("movie"), 7.0, QStringLiteral("2020")),
        work(QStringLiteral("Alpha"), QStringLiteral("movie"), 8.0, QStringLiteral("2020")),
        work(QStringLiteral("Alpha"), QStringLiteral("series"), 7.0, QStringLiteral("2020")),
    };
    const auto out = DiscoverySearch::rankAndMerge(QStringLiteral("Alpha"), works);
    REQUIRE(out.size() == 2);
}
