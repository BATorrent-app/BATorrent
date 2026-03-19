// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "addondialog.h"
#include "../app/addonmanager.h"
#include "../app/translator.h"
#include "thememanager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QLabel>
#include <QMessageBox>
#include <QGroupBox>

AddonDialog::AddonDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr_("addon_title"));
    setMinimumSize(550, 400);
    setStyleSheet(ThemeManager::instance().dialogStyleSheet());

    auto &tm = ThemeManager::instance();
    auto *layout = new QVBoxLayout(this);

    // Auto trackers
    auto *trackerGroup = new QGroupBox(tr_("addon_trackers_group"));
    auto *trackerLayout = new QVBoxLayout(trackerGroup);
    m_autoTrackersCheck = new QCheckBox(tr_("addon_auto_trackers"));
    m_autoTrackersCheck->setChecked(AddonManager::instance().autoTrackersEnabled());
    connect(m_autoTrackersCheck, &QCheckBox::toggled, this, [](bool checked) {
        AddonManager::instance().setAutoTrackersEnabled(checked);
    });
    trackerLayout->addWidget(m_autoTrackersCheck);
    int trackerCount = AddonManager::instance().trackerList().size();
    auto *trackerInfo = new QLabel(tr_("addon_tracker_count").arg(trackerCount));
    trackerInfo->setStyleSheet("color: #888;");
    trackerLayout->addWidget(trackerInfo);
    layout->addWidget(trackerGroup);

    // Installed addons
    auto *addonGroup = new QGroupBox(tr_("addon_installed"));
    auto *addonLayout = new QVBoxLayout(addonGroup);

    m_addonList = new QListWidget;
    m_addonList->setStyleSheet(QString(
        "QListWidget { background: %1; color: %2; border: 1px solid %3; border-radius: 6px; }"
        "QListWidget::item { padding: 8px; }"
        "QListWidget::item:selected { background: %4; }")
        .arg(tm.surfaceColor(), tm.textColor(), tm.borderColor(), tm.accentColor()));
    addonLayout->addWidget(m_addonList);

    auto *removeBtn = new QPushButton(tr_("addon_remove"));
    removeBtn->setFixedWidth(120);
    connect(removeBtn, &QPushButton::clicked, this, &AddonDialog::removeSelectedAddon);
    addonLayout->addWidget(removeBtn);
    layout->addWidget(addonGroup);

    // Install new addon
    auto *installGroup = new QGroupBox(tr_("addon_install"));
    auto *installLayout = new QHBoxLayout(installGroup);

    m_urlEdit = new QLineEdit;
    m_urlEdit->setPlaceholderText(tr_("addon_url_hint"));
    m_urlEdit->setStyleSheet(QString(
        "QLineEdit { background: %1; color: %2; border: 1px solid %3;"
        "border-radius: 6px; padding: 6px 10px; }")
        .arg(tm.surfaceColor(), tm.textColor(), tm.borderColor()));
    installLayout->addWidget(m_urlEdit);

    auto *installBtn = new QPushButton(tr_("addon_install_btn"));
    installBtn->setFixedWidth(100);
    connect(installBtn, &QPushButton::clicked, this, &AddonDialog::installAddon);
    connect(m_urlEdit, &QLineEdit::returnPressed, this, &AddonDialog::installAddon);
    installLayout->addWidget(installBtn);
    layout->addWidget(installGroup);

    // Suggested addons
    auto *suggestGroup = new QGroupBox(tr_("addon_suggested"));
    auto *suggestLayout = new QVBoxLayout(suggestGroup);
    auto *suggestHint = new QLabel(tr_("addon_suggest_hint"));
    suggestHint->setStyleSheet("color: #888;");
    suggestLayout->addWidget(suggestHint);

    struct SuggestedAddon { QString name, desc, url; };
    QList<SuggestedAddon> suggestions = {
        {"Cinemeta", "Official movie & series catalog", "https://v3-cinemeta.strem.io"},
        {"Torrentio", "Torrent streams from multiple providers", "https://torrentio.strem.fun"},
    };
    for (const auto &s : suggestions) {
        auto *row = new QHBoxLayout;
        auto *label = new QLabel(QString("<b>%1</b> — %2").arg(s.name, s.desc));
        label->setStyleSheet(QString("color: %1;").arg(tm.textColor()));
        row->addWidget(label, 1);
        auto *btn = new QPushButton(tr_("addon_install_btn"));
        btn->setFixedWidth(80);
        QString url = s.url;
        connect(btn, &QPushButton::clicked, this, [this, btn, url]() {
            AddonManager::instance().addAddon(url);
            btn->setEnabled(false);
            btn->setText("✓");
        });
        // Disable if already installed
        for (const auto &a : AddonManager::instance().addons()) {
            if (a.url == s.url) { btn->setEnabled(false); btn->setText("✓"); break; }
        }
        row->addWidget(btn);
        suggestLayout->addLayout(row);
    }
    layout->addWidget(suggestGroup);

    // Torrent Search (opt-in)
    auto *searchGroup = new QGroupBox(tr_("addon_torrent_search_group"));
    auto *searchGroupLayout = new QVBoxLayout(searchGroup);

    m_torrentSearchCheck = new QCheckBox(tr_("addon_torrent_search_enable"));
    m_torrentSearchCheck->setChecked(AddonManager::instance().torrentSearchEnabled());
    connect(m_torrentSearchCheck, &QCheckBox::toggled, this, [this](bool checked) {
        AddonManager::instance().setTorrentSearchEnabled(checked);
        m_torrentSearchUrlEdit->setEnabled(checked);
    });
    searchGroupLayout->addWidget(m_torrentSearchCheck);

    auto *urlRow = new QHBoxLayout;
    auto *urlLabel = new QLabel(tr_("addon_torrent_search_url"));
    urlLabel->setStyleSheet(QString("color: %1;").arg(tm.textColor()));
    urlRow->addWidget(urlLabel);
    m_torrentSearchUrlEdit = new QLineEdit;
    m_torrentSearchUrlEdit->setText(AddonManager::instance().torrentSearchUrl());
    m_torrentSearchUrlEdit->setPlaceholderText(tr_("addon_torrent_search_url_hint"));
    m_torrentSearchUrlEdit->setEnabled(AddonManager::instance().torrentSearchEnabled());
    m_torrentSearchUrlEdit->setStyleSheet(QString(
        "QLineEdit { background: %1; color: %2; border: 1px solid %3;"
        "border-radius: 6px; padding: 6px 10px; }")
        .arg(tm.surfaceColor(), tm.textColor(), tm.borderColor()));
    connect(m_torrentSearchUrlEdit, &QLineEdit::editingFinished, this, [this]() {
        AddonManager::instance().setTorrentSearchUrl(m_torrentSearchUrlEdit->text().trimmed());
    });
    urlRow->addWidget(m_torrentSearchUrlEdit, 1);
    searchGroupLayout->addLayout(urlRow);

    auto *searchHint = new QLabel(tr_("addon_torrent_search_hint"));
    searchHint->setStyleSheet("color: #888;");
    searchHint->setWordWrap(true);
    searchGroupLayout->addWidget(searchHint);
    layout->addWidget(searchGroup);

    // Close button
    auto *btnLayout = new QHBoxLayout;
    btnLayout->addStretch();
    auto *closeBtn = new QPushButton(tr_("btn_ok"));
    closeBtn->setFixedWidth(100);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    btnLayout->addWidget(closeBtn);
    layout->addLayout(btnLayout);

    // Connect signals
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
    QString url = m_urlEdit->text().trimmed();
    if (url.isEmpty()) return;
    AddonManager::instance().addAddon(url);
    m_urlEdit->clear();
}

void AddonDialog::removeSelectedAddon()
{
    int row = m_addonList->currentRow();
    if (row < 0) return;
    AddonManager::instance().removeAddon(row);
    refreshList();
}

void AddonDialog::refreshList()
{
    m_addonList->clear();
    auto addons = AddonManager::instance().addons();
    for (const auto &a : addons) {
        QString text = QString("%1\n%2\nTypes: %3")
            .arg(a.name, a.description, a.types.join(", "));
        m_addonList->addItem(text);
    }
}
