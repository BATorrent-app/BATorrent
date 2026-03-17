#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>

class QSpinBox;
class QDoubleSpinBox;
class QLineEdit;
class QComboBox;
class QCheckBox;

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

    void setDefaultSavePath(const QString &path);
    void setMaxDownloadSpeed(int kbps);
    void setMaxUploadSpeed(int kbps);
    void setLanguageIndex(int index);
    void setStartMinimized(bool val);
    void setUseDefaultPath(bool val);
    void setThemeIndex(int index);

    // Network
    bool dhtEnabled() const;
    int encryptionMode() const;
    int maxConnections() const;
    float seedRatioLimit() const;

    void setDhtEnabled(bool enabled);
    void setEncryptionMode(int mode);
    void setMaxConnections(int max);
    void setSeedRatioLimit(float ratio);

private slots:
    void browseSavePath();

private:
    void setupStyle();

    // General tab
    QLineEdit *m_savePathEdit;
    QSpinBox *m_maxDownSpin;
    QSpinBox *m_maxUpSpin;
    QComboBox *m_languageCombo;
    QCheckBox *m_startMinimizedCheck;
    QCheckBox *m_useDefaultPathCheck;
    QComboBox *m_themeCombo;

    // Network tab
    QCheckBox *m_dhtCheck;
    QComboBox *m_encryptionCombo;
    QSpinBox *m_maxConnSpin;
    QDoubleSpinBox *m_seedRatioSpin;
};

#endif
