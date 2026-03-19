// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>

class QSpinBox;
class QDoubleSpinBox;
class QLineEdit;
class QComboBox;
class QCheckBox;
class QLabel;

class SettingsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget *parent = nullptr);

    // General
    QString defaultSavePath() const;
    int maxDownloadSpeed() const;
    int maxUploadSpeed() const;
    int languageIndex() const;
    bool startMinimized() const;
    bool useDefaultPath() const;
    int themeIndex() const;
    bool autoShutdown() const;

    void setDefaultSavePath(const QString &path);
    void setMaxDownloadSpeed(int kbps);
    void setMaxUploadSpeed(int kbps);
    void setLanguageIndex(int index);
    void setStartMinimized(bool val);
    void setUseDefaultPath(bool val);
    void setThemeIndex(int index);
    void setAutoShutdown(bool val);

    // Network
    bool dhtEnabled() const;
    int encryptionMode() const;
    int maxConnections() const;
    float seedRatioLimit() const;

    void setDhtEnabled(bool enabled);
    void setEncryptionMode(int mode);
    void setMaxConnections(int max);
    void setSeedRatioLimit(float ratio);

    // VPN / Interface binding
    QString outgoingInterface() const;
    bool killSwitchEnabled() const;
    bool autoResumeOnReconnect() const;

    void setOutgoingInterface(const QString &iface);
    void setKillSwitchEnabled(bool enabled);
    void setAutoResumeOnReconnect(bool enabled);

    // WebUI
    bool webUiEnabled() const;
    int webUiPort() const;
    QString webUiUser() const;
    QString webUiPasswordHash() const;
    bool webUiRemoteAccess() const;

    void setWebUiEnabled(bool enabled);
    void setWebUiPort(int port);
    void setWebUiUser(const QString &user);
    void setWebUiPasswordHash(const QString &hash);
    void setWebUiRemoteAccess(bool enabled);

private slots:
    void browseSavePath();
    void refreshInterfaces();

private:
    // General tab
    QLineEdit *m_savePathEdit;
    QSpinBox *m_maxDownSpin;
    QSpinBox *m_maxUpSpin;
    QComboBox *m_languageCombo;
    QCheckBox *m_startMinimizedCheck;
    QCheckBox *m_useDefaultPathCheck;
    QComboBox *m_themeCombo;
    QCheckBox *m_autoShutdownCheck;

    // Network tab
    QCheckBox *m_dhtCheck;
    QComboBox *m_encryptionCombo;
    QSpinBox *m_maxConnSpin;
    QDoubleSpinBox *m_seedRatioSpin;

    // VPN tab
    QComboBox *m_interfaceCombo;
    QLabel *m_interfaceIpLabel;
    QCheckBox *m_killSwitchCheck;
    QCheckBox *m_autoResumeCheck;

    // WebUI tab
    QCheckBox *m_webUiCheck;
    QSpinBox *m_webUiPortSpin;
    QLineEdit *m_webUiUserEdit;
    QLineEdit *m_webUiPassEdit;
    QCheckBox *m_webUiRemoteCheck;
    QString m_webUiPasswordHash;
};

#endif
