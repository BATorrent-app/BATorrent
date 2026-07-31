// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "app/mediarefresh.h"

#include "services/security/secretstore.h"
#include "torrent/iengine.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSettings>
#include <QUrl>

namespace MediaRefresh {

void install(QObject *context, IEngine *eng)
{
    auto *nam = new QNetworkAccessManager(context);
    QObject::connect(eng, &IEngine::torrentFinished, context, [nam](const QString &, const QString &) {
        QSettings st;
        if (st.value("plexEnabled", false).toBool()) {
            const QString url = st.value("plexUrl").toString();
            const QString token = SecretStore::instance().get("plexToken");
            if (!url.isEmpty() && !token.isEmpty()) {
                QNetworkRequest req(QUrl(url + "/library/sections/all/refresh?X-Plex-Token=" + token));
                req.setHeader(QNetworkRequest::UserAgentHeader, "BATorrent");
                auto *r = nam->get(req);
                QObject::connect(r, &QNetworkReply::finished, r, &QNetworkReply::deleteLater);
            }
        }
        if (st.value("jellyfinEnabled", false).toBool()) {
            const QString url = st.value("jellyfinUrl").toString();
            const QString key = SecretStore::instance().get("jellyfinApiKey");
            if (!url.isEmpty() && !key.isEmpty()) {
                QNetworkRequest req(QUrl(url + "/Library/Refresh?api_key=" + key));
                req.setHeader(QNetworkRequest::UserAgentHeader, "BATorrent");
                auto *r = nam->post(req, QByteArray());
                QObject::connect(r, &QNetworkReply::finished, r, &QNetworkReply::deleteLater);
            }
        }
    });
}

} // namespace MediaRefresh
