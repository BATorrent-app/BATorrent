// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef UTILS_H
#define UTILS_H

#include <QString>

inline QString formatSize(qint64 bytes)
{
    if (bytes < 1024) return QString::number(bytes) + " B";
    if (bytes < 1024 * 1024) return QString::number(bytes / 1024.0, 'f', 1) + " KB";
    if (bytes < 1024LL * 1024 * 1024) return QString::number(bytes / (1024.0 * 1024.0), 'f', 1) + " MB";
    return QString::number(bytes / (1024.0 * 1024.0 * 1024.0), 'f', 2) + " GB";
}

inline QString formatSpeed(int bps)
{
    if (bps < 1024) return QString::number(bps) + " B/s";
    if (bps < 1024 * 1024) return QString::number(bps / 1024.0, 'f', 1) + " KB/s";
    return QString::number(bps / (1024.0 * 1024.0), 'f', 1) + " MB/s";
}

#endif
