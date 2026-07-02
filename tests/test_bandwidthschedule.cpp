#include <catch2/catch_test_macros.hpp>
#include "torrent/bandwidthschedule.h"

using bat::inBandwidthSchedule;

static constexpr int ALL_DAYS = 0x7F;   // Mon..Sun

TEST_CASE("schedule: a normal (non-wrapping) window is [from, to)") {
    // 01:00 -> 07:00, every day
    REQUIRE_FALSE(inBandwidthSchedule(0, 0, ALL_DAYS, 1, 7));   // 00:00 before
    REQUIRE(inBandwidthSchedule(0, 1, ALL_DAYS, 1, 7));         // 01:00 start (inclusive)
    REQUIRE(inBandwidthSchedule(0, 6, ALL_DAYS, 1, 7));         // 06:00 inside
    REQUIRE_FALSE(inBandwidthSchedule(0, 7, ALL_DAYS, 1, 7));   // 07:00 end (exclusive)
    REQUIRE_FALSE(inBandwidthSchedule(0, 12, ALL_DAYS, 1, 7));  // midday out
}

TEST_CASE("schedule: a window that wraps midnight (22 -> 6)") {
    REQUIRE(inBandwidthSchedule(0, 22, ALL_DAYS, 22, 6));       // 22:00 start
    REQUIRE(inBandwidthSchedule(0, 23, ALL_DAYS, 22, 6));       // 23:00
    REQUIRE(inBandwidthSchedule(0, 0, ALL_DAYS, 22, 6));        // 00:00 next day
    REQUIRE(inBandwidthSchedule(0, 5, ALL_DAYS, 22, 6));        // 05:00
    REQUIRE_FALSE(inBandwidthSchedule(0, 6, ALL_DAYS, 22, 6));  // 06:00 end (exclusive)
    REQUIRE_FALSE(inBandwidthSchedule(0, 12, ALL_DAYS, 22, 6)); // midday out
}

TEST_CASE("schedule: the day mask gates the window") {
    const int monOnly = 1 << 0;
    const int sunOnly = 1 << 6;
    REQUIRE(inBandwidthSchedule(0, 3, monOnly, 1, 7));          // Monday enabled
    REQUIRE_FALSE(inBandwidthSchedule(1, 3, monOnly, 1, 7));    // Tuesday not
    REQUIRE(inBandwidthSchedule(6, 3, sunOnly, 1, 7));          // Sunday enabled
    REQUIRE_FALSE(inBandwidthSchedule(0, 3, 0, 1, 7));          // no days ⇒ never
}

TEST_CASE("schedule: out-of-range weekday never matches") {
    REQUIRE_FALSE(inBandwidthSchedule(-1, 3, ALL_DAYS, 1, 7));
    REQUIRE_FALSE(inBandwidthSchedule(7, 3, ALL_DAYS, 1, 7));
}
