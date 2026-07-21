// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef SERVICES_SECURITY_BLOCKLISTUPDATER_H
#define SERVICES_SECURITY_BLOCKLISTUPDATER_H

// Downloads a peer blocklist (P2P text format, optionally gzipped) and caches it
// on disk, so the engine can drop connections to known-bad IP ranges before the
// handshake — the "block known bad peers" toggle. Opt-in: it fetches an external
// list. The cached file feeds SessionManager::loadAutoBlocklist, which merges it
// with any manual list into a single ip_filter.

#include <QObject>
#include <QString>
#include <QUrl>

class QNetworkAccessManager;

class BlocklistUpdater : public QObject
{
    Q_OBJECT
public:
    explicit BlocklistUpdater(QObject *parent = nullptr);

    // A stable, purpose-built free endpoint (no auth/captcha): iblocklist "level1"
    // (anti-P2P / known-bad orgs), gzipped P2P format. Overridable via settings.
    static QUrl defaultUrl();
    static QString cachePath();
    // true when the cache is missing or older than `days` — caller decides to refresh.
    static bool cacheStale(int days = 7);

public slots:
    void update(const QUrl &url = defaultUrl());

signals:
    void ready(const QString &cachePath, int bytes);
    void failed(const QString &error);

private:
    QNetworkAccessManager *m_nam;
    bool m_busy = false;
};

#endif // SERVICES_SECURITY_BLOCKLISTUPDATER_H
