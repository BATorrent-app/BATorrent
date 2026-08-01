// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details
//
// QmlSessionBridge — make-room list + active/seeding/resume projections.

#include "bridges/session/qmlsessionbridge.h"
#include "torrent/sessionmanager.h"
#include "services/metadata/metadataresolver.h"
#include "services/platform/utils.h"

#include <QVariantList>
#include <QVariantMap>
#include <QSettings>
#include <QDateTime>

QVariantList QmlSessionBridge::makeRoomList() const
{
    QVariantList out;
    const int n = m_session->torrentCount();
    out.reserve(n);
    for (int i = 0; i < n; ++i) {
        const TorrentInfo info = m_session->torrentAt(i);
        QVariantMap m;
        m["infoHash"]   = m_session->torrentHashAt(i);
        m["name"]       = info.name;
        m["sizeBytes"]  = static_cast<qint64>(info.totalSize);
        m["size"]       = formatSize(info.totalSize);
        m["addedTime"]  = info.addedTime;
        m["category"]   = info.category;
        m["paused"]     = info.paused;
        m["completed"]  = info.completed || info.finished;
        m["seeding"]    = info.completed && !info.paused && info.uploadRate > 0;
        out << m;
    }
    return out;
}

void QmlSessionBridge::removeTorrentByHash(const QString &hash, bool deleteFiles, bool permanent)
{
    if (hash.isEmpty()) return;
    const int n = m_session->torrentCount();
    for (int i = 0; i < n; ++i) {
        if (m_session->torrentHashAt(i) != hash) continue;
        m_session->removeTorrent(i, deleteFiles, permanent);
        if (m_selectedIndex == i) { m_selectedIndex = -1; emit selectionChanged(); emit selectionListsChanged(); }
        emit toast(permanent ? tr_("remove_deleted") : tr_("remove_trashed"), QString());
        return;
    }
}

// Currently-downloading torrents for the nav-rail mini card: cover + name + % +
// down speed. Stable order (torrentAt index) so the QML carousel stays consistent.
QVariantList QmlSessionBridge::activeDownloads() const
{
    QVariantList out;
    const int n = m_session->torrentCount();
    for (int i = 0; i < n; ++i) {   // navigate via hover arrows, so no cap
        const TorrentInfo info = m_session->torrentAt(i);
        // incomplete = a download in flight; paused/queued ones still count so the
        // card never empties when disk-low auto-pause or the user paused everything.
        if (info.completed || info.finished) continue;
        const QString hash = m_session->torrentHashAt(i);
        QString poster;
        if (m_resolver && m_resolver->hasCached(hash)) {
            const auto meta = m_resolver->cached(hash);
            if (!meta.posterPath.isEmpty())
                poster = QUrl::fromLocalFile(meta.posterPath).toString();
        }
        QVariantMap m;
        m["infoHash"]  = hash;
        m["title"]     = info.name;
        m["progress"]  = double(info.progress);
        m["downSpeed"] = formatSize(info.downloadRate) + QStringLiteral("/s");
        m["paused"]    = info.paused;
        m["poster"]    = poster;
        // active (moving) downloads lead; paused/stalled fall to the back
        if (!info.paused && info.downloadRate > 0) out.prepend(m); else out << m;
    }
    return out;
}

QVariantList QmlSessionBridge::seedingTransfers() const
{
    QVariantList out;
    const int n = m_session->torrentCount();
    for (int i = 0; i < n; ++i) {
        const TorrentInfo info = m_session->torrentAt(i);
        if (!info.completed) continue;   // any finished torrent (paused or seeding) — the card's fallback
        const QString hash = m_session->torrentHashAt(i);
        QString poster;
        if (m_resolver && m_resolver->hasCached(hash)) {
            const auto meta = m_resolver->cached(hash);
            if (!meta.posterPath.isEmpty())
                poster = QUrl::fromLocalFile(meta.posterPath).toString();
        }
        QVariantMap m;
        m["infoHash"] = hash;
        m["title"]    = info.name;
        m["progress"] = 1.0;
        m["seeding"]  = true;
        m["paused"]   = info.paused;
        m["upSpeed"]  = formatSize(info.uploadRate) + QStringLiteral("/s");
        m["ratio"]    = QString::number(double(info.ratio), 'f', 2);
        m["poster"]   = poster;
        // actively-uploading torrents first so the card leads with live sharing
        if (info.uploadRate > 0) out.prepend(m); else out << m;
    }
    return out;
}

// Continue watching / playing for the nav-rail slot when you're on the Downloads
// tab (the downloads are already on screen there, so show what's resumable instead).
QVariantList QmlSessionBridge::resumeItems() const
{
    static QVariantList cached; static qint64 last = 0;
    const qint64 now = QDateTime::currentSecsSinceEpoch();
    if (last != 0 && now - last < 8) return cached;
    last = now;

    auto fmtPlay = [](qint64 s) {
        const qint64 h = s / 3600, m = (s % 3600) / 60;
        return h > 0 ? QStringLiteral("%1h %2m").arg(h).arg(m) : QStringLiteral("%1m").arg(m);
    };
    QVector<QPair<qint64, QVariantMap>> rows;   // (recency ms, item)
    for (const QVariant &v : movieLibrary()) {
        const QVariantMap mv = v.toMap();
        if (mv.value(QStringLiteral("resumeMs")).toLongLong() <= 0) continue;
        const double pct = mv.value(QStringLiteral("watchedPct")).toDouble();
        QVariantMap o;
        o["kind"] = QStringLiteral("movie");
        o["infoHash"]  = mv.value(QStringLiteral("infoHash"));
        o["fileIndex"] = mv.value(QStringLiteral("fileIndex"));
        o["title"]     = mv.value(QStringLiteral("title"));
        o["poster"]    = mv.value(QStringLiteral("poster"));
        o["progress"]  = pct;
        o["metric"]    = QString::number(int(pct * 100)) + QStringLiteral("%");
        rows.append({ mv.value(QStringLiteral("resumeAt")).toLongLong(), o });
    }
    for (const QVariant &v : gameLibrary()) {
        const QVariantMap g = v.toMap();
        if (g.value(QStringLiteral("lastPlayed")).toLongLong() <= 0) continue;
        QVariantMap o;
        o["kind"] = QStringLiteral("game");
        o["infoHash"] = g.value(QStringLiteral("infoHash"));
        o["title"]    = g.value(QStringLiteral("title"));
        o["poster"]   = g.value(QStringLiteral("poster"));
        o["progress"] = 0.0;
        o["metric"]   = fmtPlay(g.value(QStringLiteral("playSeconds")).toLongLong());
        rows.append({ g.value(QStringLiteral("lastPlayed")).toLongLong(), o });
    }
    std::sort(rows.begin(), rows.end(), [](const auto &a, const auto &b){ return a.first > b.first; });
    QVariantList out;
    for (int i = 0; i < rows.size() && i < 8; ++i) out << rows[i].second;
    cached = out;
    return out;
}

