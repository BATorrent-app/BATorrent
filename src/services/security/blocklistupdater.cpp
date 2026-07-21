// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "services/security/blocklistupdater.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>
#include <QSaveFile>
#include <QDateTime>

#include <zlib.h>

namespace {
// Inflate a gzip stream (16 + MAX_WBITS selects the gzip wrapper). Bounded output
// growth; returns empty on any zlib error — the caller treats that as a failure.
QByteArray gunzip(const QByteArray &in)
{
    if (in.size() < 2) return {};
    z_stream s{};
    if (inflateInit2(&s, 16 + MAX_WBITS) != Z_OK) return {};
    s.next_in = reinterpret_cast<Bytef *>(const_cast<char *>(in.data()));
    s.avail_in = static_cast<uInt>(in.size());

    QByteArray out;
    char buf[64 * 1024];
    int ret = Z_OK;
    do {
        s.next_out = reinterpret_cast<Bytef *>(buf);
        s.avail_out = sizeof(buf);
        ret = inflate(&s, Z_NO_FLUSH);
        if (ret != Z_OK && ret != Z_STREAM_END) { inflateEnd(&s); return {}; }
        out.append(buf, sizeof(buf) - s.avail_out);
        // sanity cap: a peer blocklist is a few MB; refuse a decompression bomb
        if (out.size() > 128 * 1024 * 1024) { inflateEnd(&s); return {}; }
    } while (ret != Z_STREAM_END);
    inflateEnd(&s);
    return out;
}
}

BlocklistUpdater::BlocklistUpdater(QObject *parent)
    : QObject(parent), m_nam(new QNetworkAccessManager(this))
{
    m_nam->setRedirectPolicy(QNetworkRequest::NoLessSafeRedirectPolicy);
}

QUrl BlocklistUpdater::defaultUrl()
{
    return QUrl(QStringLiteral(
        "https://list.iblocklist.com/?list=bt_level1&fileformat=p2p&archiveformat=gz"));
}

QString BlocklistUpdater::cachePath()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                      + QStringLiteral("/blocklist");
    QDir().mkpath(dir);
    return dir + QStringLiteral("/badpeers.p2p");
}

bool BlocklistUpdater::cacheStale(int days)
{
    QFileInfo fi(cachePath());
    if (!fi.exists() || fi.size() == 0) return true;
    return fi.lastModified().daysTo(QDateTime::currentDateTime()) >= days;
}

void BlocklistUpdater::update(const QUrl &url)
{
    if (m_busy || !url.isValid()) return;
    m_busy = true;

    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("BATorrent"));
    QNetworkReply *reply = m_nam->get(req);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        m_busy = false;

        if (reply->error() != QNetworkReply::NoError) {
            emit failed(reply->errorString());
            return;
        }
        QByteArray body = reply->readAll();
        // gzip magic (1f 8b) — the default endpoint is gzipped, but a plain-text
        // URL is passed through untouched.
        if (body.size() >= 2 && static_cast<unsigned char>(body[0]) == 0x1f
                             && static_cast<unsigned char>(body[1]) == 0x8b) {
            const QByteArray inflated = gunzip(body);
            if (inflated.isEmpty()) { emit failed(QStringLiteral("could not decompress the blocklist")); return; }
            body = inflated;
        }
        if (body.isEmpty()) { emit failed(QStringLiteral("empty blocklist")); return; }

        QSaveFile out(cachePath());
        if (!out.open(QIODevice::WriteOnly) || out.write(body) != body.size() || !out.commit()) {
            emit failed(QStringLiteral("could not write the blocklist cache"));
            return;
        }
        emit ready(cachePath(), static_cast<int>(body.size()));
    });
}
