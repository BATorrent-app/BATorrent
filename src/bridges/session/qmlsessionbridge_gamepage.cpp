// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details
//
// QmlSessionBridge: what a game's page in the HUB says beyond play time:
// where its saves are (and a copy of them), which languages and extras the
// repack shipped, checking its files, launch options, and its own uninstaller.

#include "bridges/session/qmlsessionbridge.h"
#include "torrent/sessionmanager.h"
#include "services/games/achievementfiles.h"
#include "services/integrations/installerprofile.h"
#include "services/platform/utils.h"   // formatSize

#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QProcess>
#include <QRegularExpression>
#include <QSettings>
#include <QStandardPaths>
#include <QUrl>

#if defined(Q_OS_WIN)
#include <windows.h>
#include <shellapi.h>
#endif

namespace {

// How many files a folder holds and when the newest was written, capped so a
// huge folder costs a bounded walk.
struct FolderStats { int files = 0; qint64 newestMs = 0; qint64 bytes = 0; };
FolderStats statsOf(const QString &dir, int cap = 20000)
{
    FolderStats s;
    QDirIterator it(dir, QDir::Files | QDir::Hidden | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (it.hasNext() && s.files < cap) {
        it.next();
        const QFileInfo fi = it.fileInfo();
        ++s.files;
        s.bytes += fi.size();
        s.newestMs = qMax(s.newestMs, fi.lastModified().toMSecsSinceEpoch());
    }
    return s;
}

bool copyTree(const QString &from, const QString &to)
{
    QDir src(from);
    if (!QDir().mkpath(to)) return false;
    for (const QFileInfo &fi : src.entryInfoList(QDir::Files | QDir::Hidden | QDir::Dirs | QDir::NoDotAndDotDot)) {
        const QString dest = QDir(to).filePath(fi.fileName());
        if (fi.isDir()) { if (!copyTree(fi.absoluteFilePath(), dest)) return false; }
        else if (!QFile::copy(fi.absoluteFilePath(), dest)) return false;
    }
    return true;
}

// "fg-selective-brazilian.bin" -> "Brazilian", "fg-optional-bonus-content.bin" -> "Bonus content"
QString partName(const QString &slug)
{
    QString s = slug;
    s.replace(QLatin1Char('-'), QLatin1Char(' ')).replace(QLatin1Char('_'), QLatin1Char(' '));
    s = s.simplified();
    if (!s.isEmpty()) s[0] = s[0].toUpper();
    return s;
}

} // namespace

// The folder a cracked game keeps its saves in. The emulators that write
// achievements keep the saves beside them ("remote" for Goldberg/GSE, CODEX,
// RUNE, EMPRESS; "Saves" for OnlineFix), so the achievement files found for
// the game point at them; a folder with nothing in it is not one.
QString QmlSessionBridge::saveFolderOf(const QString &infoHash) const
{
    const QString gameDir = achievementDir(infoHash);
    int appId = AchievementFiles::appIdFromGameDir(gameDir);
    if (appId <= 0) appId = m_achAppId.value(infoHash, 0);
    if (appId <= 0) return {};

    QStringList tries;
    for (const auto &c : AchievementFiles::candidates(appId, gameDir, AchievementFiles::systemRoots())) {
        const QFileInfo fi(c.path);
        if (!fi.exists()) continue;
        const QDir dir = fi.dir();
        tries << dir.filePath(QStringLiteral("remote"));
        if (dir.dirName().compare(QLatin1String("Stats"), Qt::CaseInsensitive) == 0)
            tries << QDir(dir.absolutePath() + QStringLiteral("/..")).filePath(QStringLiteral("Saves"));
        tries << dir.filePath(QStringLiteral("Saves"));
    }
    // the usual places, for a game that saved before it unlocked anything
    const AchievementFiles::Roots r = AchievementFiles::systemRoots();
    const QString id = QString::number(appId);
    if (!r.publicDocs.isEmpty()) {
        tries << r.publicDocs + QStringLiteral("/Steam/CODEX/") + id + QStringLiteral("/remote")
              << r.publicDocs + QStringLiteral("/Steam/RUNE/") + id + QStringLiteral("/remote")
              << r.publicDocs + QStringLiteral("/OnlineFix/") + id + QStringLiteral("/Saves");
    }
    if (!r.appData.isEmpty()) {
        tries << r.appData + QStringLiteral("/Goldberg SteamEmu Saves/") + id + QStringLiteral("/remote")
              << r.appData + QStringLiteral("/GSE Saves/") + id + QStringLiteral("/remote")
              << r.appData + QStringLiteral("/EMPRESS/remote/") + id;
    }
    for (const QString &t : tries) {
        const QString p = QDir::cleanPath(t);
        if (QFileInfo(p).isDir() && !QDir(p).isEmpty(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot))
            return p;
    }
    return {};
}

QVariantMap QmlSessionBridge::gameSaves(const QString &infoHash) const
{
    const QString dir = saveFolderOf(infoHash);
    if (dir.isEmpty()) return {};
    const FolderStats s = statsOf(dir);
    QVariantMap m;
    m[QStringLiteral("path")] = QDir::toNativeSeparators(dir);
    m[QStringLiteral("count")] = s.files;
    m[QStringLiteral("lastMs")] = s.newestMs;
    return m;
}

// A dated copy under Documents/BATorrent Saves/<title>/, so a reinstall or a
// different crack can't take them. Returns the copy's folder, "" on failure.
QString QmlSessionBridge::backUpSaves(const QString &infoHash, const QString &title)
{
    const QString dir = saveFolderOf(infoHash);
    if (dir.isEmpty()) return {};
    QString safe = title;
    safe.replace(QRegularExpression(QStringLiteral(R"([<>:"/\\|?*])")), QStringLiteral(" "));
    const QString root = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
                         + QStringLiteral("/BATorrent Saves/") + safe.simplified();
    const QString dest = root + QLatin1Char('/') + QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH-mm-ss"));
    if (!copyTree(dir, dest)) return {};
    return QDir::toNativeSeparators(dest);
}

void QmlSessionBridge::openPath(const QString &path)
{
    if (!path.isEmpty()) QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}

// FitGirl ships languages and extras as separate files the person may have
// left out: "fg-selective-<language>.bin" and "fg-optional-<extra>.bin".
// {languages: [{name, on}], extras: [{name, on}]}; {} when the release has none.
QVariantMap QmlSessionBridge::gameExtras(const QString &infoHash) const
{
    const int row = m_session->torrentIndexByInfoHash(infoHash);
    if (row < 0) return {};
    static const QRegularExpression partRe(QStringLiteral(R"(fg-(selective|optional)-(.+?)\.bin$)"),
                                           QRegularExpression::CaseInsensitiveOption);
    QVariantList langs, extras;
    QSet<QString> seen;
    for (const FileInfo &f : m_session->filesAt(row)) {
        const QRegularExpressionMatch m = partRe.match(QFileInfo(f.path).fileName());
        if (!m.hasMatch()) continue;
        const QString name = partName(m.captured(2));
        const QString key = m.captured(1).toLower() + QLatin1Char('/') + name;
        if (seen.contains(key)) continue;
        seen.insert(key);
        const QVariantMap part{ { QStringLiteral("name"), name }, { QStringLiteral("on"), f.priority > 0 } };
        (m.captured(1).compare(QLatin1String("selective"), Qt::CaseInsensitive) == 0 ? langs : extras) << part;
    }
    if (langs.isEmpty() && extras.isEmpty()) return {};
    return { { QStringLiteral("languages"), langs }, { QStringLiteral("extras"), extras } };
}

void QmlSessionBridge::recheckGame(const QString &infoHash)
{
    const int row = m_session->torrentIndexByInfoHash(infoHash);
    if (row >= 0) m_session->forceRecheck(row);
}

QString QmlSessionBridge::gameLaunchArgs(const QString &infoHash) const
{
    return QSettings().value(QStringLiteral("gameArgs/") + infoHash).toString();
}

void QmlSessionBridge::setGameLaunchArgs(const QString &infoHash, const QString &args)
{
    QSettings s;
    if (args.trimmed().isEmpty()) s.remove(QStringLiteral("gameArgs/") + infoHash);
    else s.setValue(QStringLiteral("gameArgs/") + infoHash, args.trimmed());
}

// The game's own uninstaller, from where Windows says it is installed (the
// Uninstall entry whose folder holds the game's exe): never a folder guessed
// and deleted by hand. {exe, size}; {} when there is none to run.
QVariantMap QmlSessionBridge::gameUninstaller(const QString &infoHash)
{
    if (m_uninstallCache.contains(infoHash)) return m_uninstallCache.value(infoHash);
    QVariantMap out;
    const QString exe = QDir::cleanPath(QDir::fromNativeSeparators(gameExe(infoHash)));
    if (!exe.isEmpty()) {
        for (const auto &e : InstallerProfile::uninstallEntries()) {
            const QString loc = QDir::cleanPath(QDir::fromNativeSeparators(e.installLocation));
            if (loc.isEmpty() || !exe.startsWith(loc + QLatin1Char('/'), Qt::CaseInsensitive)) continue;
            const QStringList unins = QDir(loc).entryList({ QStringLiteral("unins*.exe") }, QDir::Files);
            if (unins.isEmpty()) break;
            out[QStringLiteral("exe")] = QDir(loc).filePath(unins.first());
            out[QStringLiteral("size")] = formatSize(statsOf(loc, 200000).bytes);
            break;
        }
    }
    m_uninstallCache.insert(infoHash, out);
    return out;
}

bool QmlSessionBridge::runUninstaller(const QString &infoHash)
{
    const QString exe = gameUninstaller(infoHash).value(QStringLiteral("exe")).toString();
    if (exe.isEmpty()) return false;
    m_uninstallCache.remove(infoHash);
#if defined(Q_OS_WIN)
    // uninstallers ask for elevation: ShellExecute shows the UAC prompt
    const auto r = reinterpret_cast<INT_PTR>(
        ShellExecuteW(nullptr, L"open", reinterpret_cast<const wchar_t *>(QDir::toNativeSeparators(exe).utf16()),
                      nullptr, nullptr, SW_SHOWNORMAL));
    return r > 32;
#else
    return QProcess::startDetached(exe, {});
#endif
}
