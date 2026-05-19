// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "progressdelegate.h"
#include "thememanager.h"
#include "torrentmodel.h"
#include <QPainter>
#include <QPainterPath>
#include <QAbstractItemView>
#include <QtMath>

ProgressDelegate::ProgressDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
    m_elapsed.start();

    // Trigger repaint for shimmer animation
    m_animTimer = new QTimer(this);
    connect(m_animTimer, &QTimer::timeout, this, [this, parent]() {
        if (auto *view = qobject_cast<QAbstractItemView *>(parent))
            view->viewport()->update();
    });
    m_animTimer->start(50); // 20fps for shimmer
}

void ProgressDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                             const QModelIndex &index) const
{
    QVariant progressVar = index.data(Qt::UserRole);
    if (!progressVar.isValid()) {
        QStyledItemDelegate::paint(painter, option, index);
        return;
    }

    float progress = progressVar.toFloat();
    const auto &tm = ThemeManager::instance();

    const QString stateKey = index.data(TorrentModel::StateKeyRole).toString();
    QColor fillColor;
    if (stateKey == QLatin1String("seeding") || stateKey == QLatin1String("finished")
            || progress >= 1.0f)
        fillColor = QColor(tm.stateSeedingColor());
    else if (stateKey == QLatin1String("paused") || stateKey == QLatin1String("queued"))
        fillColor = QColor(tm.statePausedColor());
    else if (stateKey == QLatin1String("error"))
        fillColor = QColor(tm.stateErrorColor());
    else
        fillColor = QColor(tm.stateDownloadingColor());

    if (stateKey == QLatin1String("finished"))
        fillColor = QColor(tm.stateFinishedColor());

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    // Full-cell filled bar with the percentage overlaid centered — classic
    // torrent-client look (uTorrent / qBittorrent default), more legible at a
    // glance than a slim pill + text. Background track in surfaceAlt, fill in
    // the state color, text in white when fill is wide enough to read against
    // it, otherwise text color.
    constexpr int kBarHeight = 18;
    constexpr int kMargin = 8;
    const QRect track(option.rect.left() + kMargin,
                      option.rect.center().y() - kBarHeight / 2,
                      option.rect.width() - 2 * kMargin,
                      kBarHeight);
    constexpr int kRadius = 4;

    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(tm.surfaceAltColor()));
    painter->drawRoundedRect(track, kRadius, kRadius);

    if (progress > 0.001f) {
        const int fillW = static_cast<int>(track.width() * progress);
        const QRect fillRect(track.left(), track.top(), fillW, track.height());
        painter->setBrush(fillColor);
        // Round only the leading edge; if fill spans full width the track's
        // own rounding handles the trailing edge already.
        if (fillW < track.width()) {
            QPainterPath path;
            path.moveTo(track.left() + kRadius, track.top());
            path.lineTo(fillRect.right(), track.top());
            path.lineTo(fillRect.right(), track.bottom());
            path.lineTo(track.left() + kRadius, track.bottom());
            path.arcTo(track.left(), track.bottom() - 2 * kRadius,
                       2 * kRadius, 2 * kRadius, 270, -90);
            path.lineTo(track.left(), track.top() + kRadius);
            path.arcTo(track.left(), track.top(),
                       2 * kRadius, 2 * kRadius, 180, -90);
            path.closeSubpath();
            painter->drawPath(path);
        } else {
            painter->drawRoundedRect(fillRect, kRadius, kRadius);
        }
    }

    // Percentage centered on the bar. White when over the colored fill,
    // text color over the empty track.
    QFont f = painter->font();
    f.setPointSize(9);
    f.setWeight(QFont::DemiBold);
    painter->setFont(f);
    const QString text = QString::number(progress * 100.0, 'f', 1) + "%";
    const QFontMetrics fm(f);
    const int textW = fm.horizontalAdvance(text);
    const int textCenterX = track.center().x();
    const int fillEdge = track.left() + static_cast<int>(track.width() * progress);
    const bool overFill = (textCenterX - textW / 2) < fillEdge - 4;
    painter->setPen(overFill ? QColor("#ffffff") : QColor(tm.textColor()));
    painter->drawText(track, Qt::AlignCenter, text);

    painter->restore();
}
