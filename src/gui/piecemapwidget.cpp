// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "piecemapwidget.h"
#include "thememanager.h"
#include <QPainter>

PieceMapWidget::PieceMapWidget(QWidget *parent)
    : QWidget(parent)
{
    setMinimumHeight(60);
}

void PieceMapWidget::setPieces(const std::vector<bool> &pieces)
{
    m_pieces = pieces;
    update();
}

void PieceMapWidget::paintEvent(QPaintEvent * /*event*/)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, false);

    const auto &tm = ThemeManager::instance();

    // Fill background
    painter.fillRect(rect(), QColor(tm.surfaceColor()));

    if (m_pieces.empty())
        return;

    const int cellSize = 8;
    const int spacing = 1;
    const int step = cellSize + spacing;
    const int margin = 4;

    int availWidth = width() - 2 * margin;
    int cols = qMax(1, availWidth / step);
    int numPieces = static_cast<int>(m_pieces.size());

    // If more pieces than cells, group pieces into blocks
    int piecesPerBlock = qMax(1, (numPieces + cols * 500 - 1) / (cols * 500));
    int totalBlocks = (numPieces + piecesPerBlock - 1) / piecesPerBlock;

    QColor downloaded(tm.accentColor());
    QColor missing(tm.borderColor());

    for (int i = 0; i < totalBlocks; ++i) {
        int col = i % cols;
        int row = i / cols;
        int x = margin + col * step;
        int y = margin + row * step;

        if (y + cellSize > height())
            break;

        // Check if any piece in this block is downloaded
        int startPiece = i * piecesPerBlock;
        int endPiece = qMin(startPiece + piecesPerBlock, numPieces);
        int have = 0;
        for (int p = startPiece; p < endPiece; ++p) {
            if (m_pieces[p])
                ++have;
        }

        if (have == (endPiece - startPiece)) {
            painter.fillRect(x, y, cellSize, cellSize, downloaded);
        } else if (have > 0) {
            // Partial: blend color
            float ratio = static_cast<float>(have) / (endPiece - startPiece);
            QColor partial = QColor::fromRgbF(
                missing.redF() + (downloaded.redF() - missing.redF()) * ratio,
                missing.greenF() + (downloaded.greenF() - missing.greenF()) * ratio,
                missing.blueF() + (downloaded.blueF() - missing.blueF()) * ratio);
            painter.fillRect(x, y, cellSize, cellSize, partial);
        } else {
            painter.fillRect(x, y, cellSize, cellSize, missing);
        }
    }
}
