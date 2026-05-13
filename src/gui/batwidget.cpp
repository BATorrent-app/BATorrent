// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "batwidget.h"
#include "thememanager.h"
#include <QPainter>

BatWidget::BatWidget(QWidget *parent)
    : QWidget(parent)
{
    // Source is 1024×1024 but we always render at 100 px. Pre-scale once
    // here so paintEvent doesn't redo a smooth-transform from 1024 every
    // time the window repaints (resize, hover, focus all trigger paints).
    // Render at 2× the target on the off chance the widget winds up on a
    // HiDPI screen, then Qt's painter downscales the cached pixmap.
    QPixmap raw(":/images/logo1.png");
    m_logo = raw.scaled(200, 200, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    m_logo.setDevicePixelRatio(2.0);
    m_message = "Drop a .torrent file or magnet link here";
}

void BatWidget::setMessage(const QString &msg)
{
    m_message = msg;
    update();
}

void BatWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::SmoothPixmapTransform);
    p.setRenderHint(QPainter::Antialiasing);

    int w = width();
    int h = height();

    const auto &tm = ThemeManager::instance();

    // Dashed drop zone border
    int margin = 32;
    QRect dropRect(margin, margin, w - 2 * margin, h - 2 * margin);
    QPen dashPen(QColor(tm.borderColor()));
    dashPen.setWidth(2);
    dashPen.setStyle(Qt::DashLine);
    dashPen.setDashOffset(4);
    p.setPen(dashPen);
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(dropRect, 16, 16);

    // Logo was pre-scaled in the constructor; we just draw the cached
    // pixmap. The 100-logical-px target matches what the previous
    // implementation rendered, but no SmoothTransformation work happens per
    // paint.
    int logoSize = 100;
    int lx = (w - logoSize) / 2;
    int ly = h / 2 - logoSize / 2 - 24;
    p.drawPixmap(QRect(lx, ly, logoSize, logoSize), m_logo);

    // Primary message text below
    if (!m_message.isEmpty()) {
        QColor textColor(tm.textColor());
        p.setPen(textColor);
        QFont f = font();
        f.setPointSize(12);
        f.setWeight(QFont::DemiBold);
        p.setFont(f);
        p.drawText(QRect(0, ly + logoSize + 16, w, 30),
                    Qt::AlignHCenter, m_message);

        // Sub-message
        QColor mutedColor(tm.mutedColor());
        p.setPen(mutedColor);
        f.setPointSize(10);
        f.setWeight(QFont::Normal);
        p.setFont(f);
        p.drawText(QRect(0, ly + logoSize + 44, w, 24),
                    Qt::AlignHCenter, "or use File > Open / Add Magnet");
    }
}
