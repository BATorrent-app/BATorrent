// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#pragma once

#include <QString>
#include <QtGlobal>
#include <algorithm>

// When a file still downloading can be handed to the player. FFmpeg probes
// the head; an MP4 also needs its 'moov' atom, which usually sits at EOF. MKV
// keeps only its cues there, and those are for seeking, not for starting.
namespace StreamGate {

constexpr qint64 kMinHead = 3LL * 1024 * 1024;
constexpr qint64 kMaxHead = 10LL * 1024 * 1024;
constexpr qint64 kHeadSecs = 3;

// The runtime is unknown until the player opens the file, so the bitrate is
// guessed from the size over a short episode's runtime. A movie comes out
// above its real bitrate, which only makes its deadlines tighter.
constexpr qint64 kAssumedRuntimeSecs = 30 * 60;
constexpr qint64 kMinBytesPerSec = 256 * 1024;
constexpr qint64 kWindowSecs = 30;
constexpr int kMinWindowPieces = 4;
constexpr int kMaxWindowPieces = 128;
constexpr int kTailPieces = 2;
constexpr int kNearAheadWindows = 10;   // past the window, ~5 min more gets the next priority down

inline qint64 bytesPerSec(qint64 fileSize)
{
    return std::max(fileSize / kAssumedRuntimeSecs, kMinBytesPerSec);
}

// When playback should reach a byte `distance` past the reader, in ms.
inline int deadlineMs(qint64 distance, qint64 fileSize)
{
    if (distance <= 0) return 0;
    return int(std::min<qint64>(distance * 1000 / bytesPerSec(fileSize), 10 * 60 * 1000));
}

// Pieces covering kWindowSecs of playback ahead of the reader.
inline int windowPieces(qint64 pieceSize, qint64 fileSize)
{
    if (pieceSize <= 0) return kMinWindowPieces;
    const qint64 n = (kWindowSecs * bytesPerSec(fileSize) + pieceSize - 1) / pieceSize;
    return int(std::clamp<qint64>(n, kMinWindowPieces, kMaxWindowPieces));
}

// Contiguous bytes from the start of the file wanted before opening it: a few
// seconds of playback. The player keeps buffering past this on its own.
inline qint64 headBytes(qint64 fileSize)
{
    if (fileSize <= 0) return 0;
    return std::min(fileSize, std::clamp(kHeadSecs * bytesPerSec(fileSize), kMinHead, kMaxHead));
}

inline bool needsTail(const QString &fileName)
{
    QString n = fileName.toLower();
    if (n.endsWith(QLatin1String(".!bt"))) n.chop(4);
    return n.endsWith(QLatin1String(".mp4")) || n.endsWith(QLatin1String(".m4v"))
        || n.endsWith(QLatin1String(".mov"));
}

inline bool ready(qint64 contiguousHead, qint64 fileSize, bool haveTail, bool tailNeeded = true)
{
    return fileSize > 0 && (haveTail || !tailNeeded) && contiguousHead >= headBytes(fileSize);
}

// 0..1 for the buffering overlay: the head is most of the wait, the tail the rest.
inline double progress(qint64 contiguousHead, qint64 fileSize, bool haveTail, bool tailNeeded = true)
{
    const qint64 want = headBytes(fileSize);
    if (want <= 0) return 0.0;
    const double head = double(std::min(contiguousHead, want)) / double(want);
    if (!tailNeeded) return head;
    return head * 0.9 + (haveTail ? 0.1 : 0.0);
}

} // namespace StreamGate
