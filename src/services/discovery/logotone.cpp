// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "services/discovery/logotone.h"

#include <QImage>

namespace LogoTone {

bool isDark(const QImage &logo)
{
    if (logo.isNull()) return false;
    const QImage img = (logo.width() > 160 ? logo.scaledToWidth(160, Qt::FastTransformation) : logo)
                           .convertToFormat(QImage::Format_ARGB32);
    double weight = 0, light = 0;
    for (int y = 0; y < img.height(); ++y) {
        const QRgb *row = reinterpret_cast<const QRgb *>(img.constScanLine(y));
        for (int x = 0; x < img.width(); ++x) {
            const double a = qAlpha(row[x]) / 255.0;
            if (a < 0.15) continue;
            light += a * (0.2126 * qRed(row[x]) + 0.7152 * qGreen(row[x]) + 0.0722 * qBlue(row[x])) / 255.0;
            weight += a;
        }
    }
    return weight > 0 && light / weight < 0.35;
}

} // namespace LogoTone
