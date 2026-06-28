// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef SUBTITLEPARSER_H
#define SUBTITLEPARSER_H

#include <QList>
#include <QString>

struct SubtitleCue {
    qint64 startMs = 0;
    qint64 endMs = 0;
    QString text;   // StyledText-safe: <i>/<b> kept, ASS/unknown tags stripped
};

// SRT/VTT parser for the built-in player's external-subtitle overlay. No
// QtMultimedia involvement — cues render as a synced Text over the video.
namespace SubtitleParser {

// Reads the file (UTF-8 with fallback to Latin-1 — .srt in the wild is often
// unlabeled Windows-1252) and parses by extension/content.
QList<SubtitleCue> parseFile(const QString &path);

QList<SubtitleCue> parseSrt(const QString &content);
QList<SubtitleCue> parseVtt(const QString &content);

}

#endif
