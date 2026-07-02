// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details
//
// QmlSessionBridge — selection getters slice. The read-only QML properties that
// project the currently-selected torrent (name/size/hash/speeds/eta/ratio/state
// + the resolved metadata: poster, description, title, info line). Split out of
// qmlsessionbridge.cpp verbatim; no behaviour change.

#include "bridges/qmlsessionbridge.h"
#include "torrent/sessionmanager.h"   // full IEngine + TorrentInfo
#include "services/metadata/metadataresolver.h"
#include "services/platform/utils.h"  // formatSize / formatSpeed

#include <QDateTime>
#include <QFileInfo>

QString QmlSessionBridge::selectedName() const
{
    if (!hasSelection()) return {};
    return m_session->torrentAt(m_selectedIndex).name;
}

QString QmlSessionBridge::selectedSize() const
{
    if (!hasSelection()) return {};
    return formatSize(m_session->torrentAt(m_selectedIndex).totalSize);
}

QString QmlSessionBridge::selectedHash() const
{
    if (!hasSelection()) return {};
    QString h = m_session->torrentHashAt(m_selectedIndex);
    if (h.size() > 14) return h.left(8) + QStringLiteral("…") + h.right(4);
    return h;
}

QString QmlSessionBridge::selectedDownloaded() const
{
    if (!hasSelection()) return {};
    auto info = m_session->torrentAt(m_selectedIndex);
    return QString("%1 (%2%)").arg(formatSize(info.totalDone))
                              .arg(info.progress * 100.0, 0, 'f', 1);
}

QString QmlSessionBridge::selectedDownSpeed() const
{
    if (!hasSelection()) return QStringLiteral("—");
    return formatSpeed(m_session->torrentAt(m_selectedIndex).downloadRate);
}

QString QmlSessionBridge::selectedUpSpeed() const
{
    if (!hasSelection()) return QStringLiteral("—");
    return formatSpeed(m_session->torrentAt(m_selectedIndex).uploadRate);
}

QString QmlSessionBridge::selectedEta() const
{
    if (!hasSelection()) return QStringLiteral("—");
    auto info = m_session->torrentAt(m_selectedIndex);
    if (info.downloadRate <= 0 || info.totalSize <= info.totalDone) return QStringLiteral("—");
    qint64 secs = (info.totalSize - info.totalDone) / info.downloadRate;
    if (secs < 60) return QString("%1s").arg(secs);
    if (secs < 3600) return QString("%1m %2s").arg(secs / 60).arg(secs % 60);
    return QString("%1h %2m").arg(secs / 3600).arg((secs % 3600) / 60);
}

int QmlSessionBridge::selectedSeeds() const
{
    return hasSelection() ? m_session->torrentAt(m_selectedIndex).numSeeds : 0;
}

int QmlSessionBridge::selectedPeers() const
{
    return hasSelection() ? m_session->torrentAt(m_selectedIndex).numPeers : 0;
}

QString QmlSessionBridge::selectedRatio() const
{
    if (!hasSelection()) return {};
    return QString::number(m_session->torrentAt(m_selectedIndex).ratio, 'f', 2);
}

double QmlSessionBridge::selectedProgress() const
{
    if (!hasSelection()) return 0.0;
    return m_session->torrentAt(m_selectedIndex).progress;
}

QString QmlSessionBridge::selectedUploaded() const
{
    if (!hasSelection()) return {};
    return formatSize(m_session->torrentAt(m_selectedIndex).totalUploaded);
}

QString QmlSessionBridge::selectedAvailability() const
{
    if (!hasSelection()) return {};
    return QString::number(m_session->torrentAt(m_selectedIndex).availability, 'f', 2);
}

QString QmlSessionBridge::selectedAdded() const
{
    if (!hasSelection()) return {};
    const qint64 t = m_session->torrentAt(m_selectedIndex).addedTime;
    if (t <= 0) return QStringLiteral("—");
    const QDateTime dt = QDateTime::fromSecsSinceEpoch(t);
    if (dt.date() == QDate::currentDate())
        return QStringLiteral("Today ") + dt.toString(QStringLiteral("HH:mm"));
    return dt.toString(QStringLiteral("dd MMM HH:mm"));
}

QString QmlSessionBridge::selectedPath() const
{
    if (!hasSelection()) return {};
    return m_session->torrentAt(m_selectedIndex).savePath;
}

QString QmlSessionBridge::selectedState() const
{
    if (!hasSelection()) return {};
    return m_session->torrentAt(m_selectedIndex).stateString;
}

QString QmlSessionBridge::selectedPoster() const
{
    if (!hasSelection() || !m_resolver) return {};
    QString hash = m_session->torrentHashAt(m_selectedIndex);
    if (m_resolver->hasCached(hash)) {
        auto meta = m_resolver->cached(hash);
        if (meta.valid && !meta.posterPath.isEmpty()) {
            // mtime cache-bust so "fix cover" refreshes the detail-panel art too
            const qint64 mt = QFileInfo(meta.posterPath).lastModified().toMSecsSinceEpoch();
            return meta.posterPath + QStringLiteral("?v=") + QString::number(mt);
        }
    }
    return {};
}

QString QmlSessionBridge::selectedDescription() const
{
    if (!hasSelection() || !m_resolver) return {};
    QString hash = m_session->torrentHashAt(m_selectedIndex);
    if (m_resolver->hasCached(hash)) {
        auto meta = m_resolver->cached(hash);
        if (meta.valid) return meta.description;
    }
    return {};
}

QString QmlSessionBridge::selectedMetaTitle() const
{
    if (!hasSelection() || !m_resolver) return {};
    QString hash = m_session->torrentHashAt(m_selectedIndex);
    if (m_resolver->hasCached(hash)) {
        auto meta = m_resolver->cached(hash);
        if (meta.valid) return meta.title;
    }
    return {};
}

QString QmlSessionBridge::selectedMetaInfo() const
{
    if (!hasSelection() || !m_resolver) return {};
    QString hash = m_session->torrentHashAt(m_selectedIndex);
    if (!m_resolver->hasCached(hash)) return {};
    auto meta = m_resolver->cached(hash);
    if (!meta.valid) return {};
    QStringList parts;
    if (meta.year > 0) parts << QString::number(meta.year);
    if (!meta.genres.isEmpty()) parts << meta.genres.mid(0, 3).join(QStringLiteral(", "));
    if (meta.rating > 0) parts << QStringLiteral("%1/10").arg(meta.rating, 0, 'f', 1);
    return parts.join(QStringLiteral(" · "));
}
