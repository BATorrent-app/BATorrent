// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef SERVICES_DOWNLOADS_RANGEPLAN_H
#define SERVICES_DOWNLOADS_RANGEPLAN_H

// Pure decision for the segmented HTTP downloader: split a file of `totalSize`
// bytes into up to `numConns` contiguous byte ranges. Kept network-free so the
// segment math (the part that must be exactly right, or bytes land at the wrong
// offset) is unit-testable in isolation.

#include <algorithm>
#include <cstdint>
#include <vector>

namespace bat {

// Half-open is avoided: HTTP Range is inclusive on both ends ("bytes=start-end"
// covers start..end), so `end` here is the last byte of the segment.
struct ByteRange {
    std::int64_t start = 0;
    std::int64_t end = 0;                 // inclusive
    std::int64_t size() const { return end - start + 1; }
};

// Splits [0, totalSize) into at most `numConns` near-equal inclusive ranges.
//   - totalSize <= 0            → empty (unknown size ⇒ caller uses 1 stream)
//   - numConns  <= 1            → a single range covering the whole file
//   - more conns than bytes     → clamped to one range per byte (no empties)
// Every returned range is non-empty, ranges are contiguous and cover exactly
// [0, totalSize-1], and the remainder is spread over the first segments so the
// last one is never the fat one.
inline std::vector<ByteRange> planSegments(std::int64_t totalSize, int numConns)
{
    std::vector<ByteRange> out;
    if (totalSize <= 0) return out;

    std::int64_t n = numConns < 1 ? 1 : static_cast<std::int64_t>(numConns);
    n = std::min(n, totalSize);           // never more segments than bytes

    const std::int64_t base = totalSize / n;
    const std::int64_t rem  = totalSize % n;   // first `rem` segments get +1

    out.reserve(static_cast<std::size_t>(n));
    std::int64_t pos = 0;
    for (std::int64_t i = 0; i < n; ++i) {
        const std::int64_t len = base + (i < rem ? 1 : 0);
        out.push_back(ByteRange{pos, pos + len - 1});
        pos += len;
    }
    return out;
}

} // namespace bat

#endif
