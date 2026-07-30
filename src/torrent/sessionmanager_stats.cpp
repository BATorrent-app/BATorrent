// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details
//
// SessionManager — session/global stats + Wrapped recap. Split out of
// sessionmanager.cpp verbatim; no behaviour change.

#include "torrent/sessionmanager.h"

#include <QSettings>
#include <QJsonObject>
#include <QVariantMap>
#include <QVariantList>
#include <QMap>
#include <QVector>
#include <algorithm>

qint64 SessionManager::globalDownloaded() const
{
    qint64 sessionDown = 0;
    for (const auto &h : m_torrents) {
        if (!h.is_valid()) continue;
        sessionDown += cachedStatus(h).total_payload_download;
    }
    return m_globalDownBase + sessionDown;
}

qint64 SessionManager::globalUploaded() const
{
    qint64 sessionUp = 0;
    for (const auto &h : m_torrents) {
        if (!h.is_valid()) continue;
        sessionUp += cachedStatus(h).total_payload_upload;
    }
    return m_globalUpBase + sessionUp;
}

float SessionManager::globalRatio() const
{
    qint64 down = m_globalDownBase, up = m_globalUpBase;
    for (const auto &h : m_torrents) {
        if (!h.is_valid()) continue;
        auto st = cachedStatus(h);
        down += st.total_payload_download;
        up += st.total_payload_upload;
    }
    return down > 0 ? static_cast<float>(up) / static_cast<float>(down) : 0.0f;
}

QVariantMap SessionManager::statsWrapped(int year) const
{
    QVariantMap out;
    out["year"] = year;
    if (!m_statsHistory) return out;
    const QJsonObject days = m_statsHistory->days();
    const QString prefix = QString::number(year) + QLatin1Char('-');
    qint64 down = 0, up = 0, added = 0, completed = 0, busiestDown = 0;
    int activeDays = 0;
    QString busiestDay;
    QMap<QString, int> cats;
    QVector<qint64> months(12, 0);
    for (auto it = days.begin(); it != days.end(); ++it) {
        if (!it.key().startsWith(prefix)) continue;
        const QJsonObject e = it.value().toObject();
        const qint64 d = qint64(e.value(QStringLiteral("down")).toDouble());
        const qint64 u = qint64(e.value(QStringLiteral("up")).toDouble());
        down += d; up += u;
        added += qint64(e.value(QStringLiteral("added")).toDouble());
        completed += qint64(e.value(QStringLiteral("completed")).toDouble());
        if (d > 0 || u > 0 || e.value(QStringLiteral("added")).toDouble() > 0) ++activeDays;
        if (d > busiestDown) { busiestDown = d; busiestDay = it.key(); }
        const int month = it.key().mid(5, 2).toInt();
        if (month >= 1 && month <= 12) months[month - 1] += d;
        const QJsonObject c = e.value(QStringLiteral("cats")).toObject();
        for (auto ci = c.begin(); ci != c.end(); ++ci) cats[ci.key()] += ci.value().toInt();
    }
    out["down"] = down; out["up"] = up;
    out["added"] = added; out["completed"] = completed;
    out["activeDays"] = activeDays;
    out["busiestDay"] = busiestDay; out["busiestDown"] = busiestDown;

    QList<QPair<QString, int>> sorted;
    for (auto ci = cats.begin(); ci != cats.end(); ++ci) sorted.append({ ci.key(), ci.value() });
    std::sort(sorted.begin(), sorted.end(), [](const QPair<QString,int> &a, const QPair<QString,int> &b) {
        return a.second > b.second;
    });
    QVariantList catList;
    for (const auto &p : sorted) { QVariantMap cm; cm["name"] = p.first; cm["count"] = p.second; catList << cm; }
    out["categories"] = catList;

    QVariantList monthList;
    for (int i = 0; i < 12; ++i) monthList << QVariant(qlonglong(months[i]));
    out["months"] = monthList;
    return out;
}

qint64 SessionManager::sessionDownloaded() const
{
    qint64 sessionDown = 0;
    for (const auto &h : m_torrents) {
        if (!h.is_valid()) continue;
        sessionDown += cachedStatus(h).total_payload_download;
    }
    return sessionDown;
}

qint64 SessionManager::sessionUploaded() const
{
    qint64 sessionUp = 0;
    for (const auto &h : m_torrents) {
        if (!h.is_valid()) continue;
        sessionUp += cachedStatus(h).total_payload_upload;
    }
    return sessionUp;
}

DetailedStats SessionManager::detailedStats() const
{
    DetailedStats ds;
    for (const auto &h : m_torrents) {
        if (!h.is_valid()) continue;
        auto st = cachedStatus(h);
        ds.peersCount += st.num_peers;
        ds.totalWasted += st.total_redundant_bytes + st.total_failed_bytes;
    }
    ds.hasIncomingConnections = m_session.is_listening();
    return ds;
}

void SessionManager::incrementTorrentCount()
{
    if (m_statsHistory) m_statsHistory->recordAdded();
    QSettings settings("BATorrent", "BATorrent");
    int count = settings.value("totalTorrentsAdded", 0).toInt();
    settings.setValue("totalTorrentsAdded", count + 1);
}

int SessionManager::totalTorrentsAdded() const
{
    QSettings settings("BATorrent", "BATorrent");
    return settings.value("totalTorrentsAdded", 0).toInt();
}

