#include <catch2/catch_test_macros.hpp>
#include "services/discovery/discoveryassemble.h"

#include <QVariantMap>

static QVariantMap card(const QString &title, const QString &type,
                        const QString &poster = {},
                        const QString &backdrop = {},
                        const QString &overview = {})
{
    QVariantMap m;
    m.insert(QStringLiteral("title"), title);
    m.insert(QStringLiteral("type"), type);
    m.insert(QStringLiteral("poster"), poster);
    m.insert(QStringLiteral("backdrop"), backdrop);
    m.insert(QStringLiteral("overview"), overview);
    return m;
}

TEST_CASE("mergeShelfByPoster appends unique posters", "[discoveryassemble]") {
    const QVariantList existing{
        card(QStringLiteral("A"), QStringLiteral("movie"), QStringLiteral("p1")),
    };
    const QVariantList incoming{
        card(QStringLiteral("A-dup"), QStringLiteral("movie"), QStringLiteral("p1")),
        card(QStringLiteral("B"), QStringLiteral("movie"), QStringLiteral("p2")),
    };
    const auto merged = DiscoveryAssemble::mergeShelfByPoster(existing, incoming);
    REQUIRE(merged.size() == 2);
    REQUIRE(merged[1].toMap().value(QStringLiteral("title")).toString()
            == QStringLiteral("B"));
}

TEST_CASE("rowsFromAccum dedupes across shelves and tags genre", "[discoveryassemble]") {
    QMap<int, QVariantMap> accum;
    QVariantMap row0;
    row0.insert(QStringLiteral("label"), QStringLiteral("Trending"));
    row0.insert(QStringLiteral("items"), QVariantList{
        card(QStringLiteral("Elden Ring"), QStringLiteral("game"), QStringLiteral("e")),
        card(QStringLiteral("Dune"), QStringLiteral("movie"), QStringLiteral("d")),
    });
    accum.insert(0, row0);

    QVariantMap rowRpg;
    rowRpg.insert(QStringLiteral("label"), QStringLiteral("RPG"));
    rowRpg.insert(QStringLiteral("items"), QVariantList{
        card(QStringLiteral("Elden Ring"), QStringLiteral("game"), QStringLiteral("e2")),
        card(QStringLiteral("Hades"), QStringLiteral("game"), QStringLiteral("h")),
    });
    accum.insert(3, rowRpg);

    const auto rows = DiscoveryAssemble::rowsFromAccum(accum);
    REQUIRE(rows.size() == 2);
    REQUIRE(rows[0].toMap().value(QStringLiteral("items")).toList().size() == 2);
    REQUIRE(rows[1].toMap().value(QStringLiteral("genre")).toString()
            == QStringLiteral("rpg"));
    REQUIRE(rows[1].toMap().value(QStringLiteral("items")).toList().size() == 1);
    REQUIRE(rows[1].toMap().value(QStringLiteral("items")).toList()[0]
                .toMap().value(QStringLiteral("title")).toString()
            == QStringLiteral("Hades"));
}

TEST_CASE("heroFromAccum round-robins rows needing backdrop+overview", "[discoveryassemble]") {
    QMap<int, QVariantMap> accum;
    QVariantMap games;
    games.insert(QStringLiteral("items"), QVariantList{
        card(QStringLiteral("G1"), QStringLiteral("game"), {},
             QStringLiteral("bg1"), QStringLiteral("ov1")),
        card(QStringLiteral("G2"), QStringLiteral("game"), {},
             QStringLiteral("bg2"), QStringLiteral("ov2")),
    });
    accum.insert(0, games);

    QVariantMap movies;
    movies.insert(QStringLiteral("items"), QVariantList{
        card(QStringLiteral("M1"), QStringLiteral("movie"), {},
             QStringLiteral("bm1"), QStringLiteral("om1")),
        card(QStringLiteral("Skip"), QStringLiteral("movie")),  // no backdrop
    });
    accum.insert(10, movies);

    const auto hero = DiscoveryAssemble::heroFromAccum(accum, 6);
    REQUIRE(hero.size() == 3);
    REQUIRE(hero[0].toMap().value(QStringLiteral("title")).toString()
            == QStringLiteral("G1"));
    REQUIRE(hero[1].toMap().value(QStringLiteral("title")).toString()
            == QStringLiteral("M1"));
    REQUIRE(hero[2].toMap().value(QStringLiteral("title")).toString()
            == QStringLiteral("G2"));
}
