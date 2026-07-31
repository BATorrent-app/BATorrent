// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details
//
// QmlSessionBridge — sidecar / file subtitle loading for the player.

#include "bridges/session/qmlsessionbridge.h"
#include "torrent/sessionmanager.h"
#include "services/subtitles/subtitleparser.h"
#include "services/platform/utils.h"

#include <QDir>
#include <QFileInfo>
#include <QVariantList>
#include <QVariantMap>

QVariantList QmlSessionBridge::loadSubtitleFile(const QString &path)
{
    QString p = path;
    if (p.startsWith(QLatin1String("file://"))) p = QUrl(p).toLocalFile();
    QVariantList out;
    const auto cues = SubtitleParser::parseFile(p);
    out.reserve(cues.size());
    for (const auto &c : cues) {
        QVariantMap m;
        m["start"] = c.startMs;
        m["end"] = c.endMs;
        m["text"] = c.text;
        out << m;
    }
    return out;
}

QString QmlSessionBridge::findSidecarSubtitle(const QString &infoHash, int fileIndex)
{
    const int row = m_session->torrentIndexByInfoHash(infoHash);
    if (row < 0) return {};
    QString video = m_session->streamFilePath(row, fileIndex);
    if (video.isEmpty()) return {};
    if (video.endsWith(QLatin1String(".!bt"))) video.chop(4);
    const QFileInfo vi(video);
    const QString base = vi.completeBaseName();
    QDir dir = vi.dir();
    for (const char *ext : {"srt", "vtt"}) {
        const QString exact = dir.filePath(base + QLatin1Char('.') + QLatin1String(ext));
        if (QFileInfo::exists(exact)) return exact;
    }
    // language-suffixed sidecars ("Movie.pt-BR.srt") sort first by name
    const QStringList matches = dir.entryList({base + QStringLiteral("*.srt"), base + QStringLiteral("*.vtt")},
                                              QDir::Files, QDir::Name);
    return matches.isEmpty() ? QString() : dir.filePath(matches.first());
}

