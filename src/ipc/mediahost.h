// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef BATORRENT_MEDIAHOST_H
#define BATORRENT_MEDIAHOST_H

#include <QAudioOutput>
#include <QMediaPlayer>
#include <QObject>
#include <QPointer>
#include <QVideoSink>
#include <array>
#include <memory>
#include "ipc/mediaprotocol.h"
#include "ipc/sharedsegment.h"

class QLocalSocket;
class RemoteSource;

// The media child's side: owns the real QMediaPlayer (ffmpeg + audio out),
// reads every byte through RemoteSource and writes frames into the ring the
// UI shares with it. Connects to the UI and quits when the UI goes away.
class MediaHost : public QObject
{
    Q_OBJECT
public:
    // `sandboxed`: confine this process once connected. Only the real --media
    // child asks for it; a test hosting one in-process must not.
    explicit MediaHost(const QString &serverName, bool sandboxed = false, QObject *parent = nullptr);
    ~MediaHost() override;

    bool start(int timeoutMs = 5000);

private:
    void onReadyRead();
    void dispatch(const QString &method, const QByteArray &args);
    void openSource(qint32 generation, qint64 size, const QString &hint);
    void closeSource();
    void attachRing(qint32 generation, qint64 slotBytes, const QString &token);
    void onFrame(const QVideoFrame &frame);
    void scheduleState();
    void sendState();
    void sendTracks();
    void refuse(const QString &why);
    void sendEvent(const QString &name, const QByteArray &args);

    QString m_serverName;
    bool m_wantSandbox = false;
    QLocalSocket *m_sock = nullptr;
    QByteArray m_buf;

    QMediaPlayer m_player;
    QAudioOutput m_audio;
    QVideoSink m_sink;
    QPointer<RemoteSource> m_source;
    qint32 m_sourceGen = 0;

    std::unique_ptr<SharedSegment> m_ring;
    qint64 m_slotBytes = 0;
    qint32 m_generation = 0;
    qint64 m_ringAsked = 0;
    std::array<bool, media::kSlots> m_busy{};
    bool m_stateQueued = false;
};

#endif
