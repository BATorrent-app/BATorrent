// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details
//
// QmlSessionBridge — selected torrent detail panels (peers/files/trackers/pieces).

#include "bridges/qmlsessionbridge.h"
#include "torrent/sessionmanager.h"
#include "services/integrations/geoip.h"
#include "services/platform/utils.h"

#include <QVariantMap>
#include <QVariantList>

void QmlSessionBridge::setDetailPeersActive(bool active)
{
    if (m_detailPeersActive == active) return;
    m_detailPeersActive = active;
    if (active) {
        // Show a placeholder instantly, then build the (heavy) peer list off the
        // click on the next event-loop turn — opening the tab never blocks.
        m_peersLoading = true;
        m_peerCache.clear();
        emit selectionListsChanged();
        QTimer::singleShot(0, this, [this]() { rebuildPeerCache(); });
    } else {
        m_peerCache.clear();
        m_peersLoading = false;
    }
}

// Cheap getter — the QML binding reads the cache, never touches libtorrent.
QVariantList QmlSessionBridge::selectedPeerList() const { return m_peerCache; }

// Heavy build (peersAt pulls every peer from libtorrent). Runs deferred / per
// tick while the Peers tab is open — never on the QML binding path or the click.
void QmlSessionBridge::rebuildPeerCache()
{
    if (!m_detailPeersActive || !hasSelection()) {
        if (!m_peerCache.isEmpty()) { m_peerCache.clear(); emit selectionListsChanged(); }
        m_peersLoading = false;
        return;
    }
    QVariantList out;
    // A big swarm (9k+ peers) froze the UI: a QVariantMap + geo lookup per peer.
    // Cap to the 500 most active (capped inside peersAt, before the QString-heavy
    // conversion); the total count is shown in the panel.
    auto peers = m_session->peersAt(m_selectedIndex, 500);
    out.reserve(peers.size());
    for (const auto &p : peers) {
        QVariantMap m;
        m["ip"]       = p.ip;
        m["port"]     = p.port;
        m["client"]   = p.client;
        m["downSpeed"]= formatSpeed(p.downloadRate);
        m["upSpeed"]  = formatSpeed(p.uploadRate);
        m["progress"] = p.progress;
        const QString cc = m_geoIp->cachedCountry(p.ip);
        if (cc.isEmpty())
            m_geoIp->resolve(p.ip);
        // Windows has no color-emoji flag glyphs (regional indicators render as
        // letter pairs/boxes) — the QML side falls back to this bare code there.
        m["cc"]   = cc.toUpper();
        m["flag"] = cc.isEmpty() ? QString() : countryCodeToFlag(cc);
        out << m;
    }
    m_peerCache = out;
    m_peersLoading = false;
    emit selectionListsChanged();
}

QVariantList QmlSessionBridge::selectedFiles() const
{
    QVariantList out;
    if (!hasSelection()) return out;
    auto files = m_session->filesAt(m_selectedIndex);
    out.reserve(files.size());
    for (const auto &f : files) {
        QVariantMap m;
        m["path"]     = f.path;
        m["size"]     = formatSize(f.size);
        m["progress"] = f.progress;
        m["priority"] = f.priority;
        out << m;
    }
    return out;
}

QVariantList QmlSessionBridge::selectedTrackers() const
{
    QVariantList out;
    if (!hasSelection()) return out;
    auto trackers = m_session->trackersAt(m_selectedIndex);
    out.reserve(trackers.size());
    for (const auto &t : trackers) {
        QVariantMap m;
        m["url"]    = t.url;
        m["tier"]   = t.tier;
        m["status"] = t.status;
        out << m;
    }
    return out;
}

QVariantMap QmlSessionBridge::selectedPieces() const
{
    // Bounded + downsampled at the source: a large torrent has tens of thousands of
    // pieces; returning one QVariant each (re-marshalled on every stats tick) spiked
    // memory and crashed the Pieces tab on Windows. Cap to MAXCELLS buckets, each
    // holding the fraction of its piece-span that's done; real total/done stay exact.
    QVariantMap m;
    QVariantList cells;
    if (!hasSelection()) { m["total"] = 0; m["done"] = 0; m["cells"] = cells; return m; }
    const auto pieces = m_session->piecesAt(m_selectedIndex);
    const int total = int(pieces.size());
    int done = 0;
    for (bool b : pieces) if (b) ++done;

    constexpr int MAXCELLS = 2000;
    if (total <= MAXCELLS) {
        cells.reserve(total);
        for (bool b : pieces) cells << (b ? 1.0 : 0.0);
    } else {
        cells.reserve(MAXCELLS);
        for (int c = 0; c < MAXCELLS; ++c) {
            const qint64 lo = qint64(c) * total / MAXCELLS;
            const qint64 hi = qint64(c + 1) * total / MAXCELLS;
            int d = 0, cnt = 0;
            for (qint64 i = lo; i < hi; ++i) { ++cnt; if (pieces[i]) ++d; }
            cells << (cnt > 0 ? double(d) / double(cnt) : 0.0);
        }
    }
    m["total"] = total;
    m["done"]  = done;
    m["cells"] = cells;
    return m;
}

// Free-space queries + the disk-volume breakdown and the active-downloads /
// seeding / resume list projections live in qmlsessionbridge_disk.cpp.

