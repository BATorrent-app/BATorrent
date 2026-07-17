// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef SERVICES_METADATA_MKVCHAPTERS_H
#define SERVICES_METADATA_MKVCHAPTERS_H

#include <QString>
#include <QList>

// Self-contained Matroska (.mkv) chapter reader — no external binary, no
// libav. Walks the EBML tree with hard bounds checks (untrusted input:
// partial/torn downloads, malformed files) and returns the chapter list.
// Times are milliseconds. Empty on any parse failure or non-mkv input.

struct MkvChapter {
    qint64 startMs = 0;
    qint64 endMs = -1;      // -1 when the file omits ChapterTimeEnd
    QString name;
    // Classified from the name: "intro" (opening/recap) or "credits"
    // (ending/outro), else empty — drives the player's skip chip.
    QString kind;
};

// Reads at most a bounded number of chapters; seeks past big elements
// (clusters) by their declared size instead of reading them, so it stays cheap
// even on multi-GB files.
QList<MkvChapter> readMkvChapters(const QString &filePath);

#endif
