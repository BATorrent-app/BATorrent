// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "toast.h"
#include "thememanager.h"

#include <QPainter>
#include <QPainterPath>
#include <QPropertyAnimation>
#include <QTimer>
#include <QScreen>
#include <QGuiApplication>
#include <QFontMetrics>
#include <QMouseEvent>

namespace {
constexpr int kWidth = 380;
constexpr int kHeight = 86;
constexpr int kScreenMargin = 16;
constexpr int kStackSpacing = 10;
constexpr int kPaddingX = 16;
constexpr int kPaddingY = 14;
constexpr int kLogoSize = 44;
constexpr int kLogoGap = 14;
constexpr int kRadius = 10;
constexpr int kDismissMs = 5000;
constexpr int kFadeMs = 220;

QColor eyebrowColorFor(Toast::Kind kind)
{
    const auto &tm = ThemeManager::instance();
    switch (kind) {
    case Toast::Success: return QColor(tm.stateSeedingColor());
    case Toast::Warning: return QColor(tm.accentColor());
    case Toast::Error:   return QColor(tm.accentDarkColor());
    default:             return QColor(tm.textColor());
    }
}
}

QList<Toast *> Toast::s_active;

Toast::Toast(const QString &title, const QString &body, Kind kind)
    : QWidget(nullptr, Qt::Tool | Qt::FramelessWindowHint
                       | Qt::WindowStaysOnTopHint
                       | Qt::WindowDoesNotAcceptFocus),
      m_title(title), m_body(body), m_kind(kind),
      m_dismissTimer(new QTimer(this)),
      m_fadeAnim(new QPropertyAnimation(this, "windowOpacity", this))
{
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setAttribute(Qt::WA_DeleteOnClose);
    setFixedSize(kWidth, kHeight);
    setWindowOpacity(0.0);
    setCursor(Qt::PointingHandCursor);

    m_logo = ThemeManager::instance().themedLogo(kLogoSize, devicePixelRatioF());

    m_dismissTimer->setSingleShot(true);
    m_dismissTimer->setInterval(kDismissMs);
    connect(m_dismissTimer, &QTimer::timeout, this, &Toast::dismiss);
    m_fadeAnim->setDuration(kFadeMs);
}

void Toast::notify(const QString &title, const QString &body, Kind kind,
                   const QObject *receiver, const char *clickedSlot)
{
    auto *t = new Toast(title, body, kind);
    s_active.append(t);
    connect(t, &Toast::destroyed, [t]() {
        s_active.removeAll(t);
        restack();
    });
    if (receiver && clickedSlot)
        connect(t, SIGNAL(clicked()), receiver, clickedSlot);

    restack();
    t->QWidget::show();
    t->m_fadeAnim->setStartValue(0.0);
    t->m_fadeAnim->setEndValue(1.0);
    t->m_fadeAnim->start();
    t->m_dismissTimer->start();
}

void Toast::restack()
{
    QScreen *screen = QGuiApplication::primaryScreen();
    if (!screen) return;
    const QRect avail = screen->availableGeometry();
    int y = avail.bottom() - kScreenMargin - kHeight;
    for (Toast *t : s_active) {
        t->move(avail.right() - kScreenMargin - kWidth, y);
        y -= (kHeight + kStackSpacing);
    }
}

void Toast::dismiss()
{
    m_dismissTimer->stop();
    m_fadeAnim->stop();
    disconnect(m_fadeAnim, &QPropertyAnimation::finished, nullptr, nullptr);
    m_fadeAnim->setStartValue(windowOpacity());
    m_fadeAnim->setEndValue(0.0);
    connect(m_fadeAnim, &QPropertyAnimation::finished, this, &QWidget::close);
    m_fadeAnim->start();
}

void Toast::paintEvent(QPaintEvent *)
{
    const auto &tm = ThemeManager::instance();
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QRectF body = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
    QPainterPath path;
    path.addRoundedRect(body, kRadius, kRadius);
    p.fillPath(path, QColor(tm.surfaceColor()));

    // 1px accent ring on warning/error; otherwise no border (the surface
    // tone separates it from the desktop and dark BG behind the window).
    if (m_kind == Warning || m_kind == Error) {
        QPen ring(eyebrowColorFor(m_kind), 1.0);
        p.setPen(ring);
        p.drawPath(path);
    }

    // Logo on the left.
    const int logoY = (height() - kLogoSize) / 2;
    if (!m_logo.isNull())
        p.drawPixmap(kPaddingX, logoY, m_logo);

    const int textX = kPaddingX + kLogoSize + kLogoGap;
    const int textW = width() - textX - kPaddingX;

    // Eyebrow + "now" row. JSX uses fontWeight 700 (Bold) + letterSpacing 1.2.
    QFont eyebrowFont;
    eyebrowFont.setPointSize(8);
    eyebrowFont.setWeight(QFont::Bold);
    eyebrowFont.setLetterSpacing(QFont::AbsoluteSpacing, 1.2);
    p.setFont(eyebrowFont);

    const QString eyebrow = QStringLiteral("BATORRENT");
    p.setPen(eyebrowColorFor(m_kind));
    QFontMetrics ebMetrics(eyebrowFont);
    p.drawText(QRect(textX, kPaddingY, textW, 14),
               Qt::AlignLeft | Qt::AlignVCenter, eyebrow);

    QFont nowFont;
    nowFont.setPointSize(8);
    p.setFont(nowFont);
    p.setPen(QColor(tm.dimColor()));
    p.drawText(QRect(textX, kPaddingY, textW, 14),
               Qt::AlignRight | Qt::AlignVCenter, QStringLiteral("now"));

    // Title.
    QFont titleFont;
    titleFont.setPointSize(10);
    titleFont.setWeight(QFont::DemiBold);
    p.setFont(titleFont);
    p.setPen(QColor(tm.textColor()));
    const QString titleElided = QFontMetrics(titleFont)
        .elidedText(m_title, Qt::ElideRight, textW);
    p.drawText(QRect(textX, kPaddingY + 14 + 4, textW, 18),
               Qt::AlignLeft | Qt::AlignVCenter, titleElided);

    // Body (monospace — same treatment as the design's mono font).
    QFont bodyFont(QStringLiteral("Menlo"));
    if (!bodyFont.exactMatch())
        bodyFont = QFont(QStringLiteral("SF Mono"));
    if (!bodyFont.exactMatch())
        bodyFont.setFamily(QStringLiteral("Consolas"));
    bodyFont.setPointSize(9);
    p.setFont(bodyFont);
    p.setPen(QColor(tm.mutedColor()));
    const QString bodyElided = QFontMetrics(bodyFont)
        .elidedText(m_body, Qt::ElideRight, textW);
    p.drawText(QRect(textX, kPaddingY + 14 + 4 + 18 + 2, textW, 16),
               Qt::AlignLeft | Qt::AlignVCenter, bodyElided);
}

void Toast::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton)
        emit clicked();
    dismiss();
}

void Toast::enterEvent(QEnterEvent *)
{
    m_dismissTimer->stop();
}

void Toast::leaveEvent(QEvent *)
{
    m_dismissTimer->start();
}
