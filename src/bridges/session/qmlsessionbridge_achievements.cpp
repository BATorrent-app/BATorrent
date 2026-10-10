// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details
//
// QmlSessionBridge: achievements slice. Finds the Steam appid a game's own
// files declare and hands the lookup to AchievementService.

#include "bridges/session/qmlsessionbridge.h"
#include "services/games/achievementfiles.h"
#include "services/games/achievementservice.h"

#include <QFileInfo>

void QmlSessionBridge::requestAchievements(const QString &infoHash, int steamAppIdHint)
{
    if (!m_achievements) return;
    // A repack installs somewhere else: the chosen exe's folder is the game.
    const QString exe = gameExe(infoHash);
    const QString dir = (!exe.isEmpty() && QFileInfo::exists(exe)) ? QFileInfo(exe).absolutePath()
                                                                   : gameFolder(infoHash);
    // The crack's own appid file names the build actually installed; IGDB's
    // id is a match by title and only a fallback.
    int appId = AchievementFiles::appIdFromGameDir(dir);
    if (appId <= 0) appId = steamAppIdHint;
    if (appId <= 0) {
        emit achievementsReady(infoHash, {}, 0, 0);
        return;
    }
    m_achievements->request(infoHash, appId, dir);
}
