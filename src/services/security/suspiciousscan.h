// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef SUSPICIOUSSCAN_H
#define SUSPICIOUSSCAN_H

#include <QList>
#include <QString>

// Local, dependency-free malware-awareness heuristic. It does NOT decide
// "safe" — it only flags the file patterns that are overwhelmingly malware in
// a media torrent (an executable smuggled into a movie, a double extension
// like film.mp4.exe). Silent when nothing matches. No engine, no network.
// Framing rule: this is a "security warning", never an "antivirus".
namespace SuspiciousScan {

struct ScanFile {
    QString path;       // relative path inside the torrent
    qint64 size = 0;    // bytes
};

struct Finding {
    QString file;       // the offending relative path
    QString reason;     // "double_ext" | "exe_in_media" | "fake_codec"
};

// Returns one Finding per suspicious file (empty when clean).
QList<Finding> scan(const QList<ScanFile> &files);

}

#endif
