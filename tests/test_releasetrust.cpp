#include <catch2/catch_test_macros.hpp>
#include "services/metadata/releasetrust.h"

// The value of this heuristic is entirely in its false-positive rate: a warning
// on a normal release trains users to ignore the warning. Half of these tests
// exist to pin the *silent* cases.

using ReleaseTrust::assess;
using ReleaseTrust::Release;
using ReleaseTrust::Tier;

namespace {
constexpr qint64 MiB = 1024LL * 1024LL;
constexpr qint64 GiB = 1024LL * MiB;

Release movie(const QString &name, const QString &quality, qint64 size, int seeders = 50)
{
    Release r;
    r.name = name; r.quality = quality; r.sizeBytes = size; r.seeders = seeders;
    return r;
}
}

TEST_CASE("a normal release is silent") {
    const auto v = assess(movie("Dune Part Two 2024 1080p BluRay x264-GROUP", "1080p", 8 * GiB, 900));
    REQUIRE(v.tier == Tier::Ok);
    REQUIRE(v.reasons.isEmpty());
}

TEST_CASE("password bait is risky") {
    auto v = assess(movie("Some Movie 2024 1080p WEB-DL password", "1080p", 4 * GiB));
    REQUIRE(v.tier == Tier::Risky);
    REQUIRE(v.reasons.contains("trust_password"));

    REQUIRE(assess(movie("Filme 2024 [senha] 1080p", "1080p", 4 * GiB)).tier == Tier::Risky);
    REQUIRE(assess(movie("Movie.2024.passworded.1080p", "1080p", 4 * GiB)).tier == Tier::Risky);
}

TEST_CASE("a film whose title contains the word is not password bait") {
    // "Passwords" / "Password" as part of a title has no separator boundary on
    // both sides in the bait form we look for.
    const auto v = assess(movie("Passwords of the Heart 2019 1080p WEB", "1080p", 3 * GiB));
    REQUIRE(v.reasons.contains("trust_password") == false);
}

TEST_CASE("resolution far above the size is flagged as fake") {
    auto v = assess(movie("Big Movie 2024 2160p UHD", "4K", 700 * MiB));
    REQUIRE(v.tier == Tier::Risky);
    REQUIRE(v.reasons.contains("trust_fake_size"));

    REQUIRE(assess(movie("Movie 2024 1080p", "1080p", 120 * MiB)).reasons.contains("trust_fake_size"));
}

TEST_CASE("small-but-plausible encodes are not flagged") {
    // A 900 MiB x265 1080p movie is a real thing — the floor sits well below it.
    REQUIRE(assess(movie("Movie 2024 1080p x265-QxR", "1080p", 900 * MiB)).tier == Tier::Ok);
    REQUIRE(assess(movie("Movie 2024 720p", "720p", 700 * MiB)).tier == Tier::Ok);
}

TEST_CASE("episodes and season packs are exempt from the size floor") {
    REQUIRE(assess(movie("Show S01E04 1080p WEB-DL", "1080p", 300 * MiB)).tier == Tier::Ok);
    REQUIRE(assess(movie("Show 2x05 1080p", "1080p", 250 * MiB)).tier == Tier::Ok);
    REQUIRE(assess(movie("Show Season 1 COMPLETE 1080p", "1080p", 380 * MiB)).tier == Tier::Ok);
    REQUIRE(assess(movie("Serie Temporada 2 1080p", "1080p", 380 * MiB)).tier == Tier::Ok);
}

TEST_CASE("unknown size skips the size rule") {
    REQUIRE(assess(movie("Movie 2024 2160p", "4K", 0)).tier == Tier::Ok);
}

TEST_CASE("a cam rip is a caution, not a risk") {
    Release r = movie("New Movie 2026 HDCAM", "720p", 2 * GiB);
    r.source = QStringLiteral("CAM");
    const auto v = assess(r);
    REQUIRE(v.tier == Tier::Caution);
    REQUIRE(v.reasons.contains("trust_cam"));
}

TEST_CASE("uploader spam prefixes are a caution") {
    REQUIRE(assess(movie("www.TamilBlasters.com - Movie 2024 1080p", "1080p", 4 * GiB))
                .reasons.contains("trust_spam_name"));
    REQUIRE(assess(movie("[Site.io] Movie 2024 1080p", "1080p", 4 * GiB))
                .reasons.contains("trust_spam_name"));
    // ...but a normal name that merely mentions a domain later does not trip it
    REQUIRE(assess(movie("Movie 2024 1080p WEB-DL site.com", "1080p", 4 * GiB))
                .reasons.contains("trust_spam_name") == false);
}

TEST_CASE("seeders move the score but never produce a reason") {
    const auto dead = assess(movie("Movie 2024 1080p", "1080p", 4 * GiB, 0));
    const auto alive = assess(movie("Movie 2024 1080p", "1080p", 4 * GiB, 200));
    REQUIRE(dead.reasons.isEmpty());
    REQUIRE(dead.tier == Tier::Ok);
    REQUIRE(alive.score > dead.score);
}

TEST_CASE("score stays inside 0..100") {
    Release worst = movie("www.bad.com - Movie 2160p password", "4K", 10 * MiB, 0);
    worst.source = QStringLiteral("CAM");
    const auto v = assess(worst);
    REQUIRE(v.score >= 0);
    REQUIRE(v.score <= 100);
    REQUIRE(v.tier == Tier::Risky);
    REQUIRE(assess(movie("Movie 2024 1080p", "1080p", 8 * GiB, 5000)).score <= 100);
}

TEST_CASE("the worst reason comes first") {
    Release r = movie("www.bad.com - Movie 2024 2160p password", "4K", 100 * MiB);
    r.source = QStringLiteral("CAM");
    const auto v = assess(r);
    REQUIRE(v.reasons.first() == "trust_password");
}
