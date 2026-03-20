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
    bool notifSoundEnabled() const;

    void setDefaultSavePath(const QString &path);
    void setMaxDownloadSpeed(int kbps);
    void setMaxUploadSpeed(int kbps);
    void setLanguageIndex(int index);
    void setStartMinimized(bool val);
    void setUseDefaultPath(bool val);
    void setThemeIndex(int index);
    void setAutoShutdown(bool val);
    void setNotifSoundEnabled(bool val);

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

    // Proxy
    int proxyType() const;
    QString proxyHost() const;
    int proxyPort() const;
    QString proxyUser() const;
    QString proxyPass() const;

    void setProxyType(int type);
    void setProxyHost(const QString &host);
    void setProxyPort(int port);
    void setProxyUser(const QString &user);
    void setProxyPass(const QString &pass);

    // IP Filter
    QString ipFilterPath() const;
    void setIpFilterPath(const QString &path);

    // Bandwidth Scheduler
    bool schedulerEnabled() const;
    int altDownloadSpeed() const;
    int altUploadSpeed() const;
    int scheduleFromHour() const;
    int scheduleToHour() const;
    int scheduleDays() const;

    void setSchedulerEnabled(bool enabled);
    void setAltDownloadSpeed(int kbps);
    void setAltUploadSpeed(int kbps);
    void setScheduleFromHour(int hour);
    void setScheduleToHour(int hour);
    void setScheduleDays(int daysMask);

    // Media Server
    bool plexEnabled() const;
    QString plexUrl() const;
    QString plexToken() const;
    bool jellyfinEnabled() const;
    QString jellyfinUrl() const;
    QString jellyfinApiKey() const;

    void setPlexEnabled(bool enabled);
    void setPlexUrl(const QString &url);
    void setPlexToken(const QString &token);
    void setJellyfinEnabled(bool enabled);
    void setJellyfinUrl(const QString &url);
    void setJellyfinApiKey(const QString &key);

private slots:
    void browseSavePath();
    void browseIpFilter();
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
    QCheckBox *m_notifSoundCheck;

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

    // Proxy
    QComboBox *m_proxyTypeCombo;
    QLineEdit *m_proxyHostEdit;
    QSpinBox *m_proxyPortSpin;
    QLineEdit *m_proxyUserEdit;
    QLineEdit *m_proxyPassEdit;

    // IP Filter
    QLineEdit *m_ipFilterEdit;

    // Bandwidth Scheduler
    QCheckBox *m_schedulerCheck;
    QSpinBox *m_altDownSpin;
    QSpinBox *m_altUpSpin;
    QSpinBox *m_schedFromSpin;
    QSpinBox *m_schedToSpin;
    QList<QCheckBox *> m_dayChecks;

    // Media Server
    QCheckBox *m_plexCheck;
    QLineEdit *m_plexUrlEdit;
    QLineEdit *m_plexTokenEdit;
    QCheckBox *m_jellyfinCheck;
    QLineEdit *m_jellyfinUrlEdit;
    QLineEdit *m_jellyfinKeyEdit;
};

#endif
