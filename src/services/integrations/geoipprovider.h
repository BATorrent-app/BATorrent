// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef GEOIPPROVIDER_H
#define GEOIPPROVIDER_H

#include "services/integrations/geoipdb.h"

#include <QObject>
#include <QString>
#include <QNetworkAccessManager>
#include <memory>

// Keeps a GeoIpDb populated from the db-ip.com Lite (CC-BY) IP→country dataset:
// loads a cached copy on startup, or downloads the current month's gzip once and
// caches it under the app data dir. No infra of ours — db-ip serves the file.
// The DB is handed out as a shared_ptr so the peer-ranking classifier can hold
// it alive independently of this provider's lifetime.
class GeoIpProvider : public QObject
{
    Q_OBJECT
public:
    explicit GeoIpProvider(QObject *parent = nullptr);

    // Load from cache (fresh enough) or kick off a download. Idempotent-ish:
    // safe to call once at startup. cacheDir is created if missing.
    void start(const QString &cacheDir);

    std::shared_ptr<GeoIpDb> db() const { return m_db; }
    bool ready() const { return m_db && m_db->ready(); }

    // gunzip a .gz blob (db-ip ships gzip). Empty on any zlib error or if the
    // result would exceed kMaxUncompressed. Static + pure → unit-tested.
    static QByteArray gunzip(const QByteArray &gz);

signals:
    void loaded();   // emitted once the DB has parsed at least one range

private:
    void downloadMonth(int monthsBack);
    void loadFromGzFile(const QString &path);
    bool loadFromGzBytes(const QByteArray &gz);

    QString m_cacheDir;
    QNetworkAccessManager m_nam;
    std::shared_ptr<GeoIpDb> m_db;
};

#endif // GEOIPPROVIDER_H
