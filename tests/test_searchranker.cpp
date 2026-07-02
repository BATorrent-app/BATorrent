#include <catch2/catch_test_macros.hpp>
#include "services/metadata/searchranker.h"

// Characterization tests: these pin the behavior of the search relevance logic
// that used to live in SearchView.qml (sigWords / relScore), so the C++ port is
// provably equivalent. Do not "fix" these to a nicer behavior without intent —
// they encode what the UI already shipped.

using SearchRanker::significantWords;
using SearchRanker::relevanceScore;

TEST_CASE("significantWords lowercases, splits, drops empties") {
    REQUIRE(significantWords("Star Wars") == QStringList({"star", "wars"}));
    REQUIRE(significantWords("Spider-Man") == QStringList({"spider", "man"}));
    REQUIRE(significantWords("Dune 2") == QStringList({"dune", "2"}));
    REQUIRE(significantWords("  Blade   Runner  ") == QStringList({"blade", "runner"}));
}

TEST_CASE("significantWords drops stopwords the/of/a/an/and/or/to/in/on") {
    REQUIRE(significantWords("The Batman") == QStringList({"batman"}));
    REQUIRE(significantWords("Lord of the Rings") == QStringList({"lord", "rings"}));
    REQUIRE(significantWords("A Quiet Place") == QStringList({"quiet", "place"}));
    REQUIRE(significantWords("Crouching Tiger and Hidden Dragon")
            == QStringList({"crouching", "tiger", "hidden", "dragon"}));
}

TEST_CASE("significantWords on empty / all-stopword input is empty") {
    REQUIRE(significantWords("").isEmpty());
    REQUIRE(significantWords("   ").isEmpty());
    REQUIRE(significantWords("the of a an").isEmpty());
}

TEST_CASE("relevanceScore counts whole-word matches only") {
    const QStringList q = significantWords("the batman");
    REQUIRE(relevanceScore("The Batman", q) == 1);
    REQUIRE(relevanceScore("Batman Begins", q) == 1);
    REQUIRE(relevanceScore("Superman", q) == 0);
}

TEST_CASE("relevanceScore does not match substrings: blast != last") {
    const QStringList q = significantWords("blast");
    REQUIRE(relevanceScore("The Last of Us", q) == 0);
    REQUIRE(relevanceScore("Blast Corps", q) == 1);
}

TEST_CASE("relevanceScore is case-insensitive and sums distinct query hits") {
    const QStringList q = significantWords("lord of the rings");   // -> {lord, rings}
    REQUIRE(relevanceScore("LORD OF THE RINGS", q) == 2);
    REQUIRE(relevanceScore("The Rings of Power", q) == 1);
}

TEST_CASE("relevanceScore with no significant query words is zero") {
    REQUIRE(relevanceScore("Anything At All", QStringList()) == 0);
    REQUIRE(relevanceScore("The Of A", significantWords("the of a")) == 0);
}
