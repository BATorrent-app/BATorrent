// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef MEDIASNIFF_H
#define MEDIASNIFF_H

#include <QByteArray>
#include <QString>
#include <QtGlobal>

// What a file named like a video really is, read from its first bytes. Pure:
// no I/O, no decoder, so untrusted data never reaches ffmpeg to be judged.
// Only a positive match on something that is not media counts against a file;
// an unrecognised container is given the benefit of the doubt.
namespace MediaSniff {

enum class Kind {
    Unknown,
    Matroska, Mp4, Avi, MpegTs, MpegPs, Flv, Asf,
    Executable, Archive, Document, WebPage,
};

enum class Verdict {
    NeedMore,    // the head isn't on disk yet
    Ok,
    Disguised,   // not a video at all: quarantine
    Lure,        // a real ASF that sends the viewer to a "licence"/"codec" page
};

constexpr qint64 kHeadBytes = 64 * 1024;

qint64 headWanted(qint64 fileSize);
bool isVideoName(const QString &fileName);
Kind identify(const QByteArray &head);
bool asfHasLure(const QByteArray &head);
Verdict judge(const QByteArray &head, qint64 fileSize);

// Stable key for the UI copy ("a program", "a compressed archive", …).
QString kindKey(Kind kind);

}

#endif
