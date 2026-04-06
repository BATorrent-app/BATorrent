// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef PIECEMAPWIDGET_H
#define PIECEMAPWIDGET_H

#include <QWidget>
#include <vector>

class PieceMapWidget : public QWidget
{
    Q_OBJECT
public:
    explicit PieceMapWidget(QWidget *parent = nullptr);

    void setPieces(const std::vector<bool> &pieces);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    std::vector<bool> m_pieces;
};

#endif
