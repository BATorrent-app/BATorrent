// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details
//
// QmlSessionBridge: achievements slice. Finds the Steam appid a game's own
// files declare and hands the lookup to AchievementService.

#include "bridges/session/qmlsessionbridge.h"
#include "services/games/achievementfiles.h"
#include "services/games/achievementledger.h"
#include "services/games/achievementservice.h"
#include "services/games/achievementwatch.h"
#include "services/games/steamstore.h"
#include "services/platform/settingsbackup.h"
#include "services/platform/soundplayer.h"
#include "services/platform/translator.h"

#include <QFileInfo>
#include <QSettings>

bool QmlSessionBridge::exportAchievements(const QString &fileUrl)
{
    const QString path = SettingsBackup::localPath(fileUrl);
    const bool ok = !path.isEmpty() && m_achLedger && m_achLedger->exportTo(path);
    qInfo().noquote() << "[achievements] export to" << path << (ok ? "done" : "failed");
    emit toast(ok ? tr_("ach_export_done") : tr_("ach_export_failed"), QFileInfo(path).fileName());
    return ok;
}

void QmlSessionBridge::importAchievements(const QString &fileUrl)
{
    const QString path = SettingsBackup::localPath(fileUrl);
    const int added = m_achLedger && !path.isEmpty() ? m_achLedger->importFrom(path) : -1;
    qInfo().noquote() << "[achievements] import from" << path << "added" << added;
    if (added < 0) emit toast(tr_("ach_import_failed"), QFileInfo(path).fileName());
    else emit toast(tr_("ach_import_done").arg(added), QFileInfo(path).fileName());
    if (added > 0) emit gamesChanged();   // open game pages ask for their list again
}

QString QmlSessionBridge::achievementDir(const QString &infoHash) const
{
    // A repack installs somewhere else: the chosen exe's folder is the game.
    const QString exe = gameExe(infoHash);
    return (!exe.isEmpty() && QFileInfo::exists(exe)) ? QFileInfo(exe).absolutePath() : gameFolder(infoHash);
}

void QmlSessionBridge::requestAchievements(const QString &infoHash, int steamAppIdHint, const QString &title)
{
    if (!m_achievements) return;
    if (!title.isEmpty()) m_achTitle.insert(infoHash, title);
    const QString dir = achievementDir(infoHash);
    // The crack's own appid file names the build actually installed; IGDB's
    // id is a match by title and only a fallback.
    int appId = AchievementFiles::appIdFromGameDir(dir);
    if (appId <= 0) appId = steamAppIdHint;
    m_achievements->request(infoHash, appId, dir, title);
}

QVariantMap QmlSessionBridge::steamArt(int appId) const
{
    if (appId <= 0) return {};
    return { { QStringLiteral("hero"), SteamStore::heroUrl(appId) },
             { QStringLiteral("logo"), SteamStore::logoUrl(appId) } };
}

void QmlSessionBridge::onAchievementsLookedUp(const QString &key, int appId, const QVariantList &list)
{
    if (appId > 0) m_achAppId.insert(key, appId);
    const QStringList waiting = m_achAnnounce.take(key);
    for (const QString &api : waiting) {
        for (const QVariant &v : list) {
            const QVariantMap a = v.toMap();
            if (a.value(QStringLiteral("apiName")).toString() != api) continue;
            const bool rare = a.value(QStringLiteral("rare")).toBool();
            // no sound of our own over a game: it has its own
            // how far along: the ones already marked, and this one if the list lags
            int done = 0;
            for (const QVariant &o : list)
                if (o.toMap().value(QStringLiteral("unlocked")).toBool()) ++done;
            if (!a.value(QStringLiteral("unlocked")).toBool()) ++done;
            emit achievementUnlocked(m_achTitle.value(key), a.value(QStringLiteral("name")).toString(),
                                     a.value(QStringLiteral("icon")).toString(),
                                     a.value(QStringLiteral("percentLabel")).toString(), rare,
                                     a.value(QStringLiteral("description")).toString(), done, int(list.size()));
            break;
        }
    }
}

void QmlSessionBridge::startAchievementWatch(const QString &infoHash)
{
    if (!m_achWatch || !QSettings().value(QStringLiteral("notifyAchievements"), true).toBool()) return;
    const QString dir = achievementDir(infoHash);
    int appId = AchievementFiles::appIdFromGameDir(dir);
    if (appId <= 0) appId = m_achAppId.value(infoHash);
    m_achWatch->start(infoHash, appId, dir);
}

void QmlSessionBridge::stopAchievementWatch(const QString &infoHash)
{
    if (m_achWatch) m_achWatch->stop(infoHash);
}
