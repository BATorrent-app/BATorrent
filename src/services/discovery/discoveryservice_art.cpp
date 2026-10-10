// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details
//
// DiscoveryService: facts about a title's artwork itself (is its logo dark?).

#include "services/discovery/discoveryservice.h"
#include "services/discovery/logotone.h"

#include <QImage>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QUrl>

void DiscoveryService::fetchLogoTone(const QString &url)
{
    const QUrl u(url);
    if (url.isEmpty() || !(u.scheme() == QLatin1String("https") || u.scheme() == QLatin1String("http")))
        return;
    const auto hit = m_logoTone.constFind(url);
    if (hit != m_logoTone.constEnd()) {
        const bool dark = *hit;
        // Queued, so a logo that asks while it is being created can still hear it.
        QTimer::singleShot(0, this, [this, url, dark]() { emit logoToneReady(url, dark); });
        return;
    }
    if (m_logoTonePending.contains(url)) return;
    m_logoTonePending.insert(url);
    QNetworkRequest req(u);
    req.setTransferTimeout(12000);
    QNetworkReply *reply = m_nam->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, url]() {
        reply->deleteLater();
        m_logoTonePending.remove(url);
        // Unreadable reads as not dark: the logo then just shows as it is.
        bool dark = false;
        if (reply->error() == QNetworkReply::NoError)
            dark = LogoTone::isDark(QImage::fromData(reply->read(8 * 1024 * 1024)));
        m_logoTone.insert(url, dark);
        emit logoToneReady(url, dark);
    });
}
