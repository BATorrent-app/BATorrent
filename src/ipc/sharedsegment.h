// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef BATORRENT_SHAREDSEGMENT_H
#define BATORRENT_SHAREDSEGMENT_H

#include <QString>
#include <QtGlobal>
#include <memory>

// The media child's frame ring. The UI owns it: it creates the segment, keeps
// a read-only view and hands the child a token to map it writable, so a
// sandboxed child never needs the right to create shared memory itself.
// Not QSharedMemory: its POSIX backend refuses every name on macOS, and the
// SysV fallback there caps a segment at 4 MB, under three 1080p frames.
class SharedSegment
{
public:
    // Owner side. `seed` names the segment where the platform needs a name.
    static std::unique_ptr<SharedSegment> create(const QString &seed, qint64 size);
    // Peer side, from the token shareWith() produced.
    static std::unique_ptr<SharedSegment> openWritable(const QString &token);

    ~SharedSegment();
    SharedSegment(const SharedSegment &) = delete;
    SharedSegment &operator=(const SharedSegment &) = delete;

    // What the peer needs to map this: a name on POSIX, a handle duplicated
    // into `peerPid` on Windows. Empty on failure.
    QString shareWith(qint64 peerPid);
    // Drop the name once the peer has mapped it, so nothing else can.
    void unpublish();

    uchar *data() { return m_data; }
    const uchar *constData() const { return m_data; }
    qint64 size() const { return m_size; }

private:
    SharedSegment() = default;

    uchar *m_data = nullptr;
    qint64 m_size = 0;
    QString m_name;
    void *m_handle = nullptr;
};

#endif
