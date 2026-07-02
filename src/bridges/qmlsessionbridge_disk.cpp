// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details
//
// QmlSessionBridge — disk & transfer-projection slice. Free-space queries and
// the per-volume breakdown, plus the QML list models for active downloads,
// seeding transfers, and resume-me items, and the default save path. Split out
// of qmlsessionbridge.cpp verbatim; no behaviour change.

#include "bridges/qmlsessionbridge.h"
#include "torrent/sessionmanager.h"   // full IEngine + TorrentInfo
#include "services/metadata/metadataresolver.h"
#include "services/platform/utils.h"  // formatSize

#include <QStorageInfo>
#include <QDir>
#include <QFileInfo>
#include <QSettings>
#include <QStandardPaths>
#include <QDateTime>

// Free space on the default save volume, polled at most every 5s (the status bar
// binds to statsChanged, which ticks every second). Single source of truth for
// "free disk": one cached read shared by the status bar, search disk-fit,
// add-guard and auto-pause — so every screen agrees instead of each polling at
// its own moment.
qint64 QmlSessionBridge::freeSaveBytes() const
{
    static qint64 cached = -1;
    static qint64 lastCheck = 0;
    const qint64 now = QDateTime::currentSecsSinceEpoch();
    if (cached < 0 || now - lastCheck >= 5) {
        lastCheck = now;
        const QString path = defaultSavePath();
        QStorageInfo si(path.isEmpty() ? QDir::homePath() : path);
        cached = si.isValid() ? si.bytesAvailable() : -1;
    }
    return cached;
}

QString QmlSessionBridge::freeDiskSpace() const
{
    const qint64 b = freeSaveBytes();
    return b >= 0 ? formatSize(b) : QString();
}

// Fraction of the save volume that's used (0..1) — drives the sidebar disk bar.
double QmlSessionBridge::diskUsedFraction() const
{
    static double cached = 0;
    static qint64 lastCheck = 0;
    const qint64 now = QDateTime::currentSecsSinceEpoch();
    if (lastCheck == 0 || now - lastCheck >= 10) {
        lastCheck = now;
        const QString path = defaultSavePath();
        QStorageInfo si(path.isEmpty() ? QDir::homePath() : path);
        const qint64 total = si.isValid() ? si.bytesTotal() : 0;
        cached = total > 0 ? double(total - si.bytesAvailable()) / double(total) : 0.0;
    }
    return cached;
}

// One entry per distinct volume torrents save to (default + per-category paths).
QVariantList QmlSessionBridge::diskVolumes() const
{
    static QVariantList cached;
    static qint64 lastCheck = 0;
    const qint64 now = QDateTime::currentSecsSinceEpoch();
    if (lastCheck != 0 && now - lastCheck < 10) return cached;
    lastCheck = now;

    QStringList paths;
    paths << defaultSavePath();
    const auto catPaths = m_session->allCategorySavePaths();
    for (auto it = catPaths.constBegin(); it != catPaths.constEnd(); ++it)
        if (!it.value().isEmpty()) paths << it.value();

    QVariantList out;
    QSet<QString> seenRoots;
    for (QString p : paths) {
        if (p.isEmpty()) p = QDir::homePath();
        QStorageInfo si(p);
        if (!si.isValid() || si.bytesTotal() <= 0) continue;
        const QString root = QString::fromUtf8(si.rootPath().toUtf8());
        if (seenRoots.contains(root)) continue;
        seenRoots.insert(root);

        QVariantMap m;
        QString name = si.displayName();
        if (name.isEmpty()) name = root;
        m["name"] = name;
        m["free"] = formatSize(si.bytesAvailable());
        m["freeBytes"] = si.bytesAvailable();
        m["usedFraction"] = double(si.bytesTotal() - si.bytesAvailable()) / double(si.bytesTotal());
        out << m;
    }
    cached = out;
    return out;
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
        if (info.completed || info.progress >= 1.0f) continue;
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

QString QmlSessionBridge::defaultSavePath() const
{
    QSettings s;
    QString p = s.value(QStringLiteral("lastSavePath")).toString();
    if (!p.isEmpty() && QDir(p).exists()) return p;
    return QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
}

