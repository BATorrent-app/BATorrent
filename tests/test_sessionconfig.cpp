#include <catch2/catch_test_macros.hpp>
#include "torrent/sessionconfig.h"
#include "torrent/types.h"

TEST_CASE("listenInterfaces: dual-stack by default", "[sessionconfig]") {
    REQUIRE(SessionConfig::listenInterfaces(QStringLiteral("0.0.0.0"), 6881, false)
            == QStringLiteral("0.0.0.0:6881,[::]:6881"));
}

TEST_CASE("listenInterfaces: forceIpv4 or bound IP is single", "[sessionconfig]") {
    REQUIRE(SessionConfig::listenInterfaces(QStringLiteral("0.0.0.0"), 6881, true)
            == QStringLiteral("0.0.0.0:6881"));
    REQUIRE(SessionConfig::listenInterfaces(QStringLiteral("192.168.1.2"), 6881, false)
            == QStringLiteral("192.168.1.2:6881"));
}

TEST_CASE("expandOnCompleteCommand quotes placeholders", "[sessionconfig]") {
    const QString cmd = SessionConfig::expandOnCompleteCommand(
        QStringLiteral("notify %N %D %H %Z %F"),
        QStringLiteral("It's a trap"),
        QStringLiteral("/tmp/dl"),
        QStringLiteral("abc"),
        42);
    REQUIRE(cmd == QStringLiteral(
        "notify 'It'\\''s a trap' '/tmp/dl' 'abc' '42' '/tmp/dl/It'\\''s a trap'"));
}

TEST_CASE("parseExtractPasswords splits on ; and newlines", "[sessionconfig]") {
    const auto pw = SessionConfig::parseExtractPasswords(
        QStringLiteral(" one ;two\n three \n;"));
    REQUIRE(pw == QStringList({QStringLiteral("one"), QStringLiteral("two"),
                               QStringLiteral("three")}));
}

TEST_CASE("autoCompleteSecondsFromIndex maps combo days", "[sessionconfig]") {
    REQUIRE(SessionConfig::autoCompleteSecondsFromIndex(0) == 0);
    REQUIRE(SessionConfig::autoCompleteSecondsFromIndex(1) == 86400);
    REQUIRE(SessionConfig::autoCompleteSecondsFromIndex(5) == 30 * 86400);
    REQUIRE(SessionConfig::autoCompleteSecondsFromIndex(99) == 0);
    REQUIRE(SessionConfig::autoCompleteSecondsFromIndex(-1) == 0);
}

TEST_CASE("portStatusCode", "[sessionconfig]") {
    REQUIRE(SessionConfig::portStatusCode(false, false) == 3);
    REQUIRE(SessionConfig::portStatusCode(true, true) == 1);
    REQUIRE(SessionConfig::portStatusCode(true, false) == 2);
}

TEST_CASE("planContentLayout strips common root", "[sessionconfig]") {
    const auto plan = SessionConfig::planContentLayout(
        2, "Game", {"Game/a.exe", "Game/b.dll"});
    REQUIRE(plan.size() == 2);
    REQUIRE(plan.at(0) == "a.exe");
    REQUIRE(plan.at(1) == "b.dll");
}

TEST_CASE("planContentLayout creates subfolder for single file", "[sessionconfig]") {
    const auto plan = SessionConfig::planContentLayout(1, "Movie", {"movie.mkv"});
    REQUIRE(plan.size() == 1);
    REQUIRE(plan.at(0) == "Movie/movie.mkv");
}

TEST_CASE("planContentLayout no-op for original or mismatched roots", "[sessionconfig]") {
    REQUIRE(SessionConfig::planContentLayout(0, "X", {"a/b"}).empty());
    REQUIRE(SessionConfig::planContentLayout(2, "X", {"A/a", "B/b"}).empty());
}

TEST_CASE("patchAdvancedKey maps known adv keys", "[sessionconfig]") {
    AdvancedSettings a;
    REQUIRE(SessionConfig::patchAdvancedKey(a, QStringLiteral("advAioThreads"), 12));
    REQUIRE(a.aioThreads == 12);
    REQUIRE(SessionConfig::patchAdvancedKey(a, QStringLiteral("advChokingAlgo"), 1));
    REQUIRE(a.chokingAlgorithm == 2);
    REQUIRE(SessionConfig::patchAdvancedKey(a, QStringLiteral("advIgnoreLan"), false));
    REQUIRE_FALSE(a.ignoreLimitsOnLAN);
    REQUIRE_FALSE(SessionConfig::patchAdvancedKey(a, QStringLiteral("advNope"), 1));
}

TEST_CASE("excludedFileIndexes matches case-insensitive regexes", "[sessionconfig]") {
    const QStringList paths{
        QStringLiteral("Movie/feature.mkv"),
        QStringLiteral("Movie/sample.mkv"),
        QStringLiteral("Movie/Subs/en.srt"),
    };
    const auto hit = SessionConfig::excludedFileIndexes(
        {QStringLiteral("sample"), QStringLiteral("\\.srt$"), QStringLiteral("(")},
        paths);
    REQUIRE(hit == std::vector<int>({1, 2}));
    REQUIRE(SessionConfig::excludedFileIndexes({}, paths).empty());
}
