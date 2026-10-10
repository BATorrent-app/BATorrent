// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "bridges/isolatedmediaplayer.h"
#include "ipc/ipcprotocol.h"
#include "ipc/mediaframe.h"

#include <QCoreApplication>
#include <QDebug>
#include <QLocalSocket>
#include <QLocale>
#include <QMediaPlayer>
#include <QProcess>
#include <QTimer>
#include <QVideoSink>
#include <algorithm>

namespace {

constexpr int kConnectIntervalMs = 50;
constexpr int kConnectTries = 100;
constexpr int kMaxTracks = 64;
constexpr int kMaxText = 4096;

QList<QMediaMetaData> toMetaData(const QList<media::Track> &tracks)
{
    QList<QMediaMetaData> out;
    for (const media::Track &t : tracks.mid(0, kMaxTracks)) {
        QMediaMetaData m;
        if (t.language > 0 && t.language <= int(QLocale::LastLanguage))
            m.insert(QMediaMetaData::Language, QVariant::fromValue(QLocale::Language(t.language)));
        if (!t.title.isEmpty()) m.insert(QMediaMetaData::Title, t.title.left(kMaxText));
        out.append(m);
    }
    return out;
}

}

IsolatedMediaPlayer::IsolatedMediaPlayer(QObject *parent) : QObject(parent) {}

IsolatedMediaPlayer::~IsolatedMediaPlayer()
{
    teardown();
}

QVideoSink *IsolatedMediaPlayer::sink() const
{
    if (!m_videoOutput) return nullptr;
    if (auto *s = qobject_cast<QVideoSink *>(m_videoOutput.data())) return s;
    return m_videoOutput->property("videoSink").value<QVideoSink *>();
}

void IsolatedMediaPlayer::setSource(const QUrl &source)
{
    if (source == m_source) return;
    m_source = source;
    emit sourceChanged();
    if (source.isEmpty()) {
        teardown();
        applyState(media::State{});
        applyTracks(media::Tracks{});
        if (QVideoSink *s = sink()) s->setVideoFrame({});
        return;
    }
    // A sandboxed child is bound to the source it opened: a new one gets a new child.
    if (m_proc) teardown();
    ensureChild();
    send(QStringLiteral("source"), media::encode(source));
}

void IsolatedMediaPlayer::setVideoOutput(QObject *output)
{
    if (output == m_videoOutput) return;
    m_videoOutput = output;
    emit videoOutputChanged();
}

void IsolatedMediaPlayer::setVolume(float volume)
{
    volume = std::clamp(volume, 0.0f, 1.0f);
    if (qFuzzyCompare(volume, m_volume)) return;
    m_volume = volume;
    emit volumeChanged();
    if (m_proc || m_sock) send(QStringLiteral("volume"), media::encode(m_volume));
}

void IsolatedMediaPlayer::setMuted(bool muted)
{
    if (muted == m_muted) return;
    m_muted = muted;
    emit mutedChanged();
    if (m_proc || m_sock) send(QStringLiteral("muted"), media::encode(m_muted));
}

void IsolatedMediaPlayer::setPosition(qint64 ms)
{
    ms = std::max<qint64>(0, ms);
    send(QStringLiteral("seek"), media::encode(ms));
    // Show the target now: waiting a round trip makes the scrubber snap back.
    if (m_state.position != ms) { m_state.position = ms; emit positionChanged(ms); }
}

void IsolatedMediaPlayer::setPlaybackRate(qreal rate)
{
    send(QStringLiteral("rate"), media::encode(double(rate)));
}

void IsolatedMediaPlayer::setActiveAudioTrack(int index)
{
    send(QStringLiteral("audioTrack"), media::encode(qint32(index)));
}

void IsolatedMediaPlayer::setActiveSubtitleTrack(int index)
{
    send(QStringLiteral("subTrack"), media::encode(qint32(index)));
}

void IsolatedMediaPlayer::play()
{
    if (m_source.isEmpty()) return;
    if (!m_proc && !m_sock) {
        ensureChild();
        send(QStringLiteral("source"), media::encode(m_source));
    }
    send(QStringLiteral("play"));
}

void IsolatedMediaPlayer::pause() { send(QStringLiteral("pause")); }
void IsolatedMediaPlayer::stop() { send(QStringLiteral("stop")); }

void IsolatedMediaPlayer::ensureChild()
{
    if (m_proc || m_sock) return;
    static int counter = 0;
    m_serverName = QStringLiteral("batorrent-media-%1-%2")
                       .arg(QCoreApplication::applicationPid()).arg(++counter);
    m_proc = new QProcess(this);
    m_proc->setProcessChannelMode(QProcess::ForwardedChannels);
    connect(m_proc, &QProcess::finished, this, [this](int code, QProcess::ExitStatus status) {
        if (m_tearingDown) return;
        qWarning() << "[media] decoder exited" << code << (status == QProcess::CrashExit ? "(crash)" : "");
        onChildLost();
    });
    m_proc->start(QCoreApplication::applicationFilePath(), { QStringLiteral("--media"), m_serverName });
    attachTo(m_serverName);
    send(QStringLiteral("volume"), media::encode(m_volume));
    send(QStringLiteral("muted"), media::encode(m_muted));
}

void IsolatedMediaPlayer::attachTo(const QString &serverName)
{
    m_serverName = serverName;
    m_ready = false;
    m_sock = new QLocalSocket(this);
    connect(m_sock, &QLocalSocket::readyRead, this, &IsolatedMediaPlayer::onReadyRead);
    connect(m_sock, &QLocalSocket::connected, this, [this] { m_connectTimer->stop(); });
    connect(m_sock, &QLocalSocket::disconnected, this, [this] {
        if (!m_tearingDown && m_ready) onChildLost();
    });
    m_connectTries = 0;
    if (!m_connectTimer) {
        m_connectTimer = new QTimer(this);
        m_connectTimer->setInterval(kConnectIntervalMs);
        connect(m_connectTimer, &QTimer::timeout, this, &IsolatedMediaPlayer::tryConnect);
    }
    m_connectTimer->start();
    tryConnect();
}

void IsolatedMediaPlayer::tryConnect()
{
    // The child needs a moment to listen; a refused connect drops the socket
    // back to Unconnected and the next tick tries again.
    if (!m_sock) { m_connectTimer->stop(); return; }
    if (m_sock->state() != QLocalSocket::UnconnectedState) return;
    if (++m_connectTries > kConnectTries) {
        m_connectTimer->stop();
        qWarning() << "[media] could not reach decoder" << m_serverName;
        onChildLost();
        return;
    }
    m_sock->connectToServer(m_serverName);
}

void IsolatedMediaPlayer::send(const QString &method, const QByteArray &args)
{
    const QByteArray payload = media::encode(quint32(0), method, args);
    if (m_ready && m_sock) ipc::writeFrame(m_sock, ipc::Kind::Request, payload);
    else if (m_proc || m_sock) m_pending.append(payload);
}

void IsolatedMediaPlayer::onReadyRead()
{
    m_buf.append(m_sock->readAll());
    ipc::drainFrames(m_buf, [this](ipc::Kind kind, const QByteArray &payload) {
        if (kind == ipc::Kind::Hello) {
            m_ready = true;
            for (const QByteArray &p : std::as_const(m_pending)) ipc::writeFrame(m_sock, ipc::Kind::Request, p);
            m_pending.clear();
            return;
        }
        if (kind != ipc::Kind::Event) return;
        QDataStream in(payload);
        in.setVersion(ipc::kStreamVersion);
        QString name; QByteArray args;
        in >> name >> args;
        if (in.status() == QDataStream::Ok) onEvent(name, args);
    });
}

void IsolatedMediaPlayer::onEvent(const QString &name, const QByteArray &args)
{
    QDataStream in(args);
    in.setVersion(ipc::kStreamVersion);
    if (name == QLatin1String("state")) {
        media::State s; in >> s;
        if (in.status() == QDataStream::Ok) applyState(s);
    } else if (name == QLatin1String("tracks")) {
        media::Tracks t; in >> t;
        if (in.status() == QDataStream::Ok) applyTracks(t);
    } else if (name == QLatin1String("ring")) {
        qint32 gen = 0; qint64 bytes = 0; in >> gen >> bytes;
        if (in.status() == QDataStream::Ok) attachRing(gen, bytes);
    } else if (name == QLatin1String("frame")) {
        qint32 gen = 0; media::FrameHeader hdr; in >> gen >> hdr;
        if (in.status() == QDataStream::Ok) showFrame(gen, hdr);
    } else if (name == QLatin1String("sub")) {
        QString text; in >> text;
        if (in.status() == QDataStream::Ok)
            if (QVideoSink *s = sink()) s->setSubtitleText(text.left(kMaxText));
    }
}

void IsolatedMediaPlayer::applyState(const media::State &in)
{
    media::State s = in;
    s.playbackState = std::clamp(s.playbackState, 0, int(QMediaPlayer::PausedState));
    s.mediaStatus = std::clamp(s.mediaStatus, 0, int(QMediaPlayer::InvalidMedia));
    s.error = std::clamp(s.error, 0, int(QMediaPlayer::AccessDeniedError));
    s.errorString = s.errorString.left(kMaxText);
    s.duration = std::max<qint64>(0, s.duration);
    s.position = std::max<qint64>(0, s.position);
    if (!(s.playbackRate > 0.0 && s.playbackRate <= 16.0)) s.playbackRate = 1.0;

    const media::State old = m_state;
    m_state = s;
    if (old.playbackState != s.playbackState) emit playbackStateChanged();
    if (old.mediaStatus != s.mediaStatus) emit mediaStatusChanged();
    if (old.error != s.error || old.errorString != s.errorString) {
        emit errorChanged();
        if (s.error != QMediaPlayer::NoError) emit errorOccurred(s.error, s.errorString);
    }
    if (old.duration != s.duration) emit durationChanged();
    if (old.position != s.position) emit positionChanged(s.position);
    if (old.seekable != s.seekable) emit seekableChanged();
    if (old.hasVideo != s.hasVideo) emit hasVideoChanged();
    if (!qFuzzyCompare(old.playbackRate, s.playbackRate)) emit playbackRateChanged();
}

void IsolatedMediaPlayer::applyTracks(const media::Tracks &t)
{
    m_audioTracks = toMetaData(t.audio);
    m_subtitleTracks = toMetaData(t.subtitles);
    emit tracksChanged();
    const int audio = t.activeAudio < m_audioTracks.size() ? std::max(-1, int(t.activeAudio)) : -1;
    const int subs = t.activeSubtitle < m_subtitleTracks.size() ? std::max(-1, int(t.activeSubtitle)) : -1;
    if (audio != m_activeAudio || subs != m_activeSubtitle) {
        m_activeAudio = audio;
        m_activeSubtitle = subs;
        emit activeTracksChanged();
    }
}

void IsolatedMediaPlayer::attachRing(qint32 generation, qint64 slotBytes)
{
    m_ring.reset();
    m_slotBytes = 0;
    m_generation = generation;
    if (slotBytes <= 0 || slotBytes > qint64(media::kMaxDimension) * media::kMaxDimension * 8) return;
    auto ring = SharedSegment::openReadOnly(SharedSegment::nameFor(
        m_serverName + QStringLiteral("-ring-") + QString::number(generation)));
    if (!ring) {
        qWarning() << "[media] could not open the frame ring";
        return;
    }
    if (ring->size() < slotBytes * media::kSlots) return;   // the child lied about its own segment
    m_ring = std::move(ring);
    m_slotBytes = slotBytes;
}

void IsolatedMediaPlayer::showFrame(qint32 generation, const media::FrameHeader &hdr)
{
    const bool usable = m_ring && generation == m_generation
                        && hdr.slot >= 0 && hdr.slot < media::kSlots;
    QVideoFrame frame;
    if (usable) {
        const uchar *base = m_ring->constData() + qint64(hdr.slot) * m_slotBytes;
        frame = media::unpack(hdr, base, m_slotBytes);
    }
    send(QStringLiteral("release"), media::encode(generation, hdr.slot));
    if (frame.isValid())
        if (QVideoSink *s = sink()) s->setVideoFrame(frame);
}

void IsolatedMediaPlayer::onChildLost()
{
    teardown();
    media::State s = m_state;
    s.playbackState = QMediaPlayer::StoppedState;
    s.mediaStatus = QMediaPlayer::InvalidMedia;
    s.error = QMediaPlayer::ResourceError;
    s.errorString = QStringLiteral("decoder stopped");
    applyState(s);
    emit decoderCrashed();
}

void IsolatedMediaPlayer::teardown()
{
    m_tearingDown = true;
    if (m_connectTimer) m_connectTimer->stop();
    if (m_sock) { m_sock->abort(); m_sock->deleteLater(); m_sock = nullptr; }
    if (m_proc) {
        m_proc->kill();
        m_proc->waitForFinished(500);
        m_proc->deleteLater();
        m_proc = nullptr;
    }
    m_ring.reset();
    m_slotBytes = 0;
    m_ready = false;
    m_pending.clear();
    m_buf.clear();
    m_tearingDown = false;
}
