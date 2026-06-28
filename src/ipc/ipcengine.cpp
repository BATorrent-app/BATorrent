// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "ipcengine.h"
#include <QLocalSocket>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QDataStream>
#include <QDebug>

IpcEngine::IpcEngine(const QString &exePath, QObject *parent)
    : IEngine(parent), m_exePath(exePath)
{
    m_serverName = QStringLiteral("batorrent-engine-%1").arg(QCoreApplication::applicationPid());
}

IpcEngine::~IpcEngine()
{
    m_shuttingDown = true;
    if (m_proc) { m_proc->terminate(); m_proc->waitForFinished(2000); }
}

void IpcEngine::spawnEngine()
{
    m_proc = new QProcess(this);
    m_proc->setProcessChannelMode(QProcess::ForwardedChannels);   // engine logs to our console
    connect(m_proc, &QProcess::finished, this, &IpcEngine::onProcFinished);
    m_proc->start(m_exePath, { QStringLiteral("--engine"), m_serverName });
}

bool IpcEngine::start()
{
    spawnEngine();
    m_sock = new QLocalSocket(this);
    connect(m_sock, &QLocalSocket::readyRead, this, &IpcEngine::onSocketReadyRead);
    // the server needs a moment to come up — retry the connect briefly
    QElapsedTimer t; t.start();
    while (t.elapsed() < 5000) {
        m_sock->connectToServer(m_serverName);
        if (m_sock->waitForConnected(100)) break;
        m_sock->abort();
    }
    const bool ok = connected();
    if (!ok) { qWarning() << "[ipc] could not connect to engine" << m_serverName; return false; }

    emit engineStatusChanged(true);
    // Pull the engine's initial snapshot synchronously so the first paint has
    // state (and torrentCount()/torrentAt() are valid right after start()).
    QElapsedTimer s; s.start();
    while (!m_gotSnapshot && connected() && s.elapsed() < 2000) {
        if (!m_sock->waitForReadyRead(100)) continue;
        m_buf.append(m_sock->readAll());
        ipc::drainFrames(m_buf, [this](ipc::Kind k, const QByteArray &p) {
            const_cast<IpcEngine *>(this)->handleFrame(k, p);
        });
    }
    return true;
}

bool IpcEngine::connected() const
{
    return m_sock && m_sock->state() == QLocalSocket::ConnectedState;
}

void IpcEngine::onProcFinished(int exitCode, QProcess::ExitStatus status)
{
    if (m_shuttingDown) return;
    qWarning() << "[ipc] engine exited code" << exitCode << "status" << status << "— respawning";
    emit engineStatusChanged(false);
    if (m_sock) m_sock->abort();
    m_proc->deleteLater();
    m_gotSnapshot = false;
    start();   // respawn + reconnect; torrents reload from resume data engine-side
}

void IpcEngine::onSocketReadyRead()
{
    if (!m_sock) return;
    m_buf.append(m_sock->readAll());
    ipc::drainFrames(m_buf, [this](ipc::Kind k, const QByteArray &p) { handleFrame(k, p); });
}

void IpcEngine::handleFrame(ipc::Kind kind, const QByteArray &payload)
{
    switch (kind) {
    case ipc::Kind::Hello:
        emit engineStatusChanged(true);
        break;
    case ipc::Kind::Event: {
        QDataStream in(payload); in.setVersion(ipc::kStreamVersion);
        QString name; QByteArray args; in >> name >> args;
        dispatchEvent(name, args);
        break;
    }
    case ipc::Kind::Reply: {
        QDataStream in(payload); in.setVersion(ipc::kStreamVersion);
        quint32 rid = 0; QByteArray res; in >> rid >> res;
        m_replies.insert(rid, res);
        break;
    }
    default:
        break;
    }
}

void IpcEngine::dispatchEvent(const QString &name, const QByteArray &args)
{
    QDataStream in(args); in.setVersion(ipc::kStreamVersion);
    if (name == QLatin1String("snapshot")) {
        in >> m_snap;
        m_gotSnapshot = true;
        emit torrentsUpdated();
    } else if (name == QLatin1String("torrentFinished")) {
        QString n, h; in >> n >> h; emit torrentFinished(n, h);
    } else if (name == QLatin1String("extractionCompleted")) {
        QString h; bool ok; in >> h >> ok; emit extractionCompleted(h, ok);
    } else if (name == QLatin1String("altSpeedsActiveChanged")) {
        bool a; in >> a; m_snap.altSpeedsActive = a; emit altSpeedsActiveChanged(a);
    } else if (name == QLatin1String("portStatusChanged")) {
        qint32 st; in >> st; m_snap.portStatus = st; emit portStatusChanged(st);
    }
}

// Blocking request→reply. Events arriving mid-wait are still dispatched (so a
// snapshot pushed while we block isn't lost); only the matching Reply ends it.
QByteArray IpcEngine::request(const QString &method, const QByteArray &args) const
{
    if (!connected()) return {};
    const quint32 id = m_nextId++;
    QByteArray payload;
    QDataStream out(&payload, QIODevice::WriteOnly);
    out.setVersion(ipc::kStreamVersion);
    out << id << method << args;
    ipc::writeFrame(m_sock, ipc::Kind::Request, payload);
    m_sock->flush();

    QElapsedTimer t; t.start();
    while (connected() && t.elapsed() < 5000) {
        if (m_replies.contains(id)) break;
        if (!m_sock->waitForReadyRead(200)) continue;
        m_buf.append(m_sock->readAll());
        ipc::drainFrames(m_buf, [this](ipc::Kind k, const QByteArray &p) {
            const_cast<IpcEngine *>(this)->handleFrame(k, p);
        });
    }
    return m_replies.take(id);   // empty if timed out
}

QByteArray IpcEngine::packIndex(int i)
{
    QByteArray a; QDataStream o(&a, QIODevice::WriteOnly); o.setVersion(ipc::kStreamVersion);
    o << qint32(i); return a;
}

// ---- hot-path reads served from the pushed snapshot (no round-trip) ----
int IpcEngine::torrentCount() const { return int(m_snap.rows.size()); }
TorrentInfo IpcEngine::torrentAt(int i) const
{ return (i >= 0 && i < m_snap.rows.size()) ? m_snap.rows.at(i) : TorrentInfo{}; }
QString IpcEngine::torrentHashAt(int i) const
{ return (i >= 0 && i < m_snap.hashes.size()) ? m_snap.hashes.at(i) : QString{}; }
int IpcEngine::torrentIndexByInfoHash(const QString &h) const { return int(m_snap.hashes.indexOf(h)); }
qint64 IpcEngine::globalUploaded() const { return m_snap.globalUploaded; }
qint64 IpcEngine::globalDownloaded() const { return m_snap.globalDownloaded; }
float IpcEngine::globalRatio() const { return m_snap.globalRatio; }
qint64 IpcEngine::sessionUploaded() const { return m_snap.sessionUploaded; }
qint64 IpcEngine::sessionDownloaded() const { return m_snap.sessionDownloaded; }
int IpcEngine::totalTorrentsAdded() const { return m_snap.totalTorrentsAdded; }
int IpcEngine::portStatus() const { return m_snap.portStatus; }
int IpcEngine::listenPort() const { return m_snap.listenPort; }
bool IpcEngine::dhtEnabled() const { return m_snap.dhtEnabled; }
bool IpcEngine::altSpeedsActive() const { return m_snap.altSpeedsActive; }
DetailedStats IpcEngine::detailedStats() const { return m_snap.detailed; }

// ---- fire-and-forget mutations ----
void IpcEngine::pauseAll()  { call(QStringLiteral("pauseAll")); }
void IpcEngine::resumeAll() { call(QStringLiteral("resumeAll")); }
void IpcEngine::pauseTorrent(int i)  { call(QStringLiteral("pauseTorrent"), packIndex(i)); }
void IpcEngine::resumeTorrent(int i) { call(QStringLiteral("resumeTorrent"), packIndex(i)); }
void IpcEngine::unmarkCompleted(int i) { call(QStringLiteral("unmarkCompleted"), packIndex(i)); }
void IpcEngine::markCompleted(int i)   { call(QStringLiteral("markCompleted"), packIndex(i)); }
void IpcEngine::stopSeedingTorrent(int i) { call(QStringLiteral("stopSeedingTorrent"), packIndex(i)); }
void IpcEngine::forceRecheck(int i)    { call(QStringLiteral("forceRecheck"), packIndex(i)); }
void IpcEngine::forceReannounce(int i) { call(QStringLiteral("forceReannounce"), packIndex(i)); }
void IpcEngine::saveResumeData() { call(QStringLiteral("saveResumeData")); }
void IpcEngine::clearRemovedHistory() { call(QStringLiteral("clearRemovedHistory")); }
void IpcEngine::setAltSpeedsActive(bool a)
{ QByteArray d; QDataStream o(&d, QIODevice::WriteOnly); o.setVersion(ipc::kStreamVersion); o << a; call(QStringLiteral("setAltSpeedsActive"), d); }

void IpcEngine::removeTorrent(int i, bool del, bool perm)
{ QByteArray d; QDataStream o(&d, QIODevice::WriteOnly); o.setVersion(ipc::kStreamVersion); o << qint32(i) << del << perm; call(QStringLiteral("removeTorrent"), d); }
void IpcEngine::setFilePriority(int ti, int fi, int pr)
{ QByteArray d; QDataStream o(&d, QIODevice::WriteOnly); o.setVersion(ipc::kStreamVersion); o << qint32(ti) << qint32(fi) << qint32(pr); call(QStringLiteral("setFilePriority"), d); }
void IpcEngine::setTorrentQueuePosition(int i, int p)
{ QByteArray d; QDataStream o(&d, QIODevice::WriteOnly); o.setVersion(ipc::kStreamVersion); o << qint32(i) << qint32(p); call(QStringLiteral("setTorrentQueuePosition"), d); }
void IpcEngine::setSequentialDownload(int i, bool seq)
{ QByteArray d; QDataStream o(&d, QIODevice::WriteOnly); o.setVersion(ipc::kStreamVersion); o << qint32(i) << seq; call(QStringLiteral("setSequentialDownload"), d); }
void IpcEngine::prioritizeFilePieceBoundaries(int ti, int fi)
{ QByteArray d; QDataStream o(&d, QIODevice::WriteOnly); o.setVersion(ipc::kStreamVersion); o << qint32(ti) << qint32(fi); call(QStringLiteral("prioritizeFilePieceBoundaries"), d); }
void IpcEngine::addTorrent(const QString &fp, const QString &sp)
{ QByteArray d; QDataStream o(&d, QIODevice::WriteOnly); o.setVersion(ipc::kStreamVersion); o << fp << sp; call(QStringLiteral("addTorrent"), d); }
void IpcEngine::addTorrentWithPriorities(const QString &fp, const QString &sp, const std::vector<int> &pr)
{ QByteArray d; QDataStream o(&d, QIODevice::WriteOnly); o.setVersion(ipc::kStreamVersion);
  o << fp << sp << qint32(pr.size()); for (int v : pr) o << qint32(v); call(QStringLiteral("addTorrentWithPriorities"), d); }
void IpcEngine::addMagnet(const QString &uri, const QString &sp, const QString &hint, int ct)
{ QByteArray d; QDataStream o(&d, QIODevice::WriteOnly); o.setVersion(ipc::kStreamVersion); o << uri << sp << hint << qint32(ct); call(QStringLiteral("addMagnet"), d); }
void IpcEngine::renameFile(int ti, int fi, const QString &p)
{ QByteArray d; QDataStream o(&d, QIODevice::WriteOnly); o.setVersion(ipc::kStreamVersion); o << qint32(ti) << qint32(fi) << p; call(QStringLiteral("renameFile"), d); }
void IpcEngine::extractTorrent(int i, const QString &pw)
{ QByteArray d; QDataStream o(&d, QIODevice::WriteOnly); o.setVersion(ipc::kStreamVersion); o << qint32(i) << pw; call(QStringLiteral("extractTorrent"), d); }
void IpcEngine::setTorrentUploadLimit(int i, int k)
{ QByteArray d; QDataStream o(&d, QIODevice::WriteOnly); o.setVersion(ipc::kStreamVersion); o << qint32(i) << qint32(k); call(QStringLiteral("setTorrentUploadLimit"), d); }
void IpcEngine::setTorrentDownloadLimit(int i, int k)
{ QByteArray d; QDataStream o(&d, QIODevice::WriteOnly); o.setVersion(ipc::kStreamVersion); o << qint32(i) << qint32(k); call(QStringLiteral("setTorrentDownloadLimit"), d); }
void IpcEngine::setTorrentTags(int i, const QStringList &t)
{ QByteArray d; QDataStream o(&d, QIODevice::WriteOnly); o.setVersion(ipc::kStreamVersion); o << qint32(i) << t; call(QStringLiteral("setTorrentTags"), d); }
void IpcEngine::setTorrentStopAfterDownload(int i, int v)
{ QByteArray d; QDataStream o(&d, QIODevice::WriteOnly); o.setVersion(ipc::kStreamVersion); o << qint32(i) << qint32(v); call(QStringLiteral("setTorrentStopAfterDownload"), d); }
void IpcEngine::setTorrentMaxSeedSeconds(int i, qint64 s)
{ QByteArray d; QDataStream o(&d, QIODevice::WriteOnly); o.setVersion(ipc::kStreamVersion); o << qint32(i) << s; call(QStringLiteral("setTorrentMaxSeedSeconds"), d); }
void IpcEngine::setTorrentCategory(int i, const QString &c)
{ QByteArray d; QDataStream o(&d, QIODevice::WriteOnly); o.setVersion(ipc::kStreamVersion); o << qint32(i) << c; call(QStringLiteral("setTorrentCategory"), d); }
void IpcEngine::setSuperSeeding(int i, bool on)
{ QByteArray d; QDataStream o(&d, QIODevice::WriteOnly); o.setVersion(ipc::kStreamVersion); o << qint32(i) << on; call(QStringLiteral("setSuperSeeding"), d); }
void IpcEngine::setForceStart(int i, bool on)
{ QByteArray d; QDataStream o(&d, QIODevice::WriteOnly); o.setVersion(ipc::kStreamVersion); o << qint32(i) << on; call(QStringLiteral("setForceStart"), d); }
void IpcEngine::moveStorage(int ti, const QString &p)
{ QByteArray d; QDataStream o(&d, QIODevice::WriteOnly); o.setVersion(ipc::kStreamVersion); o << qint32(ti) << p; call(QStringLiteral("moveStorage"), d); }
void IpcEngine::addTracker(int i, const QString &u)
{ QByteArray d; QDataStream o(&d, QIODevice::WriteOnly); o.setVersion(ipc::kStreamVersion); o << qint32(i) << u; call(QStringLiteral("addTracker"), d); }
void IpcEngine::replaceTrackers(int ti, const QStringList &u)
{ QByteArray d; QDataStream o(&d, QIODevice::WriteOnly); o.setVersion(ipc::kStreamVersion); o << qint32(ti) << u; call(QStringLiteral("replaceTrackers"), d); }

// ---- blocking detail/occasional queries ----
std::vector<FileInfo> IpcEngine::filesAt(int i) const
{ QDataStream in(request(QStringLiteral("filesAt"), packIndex(i))); in.setVersion(ipc::kStreamVersion);
  std::vector<FileInfo> v; readVec(in, v); return v; }
std::vector<TrackerInfo> IpcEngine::trackersAt(int i) const
{ QDataStream in(request(QStringLiteral("trackersAt"), packIndex(i))); in.setVersion(ipc::kStreamVersion);
  std::vector<TrackerInfo> v; readVec(in, v); return v; }
std::vector<PeerInfo> IpcEngine::peersAt(int i, int maxPeers) const
{ QByteArray d; QDataStream o(&d, QIODevice::WriteOnly); o.setVersion(ipc::kStreamVersion); o << qint32(i) << qint32(maxPeers);
  QDataStream in(request(QStringLiteral("peersAt"), d)); in.setVersion(ipc::kStreamVersion);
  std::vector<PeerInfo> v; readVec(in, v); return v; }
std::vector<bool> IpcEngine::piecesAt(int i) const
{ QDataStream in(request(QStringLiteral("piecesAt"), packIndex(i))); in.setVersion(ipc::kStreamVersion);
  std::vector<bool> v; readBoolVec(in, v); return v; }
QString IpcEngine::torrentRootPath(int i) const
{ QDataStream in(request(QStringLiteral("torrentRootPath"), packIndex(i))); in.setVersion(ipc::kStreamVersion); QString s; in >> s; return s; }
bool IpcEngine::torrentHasVideo(int i) const
{ QDataStream in(request(QStringLiteral("torrentHasVideo"), packIndex(i))); in.setVersion(ipc::kStreamVersion); bool b = false; in >> b; return b; }
bool IpcEngine::torrentHasArchives(int i) const
{ QDataStream in(request(QStringLiteral("torrentHasArchives"), packIndex(i))); in.setVersion(ipc::kStreamVersion); bool b = false; in >> b; return b; }
int IpcEngine::torrentDownloadLimit(int i) const
{ QDataStream in(request(QStringLiteral("torrentDownloadLimit"), packIndex(i))); in.setVersion(ipc::kStreamVersion); qint32 v = 0; in >> v; return v; }
int IpcEngine::torrentUploadLimit(int i) const
{ QDataStream in(request(QStringLiteral("torrentUploadLimit"), packIndex(i))); in.setVersion(ipc::kStreamVersion); qint32 v = 0; in >> v; return v; }
QStringList IpcEngine::torrentTags(int i) const
{ QDataStream in(request(QStringLiteral("torrentTags"), packIndex(i))); in.setVersion(ipc::kStreamVersion); QStringList t; in >> t; return t; }
int IpcEngine::torrentStopAfterDownload(int i) const
{ QDataStream in(request(QStringLiteral("torrentStopAfterDownload"), packIndex(i))); in.setVersion(ipc::kStreamVersion); qint32 v = 0; in >> v; return v; }
qint64 IpcEngine::torrentMaxSeedSeconds(int i) const
{ QDataStream in(request(QStringLiteral("torrentMaxSeedSeconds"), packIndex(i))); in.setVersion(ipc::kStreamVersion); qint64 v = 0; in >> v; return v; }
QString IpcEngine::torrentMagnetUri(int i) const
{ QDataStream in(request(QStringLiteral("torrentMagnetUri"), packIndex(i))); in.setVersion(ipc::kStreamVersion); QString s; in >> s; return s; }
QStringList IpcEngine::torrentFileNames(int i) const
{ QDataStream in(request(QStringLiteral("torrentFileNames"), packIndex(i))); in.setVersion(ipc::kStreamVersion); QStringList t; in >> t; return t; }
QString IpcEngine::streamFilePath(int ti, int fi) const
{ QByteArray d; QDataStream o(&d, QIODevice::WriteOnly); o.setVersion(ipc::kStreamVersion); o << qint32(ti) << qint32(fi);
  QDataStream in(request(QStringLiteral("streamFilePath"), d)); in.setVersion(ipc::kStreamVersion); QString s; in >> s; return s; }
QVariantMap IpcEngine::streamFileStats(int ti, int fi) const
{ QByteArray d; QDataStream o(&d, QIODevice::WriteOnly); o.setVersion(ipc::kStreamVersion); o << qint32(ti) << qint32(fi);
  QDataStream in(request(QStringLiteral("streamFileStats"), d)); in.setVersion(ipc::kStreamVersion); QVariantMap m; in >> m; return m; }
QVariantMap IpcEngine::statsWrapped(int year) const
{ QDataStream in(request(QStringLiteral("statsWrapped"), packIndex(year))); in.setVersion(ipc::kStreamVersion); QVariantMap m; in >> m; return m; }
bool IpcEngine::isSuperSeeding(int i) const
{ QDataStream in(request(QStringLiteral("isSuperSeeding"), packIndex(i))); in.setVersion(ipc::kStreamVersion); bool b = false; in >> b; return b; }
bool IpcEngine::isSequentialDownload(int i) const
{ QDataStream in(request(QStringLiteral("isSequentialDownload"), packIndex(i))); in.setVersion(ipc::kStreamVersion); bool b = false; in >> b; return b; }
bool IpcEngine::isForceStart(int i) const
{ QDataStream in(request(QStringLiteral("isForceStart"), packIndex(i))); in.setVersion(ipc::kStreamVersion); bool b = false; in >> b; return b; }
bool IpcEngine::restoreRemoved(const QString &hash)
{ QByteArray d; QDataStream o(&d, QIODevice::WriteOnly); o.setVersion(ipc::kStreamVersion); o << hash;
  QDataStream in(request(QStringLiteral("restoreRemoved"), d)); in.setVersion(ipc::kStreamVersion); bool b = false; in >> b; return b; }
QList<RemovedEntry> IpcEngine::recentlyRemoved() const
{ QDataStream in(request(QStringLiteral("recentlyRemoved"))); in.setVersion(ipc::kStreamVersion); QList<RemovedEntry> l; in >> l; return l; }
int IpcEngine::importFromQBittorrent(const QString &p)
{ QByteArray d; QDataStream o(&d, QIODevice::WriteOnly); o.setVersion(ipc::kStreamVersion); o << p;
  QDataStream in(request(QStringLiteral("importFromQBittorrent"), d)); in.setVersion(ipc::kStreamVersion); qint32 v = 0; in >> v; return v; }
bool IpcEngine::exportTorrent(int i, const QString &dest)
{ QByteArray d; QDataStream o(&d, QIODevice::WriteOnly); o.setVersion(ipc::kStreamVersion); o << qint32(i) << dest;
  QDataStream in(request(QStringLiteral("exportTorrent"), d)); in.setVersion(ipc::kStreamVersion); bool b = false; in >> b; return b; }
QStringList IpcEngine::categories() const
{ QDataStream in(request(QStringLiteral("categories"))); in.setVersion(ipc::kStreamVersion); QStringList t; in >> t; return t; }
QMap<QString, QString> IpcEngine::allCategorySavePaths() const
{ QDataStream in(request(QStringLiteral("allCategorySavePaths"))); in.setVersion(ipc::kStreamVersion); QMap<QString, QString> m; in >> m; return m; }
