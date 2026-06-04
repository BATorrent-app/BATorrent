// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details
//
// QR Code Model 2 generator. Implements enough of ISO/IEC 18004 to produce
// scannable codes for short URLs:
//   • Byte mode encoding (any UTF-8 string)
//   • Error-correction level M (15% recoverable)
//   • Versions 1-10 (auto-selected by length)
//   • Full Reed-Solomon over GF(256), primitive polynomial 0x11D
//   • All 8 data-masking patterns with spec-compliant scoring (§8.3.3)
//   • Format-information BCH(15,5) with mask 0x5412
//
// References:
//   ISO/IEC 18004:2015 — QR Code Model 2
//   Thonky.com QR tutorial — for capacity tables + alignment patterns
//   Nayuki QR-Code-generator — reference implementation, MIT-licensed
//
// Limitations vs full spec:
//   • No alphanumeric / kanji / ECI modes — byte mode handles everything,
//     just less efficiently for digit-only data.
//   • Versions 11-40 not supported — our use case is LAN URLs (≤ 50 chars)
//     which fit comfortably in v3-v4. v10 ceiling (~213 bytes at ECC M) is
//     2× our worst-case anyway.

#include "qrcodegen.h"

#include <QPainter>
#include <algorithm>
#include <array>

namespace qrgen {
namespace {

// ──────────────────────────────────────────────────────────────────────────
// Capacity tables (data codewords / ECC codewords / blocks) at ECC level M.
// Indexed by version-1.
// Source: ISO/IEC 18004:2015 Table 9.
// Format: {total codewords, ECC per block, num group-1 blocks, group-1 data
// codewords per block, num group-2 blocks, group-2 data codewords per block}.
struct VersionInfo {
    int totalCodewords;
    int eccPerBlock;
    int g1Blocks;
    int g1DataCw;
    int g2Blocks;
    int g2DataCw;
};

// Versions 1-10 at ECC M.
const VersionInfo kVersions[] = {
    { 26, 10, 1, 16,  0,  0}, // v1
    { 44, 16, 1, 28,  0,  0}, // v2
    { 70, 26, 1, 44,  0,  0}, // v3
    {100, 18, 2, 32,  0,  0}, // v4
    {134, 24, 2, 43,  0,  0}, // v5
    {172, 16, 4, 27,  0,  0}, // v6
    {196, 18, 4, 31,  0,  0}, // v7
    {242, 22, 2, 38,  2, 39}, // v8
    {292, 22, 3, 36,  2, 37}, // v9
    {346, 26, 4, 43,  1, 44}, // v10
};

constexpr int kMaxVersion = 10;

// Side length in modules for a given version.
int sideForVersion(int version) { return 17 + version * 4; }

// Total data codewords (excluding ECC) for a version at ECC M.
int dataCodewordsFor(int version)
{
    const auto &v = kVersions[version - 1];
    return v.g1Blocks * v.g1DataCw + v.g2Blocks * v.g2DataCw;
}

// Pick the smallest version that fits a byte-mode payload of `bytes`.
// Byte-mode overhead: 4-bit mode indicator + 8 or 16-bit count + N*8 data
// bits + ≤4 terminator bits. Count is 8 bits for v1-9 and 16 bits for v10+.
// Returns -1 if too long.
int pickVersion(int payloadBytes)
{
    for (int v = 1; v <= kMaxVersion; ++v) {
        const int countBits = (v >= 10) ? 16 : 8;
        const int neededBits = 4 + countBits + payloadBytes * 8;
        const int capacityBits = dataCodewordsFor(v) * 8;
        if (neededBits + 4 <= capacityBits) return v; // +4 for terminator headroom
    }
    return -1;
}

// ──────────────────────────────────────────────────────────────────────────
// Alignment pattern centre coordinates per version.
// v1 has none; v2 has [6,18]; etc. Source: ISO/IEC 18004 Annex E.
const std::vector<int> kAlignmentCentres[] = {
    {},                  // v1
    {6, 18},             // v2
    {6, 22},             // v3
    {6, 26},             // v4
    {6, 30},             // v5
    {6, 34},             // v6
    {6, 22, 38},         // v7
    {6, 24, 42},         // v8
    {6, 26, 46},         // v9
    {6, 28, 50},         // v10
};

// ──────────────────────────────────────────────────────────────────────────
// Galois Field 256 arithmetic, prim. poly = 0x11D (x^8 + x^4 + x^3 + x^2 + 1).
struct GF256 {
    std::array<uint8_t, 256> exp; // exp[i] = α^i
    std::array<uint8_t, 256> log; // log[α^i] = i

    GF256()
    {
        uint8_t x = 1;
        for (int i = 0; i < 255; ++i) {
            exp[i] = x;
            log[x] = static_cast<uint8_t>(i);
            x = static_cast<uint8_t>(x << 1);
            if (x == 0 || (static_cast<int>(exp[i]) << 1) >= 256) {
                // Wrap with primitive polynomial
                int xi = (static_cast<int>(exp[i]) << 1) ^ 0x11D;
                x = static_cast<uint8_t>(xi & 0xFF);
            }
        }
        exp[255] = exp[0]; // simplify mul-by-zero edge
    }

    uint8_t mul(uint8_t a, uint8_t b) const
    {
        if (a == 0 || b == 0) return 0;
        return exp[(log[a] + log[b]) % 255];
    }
};

const GF256 &gf()
{
    static GF256 g;
    return g;
}

// Generator polynomial for Reed-Solomon of degree n.
// (x − α^0)(x − α^1)...(x − α^(n-1))
std::vector<uint8_t> rsGenerator(int degree)
{
    std::vector<uint8_t> poly{1};
    const GF256 &g = gf();
    for (int i = 0; i < degree; ++i) {
        // Multiply poly by (x − α^i) → poly shifted + poly*α^i
        std::vector<uint8_t> next(poly.size() + 1, 0);
        for (size_t j = 0; j < poly.size(); ++j) {
            next[j] = static_cast<uint8_t>(next[j] ^ poly[j]);
            next[j + 1] = static_cast<uint8_t>(next[j + 1] ^ g.mul(poly[j], g.exp[i]));
        }
        poly = std::move(next);
    }
    return poly;
}

// Compute ECC bytes for `data` using a generator of `eccLen` degree.
std::vector<uint8_t> rsEncode(const std::vector<uint8_t> &data, int eccLen)
{
    const GF256 &g = gf();
    const auto gen = rsGenerator(eccLen);
    std::vector<uint8_t> result(eccLen, 0);
    for (uint8_t b : data) {
        const uint8_t factor = static_cast<uint8_t>(b ^ result.front());
        result.erase(result.begin());
        result.push_back(0);
        if (factor != 0) {
            for (int i = 0; i < eccLen; ++i)
                result[i] = static_cast<uint8_t>(result[i] ^ g.mul(gen[i + 1], factor));
        }
    }
    return result;
}

// ──────────────────────────────────────────────────────────────────────────
// Bit buffer: append-only stream of bits, byte-aligned read at the end.
struct BitBuffer {
    std::vector<bool> bits;

    void append(uint32_t value, int numBits)
    {
        for (int i = numBits - 1; i >= 0; --i)
            bits.push_back(((value >> i) & 1) != 0);
    }

    std::vector<uint8_t> toBytes() const
    {
        std::vector<uint8_t> out((bits.size() + 7) / 8, 0);
        for (size_t i = 0; i < bits.size(); ++i)
            if (bits[i])
                out[i >> 3] = static_cast<uint8_t>(out[i >> 3] | (1u << (7 - (i & 7))));
        return out;
    }
};

// ──────────────────────────────────────────────────────────────────────────
// Pack byte-mode data, add terminator, pad to capacity. Returns the full
// data codeword stream ready for ECC.
std::vector<uint8_t> packPayload(const QByteArray &data, int version)
{
    BitBuffer bb;
    const int countBits = (version >= 10) ? 16 : 8;
    bb.append(0b0100, 4);                     // byte mode indicator
    bb.append(static_cast<uint32_t>(data.size()), countBits);
    for (uint8_t b : data)
        bb.append(b, 8);

    const int capBits = dataCodewordsFor(version) * 8;
    // Terminator: up to 4 zero bits, but not past capacity.
    const int term = std::min(4, capBits - static_cast<int>(bb.bits.size()));
    bb.append(0, term);
    // Pad to byte boundary with zeros.
    while (bb.bits.size() % 8 != 0) bb.bits.push_back(false);

    auto bytes = bb.toBytes();
    // Pad bytes alternating 0xEC, 0x11 until capacity.
    const int padTarget = capBits / 8;
    bool ec = true;
    while (static_cast<int>(bytes.size()) < padTarget) {
        bytes.push_back(ec ? 0xEC : 0x11);
        ec = !ec;
    }
    return bytes;
}

// Interleave data codewords + ECC per spec §8.6 to get the final bitstream.
std::vector<uint8_t> interleaveBlocks(const std::vector<uint8_t> &data, int version)
{
    const auto &v = kVersions[version - 1];
    const int totalBlocks = v.g1Blocks + v.g2Blocks;

    // Slice payload into blocks.
    std::vector<std::vector<uint8_t>> dataBlocks;
    std::vector<std::vector<uint8_t>> eccBlocks;
    int idx = 0;
    for (int b = 0; b < v.g1Blocks; ++b) {
        std::vector<uint8_t> d(data.begin() + idx, data.begin() + idx + v.g1DataCw);
        idx += v.g1DataCw;
        eccBlocks.push_back(rsEncode(d, v.eccPerBlock));
        dataBlocks.push_back(std::move(d));
    }
    for (int b = 0; b < v.g2Blocks; ++b) {
        std::vector<uint8_t> d(data.begin() + idx, data.begin() + idx + v.g2DataCw);
        idx += v.g2DataCw;
        eccBlocks.push_back(rsEncode(d, v.eccPerBlock));
        dataBlocks.push_back(std::move(d));
    }

    // Interleave column-wise: first byte of each block, then second byte, etc.
    std::vector<uint8_t> out;
    const int maxData = std::max(v.g1DataCw, v.g2DataCw);
    for (int col = 0; col < maxData; ++col) {
        for (int b = 0; b < totalBlocks; ++b) {
            if (col < static_cast<int>(dataBlocks[b].size()))
                out.push_back(dataBlocks[b][col]);
        }
    }
    for (int col = 0; col < v.eccPerBlock; ++col) {
        for (int b = 0; b < totalBlocks; ++b)
            out.push_back(eccBlocks[b][col]);
    }
    return out;
}

// ──────────────────────────────────────────────────────────────────────────
// Module-grid construction. We use a parallel "functional" mask of cells
// that are reserved (finder, timing, alignment, format, version) so the
// data-mask step knows what NOT to flip.
struct Grid {
    int size;
    std::vector<bool> dark;
    std::vector<bool> reserved;

    Grid(int s) : size(s), dark(s * s, false), reserved(s * s, false) {}

    void set(int x, int y, bool v, bool reserve = false) {
        dark[y * size + x] = v;
        if (reserve) reserved[y * size + x] = true;
    }
    bool get(int x, int y) const { return dark[y * size + x]; }
    bool isReserved(int x, int y) const { return reserved[y * size + x]; }
};

void placeFinder(Grid &g, int x, int y)
{
    // 7x7 finder square at (x, y) (top-left corner).
    for (int dy = 0; dy < 7; ++dy) {
        for (int dx = 0; dx < 7; ++dx) {
            bool dark = (dx == 0 || dx == 6 || dy == 0 || dy == 6 ||
                         (dx >= 2 && dx <= 4 && dy >= 2 && dy <= 4));
            g.set(x + dx, y + dy, dark, true);
        }
    }
}

void placeSeparator(Grid &g, int x, int y)
{
    // 1-module white border around a finder. Skip out-of-bounds cells.
    for (int dy = -1; dy <= 7; ++dy) {
        for (int dx = -1; dx <= 7; ++dx) {
            const int nx = x + dx, ny = y + dy;
            if (nx < 0 || ny < 0 || nx >= g.size || ny >= g.size) continue;
            if (dx == -1 || dy == -1 || dx == 7 || dy == 7)
                g.set(nx, ny, false, true);
        }
    }
}

void placeAlignment(Grid &g, int cx, int cy)
{
    // 5x5 alignment at centre (cx, cy).
    for (int dy = -2; dy <= 2; ++dy) {
        for (int dx = -2; dx <= 2; ++dx) {
            bool dark = (std::abs(dx) == 2 || std::abs(dy) == 2 || (dx == 0 && dy == 0));
            g.set(cx + dx, cy + dy, dark, true);
        }
    }
}

void placeTiming(Grid &g)
{
    // Row 6 + column 6, alternating starting from black at offset 8.
    for (int i = 8; i < g.size - 8; ++i) {
        const bool dark = (i % 2 == 0);
        g.set(i, 6, dark, true);
        g.set(6, i, dark, true);
    }
}

void reserveFormat(Grid &g)
{
    // 15 format bits live around each finder. Reserve the cells so data
    // placement skips them; actual format bits placed later.
    for (int i = 0; i < 9; ++i) {
        g.set(8, i, false, true);
        g.set(i, 8, false, true);
    }
    for (int i = 0; i < 8; ++i) {
        g.set(8, g.size - 1 - i, false, true);
        g.set(g.size - 1 - i, 8, false, true);
    }
    // Always-dark module just above bottom-left finder.
    g.set(8, g.size - 8, true, true);
}

// BCH(15,5) format encoding. Input: 5-bit (2-bit ECC level + 3-bit mask).
// Output: 15-bit codeword XORed with mask 0x5412 for fixed pattern.
uint16_t bchFormat(uint16_t data5)
{
    uint16_t d = static_cast<uint16_t>(data5 << 10);
    for (int i = 4; i >= 0; --i) {
        if (d & (1 << (i + 10)))
            d ^= 0b10100110111 << i;
    }
    uint16_t result = static_cast<uint16_t>((data5 << 10) | d);
    return result ^ 0x5412;
}

void placeFormat(Grid &g, int maskNum)
{
    // ECC level M = 0b00 per spec (Table 12).
    const uint16_t data5 = static_cast<uint16_t>((0b00 << 3) | maskNum);
    const uint16_t fmt = bchFormat(data5);
    // Bit i (LSB-first) goes to specific cells per Figure 19.
    for (int i = 0; i <= 5; ++i)
        g.set(8, i, ((fmt >> i) & 1) != 0, true);
    g.set(8, 7, ((fmt >> 6) & 1) != 0, true);
    g.set(8, 8, ((fmt >> 7) & 1) != 0, true);
    g.set(7, 8, ((fmt >> 8) & 1) != 0, true);
    for (int i = 9; i <= 14; ++i)
        g.set(14 - i, 8, ((fmt >> i) & 1) != 0, true);

    for (int i = 0; i <= 7; ++i)
        g.set(g.size - 1 - i, 8, ((fmt >> i) & 1) != 0, true);
    for (int i = 8; i <= 14; ++i)
        g.set(8, g.size - 15 + i, ((fmt >> i) & 1) != 0, true);
}

// Walk the matrix in the spec's zig-zag pattern and lay down data bits,
// skipping reserved cells.
void placeData(Grid &g, const std::vector<uint8_t> &bytes)
{
    size_t bitIdx = 0;
    const int totalBits = static_cast<int>(bytes.size()) * 8;
    bool upward = true;
    for (int col = g.size - 1; col >= 1; col -= 2) {
        if (col == 6) col = 5; // skip the timing column
        for (int rowOff = 0; rowOff < g.size; ++rowOff) {
            const int y = upward ? (g.size - 1 - rowOff) : rowOff;
            for (int dx = 0; dx < 2; ++dx) {
                const int x = col - dx;
                if (g.isReserved(x, y)) continue;
                bool bit = false;
                if (static_cast<int>(bitIdx) < totalBits)
                    bit = ((bytes[bitIdx >> 3] >> (7 - (bitIdx & 7))) & 1) != 0;
                g.set(x, y, bit, false);
                ++bitIdx;
            }
        }
        upward = !upward;
    }
}

// Apply mask pattern m to non-reserved cells.
void applyMask(Grid &g, int m)
{
    for (int y = 0; y < g.size; ++y) {
        for (int x = 0; x < g.size; ++x) {
            if (g.isReserved(x, y)) continue;
            bool invert = false;
            switch (m) {
                case 0: invert = (x + y) % 2 == 0; break;
                case 1: invert = y % 2 == 0; break;
                case 2: invert = x % 3 == 0; break;
                case 3: invert = (x + y) % 3 == 0; break;
                case 4: invert = ((y / 2) + (x / 3)) % 2 == 0; break;
                case 5: invert = (x * y) % 2 + (x * y) % 3 == 0; break;
                case 6: invert = ((x * y) % 2 + (x * y) % 3) % 2 == 0; break;
                case 7: invert = ((x + y) % 2 + (x * y) % 3) % 2 == 0; break;
            }
            if (invert) g.dark[y * g.size + x] = !g.dark[y * g.size + x];
        }
    }
}

// Spec §8.3.3 penalty rules — lower score = "better" mask.
int penaltyScore(const Grid &g)
{
    int score = 0;
    const int S = g.size;

    // Rule 1: runs of 5+ same-colour modules in a row or column.
    for (int y = 0; y < S; ++y) {
        bool prev = g.get(0, y);
        int run = 1;
        for (int x = 1; x < S; ++x) {
            bool b = g.get(x, y);
            if (b == prev) { ++run; }
            else { if (run >= 5) score += 3 + (run - 5); run = 1; prev = b; }
        }
        if (run >= 5) score += 3 + (run - 5);
    }
    for (int x = 0; x < S; ++x) {
        bool prev = g.get(x, 0);
        int run = 1;
        for (int y = 1; y < S; ++y) {
            bool b = g.get(x, y);
            if (b == prev) { ++run; }
            else { if (run >= 5) score += 3 + (run - 5); run = 1; prev = b; }
        }
        if (run >= 5) score += 3 + (run - 5);
    }

    // Rule 2: 2x2 blocks of same colour.
    for (int y = 0; y < S - 1; ++y) {
        for (int x = 0; x < S - 1; ++x) {
            bool c = g.get(x, y);
            if (c == g.get(x + 1, y) && c == g.get(x, y + 1) && c == g.get(x + 1, y + 1))
                score += 3;
        }
    }

    // Rule 3: finder-like pattern in row/column (1:1:3:1:1 plus light run).
    auto check11311 = [&](int x, int y, int dx, int dy) {
        // Pattern: dark, light, dark, dark, dark, light, dark (lengths 1-1-3-1-1)
        // preceded or followed by 4 light modules.
        std::array<bool,7> p = {true,false,true,true,true,false,true};
        for (int i = 0; i < 7; ++i) {
            int xx = x + dx * i, yy = y + dy * i;
            if (xx < 0 || yy < 0 || xx >= S || yy >= S) return false;
            if (g.get(xx, yy) != p[i]) return false;
        }
        // Trailing 4 light
        bool trailing = true;
        for (int i = 7; i < 11; ++i) {
            int xx = x + dx * i, yy = y + dy * i;
            if (xx < 0 || yy < 0 || xx >= S || yy >= S) { trailing = false; break; }
            if (g.get(xx, yy)) { trailing = false; break; }
        }
        // Leading 4 light
        bool leading = true;
        for (int i = 1; i <= 4; ++i) {
            int xx = x - dx * i, yy = y - dy * i;
            if (xx < 0 || yy < 0 || xx >= S || yy >= S) { leading = false; break; }
            if (g.get(xx, yy)) { leading = false; break; }
        }
        return leading || trailing;
    };
    for (int y = 0; y < S; ++y)
        for (int x = 0; x < S; ++x) {
            if (check11311(x, y, 1, 0)) score += 40;
            if (check11311(x, y, 0, 1)) score += 40;
        }

    // Rule 4: proportion of dark vs total modules. Penalty grows with
    // distance from 50%, in 5% steps.
    int dark = 0;
    for (int i = 0; i < S * S; ++i) if (g.dark[i]) ++dark;
    const double ratio = static_cast<double>(dark) / (S * S) * 100.0;
    const int dev = static_cast<int>(std::abs(ratio - 50.0));
    score += (dev / 5) * 10;

    return score;
}

} // namespace

Matrix encode(const QString &text)
{
    const QByteArray data = text.toUtf8();
    const int version = pickVersion(data.size());
    if (version < 0) return {}; // too long

    auto payload = packPayload(data, version);
    auto interleaved = interleaveBlocks(payload, version);

    const int side = sideForVersion(version);
    Grid g(side);

    // Function patterns
    placeFinder(g, 0, 0);
    placeFinder(g, side - 7, 0);
    placeFinder(g, 0, side - 7);
    placeSeparator(g, 0, 0);
    placeSeparator(g, side - 7, 0);
    placeSeparator(g, 0, side - 7);
    placeTiming(g);
    for (int cy : kAlignmentCentres[version - 1]) {
        for (int cx : kAlignmentCentres[version - 1]) {
            // Skip alignment cells that would overlap finders.
            if (g.isReserved(cx, cy)) continue;
            placeAlignment(g, cx, cy);
        }
    }
    reserveFormat(g);
    placeData(g, interleaved);

    // Apply all 8 masks, score each, keep the best.
    Grid best = g;
    int bestScore = INT32_MAX;
    for (int m = 0; m < 8; ++m) {
        Grid candidate = g;
        applyMask(candidate, m);
        placeFormat(candidate, m);
        const int s = penaltyScore(candidate);
        if (s < bestScore) {
            bestScore = s;
            best = std::move(candidate);
        }
    }

    Matrix out;
    out.size = best.size;
    out.dark = std::move(best.dark);
    return out;
}

QPixmap renderQR(const QString &text, int pixelSize,
                 QColor fg, QColor bg, int quietZoneModules)
{
    const Matrix m = encode(text);
    if (m.size == 0) return {};

    const int totalModules = m.size + 2 * quietZoneModules;
    const int modulePx = std::max(1, pixelSize / totalModules);
    const int finalPx = modulePx * totalModules;

    QPixmap pm(finalPx, finalPx);
    pm.fill(bg);
    QPainter p(&pm);
    p.setBrush(fg);
    p.setPen(Qt::NoPen);
    for (int y = 0; y < m.size; ++y) {
        for (int x = 0; x < m.size; ++x) {
            if (!m.at(x, y)) continue;
            const int px = (quietZoneModules + x) * modulePx;
            const int py = (quietZoneModules + y) * modulePx;
            p.drawRect(px, py, modulePx, modulePx);
        }
    }
    return pm;
}

} // namespace qrgen
