// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details
//
// QmlSessionBridge: achievements slice. Finds the Steam appid a game's own
// files declare and hands the lookup to AchievementService.

#include "bridges/session/qmlsessionbridge.h"
#include "services/games/achievementfiles.h"
#include "services/games/achievementservice.h"
#include "services/games/achievementwatch.h"
#include "services/games/steamstore.h"

#include <QFileInfo>
#include <QSettings>

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
            emit achievementUnlocked(m_achTitle.value(key), a.value(QStringLiteral("name")).toString(),
                                     a.value(QStringLiteral("icon")).toString());
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
