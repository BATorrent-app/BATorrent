// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef BATORRENT_SHAREDSEGMENT_H
#define BATORRENT_SHAREDSEGMENT_H

#include <QString>
#include <QtGlobal>
#include <memory>

// A named shared-memory segment for the media child's frame ring. Not
// QSharedMemory: its POSIX backend refuses every name on macOS, and the SysV
// fallback there caps a segment at 4 MB, under three 1080p frames.
class SharedSegment
{
public:
    // Short, platform-legal name derived from `seed` (macOS allows 31 chars).
    static QString nameFor(const QString &seed);

    static std::unique_ptr<SharedSegment> create(const QString &name, qint64 size);
    // Read-only. The name is released right after, so nothing else can open it.
    static std::unique_ptr<SharedSegment> openReadOnly(const QString &name);
    static void release(const QString &name);

    ~SharedSegment();
    SharedSegment(const SharedSegment &) = delete;
    SharedSegment &operator=(const SharedSegment &) = delete;

    uchar *data() { return m_data; }
    const uchar *constData() const { return m_data; }
    qint64 size() const { return m_size; }

private:
    SharedSegment() = default;

    uchar *m_data = nullptr;
    qint64 m_size = 0;
#ifdef Q_OS_WIN
    void *m_handle = nullptr;
#endif
};

#endif
