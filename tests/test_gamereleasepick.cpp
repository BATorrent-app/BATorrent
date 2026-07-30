#include <catch2/catch_test_macros.hpp>
#include "services/metadata/gamereleasepick.h"

using GameReleasePick::Candidate;
using GameReleasePick::parseVersion;
using GameReleasePick::compareVersions;
using GameReleasePick::compareCandidates;
using GameReleasePick::best;

TEST_CASE("parseVersion reads v-prefixed and bare dotted builds", "[gamerelease]") {
    REQUIRE(parseVersion("Mecha Camilion v3.2.2 [Online-Fix]") == "3.2.2");
    REQUIRE(parseVersion("Mecha Camilion [Online-Fix] 2.4") == "2.4");
    REQUIRE(parseVersion("Game Title v1") == "1");
    REQUIRE(parseVersion("Something 2024 Repack") == "");   // year alone is not a version
    REQUIRE(parseVersion("FitGirl Repack") == "");
}

TEST_CASE("compareVersions treats empty as oldest", "[gamerelease]") {
    REQUIRE(compareVersions("3.2.2", "2.4") > 0);
    REQUIRE(compareVersions("2.4", "3.2.2") < 0);
    REQUIRE(compareVersions("2.10", "2.9") > 0);
    REQUIRE(compareVersions("", "1.0") < 0);
    REQUIRE(compareVersions("1.0", "") > 0);
    REQUIRE(compareVersions("", "") == 0);
}

TEST_CASE("catalog beats seeded indexer when versions equal/missing", "[gamerelease]") {
    Candidate catalog{true, "", "2026-07-20T00:00:00Z", 0, true};
    Candidate bitsearch{false, "", "", 50, true};
    REQUIRE(compareCandidates(catalog, bitsearch) > 0);
    REQUIRE(best({bitsearch, catalog}) == 1);
}

TEST_CASE("newer version wins even from indexer", "[gamerelease]") {
    Candidate catalog{true, "2.4", "2024-01-01T00:00:00Z", 0, true};
    Candidate bitsearch{false, "3.2.2", "", 3, true};
    REQUIRE(compareCandidates(bitsearch, catalog) > 0);
    REQUIRE(best({catalog, bitsearch}) == 1);
}

TEST_CASE("fresher catalog version beats stale BitSearch", "[gamerelease]") {
    // The Mecha Camilion case: Online-Fix catalog at 3.2.2 vs BitSearch at 2.4.
    Candidate catalog{true, "3.2.2", "2026-07-01T00:00:00Z", 0, true};
    Candidate bitsearch{false, "2.4", "", 12, true};
    REQUIRE(best({bitsearch, catalog}) == 1);
    REQUIRE(compareCandidates(catalog, bitsearch) > 0);
}

TEST_CASE("rows without a URI are never chosen", "[gamerelease]") {
    Candidate dead{true, "9.9.9", "2026-01-01T00:00:00Z", 0, false};
    Candidate live{false, "1.0", "", 5, true};
    REQUIRE(best({dead, live}) == 1);
    REQUIRE(best({dead}) == -1);
}

TEST_CASE("empty candidate list returns -1", "[gamerelease]") {
    REQUIRE(best({}) == -1);
}

TEST_CASE("higher seeders break ties when version and catalog equal", "[gamerelease]") {
    Candidate a{false, "1.0", "", 3, true};
    Candidate b{false, "1.0", "", 40, true};
    REQUIRE(compareCandidates(b, a) > 0);
    REQUIRE(best({a, b}) == 1);
}

TEST_CASE("newer uploadDate breaks catalog ties at same version", "[gamerelease]") {
    Candidate older{true, "2.0", "2024-01-01T00:00:00Z", 0, true};
    Candidate newer{true, "2.0", "2026-06-01T00:00:00Z", 0, true};
    REQUIRE(compareCandidates(newer, older) > 0);
    REQUIRE(best({older, newer}) == 1);
}

TEST_CASE("Get & Install pick prefers catalog over sparse BitSearch swarm", "[gamerelease]") {
    // Characterization of the gwResolve → pickBestResult game path.
    Candidate catalog{true, "1.2.0", "2026-07-20T00:00:00Z", 0, true};
    Candidate sparse{false, "1.2.0", "", 1, true};
    Candidate deadHttp{true, "9.0.0", "2026-07-21T00:00:00Z", 0, false};
    REQUIRE(best({sparse, deadHttp, catalog}) == 2);
}
