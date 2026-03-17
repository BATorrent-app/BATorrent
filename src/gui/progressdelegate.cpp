#include "progressdelegate.h"
#include "thememanager.h"
#include <QPainter>
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

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    QRect barRect = option.rect.adjusted(4, 4, -4, -4);
    int radius = 3;

    // Background track
    QColor trackColor(tm.surfaceColor());
    painter->setPen(Qt::NoPen);
    painter->setBrush(trackColor);
    painter->drawRoundedRect(barRect, radius, radius);

    // Progress fill
    if (progress > 0.001f) {
        int fillWidth = static_cast<int>(barRect.width() * progress);
        QRect fillRect(barRect.left(), barRect.top(), fillWidth, barRect.height());

        QColor fillColor;
        if (progress >= 1.0f)
            fillColor = QColor(0x30, 0x90, 0x50); // green when complete
        else
            fillColor = QColor(tm.accentColor());

        painter->setBrush(fillColor);
        painter->drawRoundedRect(fillRect, radius, radius);

        // Shimmer effect (only while downloading, not complete)
        if (progress < 1.0f && progress > 0.01f) {
            float elapsed = m_elapsed.elapsed() / 1000.0f;
            // Shimmer position cycles across the bar
            float shimmerPos = fmod(elapsed * 0.6f, 1.4f) - 0.2f;
            float shimmerX = fillRect.left() + shimmerPos * fillRect.width();
            float shimmerWidth = fillRect.width() * 0.25f;

            QLinearGradient shimmer(shimmerX, 0, shimmerX + shimmerWidth, 0);
            QColor highlight = fillColor.lighter(160);
            highlight.setAlpha(0);
            shimmer.setColorAt(0.0, highlight);
            highlight.setAlpha(80);
            shimmer.setColorAt(0.5, highlight);
            highlight.setAlpha(0);
            shimmer.setColorAt(1.0, highlight);

            painter->setClipRect(fillRect);
            painter->setBrush(shimmer);
            painter->drawRoundedRect(fillRect, radius, radius);
            painter->setClipping(false);
        }
    }

    // Percentage text
    QString text = QString::number(progress * 100.0, 'f', 1) + "%";
    QColor textColor(tm.textColor());
    painter->setPen(textColor);
    QFont f = painter->font();
    f.setPointSize(8);
    painter->setFont(f);
    painter->drawText(barRect, Qt::AlignCenter, text);

    painter->restore();
}
