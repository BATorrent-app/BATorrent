// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "batwidget.h"
#include "thememanager.h"
#include "../app/translator.h"

#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>

namespace {

// Self-painting circle+logo so the dashed disc always lands exactly where
// the layout puts it, no matter how the user resizes the window. The previous
// implementation drew the circle in BatWidget::paintEvent at a hardcoded
// y = height()/2 - 110 which drifted relative to the labels below it.
class LogoCircle : public QWidget
{
public:
    explicit LogoCircle(QWidget *parent = nullptr) : QWidget(parent)
    {
        setFixedSize(132, 132);
    }
protected:
    void paintEvent(QPaintEvent *) override
    {
        const auto &tm = ThemeManager::instance();
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.setRenderHint(QPainter::SmoothPixmapTransform);

        const QRectF circle = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
        QPen dash(QColor(tm.borderColor()));
        dash.setWidthF(1.0);
        dash.setStyle(Qt::CustomDashLine);
        dash.setDashPattern({2.0, 3.0});
        p.setPen(dash);
        p.setBrush(QColor(tm.surfaceColor()));
        p.drawEllipse(circle);

        const int target = 76;
        // Pull fresh on each paint so a theme switch picks up the new tint
        // without forcing a widget rebuild.
        QPixmap logo = tm.themedLogo(target, 2.0);
        p.drawPixmap(QRect((width() - target) / 2,
                           (height() - target) / 2,
                           target, target), logo);
    }
};

QWidget *makeKeycap(const QString &text, QWidget *parent)
{
    const auto &tm = ThemeManager::instance();
    auto *lbl = new QLabel(text, parent);
    lbl->setAlignment(Qt::AlignCenter);
    lbl->setStyleSheet(QString(
        "QLabel {"
        "  background: %1;"
        "  color: %2;"
        "  border: 1px solid %3;"
        "  border-radius: 4px;"
        "  padding: 2px 8px;"
        "  font-family: 'Segoe UI', 'Helvetica Neue', system-ui, sans-serif;"
        "  font-size: 10px;"
        "  font-weight: 600;"
        "  min-width: 18px;"
        "}").arg(tm.surfaceAltColor(), tm.mutedColor(), tm.borderColor()));
    return lbl;
}

}

BatWidget::BatWidget(QWidget *parent)
    : QWidget(parent)
{
    const auto &tm = ThemeManager::instance();

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(24, 24, 24, 24);
    root->setSpacing(0);
    root->addStretch(2);

    auto *circle = new LogoCircle(this);
    auto *circleRow = new QHBoxLayout;
    circleRow->setContentsMargins(0, 0, 0, 0);
    circleRow->addStretch();
    circleRow->addWidget(circle);
    circleRow->addStretch();
    root->addLayout(circleRow);
    root->addSpacing(24);

    m_defaultTitle = tr_("empty_title");
    m_defaultBody = tr_("empty_body");
    m_titleLabel = new QLabel(m_defaultTitle);
    m_titleLabel->setAlignment(Qt::AlignCenter);
    m_titleLabel->setStyleSheet(QString(
        "color: %1; font-size: 18px; font-weight: 600;").arg(tm.textColor()));
    root->addWidget(m_titleLabel);
    root->addSpacing(8);

    m_bodyLabel = new QLabel(m_defaultBody);
    m_bodyLabel->setAlignment(Qt::AlignCenter);
    m_bodyLabel->setWordWrap(true);
    m_bodyLabel->setMaximumWidth(420);
    m_bodyLabel->setMinimumHeight(40);
    m_bodyLabel->setStyleSheet(QString(
        "color: %1; font-size: 12px;").arg(tm.mutedColor()));
    QLabel *body = m_bodyLabel; // keep the rest of the function unchanged
    auto *bodyRow = new QHBoxLayout;
    bodyRow->addStretch();
    bodyRow->addWidget(body);
    bodyRow->addStretch();
    root->addLayout(bodyRow);
    root->addSpacing(20);

    auto *btnRow = new QHBoxLayout;
    btnRow->setSpacing(8);
    btnRow->addStretch();

    const QString primaryQss = QString(
        "QPushButton {"
        "  background: %1; color: #ffffff;"
        "  border: 1px solid %1; border-radius: 6px;"
        "  padding: 0 18px; min-height: 32px;"
        "  font-size: 12px; font-weight: 500;"
        "}"
        "QPushButton:hover { background: %2; border-color: %2; }"
        ).arg(tm.accentColor(), tm.accentLightColor());
    const QString ghostQss = QString(
        "QPushButton {"
        "  background: transparent; color: %1;"
        "  border: 1px solid %2; border-radius: 6px;"
        "  padding: 0 18px; min-height: 32px;"
        "  font-size: 12px; font-weight: 500;"
        "}"
        "QPushButton:hover { background: %3; border-color: %4; }"
        ).arg(tm.textColor(), tm.borderColor(),
              tm.surfaceColor(), tm.borderStrongColor());

    auto *openBtn = new QPushButton(tr_("empty_open_btn"));
    openBtn->setCursor(Qt::PointingHandCursor);
    openBtn->setStyleSheet(primaryQss);
    connect(openBtn, &QPushButton::clicked, this, &BatWidget::openFileRequested);
    btnRow->addWidget(openBtn);

    auto *magnetBtn = new QPushButton(tr_("empty_paste_btn"));
    magnetBtn->setCursor(Qt::PointingHandCursor);
    magnetBtn->setStyleSheet(ghostQss);
    connect(magnetBtn, &QPushButton::clicked, this, &BatWidget::pasteMagnetRequested);
    btnRow->addWidget(magnetBtn);

    auto *searchBtn = new QPushButton(tr_("empty_search_btn"));
    searchBtn->setCursor(Qt::PointingHandCursor);
    searchBtn->setStyleSheet(ghostQss);
    connect(searchBtn, &QPushButton::clicked, this, &BatWidget::openSearchRequested);
    btnRow->addWidget(searchBtn);

    btnRow->addStretch();
    root->addLayout(btnRow);
    root->addSpacing(20);

    auto *hintRow = new QHBoxLayout;
    hintRow->setSpacing(6);
    hintRow->addStretch();
#ifdef Q_OS_MACOS
    hintRow->addWidget(makeKeycap(QStringLiteral("⌘O"), this));
#else
    hintRow->addWidget(makeKeycap(QStringLiteral("Ctrl+O"), this));
#endif
    auto *toOpen = new QLabel(tr_("empty_hint_open"));
    toOpen->setStyleSheet(QString("color: %1; font-size: 11px;").arg(tm.dimColor()));
    hintRow->addWidget(toOpen);
    hintRow->addSpacing(20);
#ifdef Q_OS_MACOS
    hintRow->addWidget(makeKeycap(QStringLiteral("⌘V"), this));
#else
    hintRow->addWidget(makeKeycap(QStringLiteral("Ctrl+V"), this));
#endif
    auto *toPaste = new QLabel(tr_("empty_hint_paste"));
    toPaste->setStyleSheet(QString("color: %1; font-size: 11px;").arg(tm.dimColor()));
    hintRow->addWidget(toPaste);
    hintRow->addStretch();
    root->addLayout(hintRow);

    root->addStretch(3);
}

void BatWidget::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);
}

void BatWidget::setCustomMessage(const QString &title, const QString &body)
{
    if (!m_titleLabel || !m_bodyLabel) return;
    m_titleLabel->setText(title.isEmpty() ? m_defaultTitle : title);
    m_bodyLabel->setText(body.isEmpty() ? m_defaultBody : body);
}
