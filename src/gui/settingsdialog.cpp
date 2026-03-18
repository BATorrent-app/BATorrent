// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "settingsdialog.h"
#include "../app/translator.h"
#include "../gui/thememanager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
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
#include <QNetworkInterface>

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr_("settings_title"));
    setMinimumSize(500, 400);
    setStyleSheet(ThemeManager::instance().dialogStyleSheet());

    auto *tabs = new QTabWidget;

    QString labelStyle = ThemeManager::instance().formLabelStyle();

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

    tabs->addTab(generalWidget, tr_("settings_general"));

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

    auto *downLabel = new QLabel(tr_("settings_max_down"));
    downLabel->setStyleSheet(labelStyle);
    auto *upLabel = new QLabel(tr_("settings_max_up"));
    upLabel->setStyleSheet(labelStyle);
    auto *ratioLabel = new QLabel(tr_("settings_seed_ratio"));
    ratioLabel->setStyleSheet(labelStyle);

    speedLayout->addRow(downLabel, m_maxDownSpin);
    speedLayout->addRow(upLabel, m_maxUpSpin);
    speedLayout->addRow(ratioLabel, m_seedRatioSpin);

    tabs->addTab(speedWidget, tr_("settings_speed"));

    // ---- Network tab ----
    auto *networkWidget = new QWidget;
    auto *networkLayout = new QFormLayout(networkWidget);
    networkLayout->setContentsMargins(16, 16, 16, 16);
    networkLayout->setSpacing(12);

    m_maxConnSpin = new QSpinBox;
    m_maxConnSpin->setRange(10, 9999);

    m_dhtCheck = new QCheckBox(tr_("settings_enable_dht"));

    m_encryptionCombo = new QComboBox;
    m_encryptionCombo->addItem(tr_("settings_enc_enabled"));
    m_encryptionCombo->addItem(tr_("settings_enc_forced"));
    m_encryptionCombo->addItem(tr_("settings_enc_disabled"));

    auto *connLabel = new QLabel(tr_("settings_max_conn"));
    connLabel->setStyleSheet(labelStyle);
    auto *encLabel = new QLabel(tr_("settings_encryption"));
    encLabel->setStyleSheet(labelStyle);

    networkLayout->addRow(connLabel, m_maxConnSpin);
    networkLayout->addRow("", m_dhtCheck);
    networkLayout->addRow(encLabel, m_encryptionCombo);

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
    m_interfaceIpLabel->setStyleSheet(labelStyle + " color: #888;");
    vpnLayout->addRow("", m_interfaceIpLabel);

    m_killSwitchCheck = new QCheckBox(tr_("settings_kill_switch"));
    vpnLayout->addRow("", m_killSwitchCheck);

    m_autoResumeCheck = new QCheckBox(tr_("settings_auto_resume"));
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

    refreshInterfaces();

    tabs->addTab(networkWidget, tr_("settings_network"));

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

    tabs->addTab(webUiWidget, "WebUI");

    // ---- Buttons ----
    auto *btnLayout = new QHBoxLayout;
    btnLayout->addStretch();
    auto *okBtn = new QPushButton(tr_("btn_ok"));
    auto *cancelBtn = new QPushButton(tr_("btn_cancel"));
    okBtn->setFixedWidth(100);
    cancelBtn->setFixedWidth(100);
    connect(okBtn, &QPushButton::clicked, this, &QDialog::accept);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    btnLayout->addWidget(okBtn);
    btnLayout->addWidget(cancelBtn);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(tabs);
    mainLayout->addLayout(btnLayout);
}

void SettingsDialog::browseSavePath()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr_("dlg_save_to"),
                                                     m_savePathEdit->text());
    if (!dir.isEmpty())
        m_savePathEdit->setText(dir);
}

// General getters/setters
QString SettingsDialog::defaultSavePath() const { return m_savePathEdit->text(); }
int SettingsDialog::maxDownloadSpeed() const { return m_maxDownSpin->value(); }
int SettingsDialog::maxUploadSpeed() const { return m_maxUpSpin->value(); }
int SettingsDialog::languageIndex() const { return m_languageCombo->currentIndex(); }
bool SettingsDialog::startMinimized() const { return m_startMinimizedCheck->isChecked(); }
bool SettingsDialog::useDefaultPath() const { return m_useDefaultPathCheck->isChecked(); }
int SettingsDialog::themeIndex() const { return m_themeCombo->currentIndex(); }

void SettingsDialog::setDefaultSavePath(const QString &path) { m_savePathEdit->setText(path); }
void SettingsDialog::setMaxDownloadSpeed(int kbps) { m_maxDownSpin->setValue(kbps); }
void SettingsDialog::setMaxUploadSpeed(int kbps) { m_maxUpSpin->setValue(kbps); }
void SettingsDialog::setLanguageIndex(int index) { m_languageCombo->setCurrentIndex(index); }
void SettingsDialog::setStartMinimized(bool val) { m_startMinimizedCheck->setChecked(val); }
void SettingsDialog::setUseDefaultPath(bool val) { m_useDefaultPathCheck->setChecked(val); }
void SettingsDialog::setThemeIndex(int index) { m_themeCombo->setCurrentIndex(index); }

// Network getters/setters
bool SettingsDialog::dhtEnabled() const { return m_dhtCheck->isChecked(); }
int SettingsDialog::encryptionMode() const { return m_encryptionCombo->currentIndex(); }
int SettingsDialog::maxConnections() const { return m_maxConnSpin->value(); }
float SettingsDialog::seedRatioLimit() const { return static_cast<float>(m_seedRatioSpin->value()); }

void SettingsDialog::setDhtEnabled(bool enabled) { m_dhtCheck->setChecked(enabled); }
void SettingsDialog::setEncryptionMode(int mode) { m_encryptionCombo->setCurrentIndex(mode); }
void SettingsDialog::setMaxConnections(int max) { m_maxConnSpin->setValue(max); }
void SettingsDialog::setSeedRatioLimit(float ratio) { m_seedRatioSpin->setValue(static_cast<double>(ratio)); }

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
