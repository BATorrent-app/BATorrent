#include "settingsdialog.h"
#include "../core/translator.h"
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
#include <QGroupBox>

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr_("settings_title"));
    setMinimumSize(500, 400);
    setupStyle();

    auto *tabs = new QTabWidget;

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
    savePathLabel->setStyleSheet("color: #c43030; font-weight: bold;");
    generalLayout->addRow(savePathLabel, savePathLayout);

    m_languageCombo = new QComboBox;
    m_languageCombo->addItem("English");
    m_languageCombo->addItem(QString::fromUtf8("Português (BR)"));

    auto *langLabel = new QLabel(tr_("settings_language"));
    langLabel->setStyleSheet("color: #c43030; font-weight: bold;");
    generalLayout->addRow(langLabel, m_languageCombo);

    m_themeCombo = new QComboBox;
    for (const auto &name : ThemeManager::themeNames())
        m_themeCombo->addItem(name);

    auto *themeLabel = new QLabel(tr_("settings_theme"));
    themeLabel->setStyleSheet("color: #c43030; font-weight: bold;");
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
    downLabel->setStyleSheet("color: #c43030; font-weight: bold;");
    auto *upLabel = new QLabel(tr_("settings_max_up"));
    upLabel->setStyleSheet("color: #c43030; font-weight: bold;");
    auto *ratioLabel = new QLabel(tr_("settings_seed_ratio"));
    ratioLabel->setStyleSheet("color: #c43030; font-weight: bold;");

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
    connLabel->setStyleSheet("color: #c43030; font-weight: bold;");
    auto *encLabel = new QLabel(tr_("settings_encryption"));
    encLabel->setStyleSheet("color: #c43030; font-weight: bold;");

    networkLayout->addRow(connLabel, m_maxConnSpin);
    networkLayout->addRow("", m_dhtCheck);
    networkLayout->addRow(encLabel, m_encryptionCombo);

    tabs->addTab(networkWidget, tr_("settings_network"));

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

void SettingsDialog::setupStyle()
{
    setStyleSheet(R"(
        QDialog {
            background-color: #141414;
            color: #b0b0b0;
        }
        QTabWidget::pane {
            background-color: #141414;
            border: 1px solid #252525;
            border-top: 1px solid #6b2020;
        }
        QTabBar::tab {
            background-color: #1a1a1a;
            color: #707070;
            padding: 8px 20px;
            border: 1px solid #252525;
            border-bottom: none;
            font-weight: bold;
        }
        QTabBar::tab:selected {
            background-color: #1e1218;
            color: #c43030;
            border-top: 2px solid #c43030;
        }
        QLineEdit, QSpinBox, QDoubleSpinBox, QComboBox {
            background-color: #1a1a1a;
            color: #b0b0b0;
            border: 1px solid #2a2a2a;
            border-radius: 4px;
            padding: 6px;
            font-size: 12px;
        }
        QLineEdit:focus, QSpinBox:focus, QDoubleSpinBox:focus, QComboBox:focus {
            border-color: #6b2020;
        }
        QComboBox::drop-down {
            border: none;
            background-color: #1e1218;
        }
        QComboBox QAbstractItemView {
            background-color: #1a1a1a;
            color: #b0b0b0;
            selection-background-color: #2a1015;
            border: 1px solid #252525;
        }
        QPushButton {
            background-color: #2a1015;
            color: #d0d0d0;
            border: 1px solid #3d1520;
            border-radius: 4px;
            padding: 8px 16px;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: #6b2020;
            color: #ffffff;
        }
        QCheckBox {
            color: #b0b0b0;
            spacing: 8px;
        }
        QCheckBox::indicator {
            width: 16px;
            height: 16px;
            border: 1px solid #2a2a2a;
            border-radius: 3px;
            background-color: #1a1a1a;
        }
        QCheckBox::indicator:checked {
            background-color: #6b2020;
            border-color: #8b2a2a;
        }
        QLabel {
            color: #b0b0b0;
        }
        QGroupBox {
            color: #c43030;
            font-weight: bold;
            border: 1px solid #252525;
            border-radius: 4px;
            margin-top: 8px;
            padding-top: 16px;
        }
    )");
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
