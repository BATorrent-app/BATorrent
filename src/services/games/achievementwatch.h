// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#pragma once

#include "services/games/achievementfiles.h"

#include <QHash>
#include <QObject>
#include <QStringList>
#include <QTimer>

// Watches a running game's emulator files and says when something new is
// unlocked. Polled, not QFileSystemWatcher: emulators rewrite the whole file,
// and a replaced file drops out of a watcher without a word.
class AchievementWatch : public QObject
{
    Q_OBJECT
public:
    explicit AchievementWatch(const AchievementFiles::Roots &roots, QObject *parent = nullptr);

    // What is already unlocked when watching starts is the baseline, never news.
    void start(const QString &key, int appId, const QString &gameDir);
    // One last look, then forget the game: the final unlock often lands as it quits.
    void stop(const QString &key);
    bool watching(const QString &key) const { return m_games.contains(key); }

    void checkNow();

    static constexpr int kIntervalMs = 4000;

signals:
    void unlocked(const QString &key, int appId, const QString &gameDir, const QStringList &apiNames);

private:
    struct Game {
        int appId = 0;
        QString dir;
        QList<AchievementFiles::Candidate> files;
        AchievementFormats::Unlocks seen;
    };
    void check(const QString &key, Game &g);

    AchievementFiles::Roots m_roots;
    QHash<QString, Game> m_games;
    QTimer m_timer;
};
