// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef ISOLATEDMEDIAPLAYER_H
#define ISOLATEDMEDIAPLAYER_H

#include <QList>
#include <QMediaMetaData>
#include <QObject>
#include <QPointer>
#include <QUrl>
#include <memory>
#include "ipc/mediaprotocol.h"
#include "ipc/sharedsegment.h"

class ContainedLaunch;
class MediaChannel;
class MediaFeed;
class QLocalSocket;
class QProcess;
class QVideoSink;

// Drop-in for QML's MediaPlayer whose decoder runs in a `--media` child
// (internal/ISOLATED_DECODER_PLAN.md). Same property names and enum values,
// so the player QML binds to either. Everything the child sends is untrusted.
class IsolatedMediaPlayer : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QUrl source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(QObject *videoOutput READ videoOutput WRITE setVideoOutput NOTIFY videoOutputChanged)
    Q_PROPERTY(float volume READ volume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(bool muted READ muted WRITE setMuted NOTIFY mutedChanged)
    Q_PROPERTY(int playbackState READ playbackState NOTIFY playbackStateChanged)
    Q_PROPERTY(int mediaStatus READ mediaStatus NOTIFY mediaStatusChanged)
    Q_PROPERTY(int error READ error NOTIFY errorChanged)
    Q_PROPERTY(QString errorString READ errorString NOTIFY errorChanged)
    Q_PROPERTY(qint64 duration READ duration NOTIFY durationChanged)
    Q_PROPERTY(qint64 position READ position WRITE setPosition NOTIFY positionChanged)
    Q_PROPERTY(bool seekable READ seekable NOTIFY seekableChanged)
    Q_PROPERTY(bool hasVideo READ hasVideo NOTIFY hasVideoChanged)
    Q_PROPERTY(qreal playbackRate READ playbackRate WRITE setPlaybackRate NOTIFY playbackRateChanged)
    Q_PROPERTY(QList<QMediaMetaData> audioTracks READ audioTracks NOTIFY tracksChanged)
    Q_PROPERTY(QList<QMediaMetaData> subtitleTracks READ subtitleTracks NOTIFY tracksChanged)
    Q_PROPERTY(int activeAudioTrack READ activeAudioTrack WRITE setActiveAudioTrack NOTIFY activeTracksChanged)
    Q_PROPERTY(int activeSubtitleTrack READ activeSubtitleTrack WRITE setActiveSubtitleTrack NOTIFY activeTracksChanged)

public:
    explicit IsolatedMediaPlayer(QObject *parent = nullptr);
    ~IsolatedMediaPlayer() override;

    QUrl source() const { return m_source; }
    void setSource(const QUrl &source);
    QObject *videoOutput() const { return m_videoOutput; }
    void setVideoOutput(QObject *output);
    float volume() const { return m_volume; }
    void setVolume(float volume);
    bool muted() const { return m_muted; }
    void setMuted(bool muted);
    int playbackState() const { return m_state.playbackState; }
    int mediaStatus() const { return m_state.mediaStatus; }
    int error() const { return m_state.error; }
    QString errorString() const { return m_state.errorString; }
    qint64 duration() const { return m_state.duration; }
    qint64 position() const { return m_state.position; }
    void setPosition(qint64 ms);
    bool seekable() const { return m_state.seekable; }
    bool hasVideo() const { return m_state.hasVideo; }
    qreal playbackRate() const { return m_state.playbackRate; }
    void setPlaybackRate(qreal rate);
    QList<QMediaMetaData> audioTracks() const { return m_audioTracks; }
    QList<QMediaMetaData> subtitleTracks() const { return m_subtitleTracks; }
    int activeAudioTrack() const { return m_activeAudio; }
    void setActiveAudioTrack(int index);
    int activeSubtitleTrack() const { return m_activeSubtitle; }
    void setActiveSubtitleTrack(int index);

    Q_INVOKABLE void play();
    Q_INVOKABLE void pause();
    Q_INVOKABLE void stop();

    // Test seam: open the channel but start no child; a MediaHost in the same
    // process connects to the returned name instead.
    QString listenWithoutChild();

signals:
    void sourceChanged();
    void videoOutputChanged();
    void volumeChanged();
    void mutedChanged();
    void playbackStateChanged();
    void mediaStatusChanged();
    void errorChanged();
    void errorOccurred(int error, const QString &errorString);
    void durationChanged();
    void positionChanged(qint64 position);
    void seekableChanged();
    void hasVideoChanged();
    void playbackRateChanged();
    void tracksChanged();
    void activeTracksChanged();
    void decoderCrashed();

private:
    bool ensureChild();
    bool openChannel(const QString &peerSid);
    void adopt(QLocalSocket *socket);
    void startFeed();
    void onReadyRead();
    void onEvent(const QString &name, const QByteArray &args);
    void applyState(const media::State &s);
    void applyTracks(const media::Tracks &t);
    void shareRing(qint64 slotBytes);
    void showFrame(qint32 generation, const media::FrameHeader &hdr);
    void failWith(int error, const QString &why);
    void onChildLost();
    void teardown();
    void send(const QString &method, const QByteArray &args = {});
    QVideoSink *sink() const;

    QUrl m_source;
    QPointer<QObject> m_videoOutput;
    float m_volume = 1.0f;
    bool m_muted = false;
    media::State m_state;
    QList<QMediaMetaData> m_audioTracks;
    QList<QMediaMetaData> m_subtitleTracks;
    int m_activeAudio = -1;
    int m_activeSubtitle = -1;

    QString m_serverName;
    MediaChannel *m_channel = nullptr;
    QProcess *m_proc = nullptr;
    std::unique_ptr<ContainedLaunch> m_launch;
    qint64 m_childPid = 0;
    QLocalSocket *m_sock = nullptr;
    bool m_ready = false;
    bool m_tearingDown = false;
    QList<QByteArray> m_pending;
    QByteArray m_buf;

    MediaFeed *m_feed = nullptr;
    qint32 m_sourceGen = 0;
    // What the QML asked for before the child had a source to apply it to:
    // a stream's size is learned asynchronously, so "open" can trail "play".
    bool m_opened = false;
    int m_wantState = 0;          // QMediaPlayer::PlaybackState
    qint64 m_pendingSeek = -1;

    std::unique_ptr<SharedSegment> m_ring;
    qint32 m_generation = 0;
    qint64 m_slotBytes = 0;
};

#endif
