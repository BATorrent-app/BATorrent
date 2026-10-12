// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#pragma once

#include <QHash>
#include <QNetworkAccessManager>
#include <QObject>
#include <QString>
#include <QVariantList>

// A game's achievements: Steam's schema and global percentages (cached on disk
// for a week, a stale copy used when Steam can't be reached), crossed with the
// unlocks the game's emulator wrote. One request in flight per appid.
class AchievementLedger;

class AchievementService : public QObject
{
    Q_OBJECT
public:
    explicit AchievementService(const QString &cacheDir, QObject *parent = nullptr);

    // `key` comes back on the signal so the caller can tell its games apart.
    // Without an appid, Steam's store is searched for `title`.
    void request(const QString &key, int appId, const QString &gameDir, const QString &title = {});

    static QString steamApiKey();

    // Where unlocks outlive the emulator's own files. Not owned; may be null.
    void setLedger(AchievementLedger *ledger) { m_ledger = ledger; }

signals:
    // total is 0 when no schema could be had: the list then holds only what is
    // unlocked, under its raw ids.
    void ready(const QString &key, int appId, const QVariantList &achievements, int unlocked, int total);

private:
    struct Waiting { QString key; QString gameDir; };
    void lookUp(const QString &key, const QString &gameDir, const QString &title);
    void fetch(int appId);
    void finish(int appId, const QByteArray &schema, const QByteArray &percent);
    QString cachePath(int appId) const;

    QNetworkAccessManager m_nam;
    QString m_cacheDir;
    QHash<int, QList<Waiting>> m_waiting;
    QHash<QString, int> m_appIdByTitle;   // store-search answers for the session, 0 = not on Steam
    AchievementLedger *m_ledger = nullptr;
};
