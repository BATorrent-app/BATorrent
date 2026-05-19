// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "addondialog.h"
#include "../app/addonmanager.h"
#include "../app/translator.h"
#include "thememanager.h"

#include <QCheckBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

namespace {

QLabel *makeEyebrow(const QString &text, QWidget *parent)
{
    const auto &tm = ThemeManager::instance();
    auto *lbl = new QLabel(text.toUpper(), parent);
    QFont f; f.setPointSize(8); f.setWeight(QFont::Bold);
    f.setLetterSpacing(QFont::AbsoluteSpacing, 1.4);
    lbl->setFont(f);
    lbl->setStyleSheet(QString("color: %1; background: transparent;").arg(tm.dimColor()));
    return lbl;
}

QFrame *makeCard()
{
    const auto &tm = ThemeManager::instance();
    auto *card = new QFrame;
    card->setObjectName(QStringLiteral("addonCard"));
    card->setStyleSheet(QString(
        "QFrame#addonCard { background: %1; border: none; border-radius: 8px; }"
        ).arg(tm.panelColor()));
    return card;
}

QFont monoFont(int pointSize)
{
    QFont f("Menlo");
    f.setStyleHint(QFont::Monospace);
    f.setPointSize(pointSize);
    return f;
}

} // namespace

AddonDialog::AddonDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr_("addon_title"));
    setMinimumSize(620, 520);
    resize(680, 580);

    const auto &tm = ThemeManager::instance();

    setStyleSheet(QString(
        "QDialog {"
        "  background: qradialgradient(cx:0.5, cy:0, radius:0.7,"
        "      stop:0 %10,"
        "      stop:1 %1);"
        "  color: %2;"
        "}"
        "QLabel { background: transparent; color: %2; }"
        "QScrollArea { background: transparent; border: none; }"
        "QScrollArea > QWidget > QWidget { background: transparent; }"
        "QLineEdit {"
        "  background: %3; color: %2;"
        "  border: 1px solid %4; border-radius: 6px;"
        "  padding: 7px 10px; font-size: 11px;"
        "  selection-background-color: %5;"
        "}"
        "QLineEdit:focus { border-color: %5; }"
        "QLineEdit:disabled { color: %8; background: %7; }"
        "QListWidget {"
        "  background: %7; color: %2;"
        "  border: 1px solid %4; border-radius: 6px;"
        "  alternate-background-color: %3; outline: none;"
        "}"
        "QListWidget::item { padding: 10px 12px; border-bottom: 1px solid %4; }"
        "QListWidget::item:selected { background: %6; color: %2; }"
        "QCheckBox { color: %2; spacing: 8px; font-size: 11px; background: transparent; }"
        "QCheckBox::indicator {"
        "  width: 14px; height: 14px;"
        "  border: 1px solid %4; border-radius: 4px;"
        "  background-color: %3;"
        "}"
        "QCheckBox::indicator:checked { background-color: %5; border-color: %5; }"
        "#primaryBtn {"
        "  background: %5; color: #ffffff;"
        "  border: none; border-radius: 6px;"
        "  padding: 8px 22px; font-size: 11px; font-weight: 600;"
        "}"
        "#primaryBtn:hover { background: %9; }"
        "#primaryBtn:disabled { background: %4; color: %8; }"
        "#ghostBtn {"
        "  background: transparent; color: %2;"
        "  border: 1px solid %4; border-radius: 6px;"
        "  padding: 8px 18px; font-size: 11px; font-weight: 500;"
        "}"
        "#ghostBtn:hover { background: %3; }"
        "#ghostBtn:disabled { color: %8; border-color: %4; }"
        "#dangerBtn {"
        "  background: transparent; color: %2;"
        "  border: 1px solid %4; border-radius: 6px;"
        "  padding: 8px 18px; font-size: 11px; font-weight: 500;"
        "}"
        "#dangerBtn:hover { background: rgba(220,38,38,0.12); border-color: %5; color: %5; }"
        "#dangerBtn:disabled { color: %8; border-color: %4; }"
        "#chipBtn {"
        "  background: %3; color: %2;"
        "  border: 1px solid %4; border-radius: 999px;"
        "  padding: 5px 14px; font-size: 10px; font-weight: 600;"
        "}"
        "#chipBtn:hover { background: %6; border-color: %5; color: %5; }"
        "#chipBtn:disabled { background: transparent; color: %8; border-color: %4; }"
        ).arg(tm.bgColor(), tm.textColor(), tm.surfaceColor(),
              tm.borderColor(), tm.accentColor(), tm.accentTintColor(),
              tm.panelColor(), tm.dimColor(), tm.accentLightColor(),
              tm.accentTintForGradient(10)));

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(32, 28, 32, 24);
    root->setSpacing(0);

    auto *eyebrow = new QLabel(tr_("addon_eyebrow").toUpper());
    {
        QFont f; f.setPointSize(8); f.setWeight(QFont::Bold);
        f.setLetterSpacing(QFont::AbsoluteSpacing, 1.2);
        eyebrow->setFont(f);
        eyebrow->setStyleSheet(QString("color: %1;").arg(tm.accentColor()));
    }
    root->addWidget(eyebrow);
    root->addSpacing(6);

    auto *heading = new QLabel(tr_("addon_heading"));
    {
        QFont f; f.setPointSize(18); f.setWeight(QFont::Bold);
        f.setLetterSpacing(QFont::AbsoluteSpacing, -0.3);
        heading->setFont(f);
        heading->setStyleSheet(QString("color: %1;").arg(tm.textColor()));
    }
    root->addWidget(heading);
    root->addSpacing(4);

    auto *subtitle = new QLabel(tr_("addon_subtitle"));
    subtitle->setWordWrap(true);
    {
        QFont f; f.setPointSize(11);
        subtitle->setFont(f);
        subtitle->setStyleSheet(QString("color: %1;").arg(tm.mutedColor()));
    }
    root->addWidget(subtitle);
    root->addSpacing(18);

    auto *scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    auto *content = new QWidget;
    auto *col = new QVBoxLayout(content);
    col->setContentsMargins(0, 0, 6, 0);
    col->setSpacing(16);

    // --- Installed addons card ---
    auto *installedCard = makeCard();
    auto *installedCol = new QVBoxLayout(installedCard);
    installedCol->setContentsMargins(18, 16, 18, 16);
    installedCol->setSpacing(10);

    auto *installedHeader = new QHBoxLayout;
    installedHeader->setSpacing(8);
    installedHeader->addWidget(makeEyebrow(tr_("addon_installed"), installedCard));
    installedHeader->addStretch();
    auto *removeBtn = new QPushButton(tr_("action_remove"));
    removeBtn->setObjectName(QStringLiteral("dangerBtn"));
    removeBtn->setCursor(Qt::PointingHandCursor);
    removeBtn->setEnabled(false);
    connect(removeBtn, &QPushButton::clicked, this, &AddonDialog::removeSelectedAddon);
    installedHeader->addWidget(removeBtn);
    installedCol->addLayout(installedHeader);

    m_addonList = new QListWidget;
    m_addonList->setMinimumHeight(140);
    m_addonList->setAlternatingRowColors(true);
    connect(m_addonList, &QListWidget::itemSelectionChanged, this, [this, removeBtn]() {
        removeBtn->setEnabled(m_addonList->currentRow() >= 0);
    });
    installedCol->addWidget(m_addonList);

    auto *emptyHint = new QLabel(tr_("addon_empty_hint"));
    emptyHint->setWordWrap(true);
    emptyHint->setObjectName(QStringLiteral("addonEmptyHint"));
    emptyHint->setStyleSheet(QString("color: %1; font-size: 11px;").arg(tm.mutedColor()));
    installedCol->addWidget(emptyHint);

    col->addWidget(installedCard);

    // --- Install via URL card ---
    auto *installCard = makeCard();
    auto *installCol = new QVBoxLayout(installCard);
    installCol->setContentsMargins(18, 16, 18, 16);
    installCol->setSpacing(10);

    installCol->addWidget(makeEyebrow(tr_("addon_install"), installCard));

    auto *installRow = new QHBoxLayout;
    installRow->setSpacing(8);
    m_urlEdit = new QLineEdit;
    m_urlEdit->setPlaceholderText(tr_("addon_url_hint"));
    m_urlEdit->setFont(monoFont(10));
    installRow->addWidget(m_urlEdit, 1);

    auto *installBtn = new QPushButton(tr_("addon_install_btn"));
    installBtn->setObjectName(QStringLiteral("primaryBtn"));
    installBtn->setCursor(Qt::PointingHandCursor);
    connect(installBtn, &QPushButton::clicked, this, &AddonDialog::installAddon);
    connect(m_urlEdit, &QLineEdit::returnPressed, this, &AddonDialog::installAddon);
    installRow->addWidget(installBtn);
    installCol->addLayout(installRow);

    col->addWidget(installCard);

    // --- Suggested addons card ---
    auto *suggestCard = makeCard();
    auto *suggestCol = new QVBoxLayout(suggestCard);
    suggestCol->setContentsMargins(18, 16, 18, 16);
    suggestCol->setSpacing(12);

    suggestCol->addWidget(makeEyebrow(tr_("addon_suggested"), suggestCard));

    auto *suggestHint = new QLabel(tr_("addon_suggest_hint"));
    suggestHint->setWordWrap(true);
    suggestHint->setStyleSheet(QString("color: %1; font-size: 11px;").arg(tm.mutedColor()));
    suggestCol->addWidget(suggestHint);

    struct SuggestedAddon { QString name, desc, url; };
    const QList<SuggestedAddon> suggestions = {
        {"Cinemeta", "Official movie & series catalog", "https://v3-cinemeta.strem.io"},
        {"Torrentio", "Torrent streams from multiple providers", "https://torrentio.strem.fun"},
    };

    for (const auto &s : suggestions) {
        auto *row = new QHBoxLayout;
        row->setSpacing(10);

        auto *textCol = new QVBoxLayout;
        textCol->setSpacing(2);

        auto *nameLbl = new QLabel(s.name);
        { QFont f; f.setPointSize(12); f.setWeight(QFont::DemiBold); nameLbl->setFont(f); }
        nameLbl->setStyleSheet(QString("color: %1;").arg(tm.textColor()));
        textCol->addWidget(nameLbl);

        auto *descLbl = new QLabel(s.desc);
        descLbl->setWordWrap(true);
        descLbl->setStyleSheet(QString("color: %1; font-size: 11px;").arg(tm.mutedColor()));
        textCol->addWidget(descLbl);

        row->addLayout(textCol, 1);

        auto *btn = new QPushButton(tr_("addon_install_btn"));
        btn->setObjectName(QStringLiteral("chipBtn"));
        btn->setCursor(Qt::PointingHandCursor);
        const QString url = s.url;
        connect(btn, &QPushButton::clicked, this, [btn, url]() {
            AddonManager::instance().addAddon(url);
            btn->setEnabled(false);
            btn->setText(tr_("addon_installed_chip"));
        });
        for (const auto &a : AddonManager::instance().addons()) {
            if (a.url == s.url) {
                btn->setEnabled(false);
                btn->setText(tr_("addon_installed_chip"));
                break;
            }
        }
        row->addWidget(btn, 0, Qt::AlignVCenter);

        suggestCol->addLayout(row);
    }

    col->addWidget(suggestCard);

    // --- Auto trackers card ---
    auto *trackerCard = makeCard();
    auto *trackerCol = new QVBoxLayout(trackerCard);
    trackerCol->setContentsMargins(18, 16, 18, 16);
    trackerCol->setSpacing(10);

    trackerCol->addWidget(makeEyebrow(tr_("addon_trackers_group"), trackerCard));

    m_autoTrackersCheck = new QCheckBox(tr_("addon_auto_trackers"));
    m_autoTrackersCheck->setChecked(AddonManager::instance().autoTrackersEnabled());
    connect(m_autoTrackersCheck, &QCheckBox::toggled, this, [](bool checked) {
        AddonManager::instance().setAutoTrackersEnabled(checked);
    });
    trackerCol->addWidget(m_autoTrackersCheck);

    const int trackerCount = AddonManager::instance().trackerList().size();
    auto *trackerInfo = new QLabel(tr_("addon_tracker_count").arg(trackerCount));
    trackerInfo->setFont(monoFont(9));
    trackerInfo->setStyleSheet(QString("color: %1;").arg(tm.mutedColor()));
    trackerCol->addWidget(trackerInfo);

    col->addWidget(trackerCard);

    // --- Torrent search card ---
    auto *searchCard = makeCard();
    auto *searchCol = new QVBoxLayout(searchCard);
    searchCol->setContentsMargins(18, 16, 18, 16);
    searchCol->setSpacing(10);

    searchCol->addWidget(makeEyebrow(tr_("addon_torrent_search_group"), searchCard));

    m_torrentSearchCheck = new QCheckBox(tr_("addon_torrent_search_enable"));
    m_torrentSearchCheck->setChecked(AddonManager::instance().torrentSearchEnabled());
    searchCol->addWidget(m_torrentSearchCheck);

    auto *urlLabel = new QLabel(tr_("addon_torrent_search_url"));
    urlLabel->setStyleSheet(QString("color: %1; font-size: 11px;").arg(tm.mutedColor()));
    searchCol->addWidget(urlLabel);

    m_torrentSearchUrlEdit = new QLineEdit;
    m_torrentSearchUrlEdit->setText(AddonManager::instance().torrentSearchUrl());
    m_torrentSearchUrlEdit->setPlaceholderText(tr_("addon_torrent_search_url_hint"));
    m_torrentSearchUrlEdit->setEnabled(AddonManager::instance().torrentSearchEnabled());
    m_torrentSearchUrlEdit->setFont(monoFont(10));
    connect(m_torrentSearchCheck, &QCheckBox::toggled, this, [this](bool checked) {
        AddonManager::instance().setTorrentSearchEnabled(checked);
        m_torrentSearchUrlEdit->setEnabled(checked);
    });
    connect(m_torrentSearchUrlEdit, &QLineEdit::editingFinished, this, [this]() {
        AddonManager::instance().setTorrentSearchUrl(m_torrentSearchUrlEdit->text().trimmed());
    });
    searchCol->addWidget(m_torrentSearchUrlEdit);

    auto *searchHint = new QLabel(tr_("addon_torrent_search_hint"));
    searchHint->setWordWrap(true);
    searchHint->setStyleSheet(QString("color: %1; font-size: 11px;").arg(tm.mutedColor()));
    searchCol->addWidget(searchHint);

    col->addWidget(searchCard);

    col->addStretch(1);
    scroll->setWidget(content);
    root->addWidget(scroll, 1);

    root->addSpacing(16);

    auto *footer = new QHBoxLayout;
    footer->setSpacing(8);
    footer->addStretch();
    auto *closeBtn = new QPushButton(tr_("btn_ok"));
    closeBtn->setObjectName(QStringLiteral("primaryBtn"));
    closeBtn->setCursor(Qt::PointingHandCursor);
    closeBtn->setDefault(true);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    footer->addWidget(closeBtn);
    root->addLayout(footer);

    connect(&AddonManager::instance(), &AddonManager::addonAdded, this, [this](const AddonManifest &) {
        refreshList();
    });
    connect(&AddonManager::instance(), &AddonManager::addonError, this, [this](const QString &err) {
        QMessageBox::warning(this, tr_("dlg_error"), err);
    });

    refreshList();
}

void AddonDialog::installAddon()
{
    const QString url = m_urlEdit->text().trimmed();
    if (url.isEmpty()) return;
    AddonManager::instance().addAddon(url);
    m_urlEdit->clear();
}

void AddonDialog::removeSelectedAddon()
{
    const int row = m_addonList->currentRow();
    if (row < 0) return;
    AddonManager::instance().removeAddon(row);
    refreshList();
}

void AddonDialog::refreshList()
{
    m_addonList->clear();
    const auto addons = AddonManager::instance().addons();
    for (const auto &a : addons) {
        QString types = a.types.join(", ");
        QString text = QString("%1\n%2").arg(a.name, a.description);
        if (!types.isEmpty())
            text += QString("\n%1 · %2").arg(tr_("addon_types_label"), types);
        m_addonList->addItem(text);
    }
    if (auto *hint = findChild<QLabel *>(QStringLiteral("addonEmptyHint")))
        hint->setVisible(addons.isEmpty());
}
