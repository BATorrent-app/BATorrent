// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "ipc/mediahost.h"
#include "ipc/ipcprotocol.h"
#include "ipc/mediaframe.h"
#include "ipc/mediasandbox.h"
#include "ipc/remotesource.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QLocalSocket>
#include <QLocale>
#include <QMediaMetaData>
#include <QSet>
#include <QTimer>
#include <QUrl>

#ifdef Q_OS_MACOS
#  include <mach-o/dyld.h>
#endif

namespace {

QList<media::Track> tracksOf(const QList<QMediaMetaData> &list)
{
    QList<media::Track> out;
    for (const QMediaMetaData &m : list)
        out.append({ qint32(m.value(QMediaMetaData::Language).value<QLocale::Language>()),
                     m.stringValue(QMediaMetaData::Title) });
    return out;
}

// Where the decoder's code lives: plugins and codecs load after the sandbox is
// up, so the trees of everything already mapped stay readable, and nothing else.
QStringList codeTrees()
{
    QSet<QString> trees = { QStringLiteral("/System"), QStringLiteral("/usr/lib"),
                            QStringLiteral("/usr/share"), QStringLiteral("/Library/Apple"),
                            QStringLiteral("/private/var/db/timezone") };
#ifdef Q_OS_MACOS
    for (uint32_t i = 0; i < _dyld_image_count(); ++i) {
        QString dir = QFileInfo(QString::fromUtf8(_dyld_get_image_name(i))).absolutePath();
        const int fw = dir.indexOf(QLatin1String(".framework"));
        if (fw >= 0) dir = QFileInfo(dir.left(fw)).absolutePath();
        if (!dir.isEmpty() && dir != QLatin1String("/")) trees.insert(dir);
    }
#endif
    for (const QString &p : QCoreApplication::libraryPaths()) trees.insert(QDir(p).absolutePath());
    return QStringList(trees.cbegin(), trees.cend());
}

}

MediaHost::MediaHost(const QString &serverName, bool sandboxed, QObject *parent)
    : QObject(parent), m_serverName(serverName), m_wantSandbox(sandboxed)
{
    m_player.setAudioOutput(&m_audio);
    m_player.setVideoSink(&m_sink);

    connect(&m_player, &QMediaPlayer::positionChanged, this, &MediaHost::scheduleState);
    connect(&m_player, &QMediaPlayer::durationChanged, this, &MediaHost::scheduleState);
    connect(&m_player, &QMediaPlayer::playbackStateChanged, this, &MediaHost::scheduleState);
    connect(&m_player, &QMediaPlayer::mediaStatusChanged, this, &MediaHost::scheduleState);
    connect(&m_player, &QMediaPlayer::seekableChanged, this, &MediaHost::scheduleState);
    connect(&m_player, &QMediaPlayer::hasVideoChanged, this, &MediaHost::scheduleState);
    connect(&m_player, &QMediaPlayer::playbackRateChanged, this, &MediaHost::scheduleState);
    connect(&m_player, &QMediaPlayer::errorOccurred, this, &MediaHost::scheduleState);
    connect(&m_player, &QMediaPlayer::tracksChanged, this, &MediaHost::sendTracks);
    connect(&m_player, &QMediaPlayer::activeTracksChanged, this, &MediaHost::sendTracks);
    connect(&m_sink, &QVideoSink::videoFrameChanged, this, &MediaHost::onFrame);
    connect(&m_sink, &QVideoSink::subtitleTextChanged, this, [this](const QString &text) {
        sendEvent(QStringLiteral("sub"), media::encode(text));
    });
}

MediaHost::~MediaHost()
{
    closeSource();
}

// Connect first: once confined the child can open nothing new, socket included.
bool MediaHost::start(int timeoutMs)
{
    m_sock = new QLocalSocket(this);
    m_sock->connectToServer(m_serverName);
    if (!m_sock->waitForConnected(timeoutMs)) {
        qWarning() << "[media] cannot reach the UI:" << m_sock->errorString();
        return false;
    }
    connect(m_sock, &QLocalSocket::readyRead, this, &MediaHost::onReadyRead);
    // The UI is the only reason to exist: an orphaned decoder would keep
    // playing audio with no window.
    connect(m_sock, &QLocalSocket::disconnected, qApp, &QCoreApplication::quit);

    if (m_wantSandbox && MediaSandbox::supported()) {
        MediaSandbox::Grant grant;
        grant.readTrees = codeTrees();
        QString error;
        if (!MediaSandbox::enter(grant, &error)) {
            qWarning() << "[media] sandbox:" << error;
            refuse(QStringLiteral("sandbox unavailable"));
            m_sock->flush();
            return false;
        }
        qInfo() << "[media] sandboxed";
    }
    ipc::writeFrame(m_sock, ipc::Kind::Hello, {});
    if (m_sock->bytesAvailable() > 0) onReadyRead();
    return true;
}

void MediaHost::onReadyRead()
{
    m_buf.append(m_sock->readAll());
    ipc::drainFrames(m_buf, [this](ipc::Kind kind, const QByteArray &payload) {
        if (kind == ipc::Kind::Ping) { ipc::writeFrame(m_sock, ipc::Kind::Pong, {}); return; }
        if (kind != ipc::Kind::Request) return;
        QDataStream in(payload);
        in.setVersion(ipc::kStreamVersion);
        quint32 id = 0; QString method; QByteArray args;
        in >> id >> method >> args;
        if (in.status() == QDataStream::Ok) dispatch(method, args);
    });
}

void MediaHost::dispatch(const QString &method, const QByteArray &args)
{
    QDataStream in(args);
    in.setVersion(ipc::kStreamVersion);
    if (method == QLatin1String("open")) {
        qint32 gen = 0; qint64 size = 0; QString hint;
        in >> gen >> size >> hint;
        if (in.status() == QDataStream::Ok) openSource(gen, size, hint);
    } else if (method == QLatin1String("close")) {
        closeSource();
    } else if (method == QLatin1String("data")) {
        qint32 gen = 0; quint32 id = 0; qint64 offset = 0; QByteArray bytes;
        in >> gen >> id >> offset >> bytes;
        if (in.status() == QDataStream::Ok && gen == m_sourceGen && m_source)
            m_source->deliver(id, offset, bytes);
    } else if (method == QLatin1String("dataError")) {
        qint32 gen = 0; quint32 id = 0;
        in >> gen >> id;
        if (gen == m_sourceGen && m_source) m_source->fail(id);
    } else if (method == QLatin1String("ring")) {
        qint32 gen = 0; qint64 bytes = 0; QString token;
        in >> gen >> bytes >> token;
        if (in.status() == QDataStream::Ok) attachRing(gen, bytes, token);
    } else if (method == QLatin1String("play")) {
        m_player.play();
    } else if (method == QLatin1String("pause")) {
        m_player.pause();
    } else if (method == QLatin1String("stop")) {
        m_player.stop();
    } else if (method == QLatin1String("seek")) {
        qint64 ms = 0; in >> ms; m_player.setPosition(ms);
    } else if (method == QLatin1String("rate")) {
        double r = 1.0; in >> r; m_player.setPlaybackRate(r);
    } else if (method == QLatin1String("volume")) {
        float v = 1.0f; in >> v; m_audio.setVolume(v);
    } else if (method == QLatin1String("muted")) {
        bool m = false; in >> m; m_audio.setMuted(m);
    } else if (method == QLatin1String("audioTrack")) {
        qint32 t = -1; in >> t; m_player.setActiveAudioTrack(t);
    } else if (method == QLatin1String("subTrack")) {
        qint32 t = -1; in >> t; m_player.setActiveSubtitleTrack(t);
    } else if (method == QLatin1String("release")) {
        qint32 gen = -1, slot = -1; in >> gen >> slot;
        if (gen == m_generation && slot >= 0 && slot < media::kSlots) m_busy[size_t(slot)] = false;
    }
}

void MediaHost::openSource(qint32 generation, qint64 size, const QString &hint)
{
    closeSource();
    if (size <= 0) { refuse(QStringLiteral("empty source")); return; }
    m_sourceGen = generation;
    auto *src = new RemoteSource(size, 120000, this);
    connect(src, &RemoteSource::readRequested, this, [this, generation](quint32 id, qint64 offset, qint32 length) {
        sendEvent(QStringLiteral("read"), media::encode(generation, id, offset, length));
    }, Qt::QueuedConnection);
    m_source = src;
    // The hint only names the container; the bytes come from RemoteSource.
    m_player.setSourceDevice(src, QUrl(QStringLiteral("media.") + QFileInfo(hint).suffix()));
}

// Unblock a demuxer waiting on bytes before the player joins its thread.
void MediaHost::closeSource()
{
    if (!m_source) return;
    RemoteSource *old = m_source;
    old->abort();
    m_player.setSource(QUrl());
    m_source = nullptr;
    old->deleteLater();
}

void MediaHost::attachRing(qint32 generation, qint64 slotBytes, const QString &token)
{
    auto ring = SharedSegment::openWritable(token);
    if (!ring || slotBytes <= 0 || ring->size() < slotBytes * media::kSlots) {
        qWarning() << "[media] could not map the frame ring";
        return;
    }
    m_ring = std::move(ring);
    m_slotBytes = slotBytes;
    m_generation = generation;
    m_busy.fill(false);
    sendEvent(QStringLiteral("ringReady"), media::encode(generation));
}

void MediaHost::onFrame(const QVideoFrame &frame)
{
    if (!m_sock || !frame.isValid()) return;
    QVideoFrame f = frame;
    if (!f.map(QVideoFrame::ReadOnly)) return;
    const qint64 need = media::packedSize(f);
    if (!m_ring || need > m_slotBytes) {
        // Ask once per size; frames are dropped until the UI shares a ring.
        if (need > m_ringAsked) {
            m_ringAsked = need;
            sendEvent(QStringLiteral("needRing"), media::encode(need));
        }
        f.unmap();
        return;
    }
    int slot = -1;
    for (int i = 0; i < media::kSlots; ++i)
        if (!m_busy[size_t(i)]) { slot = i; break; }
    if (slot < 0) { f.unmap(); return; }   // UI is behind: the next frame supersedes this one

    media::FrameHeader hdr;
    uchar *base = m_ring->data() + qint64(slot) * m_slotBytes;
    const bool packed = media::pack(f, base, m_slotBytes, hdr);
    f.unmap();
    if (!packed) return;
    hdr.slot = slot;
    m_busy[size_t(slot)] = true;
    sendEvent(QStringLiteral("frame"), media::encode(m_generation, hdr));
}

void MediaHost::scheduleState()
{
    if (m_stateQueued) return;
    m_stateQueued = true;
    QTimer::singleShot(0, this, &MediaHost::sendState);
}

void MediaHost::sendState()
{
    m_stateQueued = false;
    media::State s;
    s.playbackState = qint32(m_player.playbackState());
    s.mediaStatus = qint32(m_player.mediaStatus());
    s.error = qint32(m_player.error());
    s.errorString = m_player.errorString();
    s.duration = m_player.duration();
    s.position = m_player.position();
    s.seekable = m_player.isSeekable();
    s.hasVideo = m_player.hasVideo();
    s.playbackRate = m_player.playbackRate();
    sendEvent(QStringLiteral("state"), media::encode(s));
}

void MediaHost::sendTracks()
{
    media::Tracks t;
    t.audio = tracksOf(m_player.audioTracks());
    t.subtitles = tracksOf(m_player.subtitleTracks());
    t.activeAudio = m_player.activeAudioTrack();
    t.activeSubtitle = m_player.activeSubtitleTrack();
    sendEvent(QStringLiteral("tracks"), media::encode(t));
}

void MediaHost::refuse(const QString &why)
{
    qWarning() << "[media] refused:" << why;
    media::State s;
    s.mediaStatus = qint32(QMediaPlayer::InvalidMedia);
    s.error = qint32(QMediaPlayer::AccessDeniedError);
    s.errorString = why;
    sendEvent(QStringLiteral("state"), media::encode(s));
}

void MediaHost::sendEvent(const QString &name, const QByteArray &args)
{
    if (!m_sock) return;
    ipc::writeFrame(m_sock, ipc::Kind::Event, media::encode(name, args));
}
