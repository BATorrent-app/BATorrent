// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "services/games/achievementfiles.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QQueue>
#include <QStandardPaths>
#include <QStringList>
#include <algorithm>

using AchievementFormats::Format;
using AchievementFormats::Unlocks;

namespace AchievementFiles {

namespace {

constexpr qint64 kMaxFile = 4 * 1024 * 1024;
constexpr int kMaxDepth = 4;
constexpr int kMaxEntries = 4000;

QString join(const QString &base, const QString &rel)
{
    return base.isEmpty() ? QString() : QDir::cleanPath(base + QLatin1Char('/') + rel);
}

QByteArray readCapped(const QString &path)
{
    QFile f(path);
    if (f.size() <= 0 || f.size() > kMaxFile || !f.open(QIODevice::ReadOnly)) return {};
    return f.readAll();
}

// One bounded walk of the game folder: the folders a crack sits in (they hold
// steam_api.dll or steam_appid.txt) and every steam_settings folder.
struct Scan {
    QStringList bases;
    QStringList steamSettings;
};

Scan scan(const QString &gameDir)
{
    Scan out;
    if (gameDir.isEmpty() || !QFileInfo(gameDir).isDir()) return out;
    out.bases << QDir::cleanPath(gameDir);
    QQueue<QPair<QString, int>> queue;
    queue.enqueue({ QDir::cleanPath(gameDir), 0 });
    int seen = 0;
    while (!queue.isEmpty() && seen < kMaxEntries) {
        const auto [dir, depth] = queue.dequeue();
        const QFileInfoList entries = QDir(dir).entryInfoList(
            QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot | QDir::NoSymLinks);
        bool isBase = false;
        for (const QFileInfo &e : entries) {
            if (++seen > kMaxEntries) break;
            const QString name = e.fileName().toLower();
            if (e.isDir()) {
                if (name == QLatin1String("steam_settings")) out.steamSettings << e.absoluteFilePath();
                else if (depth < kMaxDepth) queue.enqueue({ e.absoluteFilePath(), depth + 1 });
            } else if (name == QLatin1String("steam_api.dll") || name == QLatin1String("steam_api64.dll")
                       || name == QLatin1String("steam_appid.txt")) {
                isBase = true;
            }
        }
        if (isBase && !out.bases.contains(dir)) out.bases << dir;
    }
    return out;
}

QStringList subdirs(const QString &dir)
{
    QStringList out;
    for (const QFileInfo &e : QDir(dir).entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot | QDir::NoSymLinks))
        out << e.absoluteFilePath();
    return out;
}

int digitsIn(const QString &s)
{
    bool ok = false;
    const int n = s.trimmed().toInt(&ok);
    return ok && n > 0 ? n : 0;
}

} // namespace

Roots systemRoots()
{
    Roots r;
#if defined(Q_OS_WIN)
    r.appData = qEnvironmentVariable("APPDATA");
    r.localAppData = qEnvironmentVariable("LOCALAPPDATA");
    const QString pub = qEnvironmentVariable("PUBLIC");
    r.publicDocs = pub.isEmpty() ? QString() : join(pub, QStringLiteral("Documents"));
    r.programData = qEnvironmentVariable("PROGRAMDATA");
    r.documents = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
#elif defined(Q_OS_LINUX)
    const QString xdg = qEnvironmentVariable("XDG_DATA_HOME");
    r.gseSaves = join(xdg.isEmpty() ? join(QDir::homePath(), QStringLiteral(".local/share")) : xdg,
                      QStringLiteral("GSE Saves"));
#endif
    return r;
}

QList<Candidate> systemCandidates(int appId, const Roots &r)
{
    if (appId <= 0) return {};
    const QString id = QString::number(appId);
    const QList<QPair<QString, Format>> rel = {
        { join(r.publicDocs, "Steam/CODEX/" + id + "/achievements.ini"), Format::CodexIni },
        { join(r.appData, "Steam/CODEX/" + id + "/achievements.ini"), Format::CodexIni },
        { join(r.publicDocs, "Steam/RUNE/" + id + "/achievements.ini"), Format::CodexIni },
        { join(r.publicDocs, "OnlineFix/" + id + "/Stats/Achievements.ini"), Format::OnlineFixIni },
        { join(r.publicDocs, "OnlineFix/" + id + "/Achievements.ini"), Format::OnlineFixIni },
        { join(r.appData, "Goldberg SteamEmu Saves/" + id + "/achievements.json"), Format::GoldbergJson },
        { join(r.appData, "GSE Saves/" + id + "/achievements.json"), Format::GoldbergJson },
        { join(r.gseSaves, id + "/achievements.json"), Format::GoldbergJson },
        { join(r.appData, "EMPRESS/remote/" + id + "/achievements.json"), Format::GoldbergJson },
        { join(r.publicDocs, "EMPRESS/" + id + "/remote/" + id + "/achievements.json"), Format::GoldbergJson },
        { join(r.programData, "RLD!/" + id + "/achievements.ini"), Format::RldIni },
        { join(r.programData, "Steam/Player/" + id + "/stats/achievements.ini"), Format::RldIni },
        { join(r.programData, "Steam/RLD!/" + id + "/stats/achievements.ini"), Format::RldIni },
        { join(r.programData, "Steam/dodi/" + id + "/stats/achievements.ini"), Format::RldIni },
        { join(r.documents, "SKIDROW/" + id + "/SteamEmu/UserStats/achiev.ini"), Format::SkidrowIni },
        { join(r.documents, "Player/" + id + "/SteamEmu/UserStats/achiev.ini"), Format::SkidrowIni },
        { join(r.localAppData, "SKIDROW/" + id + "/SteamEmu/UserStats/achiev.ini"), Format::SkidrowIni },
        { join(r.appData, "CreamAPI/" + id + "/stats/CreamAPI.Achievements.cfg"), Format::CreamApiIni },
    };
    QList<Candidate> out;
    for (const auto &p : rel)
        if (!p.first.isEmpty()) out.append({ p.first, p.second });
    return out;
}

QList<Candidate> gameDirCandidates(int appId, const QString &gameDir)
{
    QList<Candidate> out;
    const Scan s = scan(gameDir);
    for (const QString &settings : s.steamSettings) {
        for (const QString &sub : subdirs(settings))
            if (digitsIn(QFileInfo(sub).fileName()) > 0)
                out.append({ join(sub, QStringLiteral("achievements.json")), Format::GoldbergJson });
        // gbe's local_save_path moves the whole save folder next to the game,
        // relative to the exe, which sits beside steam_settings.
        if (appId > 0) {
            const auto ini = AchievementFormats::parseIni(readCapped(join(settings, QStringLiteral("configs.user.ini"))));
            QString local = ini.value(QStringLiteral("user::saves")).value(QStringLiteral("local_save_path"));
            if (!local.isEmpty()) {
                if (QDir::isRelativePath(local)) local = join(QFileInfo(settings).absolutePath(), local);
                out.append({ join(local, QString::number(appId) + QStringLiteral("/achievements.json")),
                             Format::GoldbergJson });
            }
        }
    }
    for (const QString &base : s.bases) {
        for (const QString &profile : subdirs(join(base, QStringLiteral("3DMGAME"))))
            out.append({ join(profile, QStringLiteral("stats/achievements.ini")), Format::ThreeDmIni });
        for (const QString &profile : subdirs(join(base, QStringLiteral("Profile"))))
            out.append({ join(profile, QStringLiteral("Stats/Achievements.Bin")), Format::CodexIni });
    }
    return out;
}

QList<Candidate> candidates(int appId, const QString &gameDir, const Roots &roots)
{
    return systemCandidates(appId, roots) + gameDirCandidates(appId, gameDir);
}

Unlocks readUnlocks(const QList<Candidate> &files)
{
    Unlocks out;
    for (const Candidate &c : files) {
        const QByteArray bytes = readCapped(c.path);
        if (bytes.isEmpty()) continue;
        const Unlocks got = AchievementFormats::parse(c.format, bytes);
        for (auto it = got.cbegin(); it != got.cend(); ++it) {
            const auto have = out.find(it.key());
            if (have == out.end()) out.insert(it.key(), it.value());
            else if (it.value() > 0 && (*have == 0 || it.value() < *have)) *have = it.value();
        }
    }
    return out;
}

Unlocks readUnlocks(int appId, const QString &gameDir, const Roots &roots)
{
    return readUnlocks(candidates(appId, gameDir, roots));
}

Unlocks readUnlocks(int appId, const QString &gameDir)
{
    return readUnlocks(appId, gameDir, systemRoots());
}

AchievementFormats::Progress readProgress(const QList<Candidate> &files)
{
    AchievementFormats::Progress out;
    for (const Candidate &c : files) {
        const QByteArray bytes = readCapped(c.path);
        if (bytes.isEmpty()) continue;
        const auto got = AchievementFormats::parseProgress(c.format, bytes);
        for (auto it = got.cbegin(); it != got.cend(); ++it)
            if (!out.contains(it.key()) || it.value().first > out.value(it.key()).first)
                out.insert(it.key(), it.value());
    }
    return out;
}

QStringList newlyUnlocked(const Unlocks &before, const Unlocks &now)
{
    QList<QPair<qint64, QString>> fresh;
    for (auto it = now.cbegin(); it != now.cend(); ++it)
        if (!before.contains(it.key())) fresh.append({ it.value(), it.key() });
    std::sort(fresh.begin(), fresh.end());
    QStringList out;
    for (const auto &f : fresh) out << f.second;
    return out;
}

int appIdFromGameDir(const QString &gameDir)
{
    const Scan s = scan(gameDir);
    QStringList dirs = s.bases;
    dirs += s.steamSettings;
    for (const QString &dir : dirs) {
        const int id = digitsIn(QString::fromLatin1(readCapped(join(dir, QStringLiteral("steam_appid.txt")))
                                                       .split('\n').value(0)));
        if (id > 0) return id;
    }
    // The emulators that keep it in their own config instead.
    static const char *const inis[] = { "steam_emu.ini", "ALI213.ini", "valve.ini", "hlm.ini", "ds.ini",
                                        "steam_api.ini", "SteamConfig.ini", "OnlineFix.ini" };
    for (const QString &dir : s.bases) {
        for (const char *name : inis) {
            const auto ini = AchievementFormats::parseIni(readCapped(join(dir, QLatin1String(name))));
            for (auto sec = ini.cbegin(); sec != ini.cend(); ++sec)
                for (auto kv = sec->cbegin(); kv != sec->cend(); ++kv)
                    if (kv.key().compare(QLatin1String("AppId"), Qt::CaseInsensitive) == 0
                        || kv.key().compare(QLatin1String("RealAppId"), Qt::CaseInsensitive) == 0)
                        if (const int id = digitsIn(kv.value())) return id;
        }
    }
    return 0;
}

} // namespace AchievementFiles
