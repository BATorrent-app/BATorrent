// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Wire types for the isolated media child. Rides the engine split's framing
// (ipc::Request for commands, ipc::Event for state); frames themselves travel
// through shared memory and only their header crosses the socket.
// See internal/ISOLATED_DECODER_PLAN.md.
#ifndef BATORRENT_MEDIAPROTOCOL_H
#define BATORRENT_MEDIAPROTOCOL_H

#include <QByteArray>
#include <QDataStream>
#include <QList>
#include <QString>
#include <array>
#include "ipc/ipcprotocol.h"

namespace media {

constexpr int kSlots = 3;
constexpr int kMaxPlanes = 4;
constexpr int kMaxDimension = 8192;

struct State {
    qint32 playbackState = 0;   // QMediaPlayer::PlaybackState
    qint32 mediaStatus = 0;     // QMediaPlayer::MediaStatus
    qint32 error = 0;           // QMediaPlayer::Error
    QString errorString;
    qint64 duration = 0;
    qint64 position = 0;
    bool seekable = false;
    bool hasVideo = false;
    double playbackRate = 1.0;
};

struct Track {
    qint32 language = 0;        // QLocale::Language
    QString title;
};

struct Tracks {
    QList<Track> audio;
    QList<Track> subtitles;
    qint32 activeAudio = -1;
    qint32 activeSubtitle = -1;
};

struct Plane {
    qint32 stride = 0;
    qint64 offset = 0;
    qint64 bytes = 0;
};

struct FrameHeader {
    qint32 slot = -1;
    qint32 pixelFormat = 0;     // QVideoFrameFormat::PixelFormat
    qint32 width = 0;
    qint32 height = 0;
    qint64 startUs = -1;
    qint64 endUs = -1;
    qint32 planeCount = 0;
    std::array<Plane, kMaxPlanes> planes{};
};

inline QDataStream &operator<<(QDataStream &s, const State &v)
{
    return s << v.playbackState << v.mediaStatus << v.error << v.errorString
             << v.duration << v.position << v.seekable << v.hasVideo << v.playbackRate;
}
inline QDataStream &operator>>(QDataStream &s, State &v)
{
    return s >> v.playbackState >> v.mediaStatus >> v.error >> v.errorString
             >> v.duration >> v.position >> v.seekable >> v.hasVideo >> v.playbackRate;
}

inline QDataStream &operator<<(QDataStream &s, const Track &v) { return s << v.language << v.title; }
inline QDataStream &operator>>(QDataStream &s, Track &v) { return s >> v.language >> v.title; }

inline QDataStream &operator<<(QDataStream &s, const Tracks &v)
{
    return s << v.audio << v.subtitles << v.activeAudio << v.activeSubtitle;
}
inline QDataStream &operator>>(QDataStream &s, Tracks &v)
{
    return s >> v.audio >> v.subtitles >> v.activeAudio >> v.activeSubtitle;
}

inline QDataStream &operator<<(QDataStream &s, const FrameHeader &v)
{
    s << v.slot << v.pixelFormat << v.width << v.height << v.startUs << v.endUs << v.planeCount;
    for (const Plane &p : v.planes) s << p.stride << p.offset << p.bytes;
    return s;
}
inline QDataStream &operator>>(QDataStream &s, FrameHeader &v)
{
    s >> v.slot >> v.pixelFormat >> v.width >> v.height >> v.startUs >> v.endUs >> v.planeCount;
    for (Plane &p : v.planes) s >> p.stride >> p.offset >> p.bytes;
    return s;
}

template <typename... T>
QByteArray encode(const T &...values)
{
    QByteArray out;
    QDataStream o(&out, QIODevice::WriteOnly);
    o.setVersion(ipc::kStreamVersion);
    (o << ... << values);
    return out;
}

} // namespace media

#endif
