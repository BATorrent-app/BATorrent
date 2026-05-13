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

int Updater::compareVersions(const QString &a, const QString &b)
{
    // Split off pre-release suffix (everything after the first '-') so we
    // can apply semver's "pre-release < release" rule. "2.4.0-rc1" must be
    // treated as older than "2.4.0"; a plain int-split would call them equal
    // since 'rc1'.toInt() == 0.
    auto splitVersion = [](const QString &v) {
        int dash = v.indexOf('-');
        return std::pair<QString, QString>(
            dash >= 0 ? v.left(dash) : v,
            dash >= 0 ? v.mid(dash + 1) : QString());
    };

    auto [baseA, preA] = splitVersion(a);
    auto [baseB, preB] = splitVersion(b);

    const QStringList partsA = baseA.split('.');
    const QStringList partsB = baseB.split('.');
    const int n = qMax(partsA.size(), partsB.size());
    for (int i = 0; i < n; ++i) {
        const int va = i < partsA.size() ? partsA[i].toInt() : 0;
        const int vb = i < partsB.size() ? partsB[i].toInt() : 0;
        if (va != vb) return va < vb ? -1 : 1;
    }

    // Numeric components match. A pre-release suffix means "lower than the
    // release without one" (semver §11). If both sides have suffixes, fall
    // back to a lexicographic compare.
    if (preA.isEmpty() && !preB.isEmpty()) return 1;
    if (!preA.isEmpty() && preB.isEmpty()) return -1;
    if (preA != preB) return preA < preB ? -1 : 1;
    return 0;
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

    if (compareVersions(m_latestVersion, currentVersion) <= 0) {
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
    // On Windows: the installer .exe handles everything. We need to elevate
    // (UAC prompt) because BATorrent is typically installed in Program Files,
    // which the running app — almost always non-elevated — cannot write to.
    // Using a PowerShell script with Start-Process -Verb RunAs triggers the
    // UAC dialog; the user accepts once and the installer can write.
    QString appExe = QApplication::applicationFilePath();
    QString scriptPath = QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation))
                             .filePath("batorrent_update.ps1");

    if (newFilePath.endsWith(".exe")) {
        QFile script(scriptPath);
        if (script.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&script);
            out << "$ErrorActionPreference = 'Stop'\r\n";
            out << "try {\r\n";
            out << "  Start-Process -FilePath '"
                << QDir::toNativeSeparators(newFilePath)
                << "' -ArgumentList '/SILENT','/CLOSEAPPLICATIONS',"
                   "'/RESTARTAPPLICATIONS','/SUPPRESSMSGBOXES','/NORESTART' "
                   "-Verb RunAs -Wait\r\n";
                // Inno Setup will restart the app via /RESTARTAPPLICATIONS,
                // but launch it manually as a fallback if the installer
                // didn't (e.g. the user installed per-user without elevation
                // and the verb was already runAs but elevation was bypassed).
            out << "  if (-not (Get-Process -Name BATorrent -ErrorAction SilentlyContinue)) {\r\n";
            out << "    Start-Process -FilePath '"
                << QDir::toNativeSeparators(appExe) << "'\r\n";
            out << "  }\r\n";
            out << "} catch {}\r\n";
            out << "Remove-Item -LiteralPath $MyInvocation.MyCommand.Path "
                   "-ErrorAction SilentlyContinue\r\n";
            script.close();
        }
        QProcess::startDetached("powershell.exe",
            {"-NoProfile", "-ExecutionPolicy", "Bypass", "-WindowStyle",
             "Hidden", "-File", scriptPath});
        QApplication::quit();
        return;
    }

    // Standalone exe (zip-style) update: replace in place, possibly elevated.
    QFile script(scriptPath);
    if (script.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&script);
        out << "Start-Sleep -Seconds 3\r\n";
        out << "try {\r\n";
        out << "  Copy-Item -LiteralPath '"
            << QDir::toNativeSeparators(newFilePath)
            << "' -Destination '" << QDir::toNativeSeparators(appExe)
            << "' -Force\r\n";
        out << "} catch {\r\n";
        out << "  Start-Process -FilePath 'powershell.exe' -ArgumentList "
               "'-NoProfile','-Command',\"Copy-Item -LiteralPath '"
            << QDir::toNativeSeparators(newFilePath)
            << "' -Destination '" << QDir::toNativeSeparators(appExe)
            << "' -Force\" -Verb RunAs -Wait\r\n";
        out << "}\r\n";
        out << "Start-Process -FilePath '"
            << QDir::toNativeSeparators(appExe) << "'\r\n";
        out << "Remove-Item -LiteralPath $MyInvocation.MyCommand.Path "
               "-ErrorAction SilentlyContinue\r\n";
        script.close();
    }
    QProcess::startDetached("powershell.exe",
        {"-NoProfile", "-ExecutionPolicy", "Bypass", "-WindowStyle",
         "Hidden", "-File", scriptPath});
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
