// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#pragma once

#include "services/games/achievementfiles.h"

#include <QHash>
#include <QObject>
#include <QStringList>
#include <QTimer>
#include <functional>

// Polled, not QFileSystemWatcher: emulators rewrite the whole file, and a replaced file drops out of a watcher.
class AchievementWatch : public QObject
{
    Q_OBJECT
public:
    using RootsFor = std::function<QList<AchievementFiles::Roots>(int appId, const QString &gameDir)>;
    // Defaults to AchievementFiles::rootsFor; a test hands in its own folders.
    explicit AchievementWatch(RootsFor rootsFor = AchievementFiles::rootsFor, QObject *parent = nullptr);
    explicit AchievementWatch(const AchievementFiles::Roots &roots, QObject *parent = nullptr);

    // What is already unlocked when watching starts is the baseline, never news.
    void start(const QString &key, int appId, const QString &gameDir);
    // One last look, then forget the game: the final unlock often lands as it quits.
    void stop(const QString &key);
    bool watching(const QString &key) const { return m_games.contains(key); }

    void checkNow();

    static constexpr int kIntervalMs = 4000;
    // Every this many checks the file list is worked out again: a profile
    // folder some emulators create on the first unlock wasn't there at launch.
    static constexpr int kRescanEvery = 8;

signals:
    void unlocked(const QString &key, int appId, const QString &gameDir, const QStringList &apiNames);

private:
    struct Game {
        int appId = 0;
        QString dir;
        QList<AchievementFiles::Candidate> files;
        AchievementFormats::Unlocks seen;
        int checks = 0;
    };
    void check(const QString &key, Game &g);

    RootsFor m_rootsFor;
    QHash<QString, Game> m_games;
    QTimer m_timer;
};
