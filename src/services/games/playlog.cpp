// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "services/games/playlog.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>

namespace {

qint64 jsonInt(const QJsonValue &v)
{
    const double d = v.toDouble();
    return d > 0 ? qint64(d) : 0;
}

} // namespace

PlayLog::PlayLog(const QString &filePath)
    : m_path(filePath)
{
    load();
}

void PlayLog::load()
{
    QFile f(m_path);
    if (!f.open(QIODevice::ReadOnly)) return;
    const QJsonObject games = QJsonDocument::fromJson(f.readAll()).object()
                                  .value(QLatin1String("games")).toObject();
    for (auto it = games.begin(); it != games.end(); ++it) {
        const QJsonObject o = it.value().toObject();
        Game g;
        g.foldedSecs = jsonInt(o.value(QLatin1String("foldedSecs")));
        g.lastPlayedMs = jsonInt(o.value(QLatin1String("lastPlayed")));
        g.launches = jsonInt(o.value(QLatin1String("launches")));
        for (const QJsonValue &sv : o.value(QLatin1String("sessions")).toArray()) {
            const QJsonArray pair = sv.toArray();
            if (pair.size() != 2) continue;
            const Session s{ jsonInt(pair.at(0)), jsonInt(pair.at(1)) };
            if (s.startMs > 0 && s.secs > 0) g.sessions.append(s);
        }
        m_games.insert(it.key(), g);
    }
}

bool PlayLog::save() const
{
    QJsonObject games;
    for (auto it = m_games.constBegin(); it != m_games.constEnd(); ++it) {
        const Game &g = it.value();
        QJsonArray sessions;
        for (const Session &s : g.sessions)
            sessions.append(QJsonArray{ double(s.startMs), double(s.secs) });
        games.insert(it.key(), QJsonObject{
            { QStringLiteral("foldedSecs"), double(g.foldedSecs) },
            { QStringLiteral("lastPlayed"), double(g.lastPlayedMs) },
            { QStringLiteral("launches"), double(g.launches) },
            { QStringLiteral("sessions"), sessions },
        });
    }
    QDir().mkpath(QFileInfo(m_path).absolutePath());
    QSaveFile f(m_path);
    if (!f.open(QIODevice::WriteOnly)) return false;
    const QJsonObject root{ { QStringLiteral("v"), 1 }, { QStringLiteral("games"), games } };
    f.write(QJsonDocument(root).toJson(QJsonDocument::Compact));
    return f.commit();
}

void PlayLog::recordLaunch(const QString &hash, qint64 nowMs)
{
    Game &g = m_games[hash];
    g.lastPlayedMs = nowMs;
    ++g.launches;
    save();
}

bool PlayLog::recordSession(const QString &hash, qint64 startMs, qint64 secs)
{
    if (secs <= kMinSessionSecs || startMs <= 0) return false;
    Game &g = m_games[hash];
    g.sessions.append({ startMs, secs });
    while (g.sessions.size() > kMaxSessions)
        g.foldedSecs += g.sessions.takeFirst().secs;
    save();
    return true;
}

PlayLog::Summary PlayLog::summary(const QString &hash) const
{
    const auto it = m_games.constFind(hash);
    if (it == m_games.constEnd()) return {};
    Summary s;
    s.totalSecs = it->foldedSecs;
    for (const Session &x : it->sessions) s.totalSecs += x.secs;
    s.lastPlayedMs = it->lastPlayedMs;
    s.launches = it->launches;
    return s;
}

QList<PlayLog::Session> PlayLog::recentSessions(const QString &hash, int limit) const
{
    QList<Session> out;
    const auto it = m_games.constFind(hash);
    if (it == m_games.constEnd()) return out;
    for (int i = int(it->sessions.size()) - 1; i >= 0 && out.size() < limit; --i)
        out.append(it->sessions.at(i));
    return out;
}

void PlayLog::importTotals(const QString &hash, qint64 secs, qint64 lastPlayedMs, qint64 launches)
{
    if (m_games.contains(hash)) return;
    if (secs <= 0 && lastPlayedMs <= 0 && launches <= 0) return;
    Game g;
    g.foldedSecs = qMax<qint64>(0, secs);
    g.lastPlayedMs = qMax<qint64>(0, lastPlayedMs);
    g.launches = qMax<qint64>(0, launches);
    m_games.insert(hash, g);
}
