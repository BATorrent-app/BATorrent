// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#pragma once

#include <QHash>
#include <QList>
#include <QString>

// Per-game play history: every session with when it started and how long it
// ran, plus the launch count and last launch. Plain JSON next to the stats
// history, written atomically on every change (a change is a game launching or
// quitting, never a tick).
class PlayLog
{
public:
    struct Session {
        qint64 startMs = 0;
        qint64 secs = 0;
    };
    struct Summary {
        qint64 totalSecs = 0;
        qint64 lastPlayedMs = 0;
        qint64 launches = 0;
    };

    // Shorter than this is a launcher handing off or a crash on start, not play.
    static constexpr qint64 kMinSessionSecs = 30;
    // Older sessions fold into a running total so the file stays small.
    static constexpr int kMaxSessions = 200;

    explicit PlayLog(const QString &filePath);

    void recordLaunch(const QString &hash, qint64 nowMs);
    // False when the session was too short to count.
    bool recordSession(const QString &hash, qint64 startMs, qint64 secs);

    Summary summary(const QString &hash) const;
    QList<Session> recentSessions(const QString &hash, int limit) const;   // newest first
    bool contains(const QString &hash) const { return m_games.contains(hash); }

    // Totals kept before sessions were recorded. Only applies to a game the
    // log has never seen, so importing twice is harmless.
    void importTotals(const QString &hash, qint64 secs, qint64 lastPlayedMs, qint64 launches);

    bool save() const;

private:
    struct Game {
        qint64 foldedSecs = 0;   // sessions dropped past kMaxSessions, and imported totals
        qint64 lastPlayedMs = 0;
        qint64 launches = 0;
        QList<Session> sessions;   // oldest first
    };
    void load();

    QString m_path;
    QHash<QString, Game> m_games;
};
