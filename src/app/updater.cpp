// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "updater.h"
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkReply>
#include <QProcess>
#include <QStandardPaths>
#include <QSysInfo>

static const QString GITHUB_API =
    "https://api.github.com/repos/Mateuscruz19/BAT-Torrent/releases/latest";

static QString platformAssetName()
{
#ifdef Q_OS_WIN
    return "BATorrent-setup-x86_64.exe";
#elif defined(Q_OS_LINUX)
    return "BATorrent-linux-x86_64.AppImage";
#elif defined(Q_OS_MACOS)
    return "BATorrent-macos-x86_64.dmg";
#else
    return {};
#endif
}

Updater::Updater(QObject *parent)
    : QObject(parent)
{
}

void Updater::checkForUpdate()
{
    QUrl apiUrl(GITHUB_API);
    QNetworkRequest req(apiUrl);
    req.setHeader(QNetworkRequest::UserAgentHeader, "BATorrent");
    QNetworkReply *reply = m_nam.get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        parseReleaseInfo(reply);
    });
}

void Updater::parseReleaseInfo(QNetworkReply *reply)
{
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        emit errorOccurred(reply->errorString());
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    QJsonObject obj = doc.object();
    QString tagName = obj.value("tag_name").toString(); // e.g. "v1.4"

    // Strip leading 'v' for comparison
    m_latestVersion = tagName.startsWith('v') ? tagName.mid(1) : tagName;
    QString currentVersion = QApplication::applicationVersion();

    if (m_latestVersion <= currentVersion) {
        emit noUpdateAvailable();
        return;
    }

    // Find the right asset for this platform
    QString wantedAsset = platformAssetName();
    if (wantedAsset.isEmpty()) {
        emit errorOccurred("Unsupported platform for auto-update.");
        return;
    }

    QJsonArray assets = obj.value("assets").toArray();
    for (const QJsonValue &val : assets) {
        QJsonObject asset = val.toObject();
        if (asset.value("name").toString() == wantedAsset) {
            QString url = asset.value("browser_download_url").toString();
            emit updateAvailable(m_latestVersion, url, wantedAsset);
            return;
        }
    }

    emit errorOccurred("Could not find download for this platform.");
}

void Updater::downloadAndInstall(const QString &downloadUrl, const QString &assetName)
{
    QUrl url(downloadUrl);
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::UserAgentHeader, "BATorrent");
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                     QNetworkRequest::NoLessSafeRedirectPolicy);
    QNetworkReply *reply = m_nam.get(req);

    connect(reply, &QNetworkReply::downloadProgress,
            this, &Updater::downloadProgress);

    connect(reply, &QNetworkReply::finished, this, [this, reply, assetName]() {
        onDownloadFinished(reply, assetName);
    });
}

void Updater::onDownloadFinished(QNetworkReply *reply, const QString &assetName)
{
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        emit errorOccurred(reply->errorString());
        return;
    }

    // Save to temp directory
    QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QString destPath = QDir(tempDir).filePath(assetName);

    QFile file(destPath);
    if (!file.open(QIODevice::WriteOnly)) {
        emit errorOccurred("Could not write to: " + destPath);
        return;
    }
    file.write(reply->readAll());
    file.close();

    emit updateReady();
    launchUpdaterScript(destPath);
}

void Updater::launchUpdaterScript(const QString &newFilePath)
{
#ifdef Q_OS_WIN
    // On Windows: the installer .exe handles everything.
    // Just launch it and quit. The installer will close us if needed.
    QString appExe = QApplication::applicationFilePath();
    QString scriptPath = QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation))
                             .filePath("batorrent_update.bat");

    // If it's the setup exe, run installer then relaunch the app
    if (newFilePath.endsWith(".exe")) {
        // Batch script: run installer silently, wait for it to finish, then relaunch
        QFile script(scriptPath);
        if (script.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&script);
            out << "@echo off\r\n";
            out << "\"" << QDir::toNativeSeparators(newFilePath) << "\" /SILENT /CLOSEAPPLICATIONS\r\n";
            out << "start \"\" \"" << QDir::toNativeSeparators(appExe) << "\"\r\n";
            out << "del \"%~f0\"\r\n";
            script.close();
        }
        QProcess::startDetached("cmd.exe", {"/c", scriptPath});
        QApplication::quit();
        return;
    }

    // For zip-based update: batch script that waits, replaces, restarts
    QFile script(scriptPath);
    if (script.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&script);
        out << "@echo off\r\n";
        out << "timeout /t 3 /nobreak >nul\r\n";
        out << "copy /y \"" << QDir::toNativeSeparators(newFilePath)
            << "\" \"" << QDir::toNativeSeparators(appExe) << "\"\r\n";
        out << "start \"\" \"" << QDir::toNativeSeparators(appExe) << "\"\r\n";
        out << "del \"%~f0\"\r\n";
        script.close();
    }
    QProcess::startDetached("cmd.exe", {"/c", scriptPath});
    QApplication::quit();

#elif defined(Q_OS_LINUX)
    // For AppImage: replace the current AppImage file
    QString appImage = qEnvironmentVariable("APPIMAGE");
    if (appImage.isEmpty()) {
        // Not running as AppImage — just notify
        emit errorOccurred("Auto-update only works when running as AppImage.");
        return;
    }

    QString scriptPath = QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation))
                             .filePath("batorrent_update.sh");
    QFile script(scriptPath);
    if (script.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&script);
        out << "#!/bin/sh\n";
        out << "sleep 3\n";
        out << "cp -f \"" << newFilePath << "\" \"" << appImage << "\"\n";
        out << "chmod +x \"" << appImage << "\"\n";
        out << "\"" << appImage << "\" &\n";
        out << "rm -f \"$0\"\n";
        script.close();
        QFile::setPermissions(scriptPath,
            QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner);
    }
    QProcess::startDetached("/bin/sh", {scriptPath});
    QApplication::quit();

#elif defined(Q_OS_MACOS)
    // For macOS: open the DMG and let user drag to Applications
    QProcess::startDetached("open", {newFilePath});
    QApplication::quit();
#endif
}
