// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "ipcengine.h"
#include "ipcprotocol.h"
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
    if (ok) emit engineStatusChanged(true);
    else qWarning() << "[ipc] could not connect to engine" << m_serverName;
    return ok;
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
    start();   // respawn + reconnect; torrents reload from resume data engine-side
}

void IpcEngine::onSocketReadyRead()
{
    if (!m_sock) return;
    m_buf.append(m_sock->readAll());
    ipc::drainFrames(m_buf, [this](ipc::Kind kind, const QByteArray &) {
        if (kind == ipc::Kind::Hello) emit engineStatusChanged(true);
        // Reply frames here are stale (from fire-and-forget calls); events land in Phase 3.
    });
}

// Blocking request→reply. request() drains the socket itself; because it spins
// synchronously, the async readyRead slot can't race it (single thread).
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

    QByteArray result; bool got = false;
    QElapsedTimer t; t.start();
    while (!got && connected() && t.elapsed() < 5000) {
        if (!m_sock->waitForReadyRead(200)) continue;
        m_buf.append(m_sock->readAll());
        ipc::drainFrames(m_buf, [&](ipc::Kind kind, const QByteArray &p) {
            if (kind != ipc::Kind::Reply) return;
            QDataStream in(p); in.setVersion(ipc::kStreamVersion);
            quint32 rid = 0; QByteArray res; in >> rid >> res;
            if (rid == id) { result = res; got = true; }
        });
    }
    return result;
}

// ---- representative real implementations ----
int IpcEngine::torrentCount() const
{
    const QByteArray r = request(QStringLiteral("torrentCount"));
    QDataStream in(r); in.setVersion(ipc::kStreamVersion);
    qint32 n = 0; in >> n; return n;
}
void IpcEngine::pauseAll()  { call(QStringLiteral("pauseAll")); }
void IpcEngine::resumeAll() { call(QStringLiteral("resumeAll")); }
void IpcEngine::addMagnet(const QString &uri, const QString &savePath, const QString &coverHint, int coverType)
{
    QByteArray a; QDataStream o(&a, QIODevice::WriteOnly); o.setVersion(ipc::kStreamVersion);
    o << uri << savePath << coverHint << qint32(coverType);
    call(QStringLiteral("addMagnet"), a);
}

// ---- remaining surface: stubs returning defaults (filled in next) ----
TorrentInfo IpcEngine::torrentAt(int index) const { return {}; }   // TODO: IPC query
QString IpcEngine::torrentHashAt(int index) const { return {}; }   // TODO: IPC query
int IpcEngine::torrentIndexByInfoHash(const QString &infoHash) const { return {}; }   // TODO: IPC query
std::vector<FileInfo> IpcEngine::filesAt(int index) const { return {}; }   // TODO: IPC query
void IpcEngine::setFilePriority(int torrentIndex, int fileIndex, int priority) { call(QStringLiteral("setFilePriority")); }
void IpcEngine::setTorrentQueuePosition(int index, int position) { call(QStringLiteral("setTorrentQueuePosition")); }
void IpcEngine::resumeTorrent(int index) { call(QStringLiteral("resumeTorrent")); }
void IpcEngine::setSequentialDownload(int index, bool sequential) { call(QStringLiteral("setSequentialDownload")); }
void IpcEngine::prioritizeFilePieceBoundaries(int torrentIndex, int fileIndex) { call(QStringLiteral("prioritizeFilePieceBoundaries")); }
void IpcEngine::addTorrent(const QString &filePath, const QString &savePath) { call(QStringLiteral("addTorrent")); }
std::vector<TrackerInfo> IpcEngine::trackersAt(int index) const { return {}; }   // TODO: IPC query
QString IpcEngine::torrentRootPath(int index) const { return {}; }   // TODO: IPC query
bool IpcEngine::torrentHasVideo(int index) const { return {}; }   // TODO: IPC query
bool IpcEngine::torrentHasArchives(int index) const { return {}; }   // TODO: IPC query
int IpcEngine::torrentDownloadLimit(int index) const { return {}; }   // TODO: IPC query
QString IpcEngine::streamFilePath(int torrentIndex, int fileIndex) const { return {}; }   // TODO: IPC query
void IpcEngine::renameFile(int torrentIndex, int fileIndex, const QString &newRelativePath) { call(QStringLiteral("renameFile")); }
void IpcEngine::pauseTorrent(int index) { call(QStringLiteral("pauseTorrent")); }
qint64 IpcEngine::globalUploaded() const { return {}; }   // TODO: IPC query
float IpcEngine::globalRatio() const { return {}; }   // TODO: IPC query
qint64 IpcEngine::globalDownloaded() const { return {}; }   // TODO: IPC query
void IpcEngine::extractTorrent(int index, const QString &password) { call(QStringLiteral("extractTorrent")); }
void IpcEngine::unmarkCompleted(int index) { call(QStringLiteral("unmarkCompleted")); }
int IpcEngine::totalTorrentsAdded() const { return {}; }   // TODO: IPC query
int IpcEngine::torrentUploadLimit(int index) const { return {}; }   // TODO: IPC query
QStringList IpcEngine::torrentTags(int index) const { return {}; }   // TODO: IPC query
int IpcEngine::torrentStopAfterDownload(int index) const { return {}; }   // TODO: IPC query
qint64 IpcEngine::torrentMaxSeedSeconds(int index) const { return {}; }   // TODO: IPC query
QString IpcEngine::torrentMagnetUri(int index) const { return {}; }   // TODO: IPC query
QStringList IpcEngine::torrentFileNames(int index) const { return {}; }   // TODO: IPC query
QVariantMap IpcEngine::streamFileStats(int torrentIndex, int fileIndex) const { return {}; }   // TODO: IPC query
void IpcEngine::stopSeedingTorrent(int index) { call(QStringLiteral("stopSeedingTorrent")); }
QVariantMap IpcEngine::statsWrapped(int year) const { return {}; }   // TODO: IPC query
void IpcEngine::setTorrentUploadLimit(int index, int kbps) { call(QStringLiteral("setTorrentUploadLimit")); }
void IpcEngine::setTorrentTags(int index, const QStringList &tags) { call(QStringLiteral("setTorrentTags")); }
void IpcEngine::setTorrentStopAfterDownload(int index, int value) { call(QStringLiteral("setTorrentStopAfterDownload")); }
void IpcEngine::setTorrentMaxSeedSeconds(int index, qint64 seconds) { call(QStringLiteral("setTorrentMaxSeedSeconds")); }
void IpcEngine::setTorrentDownloadLimit(int index, int kbps) { call(QStringLiteral("setTorrentDownloadLimit")); }
void IpcEngine::setTorrentCategory(int index, const QString &category) { call(QStringLiteral("setTorrentCategory")); }
void IpcEngine::setSuperSeeding(int index, bool on) { call(QStringLiteral("setSuperSeeding")); }
void IpcEngine::setForceStart(int index, bool on) { call(QStringLiteral("setForceStart")); }
void IpcEngine::setAltSpeedsActive(bool active) { call(QStringLiteral("setAltSpeedsActive")); }
qint64 IpcEngine::sessionUploaded() const { return {}; }   // TODO: IPC query
qint64 IpcEngine::sessionDownloaded() const { return {}; }   // TODO: IPC query
void IpcEngine::saveResumeData() { call(QStringLiteral("saveResumeData")); }
bool IpcEngine::restoreRemoved(const QString &hash) { return {}; }   // TODO: IPC query
void IpcEngine::replaceTrackers(int torrentIndex, const QStringList &urls) { call(QStringLiteral("replaceTrackers")); }
void IpcEngine::removeTorrent(int index, bool deleteFiles, bool permanent) { call(QStringLiteral("removeTorrent")); }
QList<RemovedEntry> IpcEngine::recentlyRemoved() const { return {}; }   // TODO: IPC query
int IpcEngine::portStatus() const { return {}; }   // TODO: IPC query
std::vector<bool> IpcEngine::piecesAt(int index) const { return {}; }   // TODO: IPC query
std::vector<PeerInfo> IpcEngine::peersAt(int index, int maxPeers) const { return {}; }   // TODO: IPC query
void IpcEngine::moveStorage(int torrentIndex, const QString &newSavePath) { call(QStringLiteral("moveStorage")); }
void IpcEngine::markCompleted(int index) { call(QStringLiteral("markCompleted")); }
int IpcEngine::listenPort() const { return {}; }   // TODO: IPC query
bool IpcEngine::isSuperSeeding(int index) const { return {}; }   // TODO: IPC query
bool IpcEngine::isSequentialDownload(int index) const { return {}; }   // TODO: IPC query
bool IpcEngine::isForceStart(int index) const { return {}; }   // TODO: IPC query
int IpcEngine::importFromQBittorrent(const QString &defaultSavePath) { return {}; }   // TODO: IPC query
void IpcEngine::forceRecheck(int index) { call(QStringLiteral("forceRecheck")); }
void IpcEngine::forceReannounce(int index) { call(QStringLiteral("forceReannounce")); }
bool IpcEngine::exportTorrent(int index, const QString &destPath) { return {}; }   // TODO: IPC query
bool IpcEngine::dhtEnabled() const { return {}; }   // TODO: IPC query
DetailedStats IpcEngine::detailedStats() const { return {}; }   // TODO: IPC query
void IpcEngine::clearRemovedHistory() { call(QStringLiteral("clearRemovedHistory")); }
QStringList IpcEngine::categories() const { return {}; }   // TODO: IPC query
bool IpcEngine::altSpeedsActive() const { return {}; }   // TODO: IPC query
QMap<QString, QString> IpcEngine::allCategorySavePaths() const { return {}; }   // TODO: IPC query
void IpcEngine::addTracker(int index, const QString &url) { call(QStringLiteral("addTracker")); }
void IpcEngine::addTorrentWithPriorities(const QString &filePath, const QString &savePath, const std::vector<int> &filePriorities) { call(QStringLiteral("addTorrentWithPriorities")); }