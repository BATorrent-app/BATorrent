#include <catch2/catch_test_macros.hpp>
#include "services/downloads/rangeplan.h"

#include <cstdint>

using bat::ByteRange;
using bat::planSegments;

// Every plan must: be non-empty for a positive size, cover exactly [0,size-1]
// with contiguous inclusive ranges, and have no empty segment.
static void checkCovers(const std::vector<ByteRange> &segs, std::int64_t total)
{
    REQUIRE_FALSE(segs.empty());
    REQUIRE(segs.front().start == 0);
    REQUIRE(segs.back().end == total - 1);
    std::int64_t sum = 0;
    for (std::size_t i = 0; i < segs.size(); ++i) {
        REQUIRE(segs[i].size() >= 1);                       // no empty segment
        REQUIRE(segs[i].end >= segs[i].start);
        if (i > 0) REQUIRE(segs[i].start == segs[i - 1].end + 1);   // contiguous
        sum += segs[i].size();
    }
    REQUIRE(sum == total);
}

TEST_CASE("rangeplan: evenly divisible split") {
    auto s = planSegments(1000, 4);
    REQUIRE(s.size() == 4);
    for (auto &r : s) REQUIRE(r.size() == 250);
    checkCovers(s, 1000);
}

TEST_CASE("rangeplan: remainder goes to the first segments, not the last") {
    auto s = planSegments(1003, 4);   // 250,251 split -> 251,251,251,250
    REQUIRE(s.size() == 4);
    REQUIRE(s[0].size() == 251);
    REQUIRE(s[1].size() == 251);
    REQUIRE(s[2].size() == 251);
    REQUIRE(s[3].size() == 250);      // last is never the fat one
    checkCovers(s, 1003);
}

TEST_CASE("rangeplan: single connection covers the whole file") {
    auto s = planSegments(9999, 1);
    REQUIRE(s.size() == 1);
    REQUIRE(s[0].start == 0);
    REQUIRE(s[0].end == 9998);
    checkCovers(s, 9999);
}

TEST_CASE("rangeplan: numConns < 1 clamps to a single segment") {
    checkCovers(planSegments(500, 0), 500);
    checkCovers(planSegments(500, -3), 500);
    REQUIRE(planSegments(500, 0).size() == 1);
}

TEST_CASE("rangeplan: more connections than bytes -> one byte per segment, no empties") {
    auto s = planSegments(3, 8);
    REQUIRE(s.size() == 3);
    for (auto &r : s) REQUIRE(r.size() == 1);
    checkCovers(s, 3);
}

TEST_CASE("rangeplan: a one-byte file is a single one-byte segment") {
    auto s = planSegments(1, 4);
    REQUIRE(s.size() == 1);
    REQUIRE(s[0].start == 0);
    REQUIRE(s[0].end == 0);
    REQUIRE(s[0].size() == 1);
}

TEST_CASE("rangeplan: unknown / non-positive size yields an empty plan") {
    REQUIRE(planSegments(0, 4).empty());
    REQUIRE(planSegments(-1, 4).empty());
}

TEST_CASE("rangeplan: 64-bit sizes don't overflow (>4 GiB file)") {
    const std::int64_t big = std::int64_t(6) * 1024 * 1024 * 1024 + 7;   // 6 GiB + 7
    auto s = planSegments(big, 8);
    REQUIRE(s.size() == 8);
    checkCovers(s, big);
    REQUIRE(s.back().end == big - 1);   // no int truncation
}
