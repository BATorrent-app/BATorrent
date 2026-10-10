// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef BATORRENT_MEDIAHOST_H
#define BATORRENT_MEDIAHOST_H

#include <QAudioOutput>
#include <QMediaPlayer>
#include <QObject>
#include <QUrl>
#include <QVideoSink>
#include <array>
#include <memory>
#include "ipc/mediaprotocol.h"
#include "ipc/sharedsegment.h"

class QLocalServer;
class QLocalSocket;

// The media child's side: owns the real QMediaPlayer (ffmpeg + audio out) and
// publishes state over the socket and frames through a shared-memory ring.
// Serves exactly one client and quits when it goes away.
class MediaHost : public QObject
{
    Q_OBJECT
public:
    // `sandboxed`: confine this process on the first source. Only the real
    // --media child asks for it; a test hosting one in-process must not.
    explicit MediaHost(const QString &serverName, bool sandboxed = false, QObject *parent = nullptr);
    ~MediaHost() override;

    bool listen();

private:
    void onNewConnection();
    void onReadyRead();
    void dispatch(const QString &method, const QByteArray &args);
    bool confineFor(const QUrl &url);
    void refuse(const QString &why);
    void onFrame(const QVideoFrame &frame);
    bool ensureRing(qint64 slotBytes);
    void scheduleState();
    void sendState();
    void sendTracks();
    void sendEvent(const QString &name, const QByteArray &args);

    QString m_serverName;
    bool m_wantSandbox = false;
    bool m_confined = false;
    QUrl m_confinedTo;
    QLocalServer *m_server = nullptr;
    QLocalSocket *m_client = nullptr;
    QByteArray m_buf;

    QMediaPlayer m_player;
    QAudioOutput m_audio;
    QVideoSink m_sink;

    std::unique_ptr<SharedSegment> m_ring;
    QString m_ringName;
    qint64 m_slotBytes = 0;
    qint32 m_generation = 0;
    std::array<bool, media::kSlots> m_busy{};
    bool m_stateQueued = false;
};

#endif
