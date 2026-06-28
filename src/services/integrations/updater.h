// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

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

    // Returns -1 if a < b, 0 if equal, 1 if a > b. Compares dot-separated
    // numeric components ("2.10.0" > "2.3.1").
    static int compareVersions(const QString &a, const QString &b);

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
    qint64 m_expectedSize = 0;   // asset byte size from the release JSON (0 = unknown, skip check)
};

#endif
