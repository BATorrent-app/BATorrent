// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "ipc/mediahost.h"
#include "ipc/ipcprotocol.h"
#include "ipc/mediaframe.h"
#include "ipc/mediasandbox.h"

#include <QCoreApplication>
#include <QDebug>
#include <QLocalServer>
#include <QLocalSocket>
#include <QLocale>
#include <QMediaMetaData>
#include <QTimer>
#include <QUrl>
#include <QDir>
#include <QFileInfo>
#include <QSet>

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

// The child only ever plays what the UI's own stream server or the local disk
// hands it: anything else is a confused or hostile caller.
bool acceptableSource(const QUrl &url)
{
    if (url.isEmpty() || url.isLocalFile()) return true;
    return url.scheme() == QLatin1String("http") && url.host() == QLatin1String("127.0.0.1");
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
    m_player.stop();
    if (!m_ringName.isEmpty()) SharedSegment::release(m_ringName);
}

bool MediaHost::listen()
{
    m_server = new QLocalServer(this);
    m_server->setSocketOptions(QLocalServer::UserAccessOption);
    QLocalServer::removeServer(m_serverName);
    if (!m_server->listen(m_serverName)) {
        qWarning() << "[media] listen failed:" << m_server->errorString();
        return false;
    }
    connect(m_server, &QLocalServer::newConnection, this, &MediaHost::onNewConnection);
    return true;
}

void MediaHost::onNewConnection()
{
    QLocalSocket *sock = m_server->nextPendingConnection();
    if (!sock) return;
    if (m_client) { sock->disconnectFromServer(); sock->deleteLater(); return; }
    m_client = sock;
    m_server->close();
    connect(sock, &QLocalSocket::readyRead, this, &MediaHost::onReadyRead);
    // The UI is the only reason to exist: an orphaned decoder would keep
    // playing audio with no window.
    connect(sock, &QLocalSocket::disconnected, qApp, &QCoreApplication::quit);
    ipc::writeFrame(sock, ipc::Kind::Hello, {});
}

void MediaHost::onReadyRead()
{
    m_buf.append(m_client->readAll());
    ipc::drainFrames(m_buf, [this](ipc::Kind kind, const QByteArray &payload) {
        if (kind == ipc::Kind::Ping) { ipc::writeFrame(m_client, ipc::Kind::Pong, {}); return; }
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
    if (method == QLatin1String("source")) {
        QUrl url; in >> url;
        if (!acceptableSource(url)) { refuse(QStringLiteral("source not allowed")); return; }
        if (!confineFor(url)) return;
        m_player.setSource(url);
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

// Fail closed: a decoder that could not be confined never opens the file.
bool MediaHost::confineFor(const QUrl &url)
{
    if (!m_wantSandbox || url.isEmpty()) return true;
    if (m_confined) {
        if (url == m_confinedTo) return true;
        refuse(QStringLiteral("sandbox is bound to another source"));
        return false;
    }
    if (!MediaSandbox::supported()) return true;
    MediaSandbox::Grant grant;
    grant.readTrees = codeTrees();
    if (url.isLocalFile()) grant.readFile = QFileInfo(url.toLocalFile()).canonicalFilePath();
    else grant.localPort = url.port();
    QString error;
    if (!MediaSandbox::enter(grant, &error)) {
        qWarning() << "[media] sandbox:" << error;
        refuse(QStringLiteral("sandbox unavailable"));
        return false;
    }
    m_confined = true;
    m_confinedTo = url;
    qInfo() << "[media] sandboxed for" << (url.isLocalFile() ? QStringLiteral("a local file")
                                                             : QStringLiteral("port %1").arg(url.port()));
    return true;
}

void MediaHost::refuse(const QString &why)
{
    qWarning() << "[media] refused source:" << why;
    media::State s;
    s.mediaStatus = qint32(QMediaPlayer::InvalidMedia);
    s.error = qint32(QMediaPlayer::AccessDeniedError);
    s.errorString = why;
    sendEvent(QStringLiteral("state"), media::encode(s));
}

bool MediaHost::ensureRing(qint64 slotBytes)
{
    if (m_ring && slotBytes <= m_slotBytes) return true;
    const qint64 size = slotBytes + slotBytes / 4;
    const QString name = SharedSegment::nameFor(
        m_serverName + QStringLiteral("-ring-") + QString::number(m_generation + 1));
    auto ring = SharedSegment::create(name, size * media::kSlots);
    if (!ring) {
        qWarning() << "[media] could not create the frame ring";
        return false;
    }
    if (!m_ringName.isEmpty()) SharedSegment::release(m_ringName);
    m_ring = std::move(ring);
    m_ringName = name;
    m_slotBytes = size;
    ++m_generation;
    m_busy.fill(false);
    sendEvent(QStringLiteral("ring"), media::encode(m_generation, m_slotBytes));
    return true;
}

void MediaHost::onFrame(const QVideoFrame &frame)
{
    if (!m_client || !frame.isValid()) return;
    int slot = -1;
    for (int i = 0; i < media::kSlots; ++i)
        if (!m_busy[size_t(i)]) { slot = i; break; }
    if (slot < 0 && m_ring) return;   // UI is behind: drop, the next frame supersedes this one

    QVideoFrame f = frame;
    if (!f.map(QVideoFrame::ReadOnly)) return;
    media::FrameHeader hdr;
    const qint64 need = media::packedSize(f);
    if (!ensureRing(need)) { f.unmap(); return; }
    if (slot < 0) slot = 0;
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

void MediaHost::sendEvent(const QString &name, const QByteArray &args)
{
    if (!m_client) return;
    ipc::writeFrame(m_client, ipc::Kind::Event, media::encode(name, args));
}
