// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "services/discovery/igdbparse.h"

#include <QDateTime>
#include <QHash>
#include <QJsonDocument>
#include <QJsonValue>
#include <QVariantMap>
#include <algorithm>

namespace IgdbParse {

bool isFreeLiveService(const QString &name)
{
    static const QStringList needles = {
        QStringLiteral("counter-strike"), QStringLiteral("dota 2"),
        QStringLiteral("league of legends"), QStringLiteral("fortnite"),
        QStringLiteral("valorant"), QStringLiteral("apex legends"),
        QStringLiteral("warframe"), QStringLiteral("roblox"),
        QStringLiteral("marvel rivals"), QStringLiteral("the finals"),
        QStringLiteral("overwatch"), QStringLiteral("destiny 2"),
        QStringLiteral("team fortress"), QStringLiteral("warzone"),
        QStringLiteral("rocket league"), QStringLiteral("fall guys"),
        QStringLiteral("brawlhalla"), QStringLiteral("naraka"),
        QStringLiteral("smite"), QStringLiteral("paladins"),
        QStringLiteral("splitgate"), QStringLiteral("the first descendant"),
        QStringLiteral("xdefiant"), QStringLiteral("deadlock"),
        QStringLiteral("multiversus"), QStringLiteral("the day before"),
        QStringLiteral("pubg"), QStringLiteral("playerunknown"),
        QStringLiteral("delta force"), QStringLiteral("crossfire"),
        QStringLiteral("spellbreak"), QStringLiteral("hyper scape"),
        QStringLiteral("genshin"), QStringLiteral("honkai"),
        QStringLiteral("wuthering waves"), QStringLiteral("zenless zone zero"),
        QStringLiteral("tower of fantasy"), QStringLiteral("blue archive"),
        QStringLiteral("arknights"), QStringLiteral("nikke"),
        QStringLiteral("goddess of victory"), QStringLiteral("punishing: gray raven"),
        QStringLiteral("epic seven"), QStringLiteral("summoners war"),
        QStringLiteral("raid: shadow legends"), QStringLiteral("diablo immortal"),
        QStringLiteral("lost ark"), QStringLiteral("new world"),
        QStringLiteral("throne and liberty"), QStringLiteral("once human"),
        QStringLiteral("world of warcraft"), QStringLiteral("final fantasy xiv"),
        QStringLiteral("elder scrolls online"),
        QStringLiteral("star wars: the old republic"),
        QStringLiteral("guild wars"), QStringLiteral("neverwinter"),
        QStringLiteral("runescape"), QStringLiteral("eve online"),
        QStringLiteral("star trek online"), QStringLiteral("dc universe online"),
        QStringLiteral("phantasy star online"), QStringLiteral("blade and soul"),
        QStringLiteral("blade & soul"), QStringLiteral("tera online"),
        QStringLiteral("vindictus"), QStringLiteral("dauntless"),
        QStringLiteral("albion online"), QStringLiteral("lord of the rings online"),
        QStringLiteral("war thunder"), QStringLiteral("world of tanks"),
        QStringLiteral("world of warships"), QStringLiteral("crossout"),
        QStringLiteral("enlisted"), QStringLiteral("star conflict"),
        QStringLiteral("hearthstone"), QStringLiteral("legends of runeterra"),
        QStringLiteral("magic: the gathering arena"),
        QStringLiteral("yu-gi-oh! master duel"), QStringLiteral("yu-gi-oh master duel"),
        QStringLiteral("marvel snap"), QStringLiteral("mobile legends"),
        QStringLiteral("honor of kings"), QStringLiteral("clash of"),
        QStringLiteral("pokemon unite"), QStringLiteral("pokemon go"),
    };
    const QString lname = name.toLower();
    for (const QString &d : needles) {
        if (lname.contains(d))
            return true;
    }
    return false;
}

QList<QJsonObject> objectsFromArray(const QJsonArray &arr)
{
    QList<QJsonObject> objs;
    objs.reserve(arr.size());
    for (const QJsonValue &v : arr)
        objs.append(v.toObject());
    return objs;
}

QList<QJsonObject> objectsFromJson(const QByteArray &jsonArray)
{
    if (jsonArray.isEmpty())
        return {};
    return objectsFromArray(QJsonDocument::fromJson(jsonArray).array());
}

static QString firstImageUrl(const QJsonArray &arr, const QString &templateUrl)
{
    if (arr.isEmpty())
        return {};
    const QString id = arr.first().toObject().value(QLatin1String("image_id")).toString();
    if (id.isEmpty())
        return {};
    return templateUrl.arg(id);
}

QVariantList gameCards(const QList<QJsonObject> &objs, int cap)
{
    QVariantList items;
    if (cap <= 0)
        return items;
    QStringList seen;
    for (const QJsonObject &o : objs) {
        if (items.size() >= cap)
            break;
        const QString imageId = o.value(QLatin1String("cover")).toObject()
                                    .value(QLatin1String("image_id")).toString();
        if (imageId.isEmpty())
            continue;
        const QString name = o.value(QLatin1String("name")).toString();
        if (name.isEmpty() || seen.contains(name) || isFreeLiveService(name))
            continue;
        seen.append(name);

        const qint64 rel = qint64(o.value(QLatin1String("first_release_date")).toDouble());
        QString backdrop = firstImageUrl(
            o.value(QLatin1String("artworks")).toArray(),
            QStringLiteral("https://images.igdb.com/igdb/image/upload/t_1080p/%1.jpg"));
        if (backdrop.isEmpty()) {
            backdrop = firstImageUrl(
                o.value(QLatin1String("screenshots")).toArray(),
                QStringLiteral("https://images.igdb.com/igdb/image/upload/t_screenshot_huge/%1.jpg"));
        }

        QVariantMap m;
        m.insert(QStringLiteral("title"), name);
        m.insert(QStringLiteral("poster"),
                 QStringLiteral("https://images.igdb.com/igdb/image/upload/t_cover_big/%1.jpg").arg(imageId));
        m.insert(QStringLiteral("backdrop"), backdrop);
        m.insert(QStringLiteral("year"), rel > 0
                 ? QString::number(QDateTime::fromSecsSinceEpoch(rel).date().year()) : QString());
        m.insert(QStringLiteral("rating"), o.value(QLatin1String("rating")).toDouble() / 10.0);
        m.insert(QStringLiteral("overview"), o.value(QLatin1String("summary")).toString());
        m.insert(QStringLiteral("type"), QStringLiteral("game"));
        items.append(m);
    }
    return items;
}

QVariantList gameCardsFromJson(const QByteArray &jsonArray, int cap)
{
    return gameCards(objectsFromJson(jsonArray), cap);
}

QVariantList titleSearchRows(const QByteArray &jsonArray)
{
    QVariantList out;
    if (jsonArray.isEmpty())
        return out;
    const QJsonArray arr = QJsonDocument::fromJson(jsonArray).array();
    for (const QJsonValue &v : arr) {
        const QJsonObject o = v.toObject();
        const QString imageId = o.value(QLatin1String("cover")).toObject()
                                    .value(QLatin1String("image_id")).toString();
        if (imageId.isEmpty()) continue;
        const QString name = o.value(QLatin1String("name")).toString();
        if (name.isEmpty()) continue;
        const qint64 rel = qint64(o.value(QLatin1String("first_release_date")).toDouble());
        QVariantMap m;
        m.insert(QStringLiteral("title"), name);
        m.insert(QStringLiteral("poster"),
                 QStringLiteral("https://images.igdb.com/igdb/image/upload/t_cover_big/%1.jpg").arg(imageId));
        m.insert(QStringLiteral("year"), rel > 0
                 ? QString::number(QDateTime::fromSecsSinceEpoch(rel).date().year()) : QString());
        m.insert(QStringLiteral("rating"), o.value(QLatin1String("total_rating")).toDouble() / 10.0);
        m.insert(QStringLiteral("overview"), o.value(QLatin1String("summary")).toString());
        m.insert(QStringLiteral("type"), QStringLiteral("game"));
        QStringList stills;
        const QJsonArray shots = o.value(QLatin1String("screenshots")).toArray();
        for (const QJsonValue &sv : shots) {
            const QString sid = sv.toObject().value(QLatin1String("image_id")).toString();
            if (!sid.isEmpty())
                stills << QStringLiteral("https://images.igdb.com/igdb/image/upload/t_screenshot_huge/%1.jpg").arg(sid);
            if (stills.size() >= 10) break;
        }
        m.insert(QStringLiteral("stills"), stills);
        out.append(m);
    }
    return out;
}

int pickHypeTypeId(const QByteArray &popularityTypesJson, int fallback)
{
    int sellers = 0, want = 0, playing = 0;
    if (popularityTypesJson.isEmpty())
        return fallback;
    const QJsonArray arr = QJsonDocument::fromJson(popularityTypesJson).array();
    for (const QJsonValue &v : arr) {
        const QJsonObject o = v.toObject();
        const QString name = o.value(QLatin1String("name")).toString().toLower();
        const int id = o.value(QLatin1String("id")).toInt();
        if (name.contains(QLatin1String("top seller"))) sellers = id;
        else if (name.contains(QLatin1String("want to play"))) want = id;
        else if (name.contains(QLatin1String("playing"))) playing = id;
    }
    if (sellers) return sellers;
    if (want) return want;
    if (playing) return playing;
    return fallback;
}

QList<qint64> orderedGameIds(const QByteArray &primitivesJson)
{
    QList<qint64> ids;
    if (primitivesJson.isEmpty())
        return ids;
    const QJsonArray arr = QJsonDocument::fromJson(primitivesJson).array();
    for (const QJsonValue &v : arr) {
        const qint64 gid = qint64(v.toObject().value(QLatin1String("game_id")).toDouble());
        if (gid > 0 && !ids.contains(gid))
            ids.append(gid);
    }
    return ids;
}

QList<QJsonObject> sortObjectsByIdRank(const QList<QJsonObject> &objs,
                                       const QList<qint64> &rankedIds)
{
    QList<QJsonObject> sorted = objs;
    QHash<qint64, int> rank;
    for (int i = 0; i < rankedIds.size(); ++i)
        rank.insert(rankedIds[i], i);
    std::sort(sorted.begin(), sorted.end(), [&rank](const QJsonObject &a, const QJsonObject &b) {
        return rank.value(qint64(a.value(QLatin1String("id")).toDouble()), 99999)
             < rank.value(qint64(b.value(QLatin1String("id")).toDouble()), 99999);
    });
    return sorted;
}

} // namespace IgdbParse
