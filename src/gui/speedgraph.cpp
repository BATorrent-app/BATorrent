#include "speedgraph.h"
#include "thememanager.h"
#include <QPainter>
#include <QPainterPath>
#include <algorithm>

SpeedGraph::SpeedGraph(QWidget *parent)
    : QWidget(parent)
{
    setMinimumHeight(80);
    setMaximumHeight(120);
}

void SpeedGraph::addDataPoint(int downloadSpeed, int uploadSpeed)
{
    m_downloadData.append(downloadSpeed);
    m_uploadData.append(uploadSpeed);

    if (m_downloadData.size() > MaxPoints) {
        m_downloadData.removeFirst();
        m_uploadData.removeFirst();
    }

    update();
}

void SpeedGraph::setAccentColor(const QColor &color)
{
    m_accentColor = color;
    update();
}

void SpeedGraph::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    int w = width();
    int h = height();

    // Use theme colors
    const auto &tm = ThemeManager::instance();
    QColor bgColor(tm.bgColor());
    QColor gridColor(tm.surfaceColor());
    QColor textColor(tm.mutedColor());
    QColor dlColor(tm.accentColor());
    QColor ulColor(0x30, 0x60, 0xc4);

    p.fillRect(rect(), bgColor);

    if (m_downloadData.isEmpty())
        return;

    // Find max value for scaling
    int maxVal = 1024;
    for (int v : m_downloadData) maxVal = std::max(maxVal, v);
    for (int v : m_uploadData) maxVal = std::max(maxVal, v);
    maxVal = static_cast<int>(maxVal * 1.2);

    int n = m_downloadData.size();
    float xStep = static_cast<float>(w) / (MaxPoints - 1);
    float xOffset = (MaxPoints - n) * xStep;

    // Grid lines
    p.setPen(QPen(gridColor, 1));
    for (int i = 1; i < 4; ++i) {
        int y = h * i / 4;
        p.drawLine(0, y, w, y);
    }

    // Draw upload area (behind download)
    {
        QPainterPath path;
        path.moveTo(xOffset, h);
        for (int i = 0; i < n; ++i) {
            float x = xOffset + i * xStep;
            float y = h - (static_cast<float>(m_uploadData[i]) / maxVal * (h - 4));
            path.lineTo(x, y);
        }
        path.lineTo(xOffset + (n - 1) * xStep, h);
        path.closeSubpath();

        QColor fill = ulColor;
        fill.setAlpha(40);
        p.fillPath(path, fill);
        p.setPen(QPen(ulColor, 1.5));
        for (int i = 1; i < n; ++i) {
            float x0 = xOffset + (i - 1) * xStep;
            float y0 = h - (static_cast<float>(m_uploadData[i - 1]) / maxVal * (h - 4));
            float x1 = xOffset + i * xStep;
            float y1 = h - (static_cast<float>(m_uploadData[i]) / maxVal * (h - 4));
            p.drawLine(QPointF(x0, y0), QPointF(x1, y1));
        }
    }

    // Draw download area
    {
        QPainterPath path;
        path.moveTo(xOffset, h);
        for (int i = 0; i < n; ++i) {
            float x = xOffset + i * xStep;
            float y = h - (static_cast<float>(m_downloadData[i]) / maxVal * (h - 4));
            path.lineTo(x, y);
        }
        path.lineTo(xOffset + (n - 1) * xStep, h);
        path.closeSubpath();

        QColor fill = dlColor;
        fill.setAlpha(50);
        p.fillPath(path, fill);
        p.setPen(QPen(dlColor, 1.5));
        for (int i = 1; i < n; ++i) {
            float x0 = xOffset + (i - 1) * xStep;
            float y0 = h - (static_cast<float>(m_downloadData[i - 1]) / maxVal * (h - 4));
            float x1 = xOffset + i * xStep;
            float y1 = h - (static_cast<float>(m_downloadData[i]) / maxVal * (h - 4));
            p.drawLine(QPointF(x0, y0), QPointF(x1, y1));
        }
    }

    // Scale label
    QString scaleText;
    if (maxVal >= 1024 * 1024)
        scaleText = QString::number(maxVal / (1024.0 * 1024.0), 'f', 1) + " MB/s";
    else
        scaleText = QString::number(maxVal / 1024.0, 'f', 0) + " KB/s";

    p.setPen(textColor);
    p.setFont(QFont(font().family(), 8));
    p.drawText(4, 12, scaleText);

    // Legend
    int legendX = w - 140;
    p.setPen(dlColor);
    p.drawLine(legendX, 8, legendX + 14, 8);
    p.setPen(textColor);
    p.drawText(legendX + 18, 12, "Download");

    p.setPen(ulColor);
    p.drawLine(legendX + 80, 8, legendX + 94, 8);
    p.setPen(textColor);
    p.drawText(legendX + 98, 12, "Upload");
}
