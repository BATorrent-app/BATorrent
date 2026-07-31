#include <catch2/catch_test_macros.hpp>
#include "services/discovery/hublogic.h"

TEST_CASE("genreKey maps EN/PT labels", "[hublogic]") {
    REQUIRE(HubLogic::genreKey(QStringLiteral("Action RPG")) == QStringLiteral("rpg"));
    REQUIRE(HubLogic::genreKey(QStringLiteral("First-person Shooter")) == QStringLiteral("shooter"));
    REQUIRE(HubLogic::genreKey(QStringLiteral("Tiro em primeira pessoa")) == QStringLiteral("shooter"));
    REQUIRE(HubLogic::genreKey(QStringLiteral("Estratégia")) == QStringLiteral("strategy"));
    REQUIRE(HubLogic::genreKey(QStringLiteral("Ficção científica")) == QStringLiteral("scifi"));
    REQUIRE(HubLogic::genreKey(QStringLiteral("Terror")) == QStringLiteral("horror"));
    REQUIRE(HubLogic::genreKey(QStringLiteral("Ação")) == QStringLiteral("action"));
    REQUIRE(HubLogic::genreKey(QStringLiteral("Romance")).isEmpty());
}

TEST_CASE("topGenre picks the mode", "[hublogic]") {
    REQUIRE(HubLogic::topGenre({
        QStringLiteral("Action"), QStringLiteral("Horror"), QStringLiteral("Ação"),
        QStringLiteral("Indie")
    }) == QStringLiteral("action"));
    REQUIRE(HubLogic::topGenre({}).isEmpty());
}

TEST_CASE("excludeOwnedTitles skips owned and caps", "[hublogic]") {
    QVariantList cand;
    for (const char *t : {"Alpha", "Beta", "Gamma", "Delta"}) {
        QVariantMap m; m.insert(QStringLiteral("title"), QString::fromUtf8(t));
        cand << m;
    }
    const QSet<QString> owned{QStringLiteral("beta")};
    const auto out = HubLogic::excludeOwnedTitles(cand, owned, 2);
    REQUIRE(out.size() == 2);
    REQUIRE(out[0].toMap().value(QStringLiteral("title")).toString() == QStringLiteral("Alpha"));
    REQUIRE(out[1].toMap().value(QStringLiteral("title")).toString() == QStringLiteral("Gamma"));
}

TEST_CASE("applyView filters and sorts by name", "[hublogic]") {
    QVariantList list;
    for (const char *t : {"Zodiac", "Alpha", "alpine"}) {
        QVariantMap m; m.insert(QStringLiteral("title"), QString::fromUtf8(t));
        list << m;
    }
    const auto filtered = HubLogic::applyView(list, QStringLiteral("alp"), QStringLiteral("recent"));
    REQUIRE(filtered.size() == 2);
    const auto named = HubLogic::applyView(list, QString(), QStringLiteral("name"));
    REQUIRE(named.size() == 3);
    REQUIRE(named[0].toMap().value(QStringLiteral("title")).toString() == QStringLiteral("Alpha"));
}
