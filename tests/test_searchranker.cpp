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

// --- multi-title relevance (the same work searched under two names) ---

using SearchRanker::bestRelevance;

static QList<QStringList> shangChi()
{
    // exactly the pair the app now searches with: user's language + original
    return { significantWords("Shang-Chi e a Lenda dos Dez Anéis"),
             significantWords("Shang-Chi and the Legend of the Ten Rings") };
}

TEST_CASE("bestRelevance matches a release named in either title") {
    const auto titles = shangChi();
    REQUIRE(bestRelevance("Shang-Chi e a Lenda dos Dez Aneis 2021 1080p DUBLADO", titles) > 0);
    REQUIRE(bestRelevance("Shang-Chi and the Legend of the Ten Rings 2021 1080p", titles) > 0);
}

TEST_CASE("bestRelevance does not favour the longer title") {
    // The whole point of normalising: a full match must score the same whichever
    // name it was released under, even though the Portuguese title carries more
    // significant words than the English one.
    const auto titles = shangChi();
    const int pt = bestRelevance("Shang Chi e a Lenda dos Dez Aneis", titles);
    const int en = bestRelevance("Shang Chi and the Legend of the Ten Rings", titles);
    REQUIRE(pt == 100);
    REQUIRE(en == 100);
}

TEST_CASE("bestRelevance is a percentage, so partial matches rank below full ones") {
    const auto titles = shangChi();
    const int full    = bestRelevance("Shang Chi and the Legend of the Ten Rings", titles);
    const int partial = bestRelevance("Shang Chi", titles);
    REQUIRE(full == 100);
    REQUIRE(partial > 0);
    REQUIRE(partial < full);
}

TEST_CASE("bestRelevance ignores empty sets and unrelated names") {
    const auto titles = shangChi();
    REQUIRE(bestRelevance("Some Completely Other Movie", titles) == 0);
    REQUIRE(bestRelevance("Shang Chi", QList<QStringList>()) == 0);
    REQUIRE(bestRelevance("Shang Chi", { QStringList() }) == 0);
}

TEST_CASE("bestRelevance with a single title behaves like the old ranking") {
    const QList<QStringList> one = { significantWords("the batman") };
    REQUIRE(bestRelevance("The Batman 2022", one) == 100);
    REQUIRE(bestRelevance("Superman", one) == 0);
}

TEST_CASE("accents fold, so an unaccented release still matches") {
    // Releases are typed without accents far more often than with them.
    const QStringList q = significantWords("Anéis");
    REQUIRE(relevanceScore("Aneis", q) == 1);
    REQUIRE(relevanceScore("Anéis", q) == 1);
    REQUIRE(significantWords("Anéis") == QStringList{"aneis"});
    REQUIRE(significantWords("Ultimato Vingadores") == QStringList({"ultimato", "vingadores"}));
}
