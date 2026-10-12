// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "services/games/achievementledger.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>

namespace {
constexpr int kFormat = 1;
constexpr qint64 kMaxFile = 32 * 1024 * 1024;

// The earlier of two unlock times; 0 means unknown and loses to a real one.
qint64 earliest(qint64 a, qint64 b)
{
    if (a <= 0) return b;
    if (b <= 0) return a;
    return qMin(a, b);
}
} // namespace

AchievementLedger::AchievementLedger(const QString &path) : m_path(path) {}

QByteArray AchievementLedger::toJson(const Games &games)
{
    QJsonObject root;
    root.insert(QStringLiteral("format"), kFormat);
    QJsonObject out;
    for (auto g = games.cbegin(); g != games.cend(); ++g) {
        QJsonObject unlocks;
        for (auto u = g->unlocks.cbegin(); u != g->unlocks.cend(); ++u)
            unlocks.insert(u.key(), double(u.value()));
        QJsonObject game;
        if (!g->title.isEmpty()) game.insert(QStringLiteral("title"), g->title);
        game.insert(QStringLiteral("unlocks"), unlocks);
        out.insert(QString::number(g.key()), game);
    }
    root.insert(QStringLiteral("games"), out);
    return QJsonDocument(root).toJson(QJsonDocument::Indented);
}

bool AchievementLedger::fromJson(const QByteArray &json, Games *out)
{
    const QJsonObject root = QJsonDocument::fromJson(json).object();
    if (!root.value(QLatin1String("games")).isObject()) return false;
    const QJsonObject games = root.value(QLatin1String("games")).toObject();
    Games parsed;
    for (auto g = games.constBegin(); g != games.constEnd(); ++g) {
        bool ok = false;
        const int appId = g.key().toInt(&ok);
        if (!ok || appId <= 0) continue;
        const QJsonObject game = g.value().toObject();
        Game entry;
        entry.title = game.value(QLatin1String("title")).toString();
        const QJsonObject unlocks = game.value(QLatin1String("unlocks")).toObject();
        for (auto u = unlocks.constBegin(); u != unlocks.constEnd(); ++u)
            if (!u.key().isEmpty()) entry.unlocks.insert(u.key(), qint64(u.value().toDouble()));
        if (!entry.unlocks.isEmpty()) parsed.insert(appId, entry);
    }
    *out = parsed;
    return true;
}

int AchievementLedger::fold(Games &into, const Games &from)
{
    int added = 0;
    for (auto g = from.cbegin(); g != from.cend(); ++g) {
        Game &mine = into[g.key()];
        if (mine.title.isEmpty()) mine.title = g->title;
        for (auto u = g->unlocks.cbegin(); u != g->unlocks.cend(); ++u) {
            auto have = mine.unlocks.find(u.key());
            if (have == mine.unlocks.end()) { mine.unlocks.insert(u.key(), u.value()); ++added; }
            else *have = earliest(*have, u.value());
        }
    }
    return added;
}

void AchievementLedger::load() const
{
    if (m_loaded) return;
    m_loaded = true;
    QFile f(m_path);
    if (f.size() > 0 && f.size() < kMaxFile && f.open(QIODevice::ReadOnly))
        fromJson(f.readAll(), &m_games);
}

bool AchievementLedger::save() const
{
    QDir().mkpath(QFileInfo(m_path).absolutePath());
    QSaveFile out(m_path);
    if (!out.open(QIODevice::WriteOnly)) return false;
    out.write(toJson(m_games));
    return out.commit();
}

AchievementFormats::Unlocks AchievementLedger::merged(int appId, const QString &title,
                                                      const AchievementFormats::Unlocks &fromFiles)
{
    load();
    if (appId <= 0) return fromFiles;
    if (fromFiles.isEmpty() && !m_games.contains(appId)) return {};   // nothing yet, no empty entry
    const QString titleBefore = m_games.value(appId).title;
    Games seen;
    seen.insert(appId, Game{ title, fromFiles });
    const bool grew = fold(m_games, seen) > 0;
    // An entry first written without a title gets one once the schema has it.
    if (grew || m_games.value(appId).title != titleBefore) save();
    return m_games.value(appId).unlocks;
}

bool AchievementLedger::exportTo(const QString &path) const
{
    load();
    QSaveFile out(path);
    if (!out.open(QIODevice::WriteOnly)) return false;
    out.write(toJson(m_games));
    return out.commit();
}

int AchievementLedger::importFrom(const QString &path)
{
    load();
    QFile f(path);
    if (f.size() <= 0 || f.size() >= kMaxFile || !f.open(QIODevice::ReadOnly)) return -1;
    Games theirs;
    if (!fromJson(f.readAll(), &theirs)) return -1;
    const int added = fold(m_games, theirs);
    save();
    return added;
}
