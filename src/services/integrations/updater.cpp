// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "services/integrations/updater.h"
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkReply>
#include <QProcess>
#include <QStandardPaths>
#include <QSettings>
#include <QSysInfo>

static const QString GITHUB_API =
    "https://api.github.com/repos/BATorrent-app/BATorrent/releases/latest";
static const QString GITEE_API =
    "https://gitee.com/api/v5/repos/Mateuscruz19/BATorrent/releases/latest";

// Read the configured release-info endpoint. Both GitHub and Gitee expose the
// same JSON shape — `tag_name`, `assets[].browser_download_url`, `assets[].name`
// — so parseReleaseInfo doesn't need to branch by provider. The asset
// filename selection in platformAssetName() also stays identical because the
// CI publishes the same artefact names to both mirrors.
static QString releaseApiUrl()
{
    const QString channel =
        QSettings("BATorrent", "BATorrent").value("updateChannel", "github").toString();
    if (channel == "disabled") return "disabled";
    if (channel == "gitee")    return GITEE_API;
    return GITHUB_API;
}

static QString platformAssetName()
{
    const QString arch = QSysInfo::currentCpuArchitecture();
#ifdef Q_OS_WIN
    if (arch == "x86_64") return "BATorrent-setup-x86_64.exe";
    return {};
#elif defined(Q_OS_LINUX)
    if (arch == "x86_64") return "BATorrent-linux-x86_64.AppImage";
    return {};
#elif defined(Q_OS_MACOS)
    if (arch == "arm64") return "BATorrent-macos-arm64.dmg";
    return {};
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
    const QString url = releaseApiUrl();
    if (url == "disabled") {
        emit noUpdateAvailable();
        return;
    }
    QUrl apiUrl(url);
    QNetworkRequest req{apiUrl};
    req.setHeader(QNetworkRequest::UserAgentHeader, "BATorrent/" APP_VERSION);
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                     QNetworkRequest::NoLessSafeRedirectPolicy);
    req.setTransferTimeout(15000);
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
            m_expectedSize = asset.value("size").toVariant().toLongLong();  // 0 if absent (e.g. Gitee)
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

    // Integrity gate: a truncated download or an HTML error page (served with a
    // 200) would otherwise be run as an installer and could brick the install.
    // Reject anything that doesn't match the asset's published byte size.
    if (m_expectedSize > 0 && file.size() != m_expectedSize) {
        QFile::remove(destPath);
        emit errorOccurred("Download looks incomplete or corrupted — please download manually.");
        return;
    }

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
        if (!script.open(QIODevice::WriteOnly | QIODevice::Text)) {
            emit errorOccurred("Failed to create update script: " + script.errorString());
            return;
        }
        {
            QTextStream out(&script);
            out << "$ErrorActionPreference = 'Stop'\r\n";
            out << "try {\r\n";
            // Wait for the app to finish quitting, then force-kill any straggler
            // (tray instance / child) BEFORE running the installer — otherwise the
            // installer's Restart Manager sees BATorrent.exe alive and pops the
            // native "files in use / close these programs" dialog.
            out << "  $deadline = (Get-Date).AddSeconds(10)\r\n";
            out << "  while ((Get-Process -Name BATorrent -ErrorAction SilentlyContinue) -and (Get-Date) -lt $deadline) { Start-Sleep -Milliseconds 300 }\r\n";
            out << "  Get-Process -Name BATorrent -ErrorAction SilentlyContinue | Stop-Process -Force -ErrorAction SilentlyContinue\r\n";
            out << "  Start-Sleep -Milliseconds 700\r\n";
            // Fully silent; drop /CLOSEAPPLICATIONS + /RESTARTAPPLICATIONS (those
            // invoke the Restart Manager UI) since the process is already gone.
            out << "  Start-Process -FilePath '"
                << QDir::toNativeSeparators(newFilePath)
                << "' -ArgumentList '/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART','/NOCANCEL' "
                   "-Verb RunAs -Wait\r\n";
            out << "  Start-Process -FilePath '"
                << QDir::toNativeSeparators(appExe) << "'\r\n";
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
    if (!script.open(QIODevice::WriteOnly | QIODevice::Text)) {
        emit errorOccurred("Failed to create update script: " + script.errorString());
        return;
    }
    {
        QTextStream out(&script);
        // Same wait-then-force-kill as the installer path above — a blind sleep
        // left a straggler process alive long enough to fail the Copy-Item (file
        // still locked) on a slow shutdown (reported by a user: had to fully
        // kill the background process by hand before a paste/DnD fix "took").
        out << "$deadline = (Get-Date).AddSeconds(10)\r\n";
        out << "while ((Get-Process -Name BATorrent -ErrorAction SilentlyContinue) -and (Get-Date) -lt $deadline) { Start-Sleep -Milliseconds 300 }\r\n";
        out << "Get-Process -Name BATorrent -ErrorAction SilentlyContinue | Stop-Process -Force -ErrorAction SilentlyContinue\r\n";
        out << "Start-Sleep -Milliseconds 700\r\n";
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
    if (!script.open(QIODevice::WriteOnly | QIODevice::Text)) {
        emit errorOccurred("Failed to create update script: " + script.errorString());
        return;
    }
    {
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
