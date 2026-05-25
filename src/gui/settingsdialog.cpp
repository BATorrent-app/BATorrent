// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "settingsdialog.h"
#include "pairingdialog.h"
#include "../app/translator.h"
#include "../gui/thememanager.h"
#include <QPalette>
#include <QColor>
#include <QStyle>
#include <QStyleFactory>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QFormLayout>
#include <QTabWidget>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QFileDialog>
#include <QCryptographicHash>
#include <QMessageBox>
#include <QGroupBox>
#include <QLocale>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkInterface>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QCoreApplication>
#include <QProcess>
#include <QDir>
#ifdef Q_OS_WIN
#include <QSettings>
#endif

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr_("settings_title"));
    // Settings opens as an independent top-level window, not a child modal —
    // the main BATorrent window is often resized small and a child modal would
    // be cramped. Window flag makes it appear in the dock/taskbar separately.
    setWindowFlag(Qt::Window, true);
    setModal(false);
    setMinimumSize(900, 640);
    resize(960, 720);

    if (QStyle *fusion = QStyleFactory::create("Fusion"))
        setStyle(fusion);

    const auto &tm = ThemeManager::instance();
    QPalette pal = palette();
    pal.setColor(QPalette::Window, QColor(tm.bgColor()));
    pal.setColor(QPalette::Base, QColor(tm.surfaceColor()));
    pal.setColor(QPalette::AlternateBase, QColor(tm.surfaceColor()));
    pal.setColor(QPalette::Text, QColor(tm.textColor()));
    pal.setColor(QPalette::WindowText, QColor(tm.textColor()));
    pal.setColor(QPalette::Button, QColor(tm.surfaceColor()));
    pal.setColor(QPalette::ButtonText, QColor(tm.textColor()));
    pal.setColor(QPalette::ToolTipBase, QColor(tm.surfaceColor()));
    pal.setColor(QPalette::ToolTipText, QColor(tm.textColor()));
    pal.setColor(QPalette::Highlight, QColor(tm.accentColor()));
    pal.setColor(QPalette::HighlightedText, QColor("#ffffff"));
    setPalette(pal);
    setAutoFillBackground(true);

    setStyleSheet(QString(
        "QDialog { background: %1; color: %2; }"
        "QLabel { background: transparent; color: %2; }"
        "QScrollArea { background: transparent; border: none; }"
        "QScrollArea > QWidget > QWidget { background: transparent; }"

        "QTabWidget::pane { background: transparent; border: none; }"
        "QTabBar { background: transparent; alignment: left; }"
        "QTabBar::tab {"
        "  background: transparent; color: %3;"
        "  padding: 10px 18px; margin: 0;"
        "  border: none; border-bottom: 2px solid transparent;"
        "  font-size: 12px; font-weight: 500;"
        "}"
        "QTabBar::tab:selected {"
        "  color: %2; border-bottom: 2px solid %4;"
        "  font-weight: 600;"
        "}"
        "QTabBar::tab:hover:!selected { color: %2; }"

        "QLineEdit, QSpinBox, QDoubleSpinBox, QComboBox {"
        "  background: %5; color: %2;"
        "  border: 1px solid %6; border-radius: 6px;"
        "  padding: 6px 10px; font-size: 11px;"
        "  selection-background-color: %4;"
        "  min-height: 22px;"
        "}"
        "QLineEdit:focus, QSpinBox:focus, QDoubleSpinBox:focus, QComboBox:focus {"
        "  border-color: %4;"
        "}"
        "QLineEdit:disabled, QSpinBox:disabled, QDoubleSpinBox:disabled, QComboBox:disabled {"
        "  color: %7; background: %8;"
        "}"
        "QComboBox::drop-down { border: none; width: 22px; }"
        "QComboBox QAbstractItemView {"
        "  background: %5; color: %2;"
        "  border: 1px solid %6; selection-background-color: %9;"
        "  outline: none;"
        "}"
        "QSpinBox::up-button, QSpinBox::down-button,"
        "QDoubleSpinBox::up-button, QDoubleSpinBox::down-button {"
        "  background: transparent; border: none; width: 18px;"
        "}"

        "QCheckBox { color: %2; spacing: 8px; font-size: 11px; }"
        "QCheckBox::indicator {"
        "  width: 14px; height: 14px;"
        "  border: 1px solid %6; border-radius: 4px;"
        "  background: %5;"
        "}"
        "QCheckBox::indicator:checked {"
        "  background: %4; border-color: %4;"
        "}"
        "QCheckBox:disabled { color: %7; }"

        "QGroupBox {"
        "  background: %8; color: %2;"
        "  border: none; border-radius: 8px;"
        "  margin-top: 22px; padding: 24px 16px 14px 16px;"
        "  font-size: 9px; font-weight: 700;"
        "}"
        "QGroupBox::title {"
        "  subcontrol-origin: margin; subcontrol-position: top left;"
        "  left: 16px; top: 0px; padding: 0 0 6px 0;"
        "  color: %7; letter-spacing: 1.4px;"
        "}"

        "QPushButton#primaryBtn {"
        "  background: %4; color: #ffffff;"
        "  border: none; border-radius: 6px;"
        "  padding: 8px 22px; font-size: 11px; font-weight: 600;"
        "}"
        "QPushButton#primaryBtn:hover { background: %10; }"
        "QPushButton#ghostBtn {"
        "  background: transparent; color: %2;"
        "  border: 1px solid %6; border-radius: 6px;"
        "  padding: 8px 18px; font-size: 11px; font-weight: 500;"
        "}"
        "QPushButton#ghostBtn:hover { background: %5; }"
        "QPushButton {"
        "  background: %5; color: %2;"
        "  border: 1px solid %6; border-radius: 6px;"
        "  padding: 6px 14px; font-size: 11px;"
        "}"
        "QPushButton:hover { background: %8; border-color: %9; }"
        ).arg(tm.bgColor(), tm.textColor(), tm.mutedColor(),
              tm.accentColor(), tm.surfaceColor(), tm.borderColor(),
              tm.dimColor(), tm.panelColor(), tm.accentTintColor(),
              tm.accentLightColor()));

    auto *tabs = new QTabWidget;

    auto wrapInScroll = [](QWidget *content) -> QScrollArea * {
        auto *scroll = new QScrollArea;
        scroll->setWidget(content);
        scroll->setWidgetResizable(true);
        scroll->setFrameShape(QFrame::NoFrame);
        return scroll;
    };

    const QString labelStyle = QString("color: %1; font-size: 11px;")
        .arg(tm.mutedColor());

    // ---- General tab ----
    auto *generalWidget = new QWidget;
    auto *generalLayout = new QFormLayout(generalWidget);
    generalLayout->setContentsMargins(16, 16, 16, 16);
    generalLayout->setSpacing(12);

    auto *savePathLayout = new QHBoxLayout;
    m_savePathEdit = new QLineEdit;
    auto *browseBtn = new QPushButton(tr_("settings_browse"));
    browseBtn->setFixedWidth(100);
    connect(browseBtn, &QPushButton::clicked, this, &SettingsDialog::browseSavePath);
    savePathLayout->addWidget(m_savePathEdit);
    savePathLayout->addWidget(browseBtn);

    auto *savePathLabel = new QLabel(tr_("settings_default_save"));
    savePathLabel->setStyleSheet(labelStyle);
    generalLayout->addRow(savePathLabel, savePathLayout);

    m_languageCombo = new QComboBox;
    m_languageCombo->addItem("English");
    m_languageCombo->addItem(QString::fromUtf8("Português (BR)"));
    m_languageCombo->addItem(QString::fromUtf8("中文 (简体)"));
    m_languageCombo->addItem(QString::fromUtf8("日本語"));
    m_languageCombo->addItem(QString::fromUtf8("Русский"));
    m_languageCombo->addItem(QString::fromUtf8("Español (Latinoamérica)"));
    m_languageCombo->addItem("Deutsch");

    auto *langLabel = new QLabel(tr_("settings_language"));
    langLabel->setStyleSheet(labelStyle);
    generalLayout->addRow(langLabel, m_languageCombo);

    m_themeCombo = new QComboBox;
    for (const auto &name : ThemeManager::themeNames())
        m_themeCombo->addItem(name);

    auto *themeLabel = new QLabel(tr_("settings_theme"));
    themeLabel->setStyleSheet(labelStyle);
    generalLayout->addRow(themeLabel, m_themeCombo);

    m_useDefaultPathCheck = new QCheckBox(tr_("settings_use_default_path"));
    generalLayout->addRow("", m_useDefaultPathCheck);

    m_startMinimizedCheck = new QCheckBox(tr_("settings_start_tray"));
    generalLayout->addRow("", m_startMinimizedCheck);

    m_closeToTrayCheck = new QCheckBox(tr_("settings_close_to_tray"));
    generalLayout->addRow("", m_closeToTrayCheck);

    m_autoShutdownCheck = new QCheckBox(tr_("settings_auto_shutdown"));
    generalLayout->addRow("", m_autoShutdownCheck);

    m_notifSoundCheck = new QCheckBox(tr_("settings_notif_sound"));
    generalLayout->addRow("", m_notifSoundCheck);

    m_verboseLogCheck = new QCheckBox(tr_("settings_verbose_log"));
    m_verboseLogCheck->setToolTip(tr_("tip_verbose_log"));
    generalLayout->addRow("", m_verboseLogCheck);

    m_speedUnitCombo = new QComboBox;
    m_speedUnitCombo->addItem(tr_("settings_speed_unit_bytes"), 0);
    m_speedUnitCombo->addItem(tr_("settings_speed_unit_bits"), 1);
    m_speedUnitCombo->setToolTip(tr_("tip_speed_unit"));
    auto *speedUnitLabel = new QLabel(tr_("settings_speed_unit"));
    speedUnitLabel->setStyleSheet(labelStyle);
    generalLayout->addRow(speedUnitLabel, m_speedUnitCombo);

#ifndef BAT_STORE_BUILD
    m_updateChannelCombo = new QComboBox;
    m_updateChannelCombo->addItem(tr_("settings_update_channel_github"), "github");
    m_updateChannelCombo->addItem(tr_("settings_update_channel_gitee"), "gitee");
    m_updateChannelCombo->addItem(tr_("settings_update_channel_disabled"), "disabled");
    m_updateChannelCombo->setToolTip(tr_("tip_update_channel"));
    auto *updateChannelLabel = new QLabel(tr_("settings_update_channel"));
    updateChannelLabel->setStyleSheet(labelStyle);
    generalLayout->addRow(updateChannelLabel, m_updateChannelCombo);
#else
    m_updateChannelCombo = new QComboBox; // hidden, but pointer must exist
    m_updateChannelCombo->hide();
#endif

    // Auto-move completed downloads
    m_autoMoveCheck = new QCheckBox(tr_("settings_automove"));
    generalLayout->addRow("", m_autoMoveCheck);

    auto *autoMoveLayout = new QHBoxLayout;
    m_autoMovePathEdit = new QLineEdit;
    m_autoMovePathEdit->setPlaceholderText(tr_("settings_automove_path"));
    auto *autoMoveBrowseBtn = new QPushButton(tr_("settings_browse"));
    autoMoveBrowseBtn->setFixedWidth(100);
    connect(autoMoveBrowseBtn, &QPushButton::clicked, this, &SettingsDialog::browseAutoMovePath);
    autoMoveLayout->addWidget(m_autoMovePathEdit);
    autoMoveLayout->addWidget(autoMoveBrowseBtn);
    auto *autoMoveLabel = new QLabel(tr_("settings_automove_path"));
    autoMoveLabel->setStyleSheet(labelStyle);
    generalLayout->addRow(autoMoveLabel, autoMoveLayout);

    // Temp path for incomplete downloads
    auto *tempPathLayout = new QHBoxLayout;
    m_tempPathEdit = new QLineEdit;
    m_tempPathEdit->setPlaceholderText(tr_("settings_temp_path_hint"));
    m_tempPathEdit->setToolTip(tr_("tip_temp_path"));
    auto *tempBrowseBtn = new QPushButton(tr_("settings_browse"));
    tempBrowseBtn->setFixedWidth(100);
    connect(tempBrowseBtn, &QPushButton::clicked, this, [this]() {
        QString dir = QFileDialog::getExistingDirectory(this, tr_("settings_temp_path"));
        if (!dir.isEmpty()) m_tempPathEdit->setText(dir);
    });
    tempPathLayout->addWidget(m_tempPathEdit);
    tempPathLayout->addWidget(tempBrowseBtn);
    auto *tempPathLabel = new QLabel(tr_("settings_temp_path"));
    tempPathLabel->setStyleSheet(labelStyle);
    generalLayout->addRow(tempPathLabel, tempPathLayout);

    // Content layout
    m_contentLayoutCombo = new QComboBox;
    m_contentLayoutCombo->addItem(tr_("settings_layout_original"), 0);
    m_contentLayoutCombo->addItem(tr_("settings_layout_subfolder"), 1);
    m_contentLayoutCombo->addItem(tr_("settings_layout_no_subfolder"), 2);
    m_contentLayoutCombo->setToolTip(tr_("tip_content_layout"));
    auto *layoutLabel = new QLabel(tr_("settings_content_layout"));
    layoutLabel->setStyleSheet(labelStyle);
    generalLayout->addRow(layoutLabel, m_contentLayoutCombo);

    // Excluded file patterns
    m_excludedPatternsEdit = new QLineEdit;
    m_excludedPatternsEdit->setPlaceholderText(tr_("settings_excluded_hint"));
    m_excludedPatternsEdit->setToolTip(tr_("tip_excluded_patterns"));
    auto *excludedLabel = new QLabel(tr_("settings_excluded_patterns"));
    excludedLabel->setStyleSheet(labelStyle);
    generalLayout->addRow(excludedLabel, m_excludedPatternsEdit);

    // Run on completion — external command with template variables
    m_runOnCompleteEdit = new QLineEdit;
    m_runOnCompleteEdit->setPlaceholderText(tr_("settings_run_on_hint"));
    m_runOnCompleteEdit->setToolTip(tr_("tip_run_on_complete"));
    auto *runLabel = new QLabel(tr_("settings_run_on_complete"));
    runLabel->setStyleSheet(labelStyle);
    generalLayout->addRow(runLabel, m_runOnCompleteEdit);

    // Watched folder
    m_watchedFolderEdit = new QLineEdit;
    m_watchedFolderEdit->setPlaceholderText(tr_("settings_watched_hint"));
    m_watchedFolderEdit->setToolTip(tr_("tip_watched_folder"));
    auto *watchLabel = new QLabel(tr_("settings_watched_folder"));
    watchLabel->setStyleSheet(labelStyle);
    auto *watchRow = new QHBoxLayout;
    watchRow->addWidget(m_watchedFolderEdit, 1);
    auto *watchBrowse = new QPushButton(tr_("settings_browse"));
    watchBrowse->setFixedWidth(100);
    connect(watchBrowse, &QPushButton::clicked, this, [this]() {
        QString dir = QFileDialog::getExistingDirectory(this, "Watch folder");
        if (!dir.isEmpty()) m_watchedFolderEdit->setText(dir);
    });
    watchRow->addWidget(watchBrowse);
    generalLayout->addRow(watchLabel, watchRow);

    auto *defaultAppBtn = new QPushButton(tr_("settings_set_default"));
    connect(defaultAppBtn, &QPushButton::clicked, this, &SettingsDialog::setAsDefaultApp);
    generalLayout->addRow("", defaultAppBtn);

    tabs->addTab(wrapInScroll(generalWidget), tr_("settings_general"));

    // ---- Speed tab ----
    auto *speedWidget = new QWidget;
    auto *speedLayout = new QFormLayout(speedWidget);
    speedLayout->setContentsMargins(16, 16, 16, 16);
    speedLayout->setSpacing(12);

    m_maxDownSpin = new QSpinBox;
    m_maxDownSpin->setRange(0, 999999);
    m_maxDownSpin->setSuffix(" KB/s");
    m_maxDownSpin->setSpecialValueText(tr_("settings_unlimited"));

    m_maxUpSpin = new QSpinBox;
    m_maxUpSpin->setRange(0, 999999);
    m_maxUpSpin->setSuffix(" KB/s");
    m_maxUpSpin->setSpecialValueText(tr_("settings_unlimited"));

    m_seedRatioSpin = new QDoubleSpinBox;
    m_seedRatioSpin->setRange(0.0, 99.0);
    m_seedRatioSpin->setSingleStep(0.1);
    m_seedRatioSpin->setDecimals(1);
    m_seedRatioSpin->setSpecialValueText(tr_("settings_unlimited"));
    m_seedRatioSpin->setToolTip(tr_("tip_seed_ratio"));

    m_maxSeedDaysSpin = new QSpinBox;
    m_maxSeedDaysSpin->setRange(0, 365);
    m_maxSeedDaysSpin->setSuffix(" " + tr_("settings_days"));
    m_maxSeedDaysSpin->setSpecialValueText(tr_("settings_unlimited"));

    m_stopAfterDownloadCheck = new QCheckBox(tr_("settings_stop_after_download"));
    m_stopAfterDownloadCheck->setToolTip(tr_("tip_stop_after_download"));

    m_autoCompleteCombo = new QComboBox;
    m_autoCompleteCombo->setToolTip(tr_("tip_auto_complete"));
    m_autoCompleteCombo->addItem(tr_("auto_complete_never"), QVariant::fromValue<qint64>(0));
    m_autoCompleteCombo->addItem(tr_("auto_complete_1d"),  QVariant::fromValue<qint64>(86400));
    m_autoCompleteCombo->addItem(tr_("auto_complete_3d"),  QVariant::fromValue<qint64>(86400 * 3));
    m_autoCompleteCombo->addItem(tr_("auto_complete_7d"),  QVariant::fromValue<qint64>(86400 * 7));
    m_autoCompleteCombo->addItem(tr_("auto_complete_14d"), QVariant::fromValue<qint64>(86400 * 14));
    m_autoCompleteCombo->addItem(tr_("auto_complete_30d"), QVariant::fromValue<qint64>(86400 * 30));

    auto *downLabel = new QLabel(tr_("settings_max_down"));
    downLabel->setStyleSheet(labelStyle);
    auto *upLabel = new QLabel(tr_("settings_max_up"));
    upLabel->setStyleSheet(labelStyle);
    auto *ratioLabel = new QLabel(tr_("settings_seed_ratio"));
    ratioLabel->setStyleSheet(labelStyle);
    auto *seedDaysLabel = new QLabel(tr_("settings_max_seed_days"));
    seedDaysLabel->setStyleSheet(labelStyle);
    auto *autoCompleteLabel = new QLabel(tr_("settings_auto_complete"));
    autoCompleteLabel->setStyleSheet(labelStyle);

    speedLayout->addRow(downLabel, m_maxDownSpin);
    speedLayout->addRow(upLabel, m_maxUpSpin);
    speedLayout->addRow(ratioLabel, m_seedRatioSpin);
    speedLayout->addRow(seedDaysLabel, m_maxSeedDaysSpin);
    speedLayout->addRow("", m_stopAfterDownloadCheck);
    speedLayout->addRow(autoCompleteLabel, m_autoCompleteCombo);

    // Scheduler group inside speed tab
    auto *schedGroup = new QGroupBox(tr_("settings_scheduler_group"));
    auto *schedLayout = new QFormLayout(schedGroup);
    schedLayout->setSpacing(10);

    m_schedulerCheck = new QCheckBox(tr_("settings_scheduler_enable"));
    m_schedulerCheck->setToolTip(tr_("tip_scheduler"));
    schedLayout->addRow("", m_schedulerCheck);

    m_altDownSpin = new QSpinBox;
    m_altDownSpin->setRange(0, 999999);
    m_altDownSpin->setSuffix(" KB/s");
    m_altDownSpin->setSpecialValueText(tr_("settings_unlimited"));
    auto *altDownLabel = new QLabel(tr_("settings_alt_down"));
    altDownLabel->setStyleSheet(labelStyle);
    schedLayout->addRow(altDownLabel, m_altDownSpin);

    m_altUpSpin = new QSpinBox;
    m_altUpSpin->setRange(0, 999999);
    m_altUpSpin->setSuffix(" KB/s");
    m_altUpSpin->setSpecialValueText(tr_("settings_unlimited"));
    auto *altUpLabel = new QLabel(tr_("settings_alt_up"));
    altUpLabel->setStyleSheet(labelStyle);
    schedLayout->addRow(altUpLabel, m_altUpSpin);

    auto *schedTimeLayout = new QHBoxLayout;
    m_schedFromSpin = new QSpinBox;
    m_schedFromSpin->setRange(0, 23);
    m_schedFromSpin->setSuffix(":00");
    m_schedToSpin = new QSpinBox;
    m_schedToSpin->setRange(0, 23);
    m_schedToSpin->setSuffix(":00");
    schedTimeLayout->addWidget(m_schedFromSpin);
    schedTimeLayout->addWidget(new QLabel(tr_("settings_sched_to")));
    schedTimeLayout->addWidget(m_schedToSpin);
    schedTimeLayout->addStretch();
    auto *schedTimeLabel = new QLabel(tr_("settings_sched_from"));
    schedTimeLabel->setStyleSheet(labelStyle);
    schedLayout->addRow(schedTimeLabel, schedTimeLayout);

    auto *daysLayout = new QHBoxLayout;
    // Use the system locale's short day names so the schedule reads
    // naturally in every language the app supports.
    QLocale sysLocale = QLocale::system();
    for (int i = 0; i < 7; ++i) {
        // Qt uses Mon=1..Sun=7; our bit-mask is Mon=0..Sun=6.
        auto *cb = new QCheckBox(sysLocale.dayName(i + 1, QLocale::ShortFormat));
        cb->setChecked(true);
        m_dayChecks.append(cb);
        daysLayout->addWidget(cb);
    }
    schedLayout->addRow(tr_("settings_sched_days"), daysLayout);

    speedLayout->addRow(schedGroup);

    tabs->addTab(wrapInScroll(speedWidget), tr_("settings_speed"));

    // ---- Network tab ----
    auto *networkWidget = new QWidget;
    auto *networkLayout = new QFormLayout(networkWidget);
    networkLayout->setContentsMargins(16, 16, 16, 16);
    networkLayout->setSpacing(12);

    m_maxConnSpin = new QSpinBox;
    m_maxConnSpin->setRange(10, 9999);

    m_dhtCheck = new QCheckBox(tr_("settings_enable_dht"));
    m_dhtCheck->setToolTip(tr_("tip_dht"));

    m_encryptionCombo = new QComboBox;
    m_encryptionCombo->addItem(tr_("settings_enc_enabled"));
    m_encryptionCombo->addItem(tr_("settings_enc_forced"));
    m_encryptionCombo->addItem(tr_("settings_enc_disabled"));
    m_encryptionCombo->setToolTip(tr_("tip_encryption"));

    auto *connLabel = new QLabel(tr_("settings_max_conn"));
    connLabel->setStyleSheet(labelStyle);
    auto *encLabel = new QLabel(tr_("settings_encryption"));
    encLabel->setStyleSheet(labelStyle);

    m_maxActiveSpin = new QSpinBox;
    m_maxActiveSpin->setRange(0, 50);
    m_maxActiveSpin->setSpecialValueText(tr_("settings_unlimited"));
    auto *maxActiveLabel = new QLabel(tr_("settings_max_active"));
    maxActiveLabel->setStyleSheet(labelStyle);

    networkLayout->addRow(connLabel, m_maxConnSpin);
    networkLayout->addRow(maxActiveLabel, m_maxActiveSpin);
    networkLayout->addRow("", m_dhtCheck);
    networkLayout->addRow(encLabel, m_encryptionCombo);

    m_utpCheck = new QCheckBox(tr_("settings_enable_utp"));
    m_utpCheck->setToolTip(tr_("tip_utp"));
    networkLayout->addRow("", m_utpCheck);

    m_anonymousCheck = new QCheckBox(tr_("settings_anonymous_mode"));
    m_anonymousCheck->setToolTip(tr_("tip_anonymous_mode"));
    networkLayout->addRow("", m_anonymousCheck);

    m_forceIpv4Check = new QCheckBox(tr_("settings_force_ipv4"));
    m_forceIpv4Check->setToolTip(tr_("tip_force_ipv4"));
    networkLayout->addRow("", m_forceIpv4Check);

    m_ptModeCheck = new QCheckBox(tr_("settings_pt_mode"));
    m_ptModeCheck->setToolTip(tr_("tip_pt_mode"));
    networkLayout->addRow("", m_ptModeCheck);

    m_blockLeechersCheck = new QCheckBox(tr_("settings_block_leechers"));
    m_blockLeechersCheck->setToolTip(tr_("tip_block_leechers"));
    networkLayout->addRow("", m_blockLeechersCheck);

    // Listening port + the "re-randomize on each launch" toggle. Putting
    // them together makes the trade-off obvious: a fixed port is what you
    // want for UPnP port forwards; a randomized port frustrates passive
    // fingerprinting at the cost of breaking those forwards.
    m_listenPortSpin = new QSpinBox;
    m_listenPortSpin->setRange(1024, 65535);
    m_listenPortSpin->setValue(6881);
    m_listenPortSpin->setToolTip(tr_("tip_listen_port"));
    auto *portLabel = new QLabel(tr_("settings_listen_port"));
    portLabel->setStyleSheet(labelStyle);
    networkLayout->addRow(portLabel, m_listenPortSpin);

    m_randomizePortCheck = new QCheckBox(tr_("settings_random_port"));
    networkLayout->addRow("", m_randomizePortCheck);
    // Disabling the port spin when randomize is on makes the relationship
    // visually obvious; the saved value still gets used for the *next*
    // session if the user unticks the box.
    connect(m_randomizePortCheck, &QCheckBox::toggled,
            m_listenPortSpin, [this](bool randomize) {
        m_listenPortSpin->setEnabled(!randomize);
    });

    // VPN / Interface Binding group
    auto *vpnGroup = new QGroupBox(tr_("settings_vpn_group"));
    auto *vpnLayout = new QFormLayout(vpnGroup);
    vpnLayout->setSpacing(10);

    auto *ifaceLayout = new QHBoxLayout;
    m_interfaceCombo = new QComboBox;
    m_interfaceCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    auto *refreshBtn = new QPushButton(tr_("settings_refresh"));
    refreshBtn->setFixedWidth(80);
    connect(refreshBtn, &QPushButton::clicked, this, &SettingsDialog::refreshInterfaces);
    ifaceLayout->addWidget(m_interfaceCombo);
    ifaceLayout->addWidget(refreshBtn);

    auto *ifaceLabel = new QLabel(tr_("settings_interface"));
    ifaceLabel->setStyleSheet(labelStyle);
    vpnLayout->addRow(ifaceLabel, ifaceLayout);

    m_interfaceIpLabel = new QLabel;
    m_interfaceIpLabel->setStyleSheet(QString("color: %1; font-size: 11px;")
        .arg(tm.dimColor()));
    vpnLayout->addRow("", m_interfaceIpLabel);

    m_killSwitchCheck = new QCheckBox(tr_("settings_kill_switch"));
    m_killSwitchCheck->setToolTip(tr_("tip_kill_switch"));
    vpnLayout->addRow("", m_killSwitchCheck);

    m_autoResumeCheck = new QCheckBox(tr_("settings_auto_resume"));
    m_autoResumeCheck->setToolTip(tr_("tip_auto_resume"));
    m_autoResumeCheck->setEnabled(false);
    vpnLayout->addRow("", m_autoResumeCheck);

    connect(m_killSwitchCheck, &QCheckBox::toggled, m_autoResumeCheck, &QCheckBox::setEnabled);

    connect(m_interfaceCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
        QString iface = m_interfaceCombo->currentData().toString();
        if (iface.isEmpty()) {
            m_interfaceIpLabel->setText(tr_("settings_iface_any_desc"));
        } else {
            QNetworkInterface ni = QNetworkInterface::interfaceFromName(iface);
            QString ip;
            for (const auto &entry : ni.addressEntries()) {
                if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol) {
                    ip = entry.ip().toString();
                    break;
                }
            }
            m_interfaceIpLabel->setText(ip.isEmpty() ? tr_("settings_iface_no_ip") : ip);
        }
    });

    networkLayout->addRow(vpnGroup);

    // Proxy group
    auto *proxyGroup = new QGroupBox(tr_("settings_proxy_group"));
    auto *proxyLayout = new QFormLayout(proxyGroup);
    proxyLayout->setSpacing(10);

    m_proxyTypeCombo = new QComboBox;
    m_proxyTypeCombo->setToolTip(tr_("tip_proxy"));
    m_proxyTypeCombo->addItem(tr_("settings_proxy_none"));
    m_proxyTypeCombo->addItem("SOCKS5");
    m_proxyTypeCombo->addItem("HTTP");
    auto *proxyTypeLabel = new QLabel(tr_("settings_proxy_type"));
    proxyTypeLabel->setStyleSheet(labelStyle);
    proxyLayout->addRow(proxyTypeLabel, m_proxyTypeCombo);

    m_proxyHostEdit = new QLineEdit;
    m_proxyHostEdit->setPlaceholderText("127.0.0.1");
    auto *proxyHostLabel = new QLabel(tr_("settings_proxy_host"));
    proxyHostLabel->setStyleSheet(labelStyle);
    proxyLayout->addRow(proxyHostLabel, m_proxyHostEdit);

    m_proxyPortSpin = new QSpinBox;
    m_proxyPortSpin->setRange(0, 65535);
    m_proxyPortSpin->setValue(1080);
    auto *proxyPortLabel = new QLabel(tr_("settings_proxy_port"));
    proxyPortLabel->setStyleSheet(labelStyle);
    proxyLayout->addRow(proxyPortLabel, m_proxyPortSpin);

    m_proxyUserEdit = new QLineEdit;
    m_proxyUserEdit->setPlaceholderText(tr_("settings_proxy_user_hint"));
    auto *proxyUserLabel = new QLabel(tr_("settings_proxy_user"));
    proxyUserLabel->setStyleSheet(labelStyle);
    proxyLayout->addRow(proxyUserLabel, m_proxyUserEdit);

    m_proxyPassEdit = new QLineEdit;
    m_proxyPassEdit->setEchoMode(QLineEdit::Password);
    auto *proxyPassLabel = new QLabel(tr_("settings_proxy_pass"));
    proxyPassLabel->setStyleSheet(labelStyle);
    proxyLayout->addRow(proxyPassLabel, m_proxyPassEdit);

    // One-click Tor preset — most users running Tor on the same machine want
    // exactly this (default SocksPort 9050, no auth). Saves them looking up
    // the SOCKS5 fields and risking a typo.
    auto *torBtn = new QPushButton(tr_("settings_use_tor"));
    torBtn->setToolTip(tr_("tip_use_tor"));
    connect(torBtn, &QPushButton::clicked, this, [this]() {
        m_proxyTypeCombo->setCurrentIndex(1); // SOCKS5
        m_proxyHostEdit->setText(QStringLiteral("127.0.0.1"));
        m_proxyPortSpin->setValue(9050);
        m_proxyUserEdit->clear();
        m_proxyPassEdit->clear();
    });
    proxyLayout->addRow("", torBtn);

    networkLayout->addRow(proxyGroup);

    // IP Filter group
    auto *ipGroup = new QGroupBox(tr_("settings_ip_filter_group"));
    auto *ipLayout = new QFormLayout(ipGroup);
    ipLayout->setSpacing(10);

    auto *ipFilterLayout = new QHBoxLayout;
    m_ipFilterEdit = new QLineEdit;
    m_ipFilterEdit->setToolTip(tr_("tip_ip_filter"));
    m_ipFilterEdit->setPlaceholderText(tr_("settings_ip_filter_hint"));
    auto *ipBrowseBtn = new QPushButton(tr_("settings_browse"));
    ipBrowseBtn->setFixedWidth(100);
    connect(ipBrowseBtn, &QPushButton::clicked, this, &SettingsDialog::browseIpFilter);
    ipFilterLayout->addWidget(m_ipFilterEdit);
    ipFilterLayout->addWidget(ipBrowseBtn);
    auto *ipFilterLabel = new QLabel(tr_("settings_ip_filter_file"));
    ipFilterLabel->setStyleSheet(labelStyle);
    ipLayout->addRow(ipFilterLabel, ipFilterLayout);

    networkLayout->addRow(ipGroup);

    refreshInterfaces();

    tabs->addTab(wrapInScroll(networkWidget), tr_("settings_network"));

    // ---- WebUI tab ----
    auto *webUiWidget = new QWidget;
    auto *webUiLayout = new QFormLayout(webUiWidget);
    webUiLayout->setContentsMargins(16, 16, 16, 16);
    webUiLayout->setSpacing(12);

    m_webUiCheck = new QCheckBox(tr_("settings_webui_enable"));
    webUiLayout->addRow("", m_webUiCheck);

    m_webUiPortSpin = new QSpinBox;
    m_webUiPortSpin->setRange(1024, 65535);
    m_webUiPortSpin->setValue(8080);
    auto *wPortLabel = new QLabel(tr_("settings_webui_port"));
    wPortLabel->setStyleSheet(labelStyle);
    webUiLayout->addRow(wPortLabel, m_webUiPortSpin);

    auto *pairBtn = new QPushButton(tr_("pairing_button"));
    pairBtn->setToolTip(tr_("tip_pairing"));
    connect(pairBtn, &QPushButton::clicked, this, [this]() {
        PairingDialog dlg(m_webUiPortSpin->value(), this);
        dlg.exec();
    });
    webUiLayout->addRow("", pairBtn);

    m_webUiUserEdit = new QLineEdit;
    m_webUiUserEdit->setPlaceholderText("admin");
    auto *wUserLabel = new QLabel(tr_("settings_webui_user"));
    wUserLabel->setStyleSheet(labelStyle);
    webUiLayout->addRow(wUserLabel, m_webUiUserEdit);

    m_webUiPassEdit = new QLineEdit;
    m_webUiPassEdit->setEchoMode(QLineEdit::Password);
    m_webUiPassEdit->setPlaceholderText(tr_("settings_webui_pass_hint"));
    auto *wPassLabel = new QLabel(tr_("settings_webui_pass"));
    wPassLabel->setStyleSheet(labelStyle);
    webUiLayout->addRow(wPassLabel, m_webUiPassEdit);

    m_webUiRemoteCheck = new QCheckBox(tr_("settings_webui_remote"));
    webUiLayout->addRow("", m_webUiRemoteCheck);

    connect(m_webUiRemoteCheck, &QCheckBox::toggled, this, [this](bool checked) {
        if (checked) {
            QMessageBox::warning(this, tr_("settings_webui_warning_title"),
                                 tr_("settings_webui_warning_msg"));
        }
    });

    tabs->addTab(wrapInScroll(webUiWidget), "WebUI");

    // ---- Media Server tab ----
    auto *mediaWidget = new QWidget;
    auto *mediaLayout = new QFormLayout(mediaWidget);
    mediaLayout->setContentsMargins(16, 16, 16, 16);
    mediaLayout->setSpacing(12);

    auto *plexGroup = new QGroupBox("Plex");
    auto *plexLayout = new QFormLayout(plexGroup);
    plexLayout->setSpacing(10);

    m_plexCheck = new QCheckBox(tr_("settings_media_enable_plex"));
    plexLayout->addRow("", m_plexCheck);

    m_plexUrlEdit = new QLineEdit;
    m_plexUrlEdit->setPlaceholderText("http://localhost:32400");
    auto *plexUrlLabel = new QLabel("URL:");
    plexUrlLabel->setStyleSheet(labelStyle);
    plexLayout->addRow(plexUrlLabel, m_plexUrlEdit);

    m_plexTokenEdit = new QLineEdit;
    m_plexTokenEdit->setPlaceholderText("X-Plex-Token");
    auto *plexTokenLabel = new QLabel("Token:");
    plexTokenLabel->setStyleSheet(labelStyle);
    plexLayout->addRow(plexTokenLabel, m_plexTokenEdit);

    mediaLayout->addRow(plexGroup);

    auto *jellyGroup = new QGroupBox("Jellyfin / Emby");
    auto *jellyLayout = new QFormLayout(jellyGroup);
    jellyLayout->setSpacing(10);

    m_jellyfinCheck = new QCheckBox(tr_("settings_media_enable_jellyfin"));
    jellyLayout->addRow("", m_jellyfinCheck);

    m_jellyfinUrlEdit = new QLineEdit;
    m_jellyfinUrlEdit->setPlaceholderText("http://localhost:8096");
    auto *jellyUrlLabel = new QLabel("URL:");
    jellyUrlLabel->setStyleSheet(labelStyle);
    jellyLayout->addRow(jellyUrlLabel, m_jellyfinUrlEdit);

    m_jellyfinKeyEdit = new QLineEdit;
    m_jellyfinKeyEdit->setPlaceholderText("API Key");
    auto *jellyKeyLabel = new QLabel(tr_("settings_media_api_key"));
    jellyKeyLabel->setStyleSheet(labelStyle);
    jellyLayout->addRow(jellyKeyLabel, m_jellyfinKeyEdit);

    mediaLayout->addRow(jellyGroup);

    // Telegram bot notifier — same Integrations tab so the user finds all
    // outbound mirrors in one place. Token + chat ID separate so a malformed
    // token can't be mistaken for the chat selector.
    auto *telegramGroup = new QGroupBox("Telegram");
    auto *telegramLayout = new QFormLayout(telegramGroup);
    telegramLayout->setSpacing(10);

    m_telegramTokenEdit = new QLineEdit;
    m_telegramTokenEdit->setPlaceholderText("123456:ABC-DEF...");
    m_telegramTokenEdit->setEchoMode(QLineEdit::Password);
    m_telegramTokenEdit->setToolTip(tr_("tip_telegram_token"));
    auto *tgTokenLabel = new QLabel(tr_("settings_telegram_token"));
    tgTokenLabel->setStyleSheet(labelStyle);
    telegramLayout->addRow(tgTokenLabel, m_telegramTokenEdit);

    m_telegramChatIdEdit = new QLineEdit;
    m_telegramChatIdEdit->setPlaceholderText("@yourchannel or 123456789");
    m_telegramChatIdEdit->setToolTip(tr_("tip_telegram_chat"));
    auto *tgChatLabel = new QLabel(tr_("settings_telegram_chat"));
    tgChatLabel->setStyleSheet(labelStyle);
    telegramLayout->addRow(tgChatLabel, m_telegramChatIdEdit);

    m_telegramFinishedCheck = new QCheckBox(tr_("settings_telegram_finished"));
    m_telegramKillSwitchCheck = new QCheckBox(tr_("settings_telegram_killswitch"));
    m_telegramRssCheck = new QCheckBox(tr_("settings_telegram_rss"));
    m_telegramErrorCheck = new QCheckBox(tr_("settings_telegram_error"));
    telegramLayout->addRow("", m_telegramFinishedCheck);
    telegramLayout->addRow("", m_telegramKillSwitchCheck);
    telegramLayout->addRow("", m_telegramRssCheck);
    telegramLayout->addRow("", m_telegramErrorCheck);

    auto *tgTestBtn = new QPushButton(tr_("settings_telegram_test"));
    m_telegramTestResult = new QLabel;
    m_telegramTestResult->setStyleSheet(QString("color: %1; font-size: 11px;").arg(tm.mutedColor()));
    connect(tgTestBtn, &QPushButton::clicked, this, [this]() {
        // Inline test that doesn't depend on saving first — uses the values
        // in the fields right now. Posts to api.telegram.org and shows the
        // HTTP result in the label below.
        m_telegramTestResult->setText(tr_("settings_telegram_test_sending"));
        QString token = m_telegramTokenEdit->text().trimmed();
        QString chatId = m_telegramChatIdEdit->text().trimmed();
        if (token.isEmpty() || chatId.isEmpty()) {
            m_telegramTestResult->setText(tr_("settings_telegram_test_missing"));
            return;
        }
        auto *nam = new QNetworkAccessManager(this);
        QUrl url(QStringLiteral("https://api.telegram.org/bot%1/sendMessage").arg(token));
        QNetworkRequest req(url);
        req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        QJsonObject body;
        body.insert("chat_id", chatId);
        body.insert("text", QStringLiteral("🦇 BATorrent test — webhook works."));
        auto *reply = nam->post(req, QJsonDocument(body).toJson(QJsonDocument::Compact));
        connect(reply, &QNetworkReply::finished, this, [this, reply, nam]() {
            if (reply->error() == QNetworkReply::NoError)
                m_telegramTestResult->setText(tr_("settings_telegram_test_ok"));
            else
                m_telegramTestResult->setText(QStringLiteral("✗ %1").arg(reply->errorString()));
            reply->deleteLater();
            nam->deleteLater();
        });
    });
    auto *tgTestRow = new QHBoxLayout;
    tgTestRow->addWidget(tgTestBtn);
    tgTestRow->addWidget(m_telegramTestResult, 1);
    telegramLayout->addRow("", tgTestRow);

    mediaLayout->addRow(telegramGroup);

    // Discord Rich Presence — shows "Downloading X · 67%" in the user's
    // Discord profile. Empty client ID = silent disabled.
    auto *discordGroup = new QGroupBox("Discord Rich Presence");
    auto *discordLayout = new QFormLayout(discordGroup);
    discordLayout->setSpacing(10);
    m_discordEnabledCheck = new QCheckBox(tr_("settings_discord_enabled"));
    m_discordEnabledCheck->setToolTip(tr_("tip_discord_enabled"));
    discordLayout->addRow("", m_discordEnabledCheck);
    auto *discordHelp = new QLabel(tr_("settings_discord_help"));
    discordHelp->setWordWrap(true);
    discordHelp->setStyleSheet(QString("color: %1; font-size: 10px;").arg(tm.mutedColor()));
    discordHelp->setOpenExternalLinks(true);
    discordLayout->addRow("", discordHelp);
    mediaLayout->addRow(discordGroup);

    tabs->addTab(wrapInScroll(mediaWidget), tr_("settings_media_server"));

    // ---- Advanced tab (libtorrent tuning) ----
    auto *advWidget = new QWidget;
    auto *advLayout = new QFormLayout(advWidget);
    advLayout->setContentsMargins(16, 16, 16, 16);
    advLayout->setSpacing(10);

    auto addAdvSpin = [&](const QString &label, int min, int max, int def, const QString &tip) {
        auto *spin = new QSpinBox;
        spin->setRange(min, max);
        spin->setValue(def);
        if (!tip.isEmpty()) spin->setToolTip(tip);
        auto *lbl = new QLabel(label);
        lbl->setStyleSheet(labelStyle);
        advLayout->addRow(lbl, spin);
        return spin;
    };

    auto *diskGroup = new QGroupBox(tr_("adv_disk_io"));
    auto *diskLay = new QFormLayout(diskGroup);
    diskLay->setSpacing(8);
    m_advAioThreads = new QSpinBox; m_advAioThreads->setRange(1, 64); m_advAioThreads->setToolTip(tr_("tip_adv_aio"));
    m_advHashingThreads = new QSpinBox; m_advHashingThreads->setRange(1, 16); m_advHashingThreads->setToolTip(tr_("tip_adv_hashing"));
    m_advFilePoolSize = new QSpinBox; m_advFilePoolSize->setRange(10, 1000); m_advFilePoolSize->setToolTip(tr_("tip_adv_filepool"));
    m_advCheckingMem = new QSpinBox; m_advCheckingMem->setRange(1, 4096); m_advCheckingMem->setSuffix(" (x16KB)"); m_advCheckingMem->setToolTip(tr_("tip_adv_checkmem"));
    m_advSendBuffer = new QSpinBox; m_advSendBuffer->setRange(16, 8192); m_advSendBuffer->setSuffix(" KB"); m_advSendBuffer->setToolTip(tr_("tip_adv_sendbuf"));
    diskLay->addRow(tr_("adv_aio_threads"), m_advAioThreads);
    diskLay->addRow(tr_("adv_hashing_threads"), m_advHashingThreads);
    diskLay->addRow(tr_("adv_file_pool"), m_advFilePoolSize);
    diskLay->addRow(tr_("adv_checking_mem"), m_advCheckingMem);
    diskLay->addRow(tr_("adv_send_buffer"), m_advSendBuffer);
    advLayout->addRow(diskGroup);

    auto *connGroup = new QGroupBox(tr_("adv_connections"));
    auto *connLay = new QFormLayout(connGroup);
    connLay->setSpacing(8);
    m_advConnLimit = new QSpinBox; m_advConnLimit->setRange(10, 10000); m_advConnLimit->setToolTip(tr_("tip_adv_connlimit"));
    m_advConnSpeed = new QSpinBox; m_advConnSpeed->setRange(1, 200); m_advConnSpeed->setToolTip(tr_("tip_adv_connspeed"));
    m_advMaxUploadsPerTorrent = new QSpinBox; m_advMaxUploadsPerTorrent->setRange(1, 100); m_advMaxUploadsPerTorrent->setToolTip(tr_("tip_adv_maxuploads"));
    m_advMaxConnsPerTorrent = new QSpinBox; m_advMaxConnsPerTorrent->setRange(1, 500); m_advMaxConnsPerTorrent->setToolTip(tr_("tip_adv_maxconns"));
    m_advUnchokeSlots = new QSpinBox; m_advUnchokeSlots->setRange(1, 100); m_advUnchokeSlots->setToolTip(tr_("tip_adv_unchoke"));
    connLay->addRow(tr_("adv_conn_limit"), m_advConnLimit);
    connLay->addRow(tr_("adv_conn_speed"), m_advConnSpeed);
    connLay->addRow(tr_("adv_unchoke_slots"), m_advUnchokeSlots);
    connLay->addRow(tr_("adv_max_uploads"), m_advMaxUploadsPerTorrent);
    connLay->addRow(tr_("adv_max_conns"), m_advMaxConnsPerTorrent);
    advLayout->addRow(connGroup);

    auto *algoGroup = new QGroupBox(tr_("adv_algorithms"));
    auto *algoLay = new QFormLayout(algoGroup);
    algoLay->setSpacing(8);
    m_advChokingAlgo = new QComboBox;
    m_advChokingAlgo->addItem(tr_("adv_choke_fixed"), 0);
    m_advChokingAlgo->addItem(tr_("adv_choke_rate"), 1);
    m_advChokingAlgo->setToolTip(tr_("tip_adv_choking"));
    m_advSeedChokingAlgo = new QComboBox;
    m_advSeedChokingAlgo->addItem(tr_("adv_seedchoke_robin"), 0);
    m_advSeedChokingAlgo->addItem(tr_("adv_seedchoke_fastest"), 1);
    m_advSeedChokingAlgo->addItem(tr_("adv_seedchoke_antileech"), 2);
    m_advSeedChokingAlgo->setToolTip(tr_("tip_adv_seedchoking"));
    m_advRateLimitOverhead = new QCheckBox(tr_("adv_rate_overhead"));
    m_advIgnoreLimitsLAN = new QCheckBox(tr_("adv_ignore_lan"));
    m_advIgnoreLimitsLAN->setToolTip(tr_("tip_adv_ignorelan"));
    algoLay->addRow(tr_("adv_choking_algo"), m_advChokingAlgo);
    algoLay->addRow(tr_("adv_seed_choking"), m_advSeedChokingAlgo);
    algoLay->addRow("", m_advRateLimitOverhead);
    algoLay->addRow("", m_advIgnoreLimitsLAN);
    advLayout->addRow(algoGroup);

    tabs->addTab(wrapInScroll(advWidget), tr_("settings_advanced"));

    // ---- Header (eyebrow + heading) ----
    auto *eyebrow = new QLabel(tr_("settings_title").toUpper());
    {
        QFont f; f.setPointSize(8); f.setWeight(QFont::Bold);
        f.setLetterSpacing(QFont::AbsoluteSpacing, 1.4);
        eyebrow->setFont(f);
        eyebrow->setStyleSheet(QString("color: %1;").arg(tm.accentColor()));
    }

    auto *heading = new QLabel(tr_("settings_heading"));
    {
        QFont f; f.setPointSize(18); f.setWeight(QFont::Bold);
        f.setLetterSpacing(QFont::AbsoluteSpacing, -0.3);
        heading->setFont(f);
        heading->setStyleSheet(QString("color: %1;").arg(tm.textColor()));
    }

    // ---- Footer (Cancel ghost + Save primary) ----
    auto *btnLayout = new QHBoxLayout;
    btnLayout->setSpacing(8);
    btnLayout->addStretch();

    auto *cancelBtn = new QPushButton(tr_("btn_cancel"));
    cancelBtn->setObjectName(QStringLiteral("ghostBtn"));
    cancelBtn->setCursor(Qt::PointingHandCursor);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    btnLayout->addWidget(cancelBtn);

    auto *okBtn = new QPushButton(tr_("btn_ok"));
    okBtn->setObjectName(QStringLiteral("primaryBtn"));
    okBtn->setCursor(Qt::PointingHandCursor);
    okBtn->setDefault(true);
    connect(okBtn, &QPushButton::clicked, this, &QDialog::accept);
    btnLayout->addWidget(okBtn);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(32, 28, 32, 24);
    mainLayout->setSpacing(0);
    mainLayout->addWidget(eyebrow);
    mainLayout->addSpacing(6);
    mainLayout->addWidget(heading);
    mainLayout->addSpacing(16);

    // Search box: jump to first matching label/checkbox across tabs.
    auto *searchEdit = new QLineEdit;
    searchEdit->setPlaceholderText(tr_("settings_search_placeholder"));
    searchEdit->setClearButtonEnabled(true);
    searchEdit->setStyleSheet(QString(
        "QLineEdit { background: %1; color: %2; border: 1px solid %3;"
        "  border-radius: 6px; padding: 7px 10px; font-size: 11px; }"
        "QLineEdit:focus { border-color: %4; }")
        .arg(tm.surfaceColor(), tm.textColor(),
             tm.borderColor(), tm.accentColor()));
    connect(searchEdit, &QLineEdit::textChanged, this, [this, tabs](const QString &q) {
        if (q.length() < 2) return;
        // Match against every label/checkbox text under each tab. First hit
        // wins — switch to its tab and scroll/highlight via setFocus.
        for (int i = 0; i < tabs->count(); ++i) {
            QWidget *page = tabs->widget(i);
            if (!page) continue;
            for (auto *w : page->findChildren<QWidget *>()) {
                QString text;
                if (auto *l = qobject_cast<QLabel *>(w))    text = l->text();
                if (auto *c = qobject_cast<QCheckBox *>(w)) text = c->text();
                if (text.contains(q, Qt::CaseInsensitive)) {
                    tabs->setCurrentIndex(i);
                    w->setFocus(Qt::ShortcutFocusReason);
                    return;
                }
            }
        }
    });
    mainLayout->addWidget(searchEdit);
    mainLayout->addSpacing(12);

    mainLayout->addWidget(tabs, 1);
    mainLayout->addSpacing(20);
    mainLayout->addLayout(btnLayout);
}

void SettingsDialog::browseSavePath()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr_("dlg_save_to"),
                                                     m_savePathEdit->text());
    if (!dir.isEmpty())
        m_savePathEdit->setText(dir);
}

void SettingsDialog::browseAutoMovePath()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr_("settings_automove_path"),
                                                     m_autoMovePathEdit->text());
    if (!dir.isEmpty())
        m_autoMovePathEdit->setText(dir);
}

// General getters/setters
QString SettingsDialog::defaultSavePath() const { return m_savePathEdit->text(); }
int SettingsDialog::maxDownloadSpeed() const { return m_maxDownSpin->value(); }
int SettingsDialog::maxUploadSpeed() const { return m_maxUpSpin->value(); }
int SettingsDialog::languageIndex() const { return m_languageCombo->currentIndex(); }
bool SettingsDialog::startMinimized() const { return m_startMinimizedCheck->isChecked(); }
bool SettingsDialog::closeToTray() const { return m_closeToTrayCheck->isChecked(); }
bool SettingsDialog::useDefaultPath() const { return m_useDefaultPathCheck->isChecked(); }
int SettingsDialog::themeIndex() const { return m_themeCombo->currentIndex(); }

void SettingsDialog::setDefaultSavePath(const QString &path) { m_savePathEdit->setText(path); }
void SettingsDialog::setMaxDownloadSpeed(int kbps) { m_maxDownSpin->setValue(kbps); }
void SettingsDialog::setMaxUploadSpeed(int kbps) { m_maxUpSpin->setValue(kbps); }
void SettingsDialog::setLanguageIndex(int index) { m_languageCombo->setCurrentIndex(index); }
void SettingsDialog::setStartMinimized(bool val) { m_startMinimizedCheck->setChecked(val); }
void SettingsDialog::setCloseToTray(bool val) { m_closeToTrayCheck->setChecked(val); }
void SettingsDialog::setUseDefaultPath(bool val) { m_useDefaultPathCheck->setChecked(val); }
void SettingsDialog::setThemeIndex(int index) { m_themeCombo->setCurrentIndex(index); }
bool SettingsDialog::autoShutdown() const { return m_autoShutdownCheck->isChecked(); }
void SettingsDialog::setAutoShutdown(bool val) { m_autoShutdownCheck->setChecked(val); }
bool SettingsDialog::notifSoundEnabled() const { return m_notifSoundCheck->isChecked(); }
void SettingsDialog::setNotifSoundEnabled(bool val) { m_notifSoundCheck->setChecked(val); }
bool SettingsDialog::verboseLogEnabled() const { return m_verboseLogCheck->isChecked(); }
void SettingsDialog::setVerboseLogEnabled(bool val) { m_verboseLogCheck->setChecked(val); }
int SettingsDialog::speedUnit() const {
    return m_speedUnitCombo->currentData().toInt();
}
void SettingsDialog::setSpeedUnit(int unit) {
    int idx = m_speedUnitCombo->findData(unit);
    m_speedUnitCombo->setCurrentIndex(idx >= 0 ? idx : 0);
}
QString SettingsDialog::updateChannel() const {
    return m_updateChannelCombo->currentData().toString();
}
void SettingsDialog::setUpdateChannel(const QString &channel) {
    int idx = m_updateChannelCombo->findData(channel);
    m_updateChannelCombo->setCurrentIndex(idx >= 0 ? idx : 0);
}
bool SettingsDialog::autoMoveEnabled() const { return m_autoMoveCheck->isChecked(); }
QString SettingsDialog::autoMovePath() const { return m_autoMovePathEdit->text(); }
void SettingsDialog::setAutoMoveEnabled(bool val) { m_autoMoveCheck->setChecked(val); }
void SettingsDialog::setAutoMovePath(const QString &path) { m_autoMovePathEdit->setText(path); }
QString SettingsDialog::runOnComplete() const { return m_runOnCompleteEdit->text().trimmed(); }
QString SettingsDialog::watchedFolder() const { return m_watchedFolderEdit->text().trimmed(); }
void SettingsDialog::setRunOnComplete(const QString &cmd) { m_runOnCompleteEdit->setText(cmd); }
void SettingsDialog::setWatchedFolder(const QString &path) { m_watchedFolderEdit->setText(path); }
QString SettingsDialog::tempPath() const { return m_tempPathEdit->text().trimmed(); }
void SettingsDialog::setTempPath(const QString &path) { m_tempPathEdit->setText(path); }
int SettingsDialog::contentLayout() const { return m_contentLayoutCombo->currentData().toInt(); }
void SettingsDialog::setContentLayout(int layout) {
    int idx = m_contentLayoutCombo->findData(layout);
    m_contentLayoutCombo->setCurrentIndex(idx >= 0 ? idx : 0);
}
QString SettingsDialog::excludedFilePatterns() const { return m_excludedPatternsEdit->text().trimmed(); }
void SettingsDialog::setExcludedFilePatterns(const QString &patterns) { m_excludedPatternsEdit->setText(patterns); }

// Network getters/setters
bool SettingsDialog::dhtEnabled() const { return m_dhtCheck->isChecked(); }
int SettingsDialog::encryptionMode() const { return m_encryptionCombo->currentIndex(); }
int SettingsDialog::maxConnections() const { return m_maxConnSpin->value(); }
float SettingsDialog::seedRatioLimit() const { return static_cast<float>(m_seedRatioSpin->value()); }
int SettingsDialog::maxActiveDownloads() const { return m_maxActiveSpin->value(); }

void SettingsDialog::setDhtEnabled(bool enabled) { m_dhtCheck->setChecked(enabled); }
void SettingsDialog::setEncryptionMode(int mode) { m_encryptionCombo->setCurrentIndex(mode); }
void SettingsDialog::setMaxConnections(int max) { m_maxConnSpin->setValue(max); }
void SettingsDialog::setSeedRatioLimit(float ratio) { m_seedRatioSpin->setValue(static_cast<double>(ratio)); }
void SettingsDialog::setMaxActiveDownloads(int max) { m_maxActiveSpin->setValue(max); }

bool SettingsDialog::stopAfterDownload() const { return m_stopAfterDownloadCheck->isChecked(); }
int SettingsDialog::maxSeedDays() const { return m_maxSeedDaysSpin->value(); }
qint64 SettingsDialog::autoCompleteSeconds() const {
    return m_autoCompleteCombo->currentData().toLongLong();
}
void SettingsDialog::setAutoCompleteSeconds(qint64 seconds) {
    for (int i = 0; i < m_autoCompleteCombo->count(); ++i) {
        if (m_autoCompleteCombo->itemData(i).toLongLong() == seconds) {
            m_autoCompleteCombo->setCurrentIndex(i);
            return;
        }
    }
    m_autoCompleteCombo->setCurrentIndex(0);
}
void SettingsDialog::setStopAfterDownload(bool val) { m_stopAfterDownloadCheck->setChecked(val); }
void SettingsDialog::setMaxSeedDays(int days) { m_maxSeedDaysSpin->setValue(days); }

bool SettingsDialog::utpEnabled() const { return m_utpCheck->isChecked(); }
bool SettingsDialog::anonymousMode() const { return m_anonymousCheck->isChecked(); }
bool SettingsDialog::forceIpv4() const { return m_forceIpv4Check->isChecked(); }
bool SettingsDialog::ptMode() const { return m_ptModeCheck->isChecked(); }
bool SettingsDialog::randomizePort() const { return m_randomizePortCheck->isChecked(); }
int SettingsDialog::listenPort() const { return m_listenPortSpin->value(); }
void SettingsDialog::setUtpEnabled(bool enabled) { m_utpCheck->setChecked(enabled); }
void SettingsDialog::setAnonymousMode(bool val) { m_anonymousCheck->setChecked(val); }
void SettingsDialog::setForceIpv4(bool val) { m_forceIpv4Check->setChecked(val); }
void SettingsDialog::setPtMode(bool val) { m_ptModeCheck->setChecked(val); }
bool SettingsDialog::blockLeechers() const { return m_blockLeechersCheck->isChecked(); }
void SettingsDialog::setBlockLeechers(bool val) { m_blockLeechersCheck->setChecked(val); }
void SettingsDialog::setRandomizePort(bool enabled) {
    m_randomizePortCheck->setChecked(enabled);
    m_listenPortSpin->setEnabled(!enabled);
}
void SettingsDialog::setListenPort(int port) { m_listenPortSpin->setValue(port); }

// VPN getters/setters
QString SettingsDialog::outgoingInterface() const { return m_interfaceCombo->currentData().toString(); }
bool SettingsDialog::killSwitchEnabled() const { return m_killSwitchCheck->isChecked(); }
bool SettingsDialog::autoResumeOnReconnect() const { return m_autoResumeCheck->isChecked(); }

void SettingsDialog::setOutgoingInterface(const QString &iface)
{
    int idx = m_interfaceCombo->findData(iface);
    m_interfaceCombo->setCurrentIndex(idx >= 0 ? idx : 0);
}

void SettingsDialog::setKillSwitchEnabled(bool enabled) { m_killSwitchCheck->setChecked(enabled); }
void SettingsDialog::setAutoResumeOnReconnect(bool enabled) { m_autoResumeCheck->setChecked(enabled); }

void SettingsDialog::refreshInterfaces()
{
    QString current = m_interfaceCombo->currentData().toString();
    m_interfaceCombo->clear();

    // "Any" option
    m_interfaceCombo->addItem(tr_("settings_iface_any"), QString(""));

    static const QStringList vpnPrefixes = {
        "tun", "tap", "wg", "proton", "mullvad", "nordlynx", "utun"
    };

    auto interfaces = QNetworkInterface::allInterfaces();
    for (const auto &ni : interfaces) {
        if (ni.flags() & QNetworkInterface::IsLoopBack)
            continue;
        if (!(ni.flags() & QNetworkInterface::IsUp))
            continue;

        QString name = ni.name();
        QString ip;
        for (const auto &entry : ni.addressEntries()) {
            if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol) {
                ip = entry.ip().toString();
                break;
            }
        }

        // Detect VPN interfaces
        bool isVpn = false;
        for (const auto &prefix : vpnPrefixes) {
            if (name.startsWith(prefix, Qt::CaseInsensitive)) {
                isVpn = true;
                break;
            }
        }
        // Also check humanReadableName on Windows
        QString hrName = ni.humanReadableName();
        if (!isVpn) {
            for (const QString &kw : {"TAP", "TUN", "WireGuard"}) {
                if (hrName.contains(kw, Qt::CaseInsensitive)) {
                    isVpn = true;
                    break;
                }
            }
        }

        QString display = name;
        if (isVpn) display += " (VPN)";
        if (!ip.isEmpty()) display += " - " + ip;

        m_interfaceCombo->addItem(display, name);
    }

    // Restore selection
    int idx = m_interfaceCombo->findData(current);
    m_interfaceCombo->setCurrentIndex(idx >= 0 ? idx : 0);
}

// WebUI getters/setters
bool SettingsDialog::webUiEnabled() const { return m_webUiCheck->isChecked(); }
int SettingsDialog::webUiPort() const { return m_webUiPortSpin->value(); }
QString SettingsDialog::webUiUser() const { return m_webUiUserEdit->text(); }
bool SettingsDialog::webUiRemoteAccess() const { return m_webUiRemoteCheck->isChecked(); }

QString SettingsDialog::webUiPasswordHash() const
{
    if (!m_webUiPassEdit->text().isEmpty()) {
        return QString::fromUtf8(
            QCryptographicHash::hash(m_webUiPassEdit->text().toUtf8(),
                                     QCryptographicHash::Sha256).toHex());
    }
    return m_webUiPasswordHash;
}

void SettingsDialog::setWebUiEnabled(bool enabled) { m_webUiCheck->setChecked(enabled); }
void SettingsDialog::setWebUiPort(int port) { m_webUiPortSpin->setValue(port); }
void SettingsDialog::setWebUiUser(const QString &user) { m_webUiUserEdit->setText(user); }
void SettingsDialog::setWebUiPasswordHash(const QString &hash) { m_webUiPasswordHash = hash; }
void SettingsDialog::setWebUiRemoteAccess(bool enabled) { m_webUiRemoteCheck->setChecked(enabled); }

// Proxy getters/setters
int SettingsDialog::proxyType() const { return m_proxyTypeCombo->currentIndex(); }
QString SettingsDialog::proxyHost() const { return m_proxyHostEdit->text(); }
int SettingsDialog::proxyPort() const { return m_proxyPortSpin->value(); }
QString SettingsDialog::proxyUser() const { return m_proxyUserEdit->text(); }
QString SettingsDialog::proxyPass() const { return m_proxyPassEdit->text(); }

void SettingsDialog::setProxyType(int type) { m_proxyTypeCombo->setCurrentIndex(type); }
void SettingsDialog::setProxyHost(const QString &host) { m_proxyHostEdit->setText(host); }
void SettingsDialog::setProxyPort(int port) { m_proxyPortSpin->setValue(port); }
void SettingsDialog::setProxyUser(const QString &user) { m_proxyUserEdit->setText(user); }
void SettingsDialog::setProxyPass(const QString &pass) { m_proxyPassEdit->setText(pass); }

// IP Filter getters/setters
QString SettingsDialog::ipFilterPath() const { return m_ipFilterEdit->text(); }
void SettingsDialog::setIpFilterPath(const QString &path) { m_ipFilterEdit->setText(path); }

void SettingsDialog::browseIpFilter()
{
    QString file = QFileDialog::getOpenFileName(this, tr_("settings_ip_filter_file"),
                                                 QString(), "Blocklists (*.txt *.p2p *.dat);;All files (*)");
    if (!file.isEmpty())
        m_ipFilterEdit->setText(file);
}

// Scheduler getters/setters
bool SettingsDialog::schedulerEnabled() const { return m_schedulerCheck->isChecked(); }
int SettingsDialog::altDownloadSpeed() const { return m_altDownSpin->value(); }
int SettingsDialog::altUploadSpeed() const { return m_altUpSpin->value(); }
int SettingsDialog::scheduleFromHour() const { return m_schedFromSpin->value(); }
int SettingsDialog::scheduleToHour() const { return m_schedToSpin->value(); }

int SettingsDialog::scheduleDays() const
{
    int mask = 0;
    for (int i = 0; i < 7; ++i)
        if (m_dayChecks[i]->isChecked()) mask |= (1 << i);
    return mask;
}

void SettingsDialog::setSchedulerEnabled(bool enabled) { m_schedulerCheck->setChecked(enabled); }
void SettingsDialog::setAltDownloadSpeed(int kbps) { m_altDownSpin->setValue(kbps); }
void SettingsDialog::setAltUploadSpeed(int kbps) { m_altUpSpin->setValue(kbps); }
void SettingsDialog::setScheduleFromHour(int hour) { m_schedFromSpin->setValue(hour); }
void SettingsDialog::setScheduleToHour(int hour) { m_schedToSpin->setValue(hour); }

void SettingsDialog::setScheduleDays(int daysMask)
{
    for (int i = 0; i < 7; ++i)
        m_dayChecks[i]->setChecked((daysMask & (1 << i)) != 0);
}

// Media Server getters/setters
bool SettingsDialog::plexEnabled() const { return m_plexCheck->isChecked(); }
QString SettingsDialog::plexUrl() const { return m_plexUrlEdit->text(); }
QString SettingsDialog::plexToken() const { return m_plexTokenEdit->text(); }
bool SettingsDialog::jellyfinEnabled() const { return m_jellyfinCheck->isChecked(); }
QString SettingsDialog::jellyfinUrl() const { return m_jellyfinUrlEdit->text(); }
QString SettingsDialog::jellyfinApiKey() const { return m_jellyfinKeyEdit->text(); }

void SettingsDialog::setPlexEnabled(bool enabled) { m_plexCheck->setChecked(enabled); }
void SettingsDialog::setPlexUrl(const QString &url) { m_plexUrlEdit->setText(url); }
void SettingsDialog::setPlexToken(const QString &token) { m_plexTokenEdit->setText(token); }
void SettingsDialog::setJellyfinEnabled(bool enabled) { m_jellyfinCheck->setChecked(enabled); }
void SettingsDialog::setJellyfinUrl(const QString &url) { m_jellyfinUrlEdit->setText(url); }
void SettingsDialog::setJellyfinApiKey(const QString &key) { m_jellyfinKeyEdit->setText(key); }

QString SettingsDialog::telegramToken() const { return m_telegramTokenEdit->text().trimmed(); }
QString SettingsDialog::telegramChatId() const { return m_telegramChatIdEdit->text().trimmed(); }
int SettingsDialog::telegramEvents() const {
    int m = 0;
    if (m_telegramFinishedCheck->isChecked())   m |= 0x1;
    if (m_telegramKillSwitchCheck->isChecked()) m |= 0x2;
    if (m_telegramRssCheck->isChecked())        m |= 0x4;
    if (m_telegramErrorCheck->isChecked())      m |= 0x8;
    return m;
}
void SettingsDialog::setTelegramToken(const QString &t) { m_telegramTokenEdit->setText(t); }
void SettingsDialog::setTelegramChatId(const QString &id) { m_telegramChatIdEdit->setText(id); }
void SettingsDialog::setTelegramEvents(int mask) {
    m_telegramFinishedCheck->setChecked(mask & 0x1);
    m_telegramKillSwitchCheck->setChecked(mask & 0x2);
    m_telegramRssCheck->setChecked(mask & 0x4);
    m_telegramErrorCheck->setChecked(mask & 0x8);
}
bool SettingsDialog::discordEnabled() const { return m_discordEnabledCheck->isChecked(); }
void SettingsDialog::setDiscordEnabled(bool val) { m_discordEnabledCheck->setChecked(val); }

void SettingsDialog::loadAdvancedSettings(const SessionManager::AdvancedSettings &a)
{
    if (m_advAioThreads) m_advAioThreads->setValue(a.aioThreads);
    if (m_advHashingThreads) m_advHashingThreads->setValue(a.hashingThreads);
    if (m_advFilePoolSize) m_advFilePoolSize->setValue(a.filePoolSize);
    if (m_advCheckingMem) m_advCheckingMem->setValue(a.checkingMemUsage);
    if (m_advSendBuffer) m_advSendBuffer->setValue(a.sendBufferWatermark);
    if (m_advConnLimit) m_advConnLimit->setValue(a.connectionsLimit);
    if (m_advConnSpeed) m_advConnSpeed->setValue(a.connectionSpeed);
    if (m_advMaxUploadsPerTorrent) m_advMaxUploadsPerTorrent->setValue(a.maxUploadsPerTorrent);
    if (m_advMaxConnsPerTorrent) m_advMaxConnsPerTorrent->setValue(a.maxConnectionsPerTorrent);
    if (m_advUnchokeSlots) m_advUnchokeSlots->setValue(a.unchokeSlotsLimit);
    if (m_advChokingAlgo) m_advChokingAlgo->setCurrentIndex(a.chokingAlgorithm);
    if (m_advSeedChokingAlgo) m_advSeedChokingAlgo->setCurrentIndex(a.seedChokingAlgorithm);
    if (m_advRateLimitOverhead) m_advRateLimitOverhead->setChecked(a.rateLimitIpOverhead);
    if (m_advIgnoreLimitsLAN) m_advIgnoreLimitsLAN->setChecked(a.ignoreLimitsOnLAN);
}

SessionManager::AdvancedSettings SettingsDialog::collectAdvancedSettings() const
{
    SessionManager::AdvancedSettings a;
    if (m_advAioThreads) a.aioThreads = m_advAioThreads->value();
    if (m_advHashingThreads) a.hashingThreads = m_advHashingThreads->value();
    if (m_advFilePoolSize) a.filePoolSize = m_advFilePoolSize->value();
    if (m_advCheckingMem) a.checkingMemUsage = m_advCheckingMem->value();
    if (m_advSendBuffer) a.sendBufferWatermark = m_advSendBuffer->value();
    if (m_advConnLimit) a.connectionsLimit = m_advConnLimit->value();
    if (m_advConnSpeed) a.connectionSpeed = m_advConnSpeed->value();
    if (m_advMaxUploadsPerTorrent) a.maxUploadsPerTorrent = m_advMaxUploadsPerTorrent->value();
    if (m_advMaxConnsPerTorrent) a.maxConnectionsPerTorrent = m_advMaxConnsPerTorrent->value();
    if (m_advUnchokeSlots) a.unchokeSlotsLimit = m_advUnchokeSlots->value();
    if (m_advChokingAlgo) a.chokingAlgorithm = m_advChokingAlgo->currentIndex();
    if (m_advSeedChokingAlgo) a.seedChokingAlgorithm = m_advSeedChokingAlgo->currentIndex();
    if (m_advRateLimitOverhead) a.rateLimitIpOverhead = m_advRateLimitOverhead->isChecked();
    if (m_advIgnoreLimitsLAN) a.ignoreLimitsOnLAN = m_advIgnoreLimitsLAN->isChecked();
    return a;
}

void SettingsDialog::setAsDefaultApp()
{
    QString exe = QCoreApplication::applicationFilePath();
    bool ok = false;

#ifdef Q_OS_WIN
    // Windows: register via HKCU (no admin needed)
    QString nativeExe = QDir::toNativeSeparators(exe);
    QSettings reg("HKEY_CURRENT_USER\\Software\\Classes", QSettings::NativeFormat);

    // .torrent file association
    reg.setValue(".torrent/.", "BATorrent.torrent");
    reg.setValue("BATorrent.torrent/.", "BATorrent Torrent File");
    reg.setValue("BATorrent.torrent/shell/open/command/.",
                 "\"" + nativeExe + "\" \"%1\"");
    reg.setValue("BATorrent.torrent/DefaultIcon/.",
                 nativeExe + ",0");

    // magnet: protocol handler
    reg.setValue("magnet/.", "URL:Magnet Protocol");
    reg.setValue("magnet/URL Protocol", "");
    reg.setValue("magnet/shell/open/command/.",
                 "\"" + nativeExe + "\" \"%1\"");
    reg.setValue("magnet/DefaultIcon/.",
                 nativeExe + ",0");

    reg.sync();
    ok = (reg.status() == QSettings::NoError);

    // Notify Windows that file associations changed
    if (ok) {
        // SHChangeNotify via command — avoids linking shell32 directly
        QProcess::startDetached("cmd", {"/c", "assoc", ".torrent=BATorrent.torrent"});
    }
#elif defined(Q_OS_LINUX)
    // Linux: xdg-mime + .desktop file
    QProcess p;
    p.start("xdg-mime", {"default", "batorrent.desktop", "application/x-bittorrent"});
    p.waitForFinished(3000);
    ok = (p.exitCode() == 0);

    // magnet: protocol
    QProcess p2;
    p2.start("xdg-mime", {"default", "batorrent.desktop", "x-scheme-handler/magnet"});
    p2.waitForFinished(3000);
    ok = ok && (p2.exitCode() == 0);
#elif defined(Q_OS_MACOS)
    // macOS: use duti or lsregister (best effort)
    QProcess p;
    p.start("duti", {"-s", "com.batorrent.app", ".torrent", "all"});
    p.waitForFinished(3000);
    ok = (p.exitCode() == 0);
#endif

    if (ok)
        QMessageBox::information(this, "BATorrent", tr_("settings_default_success"));
    else
        QMessageBox::warning(this, "BATorrent", tr_("settings_default_failed"));
}
