#ifndef UPDATER_H
#define UPDATER_H

#include <QObject>
#include <QNetworkAccessManager>

class QNetworkReply;

class Updater : public QObject
{
    Q_OBJECT
public:
    explicit Updater(QObject *parent = nullptr);

    void checkForUpdate();
    void downloadAndInstall(const QString &downloadUrl, const QString &assetName);

    QString latestVersion() const { return m_latestVersion; }

signals:
    void updateAvailable(const QString &version, const QString &downloadUrl, const QString &assetName);
    void noUpdateAvailable();
    void downloadProgress(qint64 received, qint64 total);
    void updateReady(); // downloaded, about to restart
    void errorOccurred(const QString &message);

private:
    void parseReleaseInfo(QNetworkReply *reply);
    void onDownloadFinished(QNetworkReply *reply, const QString &assetName);
    void launchUpdaterScript(const QString &newFilePath);

    QNetworkAccessManager m_nam;
    QString m_latestVersion;
};

#endif
