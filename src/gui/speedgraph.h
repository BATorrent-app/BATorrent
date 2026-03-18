// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef SPEEDGRAPH_H
#define SPEEDGRAPH_H

#include <QWidget>
#include <QVector>

class SpeedGraph : public QWidget
{
    Q_OBJECT
public:
    explicit SpeedGraph(QWidget *parent = nullptr);

    void addDataPoint(int downloadSpeed, int uploadSpeed);
    void setAccentColor(const QColor &color);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    static constexpr int MaxPoints = 60; // 60 seconds of history

    QVector<int> m_downloadData;
    QVector<int> m_uploadData;
    QColor m_accentColor{0xc4, 0x30, 0x30};
};

#endif
