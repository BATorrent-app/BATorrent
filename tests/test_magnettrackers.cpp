#include <catch2/catch_test_macros.hpp>
#include "torrent/magnettrackers.h"

using bat::appendPublicTrackers;
using bat::publicTrackers;

TEST_CASE("magnet trackers: bare magnet gets the full public list, tiers 0..n-1") {
    std::vector<std::string> trackers;
    std::vector<int> tiers;
    appendPublicTrackers(trackers, tiers);
    REQUIRE(trackers == publicTrackers());
    REQUIRE(tiers.size() == trackers.size());
    for (size_t i = 0; i < tiers.size(); ++i)
        REQUIRE(tiers[i] == int(i));
}

TEST_CASE("magnet trackers: URI trackers keep priority — appended ones continue the tier sequence") {
    std::vector<std::string> trackers = {"udp://a/announce", "udp://b/announce"};
    std::vector<int> tiers = {0, 1};
    appendPublicTrackers(trackers, tiers);
    REQUIRE(trackers.size() == 2 + publicTrackers().size());
    REQUIRE(trackers[0] == "udp://a/announce");
    // every appended tracker sits on a tier after the magnet's own
    for (size_t i = 2; i < tiers.size(); ++i)
        REQUIRE(tiers[i] >= 2);
}

TEST_CASE("magnet trackers: a public tracker already in the URI is not duplicated") {
    std::vector<std::string> trackers = {publicTrackers().front()};
    std::vector<int> tiers = {0};
    appendPublicTrackers(trackers, tiers);
    REQUIRE(trackers.size() == publicTrackers().size());
    REQUIRE(std::count(trackers.begin(), trackers.end(), publicTrackers().front()) == 1);
}

TEST_CASE("magnet trackers: idempotent — a second pass adds nothing") {
    std::vector<std::string> trackers;
    std::vector<int> tiers;
    appendPublicTrackers(trackers, tiers);
    const auto once = trackers;
    appendPublicTrackers(trackers, tiers);
    REQUIRE(trackers == once);
    REQUIRE(tiers.size() == trackers.size());
}

TEST_CASE("magnet trackers: a tiers vector out of sync with trackers is repaired first") {
    std::vector<std::string> trackers = {"udp://a/announce", "udp://b/announce"};
    std::vector<int> tiers;   // parse gave none — lt tolerates it, we normalize
    appendPublicTrackers(trackers, tiers);
    REQUIRE(tiers.size() == trackers.size());
    REQUIRE(tiers[0] == 0);
    REQUIRE(tiers[1] == 0);
}
