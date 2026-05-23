// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details
//
// Self-contained QR Code generator. Supports byte-mode (URLs and arbitrary
// strings), error-correction level M, auto-version (1-10), and full 8-mask
// scoring per ISO/IEC 18004 §8.3.3. No external dependencies — pure Qt + STL.
//
// API:
//   QPixmap qrgen::renderQR(const QString &text, int pixelSize,
//                           QColor fg = Qt::black, QColor bg = Qt::white,
//                           int quietZoneModules = 4);

#ifndef BATORRENT_QRCODEGEN_H
#define BATORRENT_QRCODEGEN_H

#include <QColor>
#include <QPixmap>
#include <QString>
#include <vector>

namespace qrgen {

// Bit-matrix produced by encode(). Black modules are true. size is the side
// length in modules (21 for v1, growing by 4 per version up to 57 for v10).
struct Matrix {
    int size = 0;
    std::vector<bool> dark; // size*size, row-major

    bool at(int x, int y) const { return dark[y * size + x]; }
};

// Encode `text` (UTF-8 bytes) at ECC level M and the smallest version that
// fits. Returns an empty matrix (size == 0) if the text is too long for v10
// at ECC M (~213 bytes).
Matrix encode(const QString &text);

// Convenience renderer — calls encode() then rasterises onto a pixmap of
// approximately pixelSize × pixelSize. Module size is integer-rounded so the
// QR stays crisp at any zoom level. quietZoneModules is the white border
// width in modules (spec recommends 4).
QPixmap renderQR(const QString &text, int pixelSize,
                 QColor fg = Qt::black, QColor bg = Qt::white,
                 int quietZoneModules = 4);

} // namespace qrgen

#endif
