// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details
//
// QmlSessionBridge: MediaGuard glue. Every way into the player asks the guard
// first, and its verdicts reach QML with a query for finding another release.

#include "bridges/session/qmlsessionbridge.h"
#include "services/metadata/nameparser.h"
#include "services/security/mediaguard.h"
#include "torrent/iengine.h"

void QmlSessionBridge::setMediaGuard(MediaGuard *guard)
{
    m_guard = guard;
    connect(guard, &MediaGuard::quarantined, this,
            [this](const QString &hash, int, const QString &fileName, const QString &kindKey) {
        emit mediaQuarantined(hash, fileName, kindKey, searchQueryFor(hash));
    });
    connect(guard, &MediaGuard::lureFound, this,
            [this](const QString &hash, int, const QString &fileName) {
        emit mediaLure(hash, fileName, searchQueryFor(hash));
    });
}

bool QmlSessionBridge::guardRefuses(const QString &infoHash, int fileIndex)
{
    if (!m_guard) return false;
    // The renamed file no longer reads as a video, so bestVideoFile() would
    // skip it and quietly play whatever else the fake torrent carries.
    const int held = m_guard->quarantinedFileOf(infoHash);
    if (held >= 0) {
        m_guard->remind(infoHash, held);
        return true;
    }
    return fileIndex >= 0 && m_guard->admit(infoHash, fileIndex) == MediaGuard::Gate::Block;
}

QString QmlSessionBridge::searchQueryFor(const QString &infoHash) const
{
    const int row = m_session->torrentIndexByInfoHash(infoHash);
    if (row < 0) return {};
    const QString raw = m_session->torrentAt(row).name;
    const ParsedName parsed = NameParser::parse(raw);
    if (parsed.cleanTitle.isEmpty()) return raw;
    return parsed.year > 0 ? parsed.cleanTitle + QLatin1Char(' ') + QString::number(parsed.year)
                           : parsed.cleanTitle;
}
