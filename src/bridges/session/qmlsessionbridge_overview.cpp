// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details
//
// QmlSessionBridge: the Downloads inspector's overview, for when nothing is
// selected. Only what asks for a hand or is not on screen anywhere else:
// what needs you (files gone, an engine error, a download with no seeds, one
// paused half way), what finished and was never opened, and how much space
// is left once everything still downloading lands. Raw facts; the QML side
// words them.

#include "bridges/session/qmlsessionbridge.h"
#include "torrent/sessionmanager.h"
#include "services/metadata/metadataresolver.h"
#include "services/games/playlog.h"
#include "services/platform/utils.h"  // formatSize

#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QSettings>
#include <QStorageInfo>
#include <QUrl>
#include <algorithm>

QVariantMap QmlSessionBridge::overview() const
{
    QVariantList attention, ready;
    qint64 remaining = 0;
    QSettings s;
    const int n = m_session->torrentCount();
    struct Row { qint64 added; QVariantMap m; };
    QList<Row> att, rdy;
    for (int row = 0; row < n; ++row) {
        const TorrentInfo info = m_session->torrentAt(row);
        const QString hash = m_session->torrentHashAt(row);
        if (hash.isEmpty()) continue;
        const QString key = torrentStateKey(info);
        if (!info.completed && !info.finished && info.totalSize > 0)
            remaining += qMax<qint64>(0, info.totalSize - info.totalDone);

        QString poster;
        const QString title = shownTitle(hash, info.name, &poster);
        const QString ext = QFileInfo(info.name).suffix().toUpper();
        QVariantMap m;
        m[QStringLiteral("infoHash")] = hash;
        m[QStringLiteral("title")] = title;
        m[QStringLiteral("poster")] = poster;
        m[QStringLiteral("kind")] = (ext.size() >= 2 && ext.size() <= 4) ? ext : QString();

        QString reason;
        if (key == QLatin1String("missing")) reason = QStringLiteral("missing");
        else if (key == QLatin1String("error")) reason = QStringLiteral("error");
        else if (info.fetchingMetadata && fetchingTooLong(hash, info.fetchingSecs)) reason = QStringLiteral("fetching");
        else if (key == QLatin1String("downloading") && info.numSeeds == 0 && info.numPeers > 0
                 && info.downloadRate < 1024)
            reason = QStringLiteral("noseeds");
        else if (key == QLatin1String("paused") && info.progress > 0.0f && info.progress < 0.999f)
            reason = QStringLiteral("paused");
        if (!reason.isEmpty()) {
            m[QStringLiteral("reason")] = reason;
            m[QStringLiteral("detail")] = info.stateDetail;
            m[QStringLiteral("pct")] = int(info.progress * 100);
            m[QStringLiteral("left")] = formatSize(qMax<qint64>(0, info.totalSize - info.totalDone));
            m[QStringLiteral("minutes")] = int(qMax<qint64>(0, info.fetchingSecs) / 60);
            // since when, for "12 min ago": stuck fetching, or paused, when known
            const qint64 now = QDateTime::currentSecsSinceEpoch();
            if (reason == QLatin1String("fetching")) m[QStringLiteral("since")] = now - qMax<qint64>(0, info.fetchingSecs);
            else if (reason == QLatin1String("paused") && m_pausedSince.contains(hash)) m[QStringLiteral("since")] = m_pausedSince.value(hash);
            // trouble first, a pause last: it is the user's own choice
            att.append({ reason == QLatin1String("paused") ? -info.addedTime : (qint64(1) << 40) + info.addedTime, m });
            continue;
        }

        // finished and never opened
        if (key != QLatin1String("completed") && key != QLatin1String("seeding")) continue;
        if (isGameTorrent(row)) {
            if (m_playLog && m_playLog->summary(hash).lastPlayedMs > 0) continue;
            m[QStringLiteral("type")] = QStringLiteral("game");
        } else {
            const QVariantMap w = watchState(hash);
            if (w.value(QStringLiteral("videos")).toInt() < 1) continue;
            if (w.value(QStringLiteral("watched")).toBool() || w.value(QStringLiteral("resumeMs")).toLongLong() > 0) continue;
            m[QStringLiteral("type")] = QStringLiteral("video");
        }
        if (info.completedTime > 0) m[QStringLiteral("since")] = info.completedTime;
        rdy.append({ info.addedTime, m });
    }
    auto newest = [](const Row &a, const Row &b) { return a.added > b.added; };
    std::sort(att.begin(), att.end(), newest);
    std::sort(rdy.begin(), rdy.end(), newest);
    for (int i = 0; i < att.size() && i < 4; ++i) attention << att[i].m;
    for (int i = 0; i < rdy.size() && i < 3; ++i) ready << rdy[i].m;

    QVariantMap disk;
    const QString path = defaultSavePath();
    const QStorageInfo si(path.isEmpty() ? QDir::homePath() : path);
    if (si.isValid() && si.bytesTotal() > 0) {
        const qint64 free = si.bytesAvailable(), total = si.bytesTotal();
        disk[QStringLiteral("name")] = si.displayName().isEmpty() ? si.rootPath() : si.displayName();
        disk[QStringLiteral("usedFraction")] = double(total - free) / double(total);
        disk[QStringLiteral("remainingFraction")] = double(qMin(remaining, free)) / double(total);
        disk[QStringLiteral("remaining")] = formatSize(remaining);
        disk[QStringLiteral("remainingBytes")] = remaining;
        disk[QStringLiteral("freeAfter")] = formatSize(qMax<qint64>(0, free - remaining));
        disk[QStringLiteral("short")] = remaining > free;
    }

    QVariantMap out;
    out[QStringLiteral("attention")] = attention;
    out[QStringLiteral("attentionCount")] = int(att.size());
    out[QStringLiteral("ready")] = ready;
    out[QStringLiteral("disk")] = disk;
    return out;
}
