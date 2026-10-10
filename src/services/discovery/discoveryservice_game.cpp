// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details
//
// DiscoveryService: per-game IGDB lookups (similar games, a game's own page)
// for a game the user already owns or picked.

#include "services/discovery/discoveryservice.h"
#include "services/discovery/igdbparse.h"

#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QUrl>

namespace {

QString igdbQuoted(QString name)
{
    name.replace(QLatin1Char('"'), QLatin1Char(' '));
    return name;
}

} // namespace

void DiscoveryService::fetchGameRecommendations(const QString &gameName)
{
    if (gameName.trimmed().isEmpty()) { emit gameRecommendationsReady(gameName, {}); return; }
    ensureIgdbToken([this, gameName]() {
        if (m_igdbToken.isEmpty()) { emit gameRecommendationsReady(gameName, {}); return; }
        QNetworkRequest req{QUrl(QStringLiteral("https://api.igdb.com/v4/games"))};
        setIgdbHeaders(req);
        const QByteArray body = QStringLiteral(
            "search \"%1\"; fields similar_games.name, similar_games.cover.image_id,"
            " similar_games.first_release_date, similar_games.rating, similar_games.summary; limit 1;")
            .arg(igdbQuoted(gameName)).toUtf8();
        QNetworkReply *reply = m_nam->post(req, body);
        connect(reply, &QNetworkReply::finished, this, [this, reply, gameName]() {
            reply->deleteLater();
            QVariantList items;
            if (reply->error() == QNetworkReply::NoError) {
                const QJsonArray arr = QJsonDocument::fromJson(reply->readAll()).array();
                if (!arr.isEmpty()) {
                    const QJsonArray sims = arr.first().toObject()
                                                .value(QLatin1String("similar_games")).toArray();
                    items = IgdbParse::gameCards(IgdbParse::objectsFromArray(sims), 16);
                }
            } else {
                qDebug() << "[discover] IGDB similar-games error:" << reply->errorString();
            }
            emit gameRecommendationsReady(gameName, items);
        });
    });
}

void DiscoveryService::fetchGameDetails(const QString &gameName)
{
    const QString key = gameName.trimmed();
    if (key.isEmpty()) { emit gameDetailsReady(gameName, {}); return; }
    const auto hit = m_gameDetails.constFind(key);
    if (hit != m_gameDetails.constEnd()) {
        const QVariantMap details = hit.value();
        // Queued, so a tile that asks while it is being created can still hear it.
        QTimer::singleShot(0, this, [this, gameName, details]() { emit gameDetailsReady(gameName, details); });
        return;
    }
    if (!m_gameDetailsQueue.contains(key)) m_gameDetailsQueue.append(key);
    runGameDetailsQueue();
}

void DiscoveryService::runGameDetailsQueue()
{
    if (m_gameDetailsBusy || m_gameDetailsQueue.isEmpty()) return;
    m_gameDetailsBusy = true;
    const QString key = m_gameDetailsQueue.takeFirst();
    const auto next = [this]() {
        QTimer::singleShot(260, this, [this]() { m_gameDetailsBusy = false; runGameDetailsQueue(); });
    };
    ensureIgdbToken([this, key, next]() {
        if (m_igdbToken.isEmpty()) { emit gameDetailsReady(key, {}); next(); return; }
        QNetworkRequest req{QUrl(QStringLiteral("https://api.igdb.com/v4/games"))};
        setIgdbHeaders(req);
        req.setTransferTimeout(12000);
        const QByteArray body = QStringLiteral(
            "search \"%1\"; fields name,cover.image_id,first_release_date,total_rating,summary,"
            "screenshots.image_id,artworks.image_id,genres.name,"
            "involved_companies.company.name,involved_companies.developer,"
            "external_games.uid,external_games.category,external_games.external_game_source;"
            " limit 1;").arg(igdbQuoted(key)).toUtf8();
        QNetworkReply *reply = m_nam->post(req, body);
        connect(reply, &QNetworkReply::finished, this, [this, reply, key, next]() {
            reply->deleteLater();
            if (reply->error() != QNetworkReply::NoError) {
                // Not cached: a dropped connection shouldn't blank the page for the session.
                qDebug() << "[discover] IGDB game details error:" << reply->errorString();
                emit gameDetailsReady(key, {});
            } else {
                const QVariantMap details = IgdbParse::gameDetails(reply->readAll());
                m_gameDetails.insert(key, details);
                emit gameDetailsReady(key, details);
            }
            next();
        });
    });
}
