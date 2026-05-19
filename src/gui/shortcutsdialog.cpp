// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "shortcutsdialog.h"
#include "../app/translator.h"
#include "../gui/thememanager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QFont>
#include <QScrollArea>
#include <QFrame>

namespace {

QLabel *makeKeycap(const QString &text, QWidget *parent)
{
    const auto &tm = ThemeManager::instance();
    auto *cap = new QLabel(text, parent);
    cap->setAlignment(Qt::AlignCenter);
    QFont f("Menlo");
    f.setStyleHint(QFont::Monospace);
    f.setPointSize(10);
    f.setWeight(QFont::DemiBold);
    cap->setFont(f);
    cap->setStyleSheet(QString(
        "QLabel {"
        "  background: %1; color: %2;"
        "  border: 1px solid %3; border-bottom: 2px solid %3;"
        "  border-radius: 4px;"
        "  padding: 3px 8px;"
        "}"
        ).arg(tm.surfaceColor(), tm.textColor(), tm.borderColor()));
    cap->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    return cap;
}

QWidget *makeKeyCluster(const QStringList &keys, QWidget *parent)
{
    auto *w = new QWidget(parent);
    w->setStyleSheet(QStringLiteral("background: transparent;"));
    auto *row = new QHBoxLayout(w);
    row->setContentsMargins(0, 0, 0, 0);
    row->setSpacing(4);
    const auto &tm = ThemeManager::instance();
    for (int i = 0; i < keys.size(); ++i) {
        row->addWidget(makeKeycap(keys[i], w));
        if (i + 1 < keys.size()) {
            auto *plus = new QLabel(QStringLiteral("+"), w);
            QFont pf; pf.setPointSize(9);
            plus->setFont(pf);
            plus->setStyleSheet(QString("color: %1; background: transparent;").arg(tm.dimColor()));
            row->addWidget(plus);
        }
    }
    row->addStretch();
    return w;
}

QLabel *makeSectionEyebrow(const QString &text, QWidget *parent)
{
    const auto &tm = ThemeManager::instance();
    auto *lbl = new QLabel(text.toUpper(), parent);
    QFont f; f.setPointSize(8); f.setWeight(QFont::Bold);
    f.setLetterSpacing(QFont::AbsoluteSpacing, 1.4);
    lbl->setFont(f);
    lbl->setStyleSheet(QString("color: %1; background: transparent;").arg(tm.dimColor()));
    return lbl;
}

} // namespace

ShortcutsDialog::ShortcutsDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr_("shortcuts_title"));
    setFixedSize(560, 540);

    const auto &tm = ThemeManager::instance();

    setStyleSheet(QString(
        "QDialog {"
        "  background: qradialgradient(cx:0.5, cy:0, radius:0.7,"
        "      stop:0 rgba(220,38,38,0.10),"
        "      stop:1 %1);"
        "  color: %2;"
        "}"
        "QLabel { background: transparent; }"
        "QScrollArea { background: transparent; border: none; }"
        "QScrollArea > QWidget > QWidget { background: transparent; }"
        "#closeBtn {"
        "  background: transparent; color: %2;"
        "  border: 1px solid %3; border-radius: 6px;"
        "  padding: 8px 22px; font-size: 11px; font-weight: 500;"
        "}"
        "#closeBtn:hover { background: %4; }"
        ).arg(tm.bgColor(), tm.textColor(), tm.borderColor(), tm.surfaceColor()));

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(36, 32, 36, 24);
    root->setSpacing(0);

    // Header: eyebrow + heading
    auto *eyebrow = new QLabel(tr_("shortcuts_title").toUpper());
    {
        QFont f; f.setPointSize(8); f.setWeight(QFont::Bold);
        f.setLetterSpacing(QFont::AbsoluteSpacing, 1.2);
        eyebrow->setFont(f);
        eyebrow->setStyleSheet(QString("color: %1;").arg(tm.accentColor()));
    }
    root->addWidget(eyebrow);
    root->addSpacing(6);

    auto *heading = new QLabel(tr_("shortcuts_heading"));
    {
        QFont f; f.setPointSize(18); f.setWeight(QFont::Bold);
        f.setLetterSpacing(QFont::AbsoluteSpacing, -0.3);
        heading->setFont(f);
        heading->setStyleSheet(QString("color: %1;").arg(tm.textColor()));
    }
    root->addWidget(heading);
    root->addSpacing(4);

    auto *subtitle = new QLabel(tr_("shortcuts_subtitle"));
    {
        QFont f; f.setPointSize(11);
        subtitle->setFont(f);
        subtitle->setStyleSheet(QString("color: %1;").arg(tm.mutedColor()));
    }
    root->addWidget(subtitle);
    root->addSpacing(22);

#ifdef Q_OS_MAC
    const QString modKey = QStringLiteral("⌘"); // Cmd
#else
    const QString modKey = QStringLiteral("Ctrl");
#endif

    struct Entry { QStringList keys; QString action; };
    struct Section { QString title; QList<Entry> entries; };

    const QList<Section> sections = {
        { tr_("shortcuts_section_torrents"), {
            { {modKey, "O"},  tr_("action_open") },
            { {modKey, "V"},  tr_("action_magnet") },
            { {QStringLiteral("Space")},  tr_("action_pause") + " / " + tr_("action_resume") },
            { {QStringLiteral("Delete")}, tr_("action_remove") },
        }},
        { tr_("shortcuts_section_navigation"), {
            { {modKey, "A"},  tr_("filter_all_active") },
            { {modKey, ","},  tr_("action_settings") },
            { {QStringLiteral("Escape")}, tr_("btn_cancel") },
        }},
    };

    // Scrollable content area for sections
    auto *scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    auto *content = new QWidget;
    auto *contentCol = new QVBoxLayout(content);
    contentCol->setContentsMargins(0, 0, 0, 0);
    contentCol->setSpacing(20);

    for (const auto &section : sections) {
        auto *sectionCol = new QVBoxLayout;
        sectionCol->setSpacing(8);

        sectionCol->addWidget(makeSectionEyebrow(section.title, content));

        for (const auto &entry : section.entries) {
            auto *rowW = new QFrame(content);
            rowW->setStyleSheet(QString(
                "QFrame {"
                "  background: %1; border: none;"
                "  border-radius: 6px;"
                "}"
                ).arg(tm.surfaceColor()));
            auto *row = new QHBoxLayout(rowW);
            row->setContentsMargins(14, 10, 14, 10);
            row->setSpacing(16);

            auto *actionLbl = new QLabel(entry.action, rowW);
            {
                QFont f; f.setPointSize(11);
                actionLbl->setFont(f);
                actionLbl->setStyleSheet(QString("color: %1;").arg(tm.textColor()));
            }
            row->addWidget(actionLbl, 1);

            row->addWidget(makeKeyCluster(entry.keys, rowW), 0, Qt::AlignRight);

            sectionCol->addWidget(rowW);
        }

        contentCol->addLayout(sectionCol);
    }
    contentCol->addStretch();
    scroll->setWidget(content);
    root->addWidget(scroll, 1);

    root->addSpacing(16);

    auto *bottom = new QHBoxLayout;
    bottom->addStretch();
    auto *closeBtn = new QPushButton(tr_("release_notes_close"));
    closeBtn->setObjectName(QStringLiteral("closeBtn"));
    closeBtn->setCursor(Qt::PointingHandCursor);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    bottom->addWidget(closeBtn);
    root->addLayout(bottom);
}
