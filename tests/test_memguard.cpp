#include <catch2/catch_test_macros.hpp>
#include "torrent/memguard.h"

using bat::MemGuardAction;
using bat::memGuardEvaluate;

TEST_CASE("memory guard is disabled when the cap is zero or negative") {
    REQUIRE(memGuardEvaluate(999999, 0) == MemGuardAction::None);
    REQUIRE(memGuardEvaluate(999999, -1) == MemGuardAction::None);
}

TEST_CASE("memory guard never fires on an unknown (-1) reading") {
    REQUIRE(memGuardEvaluate(-1, 8192) == MemGuardAction::None);
}

TEST_CASE("memory guard stays quiet below the cap") {
    REQUIRE(memGuardEvaluate(0, 8192) == MemGuardAction::None);
    REQUIRE(memGuardEvaluate(8191, 8192) == MemGuardAction::None);
}

TEST_CASE("memory guard pauses from the cap up to (but not including) 2x") {
    REQUIRE(memGuardEvaluate(8192, 8192) == MemGuardAction::Pause);
    REQUIRE(memGuardEvaluate(12000, 8192) == MemGuardAction::Pause);
    REQUIRE(memGuardEvaluate(16383, 8192) == MemGuardAction::Pause);
}

TEST_CASE("memory guard panics at or above 2x the cap") {
    REQUIRE(memGuardEvaluate(16384, 8192) == MemGuardAction::Panic);
    REQUIRE(memGuardEvaluate(100000, 8192) == MemGuardAction::Panic);
}
