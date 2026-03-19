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
