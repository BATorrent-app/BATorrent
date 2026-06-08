// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "geoip.h"
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>

GeoIpResolver::GeoIpResolver(QObject *parent)
    : QObject(parent)
    , m_nam(new QNetworkAccessManager(this))
{
    m_rateLimiter.setSingleShot(true);
    m_rateLimiter.setInterval(200); // max 1 request per 200ms (API limit ~45/min)
    connect(&m_rateLimiter, &QTimer::timeout, this, &GeoIpResolver::processQueue);
}

void GeoIpResolver::resolve(const QString &ip)
{
    // Already cached
    if (m_cache.contains(ip)) {
        emit resolved(ip, m_cache.value(ip));
        return;
    }

    // Already tried and failed — don't re-queue (a churning swarm would otherwise
    // re-resolve every unresolvable IP on each peer-list rebuild, forever).
    if (m_failed.contains(ip))
        return;

    // Already queued
    if (m_queued.contains(ip))
        return;

    m_queue.enqueue(ip);
    m_queued.insert(ip);

    // Kick off processing if not already waiting
    if (!m_rateLimiter.isActive() && !m_requestInFlight)
        processQueue();
}

QString GeoIpResolver::cachedCountry(const QString &ip) const
{
    return m_cache.value(ip);
}

void GeoIpResolver::processQueue()
{
    if (m_queue.isEmpty() || m_requestInFlight)
        return;

    QString ip = m_queue.dequeue();
    m_queued.remove(ip);

    // Skip if resolved while queued
    if (m_cache.contains(ip)) {
        emit resolved(ip, m_cache.value(ip));
        if (!m_queue.isEmpty())
            m_rateLimiter.start();
        return;
    }

    m_requestInFlight = true;

    // ipinfo.io serves HTTPS on the free tier (~50k req/month per IP);
    // ip-api.com requires the paid plan for HTTPS. We switched providers
    // so peer IPs aren't leaked in cleartext to every router on the path
    // between BATorrent and the lookup service.
    QUrl url(QString("https://ipinfo.io/%1/country").arg(ip));
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::UserAgentHeader, "BATorrent");
    req.setRawHeader("Accept", "text/plain");

    QNetworkReply *reply = m_nam->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, ip]() {
        reply->deleteLater();
        m_requestInFlight = false;

        QString countryCode;
        if (reply->error() == QNetworkReply::NoError) {
            // /country returns just the 2-letter ISO code on its own line
            // (e.g. "US\n") so a trim is all that's needed. We fall back
            // to JSON parsing if the response looks structured, in case
            // the API changes shape under us.
            QByteArray body = reply->readAll().trimmed();
            if (body.startsWith('{')) {
                QJsonDocument doc = QJsonDocument::fromJson(body);
                countryCode = doc.object().value("country").toString();
            } else {
                countryCode = QString::fromLatin1(body);
            }
        }

        if (!countryCode.isEmpty() && countryCode.size() == 2) {
            m_cache.insert(ip, countryCode);
            emit resolved(ip, countryCode);
        } else {
            // Unresolved (error, rate-limit, or a private/CGNAT IP ipinfo returns
            // blank for) — negative-cache so it isn't retried every rebuild.
            m_failed.insert(ip);
        }

        // Schedule next request
        if (!m_queue.isEmpty())
            m_rateLimiter.start();
    });
}
