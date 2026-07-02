// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Wire protocol for the engine/UI split. Length-prefixed frames over a
// QLocalSocket; payloads are QDataStream blobs (Qt-native, version-pinned).
// See internal/ENGINE_SPLIT_PLAN.md.
#ifndef BATORRENT_IPCPROTOCOL_H
#define BATORRENT_IPCPROTOCOL_H

#include <QByteArray>
#include <QDataStream>
#include <QIODevice>
#include <functional>

namespace ipc {

// QDataStream version pinned so engine and UI agree regardless of Qt build.
constexpr QDataStream::Version kStreamVersion = QDataStream::Qt_6_0;

enum class Kind : quint8 {
    Hello   = 1,   // engine → UI once ready
    Ping    = 2,
    Pong    = 3,
    Request = 4,   // UI → engine: id, method, argsBlob
    Reply   = 5,   // engine → UI: id, resultBlob
    Event   = 6,   // engine → UI: name, argsBlob (signals/snapshots)
};

// A frame beyond this is protocol corruption or a hostile peer — a bogus
// length must not pin memory (the buffer would accumulate forever waiting for
// bytes that never come) or desync the stream via int overflow in the size check.
constexpr qsizetype kMaxFrameBytes = 64 * 1024 * 1024;

// One length-prefixed frame: [quint32 byteCount][Kind][payload].
inline void writeFrame(QIODevice *dev, Kind kind, const QByteArray &payload)
{
    QByteArray body;
    body.append(char(kind));
    body.append(payload);
    QByteArray out;
    QDataStream ds(&out, QIODevice::WriteOnly);
    ds.setVersion(kStreamVersion);
    ds << quint32(body.size());
    out.append(body);
    dev->write(out);
}

// Pull complete frames out of an accumulating buffer; calls sink(kind, payload)
// for each. Leaves a partial trailing frame in `buf` for the next read.
inline void drainFrames(QByteArray &buf, const std::function<void(Kind, const QByteArray &)> &sink)
{
    constexpr qsizetype kHeader = qsizetype(sizeof(quint32));
    for (;;) {
        if (buf.size() < kHeader) return;
        quint32 len = 0;
        {
            QDataStream ds(buf.left(kHeader));
            ds.setVersion(kStreamVersion);
            ds >> len;
        }
        if (qsizetype(len) > kMaxFrameBytes) { buf.clear(); return; }
        if (buf.size() - kHeader < qsizetype(len)) return;   // incomplete — wait for more
        const QByteArray body = buf.mid(kHeader, len);
        buf.remove(0, kHeader + qsizetype(len));
        if (body.isEmpty()) continue;
        const Kind kind = Kind(quint8(body.at(0)));
        sink(kind, body.mid(1));
    }
}

} // namespace ipc

#endif
