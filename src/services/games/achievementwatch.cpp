// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "services/games/achievementwatch.h"

AchievementWatch::AchievementWatch(const AchievementFiles::Roots &roots, QObject *parent)
    : AchievementWatch([roots](int, const QString &) { return QList<AchievementFiles::Roots>{ roots }; }, parent)
{
}

AchievementWatch::AchievementWatch(RootsFor rootsFor, QObject *parent)
    : QObject(parent), m_rootsFor(std::move(rootsFor))
{
    m_timer.setInterval(kIntervalMs);
    connect(&m_timer, &QTimer::timeout, this, &AchievementWatch::checkNow);
}

void AchievementWatch::start(const QString &key, int appId, const QString &gameDir)
{
    if (appId <= 0 || key.isEmpty()) return;
    Game g;
    g.appId = appId;
    g.dir = gameDir;
    g.files = AchievementFiles::candidates(appId, gameDir, m_rootsFor(appId, gameDir));
    g.seen = AchievementFiles::readUnlocks(g.files);
    m_games.insert(key, g);
    if (!m_timer.isActive()) m_timer.start();
}

void AchievementWatch::stop(const QString &key)
{
    auto it = m_games.find(key);
    if (it == m_games.end()) return;
    it->checks = kRescanEvery - 1;   // the last look also picks up folders made mid-session
    check(key, *it);
    m_games.erase(it);
    if (m_games.isEmpty()) m_timer.stop();
}

void AchievementWatch::checkNow()
{
    for (auto it = m_games.begin(); it != m_games.end(); ++it) check(it.key(), *it);
}

void AchievementWatch::check(const QString &key, Game &g)
{
    if (++g.checks % kRescanEvery == 0)
        g.files = AchievementFiles::candidates(g.appId, g.dir, m_rootsFor(g.appId, g.dir));
    const AchievementFormats::Unlocks now = AchievementFiles::readUnlocks(g.files);
    const QStringList fresh = AchievementFiles::newlyUnlocked(g.seen, now);
    g.seen = now;
    if (!fresh.isEmpty()) emit unlocked(key, g.appId, g.dir, fresh);
}
