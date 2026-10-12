// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "app/steamartprovider.h"

#include "services/games/steamstore.h"

#include <QEventLoop>
#include <QHash>
#include <QImage>
#include <QMutex>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QQuickTextureFactory>
#include <QRunnable>
#include <QThreadPool>

namespace {

// Store answers by appid, kept for the session: hero and logo share one, and a
// library of tiles asks for the same games again as it scrolls.
QMutex g_storeMutex;
QHash<int, QByteArray> g_store;   // empty value: asked, and the API did not answer

// A GET on this worker thread, waited for in a local loop. Empty on failure.
QByteArray fetch(QNetworkAccessManager &nam, const QString &url)
{
    QNetworkRequest req{QUrl(url)};
    req.setTransferTimeout(15000);
    QNetworkReply *reply = nam.get(req);
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    const bool ok = reply->error() == QNetworkReply::NoError;
    const QByteArray body = ok ? reply->readAll() : QByteArray();
    delete reply;   // no event loop runs on after this on the worker thread
    return body;
}

class SteamArtResponse : public QQuickImageResponse, public QRunnable
{
public:
    SteamArtResponse(const QString &id, const QSize &requestedSize)
        : m_id(id), m_size(requestedSize) { setAutoDelete(false); }

    QQuickTextureFactory *textureFactory() const override
    {
        return QQuickTextureFactory::textureFactoryForImage(m_image);
    }
    QString errorString() const override { return m_error; }

    void run() override
    {
        resolve();
        emit finished();
    }

private:
    void resolve()
    {
        const QString kind = m_id.section(QLatin1Char('/'), 0, 0);
        const int appId = m_id.section(QLatin1Char('/'), 1, 1).toInt();
        if (appId <= 0 || (kind != QLatin1String("hero") && kind != QLatin1String("logo"))) {
            m_error = QStringLiteral("bad steamart id: ") + m_id;
            return;
        }
        QNetworkAccessManager nam;

        QByteArray store;
        bool known = false;
        {
            QMutexLocker lock(&g_storeMutex);
            if (g_store.contains(appId)) { store = g_store.value(appId); known = true; }
        }
        if (!known) {
            store = fetch(nam, SteamStore::storeItemsUrl(appId));
            QMutexLocker lock(&g_storeMutex);
            g_store.insert(appId, store);
        }

        // The store answered: its path is the truth, and no path means no such
        // art (no request to a URL that would 404). It did not: the old path,
        // which still serves games from before the move.
        const QString url = store.isEmpty() ? SteamStore::legacyArtUrl(appId, kind)
                                            : SteamStore::assetUrl(store, kind);
        if (url.isEmpty()) {
            m_error = QStringLiteral("no %1 art for app %2").arg(kind).arg(appId);
            return;
        }
        if (!m_image.loadFromData(fetch(nam, url))) {
            m_error = QStringLiteral("could not load ") + url;
            return;
        }
        if (m_size.isValid() && !m_size.isEmpty())
            m_image = m_image.scaled(m_size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }

    QString m_id;
    QSize m_size;
    QImage m_image;
    QString m_error;
};

} // namespace

QQuickImageResponse *SteamArtProvider::requestImageResponse(const QString &id, const QSize &requestedSize)
{
    auto *response = new SteamArtResponse(id, requestedSize);
    QThreadPool::globalInstance()->start(response);
    return response;
}
