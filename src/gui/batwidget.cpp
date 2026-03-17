#include "batwidget.h"
#include "thememanager.h"
#include <QPainter>

BatWidget::BatWidget(QWidget *parent)
    : QWidget(parent)
{
    m_logo = QPixmap(":/images/logo1.png");
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

    int w = width();
    int h = height();

    const auto &tm = ThemeManager::instance();

    // Draw logo PNG centered
    int logoSize = 120;
    QPixmap scaled = m_logo.scaled(logoSize, logoSize,
                                    Qt::KeepAspectRatio,
                                    Qt::SmoothTransformation);
    int lx = (w - scaled.width()) / 2;
    int ly = h / 2 - scaled.height() / 2 - 20;
    p.drawPixmap(lx, ly, scaled);

    // Message text below
    if (!m_message.isEmpty()) {
        QColor textColor(tm.mutedColor());
        p.setPen(textColor);
        QFont f = font();
        f.setPointSize(11);
        p.setFont(f);
        p.drawText(QRect(0, ly + logoSize + 16, w, 30),
                    Qt::AlignHCenter, m_message);
    }
}
