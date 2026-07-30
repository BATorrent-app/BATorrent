// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details
//
// QmlSessionBridge — speed sampling / aggregate stats / palette. Split out of
// qmlsessionbridge.cpp verbatim; no behaviour change.

#include "bridges/qmlsessionbridge.h"
#include "torrent/sessionmanager.h"
#include "services/platform/utils.h"

#include <QVariantList>
#include <QVector>
#include <QSet>

static QVariantList intsToVariant(const QVector<int> &v)
{
    QVariantList out;
    out.reserve(v.size());
    for (int x : v) out << x;
    return out;
}

void QmlSessionBridge::sampleSpeeds()
{
    int dl = 0, ul = 0;
    QSet<QString> live;
    for (int i = 0; i < m_session->torrentCount(); ++i) {
        auto info = m_session->torrentAt(i);
        dl += info.downloadRate;
        ul += info.uploadRate;

        const QString h = m_session->torrentHashAt(i);
        if (h.isEmpty()) continue;
        live.insert(h);
        QVector<int> &dh = m_torrentDownHist[h];
        QVector<int> &uh = m_torrentUpHist[h];
        dh.append(info.downloadRate);
        uh.append(info.uploadRate);
        while (dh.size() > HistoryMaxPoints) dh.removeFirst();
        while (uh.size() > HistoryMaxPoints) uh.removeFirst();
    }
    // forget history for torrents that are gone (no leak across a session)
    for (auto it = m_torrentDownHist.begin(); it != m_torrentDownHist.end(); ) {
        if (!live.contains(it.key())) { m_torrentUpHist.remove(it.key()); it = m_torrentDownHist.erase(it); }
        else ++it;
    }

    m_downloadHistory.append(dl);
    m_uploadHistory.append(ul);
    while (m_downloadHistory.size() > HistoryMaxPoints) m_downloadHistory.removeFirst();
    while (m_uploadHistory.size() > HistoryMaxPoints) m_uploadHistory.removeFirst();
    emit historyChanged();
}

QVariantList QmlSessionBridge::selectedDownHistory() const
{
    if (!hasSelection()) return {};
    const QString h = m_session->torrentHashAt(m_selectedIndex);   // FULL hash (selectedHash() is abbreviated for display)
    const auto it = m_torrentDownHist.constFind(h);
    return it == m_torrentDownHist.constEnd() ? QVariantList() : intsToVariant(*it);
}

QVariantList QmlSessionBridge::selectedUpHistory() const
{
    if (!hasSelection()) return {};
    const QString h = m_session->torrentHashAt(m_selectedIndex);
    const auto it = m_torrentUpHist.constFind(h);
    return it == m_torrentUpHist.constEnd() ? QVariantList() : intsToVariant(*it);
}

QVariantList QmlSessionBridge::downloadHistory() const
{
    QVariantList out;
    out.reserve(m_downloadHistory.size());
    for (int v : m_downloadHistory) out << v;
    return out;
}

QVariantList QmlSessionBridge::uploadHistory() const
{
    QVariantList out;
    out.reserve(m_uploadHistory.size());
    for (int v : m_uploadHistory) out << v;
    return out;
}

int QmlSessionBridge::torrentCount() const { return m_session->torrentCount(); }

int QmlSessionBridge::activeCount() const      { return m_activeCount; }
int QmlSessionBridge::downloadingCount() const { return m_downloadingCount; }
int QmlSessionBridge::seedingCount() const     { return m_seedingCount; }
int QmlSessionBridge::pausedCount() const      { return m_pausedCount; }
int QmlSessionBridge::completedCount() const   { return m_completedCount; }
int QmlSessionBridge::queuedCount() const      { return m_queuedCount; }
QString QmlSessionBridge::totalDownSpeed() const { return formatSpeed(m_totalDownRate); }
QString QmlSessionBridge::totalUpSpeed() const   { return formatSpeed(m_totalUpRate); }

QString QmlSessionBridge::totalDownloaded() const { return formatSize(m_session->globalDownloaded()); }
QString QmlSessionBridge::totalUploaded() const { return formatSize(m_session->globalUploaded()); }
QString QmlSessionBridge::globalRatio() const { return QString::number(m_session->globalRatio(), 'f', 2); }

QVariantList QmlSessionBridge::torrentPalette() const
{
    QVariantList out;
    const int n = m_session->torrentCount();
    for (int i = 0; i < n; ++i) {
        const TorrentInfo info = m_session->torrentAt(i);
        if (info.name.isEmpty()) continue;
        QVariantMap m;
        m["name"] = info.name;
        m["source"] = i;
        out << m;
    }
    return out;
}

