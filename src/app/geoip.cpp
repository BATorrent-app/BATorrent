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

    // Already queued
    if (m_queue.contains(ip))
        return;

    m_queue.enqueue(ip);

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

    // Skip if resolved while queued
    if (m_cache.contains(ip)) {
        emit resolved(ip, m_cache.value(ip));
        if (!m_queue.isEmpty())
            m_rateLimiter.start();
        return;
    }

    m_requestInFlight = true;

    QUrl url(QString("http://ip-api.com/json/%1?fields=countryCode").arg(ip));
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::UserAgentHeader, "BATorrent");

    QNetworkReply *reply = m_nam->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, ip]() {
        reply->deleteLater();
        m_requestInFlight = false;

        QString countryCode;
        if (reply->error() == QNetworkReply::NoError) {
            QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
            countryCode = doc.object().value("countryCode").toString();
        }

        if (!countryCode.isEmpty()) {
            m_cache.insert(ip, countryCode);
            emit resolved(ip, countryCode);
        }

        // Schedule next request
        if (!m_queue.isEmpty())
            m_rateLimiter.start();
    });
}
