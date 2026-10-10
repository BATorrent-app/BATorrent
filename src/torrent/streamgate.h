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

constexpr qint64 kMinHead = 4LL * 1024 * 1024;
constexpr qint64 kMaxHead = 16LL * 1024 * 1024;

// Contiguous bytes from the start of the file wanted before opening it.
inline qint64 headBytes(qint64 fileSize)
{
    if (fileSize <= 0) return 0;
    return std::min(fileSize, std::clamp(fileSize / 200, kMinHead, kMaxHead));
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
