// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "posterview.h"
#include "thememanager.h"
#include "torrentmodel.h"
#include "../app/metadataresolver.h"
#include "../torrent/sessionmanager.h"
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QSortFilterProxyModel>
#include <QMouseEvent>

PosterDelegate::PosterDelegate(MetadataResolver *resolver, SessionManager *session,
                               QObject *parent)
    : QStyledItemDelegate(parent)
    , m_resolver(resolver)
    , m_session(session)
{
}

QSize PosterDelegate::sizeHint(const QStyleOptionViewItem &, const QModelIndex &) const
{
    return {160, 280};
}

void PosterDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                           const QModelIndex &index) const
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    const auto &tm = ThemeManager::instance();
    const QRect card = option.rect;
    const QRect posterRect(card.left(), card.top(), card.width(), 225);
    const QRect infoRect(card.left(), card.top() + 225, card.width(), 55);

    const auto *proxy = qobject_cast<const QSortFilterProxyModel *>(index.model());
    const QModelIndex nameIdx = proxy ? proxy->index(index.row(), TorrentModel::Name) : index;
    const QString name = nameIdx.data(Qt::DisplayRole).toString();
    const QString stateKey = nameIdx.data(TorrentModel::StateKeyRole).toString();

    int sourceRow = proxy ? proxy->mapToSource(index).row() : index.row();
    const QString hash = m_session->torrentHashAt(sourceRow);

    MetadataResult meta;
    bool hasMeta = false;
    if (!hash.isEmpty() && m_resolver->hasCached(hash)) {
        meta = m_resolver->cached(hash);
        hasMeta = meta.valid;
    }

    // Card background
    QPainterPath cardPath;
    cardPath.addRoundedRect(QRectF(card), 8, 8);
    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(tm.surfaceColor()));
    painter->drawPath(cardPath);

    // Poster area
    painter->save();
    painter->setClipPath(cardPath);

    if (hasMeta && !meta.posterPath.isEmpty()) {
        const QString &path = meta.posterPath;
        if (!m_pixmapCache.contains(path)) {
            QPixmap px(path);
            if (!px.isNull())
                m_pixmapCache[path] = px.scaled(posterRect.size(), Qt::KeepAspectRatioByExpanding,
                                                Qt::SmoothTransformation);
        }
        if (m_pixmapCache.contains(path) && !m_pixmapCache[path].isNull()) {
            const QPixmap &scaled = m_pixmapCache[path];
            int sx = (scaled.width() - posterRect.width()) / 2;
            int sy = (scaled.height() - posterRect.height()) / 2;
            painter->drawPixmap(posterRect, scaled,
                                QRect(sx, sy, posterRect.width(), posterRect.height()));
        }
    } else {
        QColor placeholderBg(tm.accentColor());
        placeholderBg.setAlphaF(0.10f);
        painter->setBrush(placeholderBg);
        painter->setPen(Qt::NoPen);
        painter->drawRect(posterRect);

        static QPixmap logoCache;
        if (logoCache.isNull())
            logoCache = tm.themedLogo(64, 2.0);
        QRect logoRect(posterRect.center().x() - 32, posterRect.center().y() - 44, 64, 64);
        painter->setOpacity(0.4);
        painter->drawPixmap(logoRect, logoCache);
        painter->setOpacity(1.0);

        QFont letterFont = painter->font();
        letterFont.setPointSize(11);
        letterFont.setWeight(QFont::DemiBold);
        painter->setFont(letterFont);
        painter->setPen(QColor(tm.mutedColor()));
        QRect textRect(posterRect.left(), logoRect.bottom() + 8, posterRect.width(), 20);
        QFontMetrics fm(letterFont);
        QString elided = fm.elidedText(name, Qt::ElideRight, textRect.width() - 12);
        painter->drawText(textRect, Qt::AlignHCenter | Qt::AlignTop, elided);
    }
    painter->restore();

    // Progress bar at bottom of poster area
    constexpr int kBarH = 3;
    QRect barRect(posterRect.left(), posterRect.bottom() - kBarH, posterRect.width(), kBarH);

    QColor barColor;
    if (stateKey == QLatin1String("seeding") || stateKey == QLatin1String("finished"))
        barColor = QColor(tm.stateSeedingColor());
    else if (stateKey == QLatin1String("downloading"))
        barColor = QColor(tm.stateDownloadingColor());
    else if (stateKey == QLatin1String("completed"))
        barColor = QColor(tm.stateCompletedColor());
    else
        barColor = QColor(tm.statePausedColor());

    const QModelIndex progIdx = proxy ? proxy->index(index.row(), TorrentModel::Progress) : index;
    float progress = progIdx.data(Qt::UserRole).toFloat();

    QColor trackColor = barColor;
    trackColor.setAlphaF(0.2f);
    painter->setPen(Qt::NoPen);
    painter->setBrush(trackColor);
    painter->drawRect(barRect);

    if (progress > 0.001f) {
        int fillW = static_cast<int>(barRect.width() * progress);
        painter->setBrush(barColor);
        painter->drawRect(QRect(barRect.left(), barRect.top(), fillW, barRect.height()));
    }

    // Title text
    QString displayTitle = (hasMeta && !meta.title.isEmpty()) ? meta.title : name;
    QFont titleFont = painter->font();
    titleFont.setPointSize(10);
    painter->setFont(titleFont);
    painter->setPen(QColor(tm.textColor()));

    QRect titleRect(infoRect.left() + 6, infoRect.top() + 4, infoRect.width() - 12, 22);
    QFontMetrics titleFm(titleFont);
    QString elidedTitle = titleFm.elidedText(displayTitle, Qt::ElideRight, titleRect.width());
    painter->drawText(titleRect, Qt::AlignLeft | Qt::AlignVCenter, elidedTitle);

    // State text
    const QModelIndex stateIdx = proxy ? proxy->index(index.row(), TorrentModel::State) : index;
    QString stateStr = stateIdx.data(Qt::DisplayRole).toString();
    QFont stateFont = painter->font();
    stateFont.setPointSize(9);
    painter->setFont(stateFont);
    painter->setPen(QColor(tm.mutedColor()));

    QRect stateRect(infoRect.left() + 6, titleRect.bottom(), infoRect.width() - 12, 20);
    QFontMetrics stateFm(stateFont);
    QString elidedState = stateFm.elidedText(stateStr, Qt::ElideRight, stateRect.width());
    painter->drawText(stateRect, Qt::AlignLeft | Qt::AlignVCenter, elidedState);

    // Selection border
    if (option.state & QStyle::State_Selected) {
        QPen selPen(QColor(tm.accentColor()), 2);
        painter->setPen(selPen);
        painter->setBrush(Qt::NoBrush);
        painter->drawRoundedRect(QRectF(card).adjusted(1, 1, -1, -1), 8, 8);
    }

    painter->restore();
}

PosterView::PosterView(MetadataResolver *resolver, SessionManager *session, QWidget *parent)
    : QListView(parent)
{
    setViewMode(QListView::IconMode);
    setFlow(QListView::LeftToRight);
    setWrapping(true);
    setResizeMode(QListView::Adjust);
    setSpacing(12);
    setGridSize(QSize(172, 292));
    setUniformItemSizes(true);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setContextMenuPolicy(Qt::CustomContextMenu);
    setItemDelegate(new PosterDelegate(resolver, session, this));
    setStyleSheet(QStringLiteral("QListView { background: transparent; border: none; }"));
}

void PosterView::selectionChanged(const QItemSelection &selected,
                                  const QItemSelection &deselected)
{
    QListView::selectionChanged(selected, deselected);
    if (selected.indexes().isEmpty())
        return;
    const QModelIndex idx = selected.indexes().first();
    if (auto *proxy = qobject_cast<const QSortFilterProxyModel *>(model())) {
        emit torrentSelected(proxy->mapToSource(idx).row());
    } else {
        emit torrentSelected(idx.row());
    }
}

void PosterView::mouseDoubleClickEvent(QMouseEvent *event)
{
    QListView::mouseDoubleClickEvent(event);
    const QModelIndex idx = indexAt(event->pos());
    if (!idx.isValid())
        return;
    if (auto *proxy = qobject_cast<const QSortFilterProxyModel *>(model())) {
        emit torrentDoubleClicked(proxy->mapToSource(idx).row());
    } else {
        emit torrentDoubleClicked(idx.row());
    }
}
