// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "services/games/steamstore.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSet>
#include <QStringList>

namespace SteamStore {

namespace {

QStringList words(const QString &s)
{
    static const QRegularExpression nonWord(QStringLiteral("[^\\p{L}\\p{N}]+"));
    return s.toLower().split(nonWord, Qt::SkipEmptyParts);
}

// Trailing words that name an edition, not a game. "game of the year" goes as
// a phrase so a title that merely ends in "the" keeps it.
QStringList dropEdition(QStringList w)
{
    static const QSet<QString> edition = {
        "complete", "edition", "goty", "definitive", "deluxe", "ultimate", "remastered", "remaster",
        "enhanced", "directors", "director", "cut", "gold", "premium", "standard", "anniversary", "hd",
    };
    for (;;) {
        if (w.size() > 4 && w.mid(w.size() - 4) == QStringList{ "game", "of", "the", "year" }) {
            w = w.mid(0, w.size() - 4);
            continue;
        }
        if (w.size() > 1 && edition.contains(w.last())) { w.removeLast(); continue; }
        return w;
    }
}

} // namespace

QString searchTerm(const QString &title)
{
    return dropEdition(words(title)).join(QLatin1Char(' '));
}

int bestAppId(const QByteArray &searchJson, const QString &title)
{
    const QStringList want = dropEdition(words(title));
    if (want.isEmpty()) return 0;
    const QJsonArray items = QJsonDocument::fromJson(searchJson).object()
                                 .value(QLatin1String("items")).toArray();
    for (const QJsonValue &v : items) {
        const QJsonObject o = v.toObject();
        if (o.value(QLatin1String("type")).toString() != QLatin1String("app")) continue;
        const int id = o.value(QLatin1String("id")).toInt();
        if (id > 0 && dropEdition(words(o.value(QLatin1String("name")).toString())) == want) return id;
    }
    return 0;
}

QString heroUrl(int appId)
{
    return appId > 0 ? QStringLiteral("image://steamart/hero/%1").arg(appId) : QString();
}

QString logoUrl(int appId)
{
    return appId > 0 ? QStringLiteral("image://steamart/logo/%1").arg(appId) : QString();
}

QString storeItemsUrl(int appId)
{
    // Plain escapes: MSVC can't take a raw string inside QStringLiteral.
    const QByteArray input = QStringLiteral(
        "{\"ids\":[{\"appid\":%1}],\"context\":{\"language\":\"english\",\"country_code\":\"US\"},"
        "\"data_request\":{\"include_assets\":true}}").arg(appId).toUtf8();
    return QStringLiteral("https://api.steampowered.com/IStoreBrowseService/GetItems/v1/?input_json=")
           + QString::fromLatin1(input.toPercentEncoding());
}

QString assetUrl(const QByteArray &storeItemsJson, const QString &kind)
{
    const QJsonArray items = QJsonDocument::fromJson(storeItemsJson).object()
        .value(QLatin1String("response")).toObject()
        .value(QLatin1String("store_items")).toArray();
    if (items.isEmpty()) return {};
    const QJsonObject assets = items.first().toObject().value(QLatin1String("assets")).toObject();
    // "steam/apps/<id>/${FILENAME}?t=<stamp>", the file named "<hash>/library_hero.jpg"
    const QString format = assets.value(QLatin1String("asset_url_format")).toString();
    const QString file = assets.value(kind == QLatin1String("logo") ? QLatin1String("library_logo")
                                                                    : QLatin1String("library_hero")).toString();
    if (format.isEmpty() || file.isEmpty()) return {};
    return QStringLiteral("https://shared.cloudflare.steamstatic.com/store_item_assets/")
           + QString(format).replace(QLatin1String("${FILENAME}"), file);
}

QString legacyArtUrl(int appId, const QString &kind)
{
    if (appId <= 0) return {};
    return QStringLiteral("https://cdn.cloudflare.steamstatic.com/steam/apps/%1/%2").arg(appId)
        .arg(kind == QLatin1String("logo") ? QStringLiteral("logo.png") : QStringLiteral("library_hero.jpg"));
}

} // namespace SteamStore
