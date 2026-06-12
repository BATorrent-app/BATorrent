// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef STATSHISTORY_H
#define STATSHISTORY_H

#include <QString>
#include <QJsonObject>

// Local per-day usage history (bytes moved, torrents added/completed, per
// category). Feeds the future "Year in Torrents" + richer Statistics — data
// not collected can't be shown retroactively, so this starts tiny and early.
// Plain JSON next to the resume data; a year is a few hundred small entries.
// Never leaves the machine.
class StatsHistory
{
public:
    explicit StatsHistory(const QString &filePath);
    ~StatsHistory();

    // absolute monotonic session counters (SessionManager::globalDownloaded /
    // globalUploaded) — deltas are computed and clamped here
    void recordTransfer(qint64 downAbs, qint64 upAbs);
    void recordAdded();
    void recordCompleted(const QString &category);

    void flush();
    QJsonObject days() const { return m_days; }

private:
    void load();
    void bump(const QString &key, qint64 delta);
    void maybeFlush();

    QString m_path;
    QJsonObject m_days;
    qint64 m_lastDown = -1;
    qint64 m_lastUp = -1;
    qint64 m_lastFlushAt = 0;
    bool m_dirty = false;
};

#endif
