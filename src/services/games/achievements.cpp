// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "services/games/achievements.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QVariantMap>
#include <algorithm>

namespace Achievements {

QList<Achievement> parseSchema(const QByteArray &json)
{
    QList<Achievement> out;
    const QJsonArray arr = QJsonDocument::fromJson(json).object()
        .value(QLatin1String("game")).toObject()
        .value(QLatin1String("availableGameStats")).toObject()
        .value(QLatin1String("achievements")).toArray();
    for (const QJsonValue &v : arr) {
        const QJsonObject o = v.toObject();
        Achievement a;
        a.apiName = o.value(QLatin1String("name")).toString();
        if (a.apiName.isEmpty()) continue;
        a.name = o.value(QLatin1String("displayName")).toString();
        if (a.name.isEmpty()) a.name = a.apiName;
        a.description = o.value(QLatin1String("description")).toString();
        a.icon = o.value(QLatin1String("icon")).toString();
        a.iconGray = o.value(QLatin1String("icongray")).toString();
        a.hidden = o.value(QLatin1String("hidden")).toInt() != 0;
        out.append(a);
    }
    return out;
}

QHash<QString, double> parsePercentages(const QByteArray &json)
{
    QHash<QString, double> out;
    const QJsonArray arr = QJsonDocument::fromJson(json).object()
        .value(QLatin1String("achievementpercentages")).toObject()
        .value(QLatin1String("achievements")).toArray();
    for (const QJsonValue &v : arr) {
        const QJsonObject o = v.toObject();
        const QString name = o.value(QLatin1String("name")).toString();
        const QJsonValue p = o.value(QLatin1String("percent"));
        bool ok = p.isDouble();
        const double pct = ok ? p.toDouble() : p.toString().toDouble(&ok);
        if (!name.isEmpty() && ok && pct >= 0 && pct <= 100) out.insert(name, pct);
    }
    return out;
}

QList<Achievement> merge(QList<Achievement> schema, const QHash<QString, double> &percent,
                         const QHash<QString, qint64> &unlocked)
{
    for (Achievement &a : schema) {
        a.percent = percent.value(a.apiName, -1);
        const auto it = unlocked.constFind(a.apiName);
        a.unlocked = it != unlocked.constEnd();
        a.unlockedAt = a.unlocked ? qMax<qint64>(0, it.value()) : 0;
    }
    std::stable_sort(schema.begin(), schema.end(), [](const Achievement &x, const Achievement &y) {
        if (x.unlocked != y.unlocked) return x.unlocked;
        return x.unlocked && x.unlockedAt > y.unlockedAt;
    });
    return schema;
}

int unlockedCount(const QList<Achievement> &list)
{
    return int(std::count_if(list.cbegin(), list.cend(), [](const Achievement &a) { return a.unlocked; }));
}

QVariantList toVariant(const QList<Achievement> &list)
{
    QVariantList out;
    out.reserve(list.size());
    for (const Achievement &a : list) {
        out << QVariantMap{
            { QStringLiteral("apiName"), a.apiName },
            { QStringLiteral("name"), a.name },
            { QStringLiteral("description"), a.description },
            { QStringLiteral("icon"), a.icon },
            { QStringLiteral("iconGray"), a.iconGray },
            { QStringLiteral("hidden"), a.hidden },
            { QStringLiteral("percent"), a.percent },
            { QStringLiteral("unlocked"), a.unlocked },
            { QStringLiteral("unlockedAt"), a.unlockedAt },
        };
    }
    return out;
}

} // namespace Achievements
