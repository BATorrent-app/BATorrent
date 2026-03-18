// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "welcomedialog.h"
#include "../app/translator.h"
#include "../gui/thememanager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QPixmap>

WelcomeDialog::WelcomeDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("BATorrent");
    setFixedSize(520, 540);

    const auto &tm = ThemeManager::instance();
    QString bg = tm.bgColor();
    QString sf = tm.surfaceColor();
    QString tx = tm.textColor();
    QString mt = tm.mutedColor();
    QString ac = tm.accentColor();
    QString acDk = tm.accentDarkColor();
    QString bd = tm.borderColor();

    setStyleSheet(QString(R"(
        QDialog { background-color: %1; color: %2; }
        QLabel { color: %2; }
        QPushButton {
            background-color: %5; color: #ffffff;
            border: none; border-radius: 8px;
            padding: 12px 36px; font-size: 14px; font-weight: 600;
        }
        QPushButton:hover { background-color: %7; }
        QCheckBox { color: %4; spacing: 8px; font-size: 11px; }
        QCheckBox::indicator {
            width: 14px; height: 14px;
            border: 1px solid %6; border-radius: 4px;
            background-color: %3;
        }
        QCheckBox::indicator:checked { background-color: %5; border-color: %5; }
    )").arg(bg, tx, sf, mt, ac, bd, tm.accentLightColor()));

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(36, 28, 36, 28);
    mainLayout->setSpacing(10);

    // Logo
    auto *logoLabel = new QLabel;
    QPixmap logo(":/images/logo1.png");
    logoLabel->setPixmap(logo.scaled(80, 80, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    logoLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(logoLabel);

    // Title
    auto *titleLabel = new QLabel(tr_("welcome_title"));
    titleLabel->setStyleSheet(QString("font-size: 22px; font-weight: 700; color: %1;").arg(tx));
    titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleLabel);

    auto *subtitleLabel = new QLabel(tr_("welcome_subtitle"));
    subtitleLabel->setStyleSheet(QString("font-size: 12px; color: %1; margin-bottom: 14px;").arg(mt));
    subtitleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(subtitleLabel);

    mainLayout->addSpacing(4);

    // Steps
    auto addStep = [&](const QString &num, const QString &titleKey, const QString &descKey) {
        auto *stepLayout = new QHBoxLayout;
        stepLayout->setSpacing(14);

        auto *numLabel = new QLabel(num);
        numLabel->setFixedSize(34, 34);
        numLabel->setAlignment(Qt::AlignCenter);
        numLabel->setStyleSheet(QString(
            "background-color: %1; color: %2; border-radius: 17px;"
            "font-size: 14px; font-weight: 700;").arg(acDk, ac));

        auto *textLayout = new QVBoxLayout;
        auto *stepTitle = new QLabel(tr_(titleKey));
        stepTitle->setStyleSheet(QString("font-size: 13px; font-weight: 600; color: %1;").arg(tx));
        auto *stepDesc = new QLabel(tr_(descKey));
        stepDesc->setWordWrap(true);
        stepDesc->setStyleSheet(QString("font-size: 11px; color: %1; line-height: 1.4;").arg(mt));

        textLayout->addWidget(stepTitle);
        textLayout->addWidget(stepDesc);
        textLayout->setSpacing(3);

        stepLayout->addWidget(numLabel, 0, Qt::AlignTop);
        stepLayout->addLayout(textLayout, 1);

        mainLayout->addLayout(stepLayout);
    };

    addStep("1", "welcome_step1_title", "welcome_step1_desc");
    addStep("2", "welcome_step2_title", "welcome_step2_desc");
    addStep("3", "welcome_step3_title", "welcome_step3_desc");

    mainLayout->addStretch();

    // Bottom
    m_dontShowCheck = new QCheckBox(tr_("welcome_dont_show"));

    auto *btnLayout = new QHBoxLayout;
    btnLayout->addWidget(m_dontShowCheck);
    btnLayout->addStretch();
    auto *gotItBtn = new QPushButton(tr_("welcome_got_it"));
    connect(gotItBtn, &QPushButton::clicked, this, &QDialog::accept);
    btnLayout->addWidget(gotItBtn);

    mainLayout->addLayout(btnLayout);
}

bool WelcomeDialog::dontShowAgain() const
{
    return m_dontShowCheck->isChecked();
}
