// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "services/integrations/debridpick.h"

#include <QJsonObject>
#include <QStringList>

namespace DebridPick {

bool looksLikeVideo(const QString &path)
{
    static const QStringList exts = {
        QStringLiteral(".mp4"), QStringLiteral(".mkv"), QStringLiteral(".avi"),
        QStringLiteral(".mov"), QStringLiteral(".wmv"), QStringLiteral(".m4v"),
        QStringLiteral(".webm"), QStringLiteral(".flv"), QStringLiteral(".mpg"),
        QStringLiteral(".mpeg"), QStringLiteral(".ts"), QStringLiteral(".m2ts"),
    };
    const QString p = path.toLower();
    for (const QString &e : exts)
        if (p.endsWith(e)) return true;
    return false;
}

// RD's `links` array has one entry per *selected* file, in file order. Walk the
// files, counting only selected ones to map into the links index, and return
// the link position of the largest selected video (else the largest selected
// file of any kind), or -1 when nothing is selected.
int bestRealDebridLinkIndex(const QJsonArray &files)
{
    int selIdx = -1;
    int bestVideo = -1; qint64 bestVideoBytes = -1;
    int bestAny = -1;   qint64 bestAnyBytes = -1;
    for (const QJsonValue &v : files) {
        const QJsonObject f = v.toObject();
        if (f.value(QStringLiteral("selected")).toInt() != 1) continue;
        ++selIdx;
        const qint64 bytes = qint64(f.value(QStringLiteral("bytes")).toDouble());
        const QString path = f.value(QStringLiteral("path")).toString();
        if (bytes > bestAnyBytes) { bestAnyBytes = bytes; bestAny = selIdx; }
        if (looksLikeVideo(path) && bytes > bestVideoBytes) {
            bestVideoBytes = bytes; bestVideo = selIdx;
        }
    }
    return bestVideo >= 0 ? bestVideo : bestAny;
}

// TorBox files carry their own `id` (index within the torrent). Return the id +
// name of the largest video file, else the largest file of any kind, else {-1}.
QPair<int, QString> bestTorBoxFile(const QJsonArray &files)
{
    int bestVideo = -1;  qint64 bestVideoBytes = -1;  QString bestVideoName;
    int bestAny = -1;    qint64 bestAnyBytes = -1;    QString bestAnyName;
    for (const QJsonValue &v : files) {
        const QJsonObject f = v.toObject();
        const int id = f.value(QStringLiteral("id")).toInt(-1);
        if (id < 0) continue;
        const qint64 bytes = qint64(f.value(QStringLiteral("size")).toDouble());
        const QString name = f.value(QStringLiteral("name")).toString();
        if (bytes > bestAnyBytes) { bestAnyBytes = bytes; bestAny = id; bestAnyName = name; }
        if (looksLikeVideo(name) && bytes > bestVideoBytes) {
            bestVideoBytes = bytes; bestVideo = id; bestVideoName = name;
        }
    }
    if (bestVideo >= 0) return {bestVideo, bestVideoName};
    return {bestAny, bestAnyName};
}

}
