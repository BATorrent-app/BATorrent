// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "welcomedialog.h"
#include "../app/translator.h"
#include "../gui/thememanager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QPixmap>
#include <QFrame>
#include <QMouseEvent>
#include <QEvent>
#include <QGraphicsDropShadowEffect>
#include <functional>

namespace {

// Click-detecting card frame. We need full layout control (icon top, title,
// description, packed tight) which QToolButton's internal icon/text engine
// fights against — using a QFrame keeps the embedded QVBoxLayout authoritative.
class ActionCard : public QFrame
{
public:
    ActionCard(const QString &iconPath,
               const QString &title,
               const QString &desc,
               std::function<void()> onClick,
               QWidget *parent)
        : QFrame(parent), m_onClick(std::move(onClick))
    {
        const auto &tm = ThemeManager::instance();
        setCursor(Qt::PointingHandCursor);
        setObjectName(QStringLiteral("actionCard"));
        setFixedHeight(106);
        m_baseBg = tm.surfaceColor();
        m_hoverBg = tm.panelColor();
        applyStyle(false);

        auto *col = new QVBoxLayout(this);
        col->setContentsMargins(18, 16, 18, 16);
        col->setSpacing(4);

        auto *icon = new QLabel(this);
        QPixmap pm(iconPath);
        icon->setPixmap(pm.scaled(16, 16, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        icon->setFixedSize(16, 16);
        icon->setStyleSheet(QStringLiteral("background: transparent;"));
        col->addWidget(icon);

        col->addSpacing(10);

        auto *titleLbl = new QLabel(title, this);
        {
            QFont f; f.setPointSize(11); f.setWeight(QFont::DemiBold);
            titleLbl->setFont(f);
        }
        titleLbl->setStyleSheet(QString("color: %1; background: transparent;").arg(tm.textColor()));
        col->addWidget(titleLbl);

        auto *descLbl = new QLabel(desc, this);
        {
            QFont f; f.setPointSize(9);
            descLbl->setFont(f);
        }
        descLbl->setStyleSheet(QString("color: %1; background: transparent;").arg(tm.mutedColor()));
        col->addWidget(descLbl);

        col->addStretch();
    }

protected:
    void mousePressEvent(QMouseEvent *e) override
    {
        if (e->button() == Qt::LeftButton && m_onClick) m_onClick();
        QFrame::mousePressEvent(e);
    }
    void enterEvent(QEnterEvent *e) override
    {
        applyStyle(true);
        QFrame::enterEvent(e);
    }
    void leaveEvent(QEvent *e) override
    {
        applyStyle(false);
        QFrame::leaveEvent(e);
    }

private:
    void applyStyle(bool hover)
    {
        setStyleSheet(QString(
            "QFrame#actionCard {"
            "  background: %1;"
            "  border: none; border-radius: 6px;"
            "}"
            ).arg(hover ? m_hoverBg : m_baseBg));
    }

    std::function<void()> m_onClick;
    QString m_baseBg;
    QString m_hoverBg;
};

} // namespace

WelcomeDialog::WelcomeDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr_("welcome_window_title"));
    setFixedSize(620, 560);

    const auto &tm = ThemeManager::instance();

    setStyleSheet(QString(
        "QDialog {"
        "  background: qradialgradient(cx:0.5, cy:0, radius:0.7,"
        "      stop:0 %7,"
        "      stop:1 %1);"
        "  color: %2;"
        "}"
        "QLabel { background: transparent; }"
        "QCheckBox { color: %3; spacing: 8px; font-size: 11px; background: transparent; }"
        "QCheckBox::indicator {"
        "  width: 14px; height: 14px;"
        "  border: 1px solid %4; border-radius: 4px;"
        "  background-color: %5;"
        "}"
        "QCheckBox::indicator:checked { background-color: %6; border-color: %6; }"
        "#closeBtn {"
        "  background: transparent; color: %3;"
        "  border: 1px solid %4; border-radius: 6px;"
        "  padding: 6px 18px; font-size: 11px; font-weight: 500;"
        "}"
        "#closeBtn:hover { background: %5; color: %2; }"
        ).arg(tm.bgColor(), tm.textColor(), tm.mutedColor(),
              tm.borderColor(), tm.surfaceColor(), tm.accentColor(),
              tm.accentTintForGradient(16)));

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(48, 36, 48, 24);
    root->setSpacing(0);

    auto *logoLabel = new QLabel;
    logoLabel->setPixmap(tm.themedLogo(80, 2.0));
    logoLabel->setFixedSize(80, 80);
    logoLabel->setScaledContents(true);
    logoLabel->setAlignment(Qt::AlignCenter);
    auto *halo = new QGraphicsDropShadowEffect;
    halo->setColor(QColor(220, 38, 38, 90));
    halo->setBlurRadius(48);
    halo->setOffset(0, 0);
    logoLabel->setGraphicsEffect(halo);
    auto *logoRow = new QHBoxLayout;
    logoRow->addStretch();
    logoRow->addWidget(logoLabel);
    logoRow->addStretch();
    root->addLayout(logoRow);
    root->addSpacing(16);

    auto *eyebrow = new QLabel(tr_("welcome_eyebrow"));
    eyebrow->setAlignment(Qt::AlignCenter);
    {
        QFont f; f.setPointSize(8); f.setWeight(QFont::Bold);
        f.setLetterSpacing(QFont::AbsoluteSpacing, 1.2);
        eyebrow->setFont(f);
        eyebrow->setStyleSheet(QString("color: %1;").arg(tm.accentColor()));
    }
    root->addWidget(eyebrow);
    root->addSpacing(6);

    auto *heading = new QLabel(tr_("welcome_heading"));
    heading->setAlignment(Qt::AlignCenter);
    {
        QFont f; f.setPointSize(18); f.setWeight(QFont::Bold);
        f.setLetterSpacing(QFont::AbsoluteSpacing, -0.3);
        heading->setFont(f);
        heading->setStyleSheet(QString("color: %1;").arg(tm.textColor()));
    }
    root->addWidget(heading);
    root->addSpacing(8);

    auto *subtitle = new QLabel(tr_("welcome_subtitle_text"));
    subtitle->setAlignment(Qt::AlignCenter);
    subtitle->setWordWrap(true);
    subtitle->setMaximumWidth(380);
    {
        QFont f; f.setPointSize(11);
        subtitle->setFont(f);
        subtitle->setStyleSheet(QString("color: %1;").arg(tm.mutedColor()));
    }
    auto *subRow = new QHBoxLayout;
    subRow->addStretch();
    subRow->addWidget(subtitle);
    subRow->addStretch();
    root->addLayout(subRow);
    root->addSpacing(24);

    auto *grid = new QGridLayout;
    grid->setHorizontalSpacing(12);
    grid->setVerticalSpacing(12);
    grid->setContentsMargins(0, 0, 0, 0);

    auto *openCard = new ActionCard(
        QStringLiteral(":/icons/open.svg"),
        tr_("welcome_card_open_title"),
        tr_("welcome_card_open_desc"),
        [this]() { emit openFileRequested(); accept(); },
        this);

    auto *magnetCard = new ActionCard(
        QStringLiteral(":/icons/magnet.svg"),
        tr_("welcome_card_magnet_title"),
        tr_("welcome_card_magnet_desc"),
        [this]() { emit pasteMagnetRequested(); accept(); },
        this);

    auto *searchCard = new ActionCard(
        QStringLiteral(":/icons/search.svg"),
        tr_("welcome_card_search_title"),
        tr_("welcome_card_search_desc"),
        [this]() { emit openSearchRequested(); accept(); },
        this);

    auto *rssCard = new ActionCard(
        QStringLiteral(":/icons/rss.svg"),
        tr_("welcome_card_rss_title"),
        tr_("welcome_card_rss_desc"),
        [this]() { emit openRssRequested(); accept(); },
        this);

    grid->addWidget(openCard,   0, 0);
    grid->addWidget(magnetCard, 0, 1);
    grid->addWidget(searchCard, 1, 0);
    grid->addWidget(rssCard,    1, 1);
    root->addLayout(grid);
    root->addStretch();

    auto *bottom = new QHBoxLayout;
    m_dontShowCheck = new QCheckBox(tr_("welcome_dont_show"));
    bottom->addWidget(m_dontShowCheck);
    bottom->addStretch();

    auto *closeBtn = new QPushButton(tr_("welcome_close"));
    closeBtn->setObjectName(QStringLiteral("closeBtn"));
    closeBtn->setCursor(Qt::PointingHandCursor);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    bottom->addWidget(closeBtn);

    root->addLayout(bottom);
}

bool WelcomeDialog::dontShowAgain() const
{
    return m_dontShowCheck->isChecked();
}
