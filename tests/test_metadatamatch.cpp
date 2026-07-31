#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "services/metadata/metadatamatch.h"

#include <QJsonArray>
#include <QJsonObject>

using Catch::Approx;

TEST_CASE("foldTitle strips diacritics and apostrophes", "[metadatamatch]") {
    REQUIRE(MetadataMatch::foldTitle(QStringLiteral("Ragnarök"))
            == QStringLiteral("ragnarok"));
    REQUIRE(MetadataMatch::foldTitle(QStringLiteral("Baldur's Gate"))
            == QStringLiteral("baldurs gate"));
    REQUIRE(MetadataMatch::foldTitle(QString::fromUtf8("Baldur\xE2\x80\x99s"))
            == QStringLiteral("baldurs")); // U+2019
}

TEST_CASE("titleSimilarity: roman numerals and stopwords", "[metadatamatch]") {
    // GTA V ↔ GTA 5 via roman canonicalization
    REQUIRE(MetadataMatch::titleSimilarity(QStringLiteral("GTA V"),
                                           QStringLiteral("GTA 5"))
            == Approx(1.0));
    // stopwords dropped: "the" / "of" / "and" / "a" / "edition"
    REQUIRE(MetadataMatch::titleSimilarity(QStringLiteral("The Lord of the Rings"),
                                           QStringLiteral("Lord Rings"))
            == Approx(1.0));
}

TEST_CASE("titleSimilarity: God of War Ragnarok vs wrong franchise",
          "[metadatamatch]") {
    const double good = MetadataMatch::titleSimilarity(
        QStringLiteral("God of War Ragnarök"),
        QStringLiteral("God of War Ragnarok"));
    const double bad = MetadataMatch::titleSimilarity(
        QStringLiteral("God of War Ragnarök"),
        QStringLiteral("Ragnarok War of Chaos"));
    // Relative pick (not absolute reject): wrong franchise can still clear the
    // 0.34 bar via shared tokens ("war"/"ragnarok"); best score still wins.
    REQUIRE(good > bad);
    REQUIRE(good >= MetadataMatch::kMinConfidence);
    REQUIRE(bad == Approx(0.5));
}

TEST_CASE("confidentTitle: exact fold or Jaccard bar", "[metadatamatch]") {
    REQUIRE(MetadataMatch::confidentTitle(QStringLiteral("Debian"),
                                          QStringLiteral("debian")));
    // Fuzzy film name for a distro query must fail (Unknown-torrent gate).
    REQUIRE_FALSE(MetadataMatch::confidentTitle(
        QStringLiteral("debian iso amd64"),
        QStringLiteral("Debian")));
    REQUIRE(MetadataMatch::confidentTitle(
        QStringLiteral("Inception"),
        QStringLiteral("Inception")));
}

TEST_CASE("escapeApicalypse doubles backslash and quote", "[metadatamatch]") {
    REQUIRE(MetadataMatch::escapeApicalypse(QStringLiteral(R"(foo"bar\baz)"))
            == QStringLiteral(R"(foo\"bar\\baz)"));
    REQUIRE(MetadataMatch::escapeApicalypse(QStringLiteral("plain"))
            == QStringLiteral("plain"));
}

TEST_CASE("shortenedSearchTitle: half tokens down to 3", "[metadatamatch]") {
    // 7 tokens → keep max(3, 4) = 4
    REQUIRE(MetadataMatch::shortenedSearchTitle(
                QStringLiteral("Garfield Kart 2 All You Can Drift"))
            == QStringLiteral("Garfield Kart 2 All"));
    // 5 tokens → keep max(3, 3) = 3
    REQUIRE(MetadataMatch::shortenedSearchTitle(
                QStringLiteral("a b c d e"))
            == QStringLiteral("a b c"));
    // 4 tokens → keep max(3, 2) = 3
    REQUIRE(MetadataMatch::shortenedSearchTitle(QStringLiteral("a b c d"))
            == QStringLiteral("a b c"));
    REQUIRE(MetadataMatch::shortenedSearchTitle(QStringLiteral("a b c")).isEmpty());
    REQUIRE(MetadataMatch::shortenedSearchTitle(QStringLiteral("a b")).isEmpty());
}

TEST_CASE("pickBestIgdbResult: highest overlap wins; year bonus",
          "[metadatamatch]") {
    QJsonArray results;
    {
        QJsonObject wrong;
        wrong.insert(QLatin1String("name"), QStringLiteral("Ragnarok War of Chaos"));
        wrong.insert(QLatin1String("first_release_date"), 1000000000);
        results.append(wrong);
    }
    {
        QJsonObject right;
        right.insert(QLatin1String("name"), QStringLiteral("God of War Ragnarok"));
        // 2022-11-09 ≈ 1667952000
        right.insert(QLatin1String("first_release_date"), 1667952000);
        results.append(right);
    }

    const auto pick = MetadataMatch::pickBestIgdbResult(
        results, QStringLiteral("God of War Ragnarök"), 2022);
    REQUIRE(pick.found);
    REQUIRE(pick.item.value(QLatin1String("name")).toString()
            == QStringLiteral("God of War Ragnarok"));
    REQUIRE(pick.bestScore >= MetadataMatch::kMinConfidence);
}

TEST_CASE("pickBestIgdbResult: below bar and no fold match → not found",
          "[metadatamatch]") {
    QJsonArray results;
    QJsonObject weak;
    weak.insert(QLatin1String("name"), QStringLiteral("Totally Unrelated"));
    results.append(weak);

    const auto pick = MetadataMatch::pickBestIgdbResult(
        results, QStringLiteral("Garfield Kart 2 All You Can Drift"), 0);
    REQUIRE_FALSE(pick.found);
}

TEST_CASE("pickBestIgdbResult: empty results → not found", "[metadatamatch]") {
    const auto pick = MetadataMatch::pickBestIgdbResult(
        QJsonArray{}, QStringLiteral("Anything"), 0);
    REQUIRE_FALSE(pick.found);
    REQUIRE(pick.bestScore == Approx(0.0));
}

TEST_CASE("applyFileTypeOverride: files beat name except Series",
          "[metadatamatch]") {
    REQUIRE(MetadataMatch::applyFileTypeOverride(
                ContentType::Movie, QStringList{})
            == ContentType::Movie);
    // Keep series from name even if files look like a single movie.
    REQUIRE(MetadataMatch::applyFileTypeOverride(
                ContentType::Series,
                {QStringLiteral("Show.S01E01.mkv")})
            == ContentType::Series);
    // Game payload outranks a movie-ish name.
    REQUIRE(MetadataMatch::applyFileTypeOverride(
                ContentType::Movie,
                {QStringLiteral("game.exe"), QStringLiteral("data.pak")})
            == ContentType::Game);
}

TEST_CASE("contentType string round-trip", "[metadatamatch]") {
    REQUIRE(MetadataMatch::contentTypeToString(ContentType::Movie)
            == QLatin1String("movie"));
    REQUIRE(MetadataMatch::contentTypeToString(ContentType::Series)
            == QLatin1String("series"));
    REQUIRE(MetadataMatch::contentTypeToString(ContentType::Game)
            == QLatin1String("game"));
    REQUIRE(MetadataMatch::contentTypeToString(ContentType::Unknown)
            == QLatin1String("unknown"));
    REQUIRE(MetadataMatch::contentTypeFromString(QStringLiteral("movie"))
            == ContentType::Movie);
    REQUIRE(MetadataMatch::contentTypeFromString(QStringLiteral("series"))
            == ContentType::Series);
    REQUIRE(MetadataMatch::contentTypeFromString(QStringLiteral("game"))
            == ContentType::Game);
    REQUIRE(MetadataMatch::contentTypeFromString(QStringLiteral("nope"))
            == ContentType::Unknown);
}

TEST_CASE("genreNamesFromIds maps known TMDB ids", "[metadatamatch]") {
    QJsonArray ids;
    ids.append(28);
    ids.append(878);
    ids.append(99999); // unknown → skipped
    REQUIRE(MetadataMatch::genreNamesFromIds(ids)
            == QStringList({QStringLiteral("Action"), QStringLiteral("Sci-Fi")}));
}
