// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "bridges/isolatedmediaplayer.h"
#include "ipc/ipcprotocol.h"
#include "ipc/mediachannel.h"
#include "ipc/mediafeed.h"
#include "ipc/mediaframe.h"

#include <QCoreApplication>
#include <QDebug>
#include <QLocalSocket>
#include <QLocale>
#include <QMediaPlayer>
#include <QProcess>
#include <QUuid>
#include <QVideoSink>
#include <algorithm>
#include <utility>

#ifdef Q_OS_WIN
#  include "ipc/appcontainer_win.h"
#else
class ContainedLaunch {};
#endif

namespace {

constexpr int kMaxTracks = 64;
constexpr int kMaxText = 4096;
constexpr qint64 kMaxSlotBytes = 64LL * 1024 * 1024;   // a 4K 16-bit frame is ~25 MB

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

bool sandboxWanted() { return !qEnvironmentVariableIsSet("BAT_MEDIA_NO_SANDBOX"); }

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
    if (ensureChild()) startFeed();
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
    send(QStringLiteral("volume"), media::encode(m_volume));
}

void IsolatedMediaPlayer::setMuted(bool muted)
{
    if (muted == m_muted) return;
    m_muted = muted;
    emit mutedChanged();
    send(QStringLiteral("muted"), media::encode(m_muted));
}

void IsolatedMediaPlayer::setPosition(qint64 ms)
{
    ms = std::max<qint64>(0, ms);
    if (m_opened) send(QStringLiteral("seek"), media::encode(ms));
    else m_pendingSeek = ms;
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
    m_wantState = QMediaPlayer::PlayingState;
    if (!m_channel) {   // the decoder died: a fresh one for the same source
        if (!ensureChild()) return;
        startFeed();
    }
    if (m_opened) send(QStringLiteral("play"));
}

void IsolatedMediaPlayer::pause()
{
    m_wantState = QMediaPlayer::PausedState;
    if (m_opened) send(QStringLiteral("pause"));
}

void IsolatedMediaPlayer::stop()
{
    m_wantState = QMediaPlayer::StoppedState;
    if (m_opened) send(QStringLiteral("stop"));
}

bool IsolatedMediaPlayer::openChannel(const QString &peerSid)
{
    m_serverName = QStringLiteral("batorrent-media-%1-%2")
                       .arg(QCoreApplication::applicationPid())
                       .arg(QUuid::createUuid().toString(QUuid::Id128).left(12));
    m_channel = new MediaChannel(this);
    connect(m_channel, &MediaChannel::connected, this, &IsolatedMediaPlayer::adopt);
    if (m_channel->listen(m_serverName, peerSid)) return true;
    qWarning() << "[media] cannot open the decoder channel";
    delete m_channel;
    m_channel = nullptr;
    return false;
}

QString IsolatedMediaPlayer::listenWithoutChild()
{
    if (!m_channel && !openChannel({})) return {};
    m_childPid = QCoreApplication::applicationPid();
    return m_serverName;
}

bool IsolatedMediaPlayer::ensureChild()
{
    if (m_channel) return true;
    QString peerSid;
#ifdef Q_OS_WIN
    if (sandboxWanted()) {
        auto launch = std::make_unique<ContainedLaunch>();
        QString error;
        // Fail closed: no container, no decoder.
        if (!launch->prepare(QCoreApplication::applicationDirPath(), &error)) {
            qWarning() << "[media] sandbox:" << error;
            failWith(QMediaPlayer::AccessDeniedError, QStringLiteral("sandbox unavailable"));
            return false;
        }
        peerSid = launch->sid();
        m_launch = std::move(launch);
    }
#endif
    if (!openChannel(peerSid)) {
        failWith(QMediaPlayer::ResourceError, QStringLiteral("decoder unavailable"));
        return false;
    }
    m_proc = new QProcess(this);
    m_proc->setProcessChannelMode(QProcess::ForwardedChannels);
#ifdef Q_OS_WIN
    if (m_launch) m_proc->setCreateProcessArgumentsModifier(m_launch->modifier());
#endif
    connect(m_proc, &QProcess::finished, this, [this](int code, QProcess::ExitStatus status) {
        if (m_tearingDown) return;
        qWarning() << "[media] decoder exited" << code << (status == QProcess::CrashExit ? "(crash)" : "");
        onChildLost();
    });
    m_proc->start(QCoreApplication::applicationFilePath(), { QStringLiteral("--media"), m_serverName });
    if (!m_proc->waitForStarted(5000)) {
        qWarning() << "[media] decoder did not start:" << m_proc->errorString();
        teardown();
        failWith(QMediaPlayer::ResourceError, QStringLiteral("decoder unavailable"));
        return false;
    }
    m_childPid = m_proc->processId();
    send(QStringLiteral("volume"), media::encode(m_volume));
    send(QStringLiteral("muted"), media::encode(m_muted));
    return true;
}

void IsolatedMediaPlayer::adopt(QLocalSocket *socket)
{
    if (m_sock) { socket->deleteLater(); return; }
    m_sock = socket;
    m_sock->setParent(this);
    connect(m_sock, &QLocalSocket::readyRead, this, &IsolatedMediaPlayer::onReadyRead);
    connect(m_sock, &QLocalSocket::disconnected, this, [this] {
        if (!m_tearingDown) onChildLost();
    });
    if (m_sock->bytesAvailable() > 0) onReadyRead();
}

// Bytes for the child come from here: it can read no file and reach no port.
void IsolatedMediaPlayer::startFeed()
{
    if (m_feed) { m_feed->disconnect(this); m_feed->deleteLater(); }
    const qint32 gen = ++m_sourceGen;
    m_opened = false;
    m_feed = new MediaFeed(m_source, this);
    connect(m_feed, &MediaFeed::ready, this, [this, gen](qint64 size) {
        send(QStringLiteral("open"), media::encode(gen, size, m_source.fileName()));
        m_opened = true;
        if (m_pendingSeek >= 0) send(QStringLiteral("seek"), media::encode(std::exchange(m_pendingSeek, -1)));
        if (m_wantState == QMediaPlayer::PlayingState) send(QStringLiteral("play"));
        else if (m_wantState == QMediaPlayer::PausedState) send(QStringLiteral("pause"));
    });
    connect(m_feed, &MediaFeed::failed, this, [this](const QString &why) {
        failWith(QMediaPlayer::ResourceError, why);
    });
    connect(m_feed, &MediaFeed::data, this, [this, gen](quint32 id, qint64 offset, const QByteArray &bytes) {
        send(QStringLiteral("data"), media::encode(gen, id, offset, bytes));
    });
    connect(m_feed, &MediaFeed::refused, this, [this, gen](quint32 id) {
        send(QStringLiteral("dataError"), media::encode(gen, id));
    });
    m_feed->start();
}

void IsolatedMediaPlayer::send(const QString &method, const QByteArray &args)
{
    const QByteArray payload = media::encode(quint32(0), method, args);
    if (m_ready && m_sock) ipc::writeFrame(m_sock, ipc::Kind::Request, payload);
    else if (m_channel) m_pending.append(payload);
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
    } else if (name == QLatin1String("read")) {
        qint32 gen = 0; quint32 id = 0; qint64 offset = 0; qint32 length = 0;
        in >> gen >> id >> offset >> length;
        if (in.status() == QDataStream::Ok && gen == m_sourceGen && m_feed) m_feed->request(id, offset, length);
    } else if (name == QLatin1String("needRing")) {
        qint64 bytes = 0; in >> bytes;
        if (in.status() == QDataStream::Ok) shareRing(bytes);
    } else if (name == QLatin1String("ringReady")) {
        qint32 gen = 0; in >> gen;
        if (gen == m_generation && m_ring) m_ring->unpublish();
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

// The UI makes the ring and keeps a read-only view; the child only gets to map
// it for writing, so the sandbox never needs the right to create memory.
void IsolatedMediaPlayer::shareRing(qint64 slotBytes)
{
    if (slotBytes <= m_slotBytes || slotBytes <= 0 || slotBytes > kMaxSlotBytes) return;
    const qint64 size = slotBytes + slotBytes / 4;
    const qint32 gen = m_generation + 1;
    auto ring = SharedSegment::create(m_serverName + QStringLiteral("-ring-") + QString::number(gen),
                                      size * media::kSlots);
    if (!ring) { qWarning() << "[media] could not create the frame ring"; return; }
    const QString token = ring->shareWith(m_childPid);
    if (token.isEmpty()) { qWarning() << "[media] could not share the frame ring"; return; }
    m_ring = std::move(ring);
    m_slotBytes = size;
    m_generation = gen;
    send(QStringLiteral("ring"), media::encode(gen, size, token));
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

void IsolatedMediaPlayer::failWith(int error, const QString &why)
{
    media::State s = m_state;
    s.playbackState = QMediaPlayer::StoppedState;
    s.mediaStatus = QMediaPlayer::InvalidMedia;
    s.error = error;
    s.errorString = why;
    applyState(s);
}

void IsolatedMediaPlayer::onChildLost()
{
    teardown();
    failWith(QMediaPlayer::ResourceError, QStringLiteral("decoder stopped"));
    emit decoderCrashed();
}

void IsolatedMediaPlayer::teardown()
{
    m_tearingDown = true;
    if (m_feed) { m_feed->disconnect(this); m_feed->deleteLater(); m_feed = nullptr; }
    if (m_sock) { m_sock->disconnect(this); m_sock->abort(); m_sock->deleteLater(); m_sock = nullptr; }
    if (m_proc) {
        m_proc->kill();
        m_proc->waitForFinished(500);
        m_proc->deleteLater();
        m_proc = nullptr;
    }
    if (m_channel) { m_channel->deleteLater(); m_channel = nullptr; }
    m_launch.reset();
    m_ring.reset();
    m_slotBytes = 0;
    m_childPid = 0;
    m_opened = false;
    m_ready = false;
    m_pending.clear();
    m_buf.clear();
    m_tearingDown = false;
}
