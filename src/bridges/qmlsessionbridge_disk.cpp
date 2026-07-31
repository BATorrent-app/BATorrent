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
#include "services/platform/translator.h"  // tr_

#include <QStorageInfo>
#include <QDir>
#include <QUrl>
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

// True when a mounted volume is somewhere a user would actually save to —
// filters out the OS's plumbing mounts (tmpfs, snapshots, synthetic volumes)
// WITHOUT rejecting the real data volume. macOS keeps user data under
// /System/Volumes/Data, so a blanket "/System/" block hid every disk and the
// gauge vanished — that path must stay.
static bool isUserVolume(const QStorageInfo &si)
{
    if (!si.isValid() || !si.isReady() || si.bytesTotal() <= 0)
        return false;
    const QByteArray fs = si.fileSystemType().toLower();
    if (fs.contains("tmpfs") || fs.contains("squash") || fs.contains("overlay")
        || fs.contains("devfs") || fs.contains("autofs"))
        return false;
    const QString root = si.rootPath();
    // macOS: "/" IS the disk (reported read-only, but it's the real store the
    // user saves to). The firmlinked /System/Volumes/* mounts — Data and the
    // synthetic ones — all duplicate it, so drop them and let "/" represent it.
    if (root.startsWith(QLatin1String("/System/Volumes/")))
        return false;
    // read-only mounts are noise (optical media, snaps) EXCEPT the macOS
    // system root, which is sealed-RO yet is the actual disk.
    if (si.isReadOnly() && root != QLatin1String("/"))
        return false;
    // Linux plumbing mounts.
    static const char *linuxJunk[] = {"/proc", "/sys", "/dev", "/run",
                                      "/boot", "/snap", "/var/lib"};
    for (const char *p : linuxJunk)
        if (root.startsWith(QLatin1String(p))) return false;
    return true;
}

// Every disk a download could land on: all user-facing mounted volumes,
// plus whatever the default/category save paths sit on (the gauge in the
// nav rotates through these; the rail lists them).
QVariantList QmlSessionBridge::diskVolumes() const
{
    static QVariantList cached;
    static qint64 lastCheck = 0;
    const qint64 now = QDateTime::currentSecsSinceEpoch();
    if (lastCheck != 0 && now - lastCheck < 10) return cached;
    lastCheck = now;

    QList<QStorageInfo> vols = QStorageInfo::mountedVolumes();
    QStringList paths;
    paths << defaultSavePath();
    const auto catPaths = m_session->allCategorySavePaths();
    for (auto it = catPaths.constBegin(); it != catPaths.constEnd(); ++it)
        if (!it.value().isEmpty()) paths << it.value();
    for (QString p : paths)
        vols << QStorageInfo(p.isEmpty() ? QDir::homePath() : p);

    const QString defaultRoot = QStorageInfo(defaultSavePath().isEmpty()
                                    ? QDir::homePath() : defaultSavePath()).rootPath();
    QVariantList out;
    QSet<QString> seenRoots;
    for (const QStorageInfo &si : vols) {
        if (!isUserVolume(si)) continue;
        const QString root = si.rootPath();
        if (seenRoots.contains(root)) continue;
        seenRoots.insert(root);

        QVariantMap m;
        QString name = si.displayName();
        if (name.isEmpty()) name = root;
        m["name"] = name;
        m["free"] = formatSize(si.bytesAvailable());
        m["freeBytes"] = si.bytesAvailable();
        m["usedFraction"] = double(si.bytesTotal() - si.bytesAvailable()) / double(si.bytesTotal());
        // default-save volume leads; the nav gauge starts its rotation there
        if (root == defaultRoot) out.prepend(m); else out << m;
    }
    cached = out;
    return out;
}

// Free bytes on whatever volume holds `path` (walks up to the nearest
// existing ancestor, so a not-yet-created subfolder still answers). -1 unknown.
double QmlSessionBridge::freeBytesAt(const QString &path) const
{
    QString p = path.trimmed();
    if (p.startsWith(QLatin1String("file://"))) p = QUrl(p).toLocalFile();
    if (p.isEmpty()) p = defaultSavePath();
    if (p.isEmpty()) p = QDir::homePath();
    QDir dir(p);
    while (!dir.exists() && !dir.isRoot())
        if (!dir.cdUp()) break;
    QStorageInfo si(dir.absolutePath());
    if (!si.isValid() || !si.isReady()) return -1;
    return double(si.bytesAvailable());
}

// MRU list of distinct save locations, offered as one-click chips in the add
// dialogs ("favorite folders": Videos on C, Games on D...).
void QmlSessionBridge::rememberSavePath(const QString &path)
{
    QString p = QDir::cleanPath(path.trimmed());
    if (p.startsWith(QLatin1String("file://"))) p = QDir::cleanPath(QUrl(p).toLocalFile());
    if (p.isEmpty() || !QDir(p).exists()) return;
    QSettings s;
    QStringList favs = s.value("favSavePaths").toStringList();
    favs.removeAll(p);
    favs.prepend(p);
    while (favs.size() > 5) favs.removeLast();
    s.setValue("favSavePaths", favs);
}

QVariantList QmlSessionBridge::favoriteSavePaths() const
{
    QVariantList out;
    const QStringList favs = QSettings().value("favSavePaths").toStringList();
    for (const QString &p : favs) {
        QDir d(p);
        if (!d.exists()) continue;
        QVariantMap m;
        m["path"] = p;
        m["label"] = d.isRoot() ? p : d.dirName();
        const double freeB = freeBytesAt(p);
        m["free"] = freeB >= 0 ? formatSize(qint64(freeB)) : QString();
        m["freeBytes"] = freeB;
        out << m;
    }
    return out;
}

// Every torrent, for the Make Room panel — the QML view sorts by size or age
// and sums a running "would reclaim" total as the user picks rows to delete.
QString QmlSessionBridge::defaultSavePath() const
{
    QSettings s;
    QString p = s.value(QStringLiteral("lastSavePath")).toString();
    if (!p.isEmpty() && QDir(p).exists()) return p;
    return QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
}

