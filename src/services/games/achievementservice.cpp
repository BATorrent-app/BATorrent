// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "services/games/achievementservice.h"
#include "services/games/achievementfiles.h"
#include "services/games/achievements.h"
#include "services/platform/contentlanguage.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPair>
#include <QSaveFile>
#include <QSettings>
#include <QUrl>
#include <QUrlQuery>
#include <memory>

namespace {

constexpr qint64 kFreshSecs = 7 * 24 * 3600;
constexpr qint64 kMaxBody = 8 * 1024 * 1024;

QUrl steamUrl(const QString &path, const QList<QPair<QString, QString>> &query)
{
    QUrl url(QStringLiteral("https://api.steampowered.com/ISteamUserStats/") + path);
    QUrlQuery q;
    for (const auto &kv : query) q.addQueryItem(kv.first, kv.second);
    url.setQuery(q);
    return url;
}

} // namespace

AchievementService::AchievementService(const QString &cacheDir, QObject *parent)
    : QObject(parent), m_cacheDir(cacheDir)
{
    m_nam.setTransferTimeout(15000);
}

QString AchievementService::steamApiKey()
{
    QString key = QSettings("BATorrent", "BATorrent").value("steamApiKey").toString();
#ifdef BAT_STEAM_KEY
    if (key.isEmpty()) key = QStringLiteral(BAT_STEAM_KEY);
#endif
    return key;
}

QString AchievementService::cachePath(int appId) const
{
    return QDir(m_cacheDir).filePath(QStringLiteral("%1_%2.json").arg(appId).arg(ContentLanguage::steam()));
}

void AchievementService::request(const QString &key, int appId, const QString &gameDir)
{
    if (appId <= 0) return;
    const bool inFlight = m_waiting.contains(appId);
    m_waiting[appId].append({ key, gameDir });
    if (inFlight) return;

    QFile f(cachePath(appId));
    if (f.size() < kMaxBody && f.open(QIODevice::ReadOnly)) {
        const QJsonObject o = QJsonDocument::fromJson(f.readAll()).object();
        const qint64 age = QDateTime::currentSecsSinceEpoch() - qint64(o.value(QLatin1String("at")).toDouble());
        if (age >= 0 && age < kFreshSecs) {
            finish(appId, o.value(QLatin1String("schema")).toString().toUtf8(),
                   o.value(QLatin1String("percent")).toString().toUtf8());
            return;
        }
    }
    fetch(appId);
}

void AchievementService::fetch(int appId)
{
    struct Pair { QByteArray schema, percent; int left = 2; bool schemaOk = false; };
    auto pair = std::make_shared<Pair>();
    const auto done = [this, appId, pair]() {
        if (--pair->left > 0) return;
        if (pair->schemaOk) {
            QDir().mkpath(m_cacheDir);
            QSaveFile out(cachePath(appId));
            if (out.open(QIODevice::WriteOnly)) {
                out.write(QJsonDocument(QJsonObject{
                    { QStringLiteral("at"), double(QDateTime::currentSecsSinceEpoch()) },
                    { QStringLiteral("schema"), QString::fromUtf8(pair->schema) },
                    { QStringLiteral("percent"), QString::fromUtf8(pair->percent) },
                }).toJson(QJsonDocument::Compact));
                out.commit();
            }
            finish(appId, pair->schema, pair->percent);
            return;
        }
        // Offline or refused: an old copy beats a list of raw ids.
        QFile f(cachePath(appId));
        if (f.size() < kMaxBody && f.open(QIODevice::ReadOnly)) {
            const QJsonObject o = QJsonDocument::fromJson(f.readAll()).object();
            finish(appId, o.value(QLatin1String("schema")).toString().toUtf8(),
                   o.value(QLatin1String("percent")).toString().toUtf8());
            return;
        }
        finish(appId, {}, pair->percent);
    };
    const auto read = [](QNetworkReply *r, QByteArray *into) {
        r->deleteLater();
        if (r->error() != QNetworkReply::NoError) return false;
        *into = r->read(kMaxBody);
        return !into->isEmpty();
    };

    const QString key = steamApiKey();
    if (key.isEmpty()) {
        pair->left = 1;
    } else {
        QNetworkReply *s = m_nam.get(QNetworkRequest(steamUrl(QStringLiteral("GetSchemaForGame/v2/"), {
            { QStringLiteral("key"), key }, { QStringLiteral("appid"), QString::number(appId) },
            { QStringLiteral("l"), ContentLanguage::steam() } })));
        connect(s, &QNetworkReply::finished, this, [s, pair, done, read]() {
            pair->schemaOk = read(s, &pair->schema) && !Achievements::parseSchema(pair->schema).isEmpty();
            done();
        });
    }
    QNetworkReply *p = m_nam.get(QNetworkRequest(steamUrl(QStringLiteral("GetGlobalAchievementPercentagesForApp/v2/"),
        { { QStringLiteral("gameid"), QString::number(appId) } })));
    connect(p, &QNetworkReply::finished, this, [p, pair, done, read]() {
        read(p, &pair->percent);
        done();
    });
}

void AchievementService::finish(int appId, const QByteArray &schema, const QByteArray &percent)
{
    const QList<Waiting> waiting = m_waiting.take(appId);
    const QList<Achievement> known = Achievements::parseSchema(schema);
    const QHash<QString, double> pct = Achievements::parsePercentages(percent);
    for (const Waiting &w : waiting) {
        const QHash<QString, qint64> unlocks = AchievementFiles::readUnlocks(appId, w.gameDir);
        QList<Achievement> list = known;
        if (list.isEmpty()) {
            for (auto it = unlocks.cbegin(); it != unlocks.cend(); ++it) {
                Achievement a;
                a.apiName = it.key();
                a.name = it.key();
                list.append(a);
            }
        }
        list = Achievements::merge(list, pct, unlocks);
        emit ready(w.key, appId, Achievements::toVariant(list), Achievements::unlockedCount(list),
                   int(known.size()));
    }
}
