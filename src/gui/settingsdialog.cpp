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

    m_autoShutdownCheck = new QCheckBox(tr_("settings_auto_shutdown"));
    generalLayout->addRow("", m_autoShutdownCheck);

    m_notifSoundCheck = new QCheckBox(tr_("settings_notif_sound"));
    generalLayout->addRow("", m_notifSoundCheck);

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

    // Scheduler group inside speed tab
    auto *schedGroup = new QGroupBox(tr_("settings_scheduler_group"));
    auto *schedLayout = new QFormLayout(schedGroup);
    schedLayout->setSpacing(10);

    m_schedulerCheck = new QCheckBox(tr_("settings_scheduler_enable"));
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
    static const QStringList dayNames = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
    for (int i = 0; i < 7; ++i) {
        auto *cb = new QCheckBox(dayNames[i]);
        cb->setChecked(true);
        m_dayChecks.append(cb);
        daysLayout->addWidget(cb);
    }
    schedLayout->addRow(tr_("settings_sched_days"), daysLayout);

    speedLayout->addRow(schedGroup);

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

    // Proxy group
    auto *proxyGroup = new QGroupBox(tr_("settings_proxy_group"));
    auto *proxyLayout = new QFormLayout(proxyGroup);
    proxyLayout->setSpacing(10);

    m_proxyTypeCombo = new QComboBox;
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

    networkLayout->addRow(proxyGroup);

    // IP Filter group
    auto *ipGroup = new QGroupBox(tr_("settings_ip_filter_group"));
    auto *ipLayout = new QFormLayout(ipGroup);
    ipLayout->setSpacing(10);

    auto *ipFilterLayout = new QHBoxLayout;
    m_ipFilterEdit = new QLineEdit;
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

    tabs->addTab(mediaWidget, tr_("settings_media_server"));

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
bool SettingsDialog::autoShutdown() const { return m_autoShutdownCheck->isChecked(); }
void SettingsDialog::setAutoShutdown(bool val) { m_autoShutdownCheck->setChecked(val); }
bool SettingsDialog::notifSoundEnabled() const { return m_notifSoundCheck->isChecked(); }
void SettingsDialog::setNotifSoundEnabled(bool val) { m_notifSoundCheck->setChecked(val); }

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
