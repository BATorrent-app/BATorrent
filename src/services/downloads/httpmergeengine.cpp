// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "services/downloads/httpmergeengine.h"
#include "services/downloads/httpdownloadmanager.h"
#include "services/downloads/httpdownload.h"
#include "services/platform/translator.h"

#include <QFileInfo>

namespace {

bool hasExt(const QString &name, const QStringList &exts)
{
    QString n = name.toLower();
    if (n.endsWith(QStringLiteral(".part"))) n.chop(5);   // in-progress marker
    for (const QString &e : exts)
        if (n.endsWith(e)) return true;
    return false;
}

} // namespace

HttpMergeEngine::HttpMergeEngine(IEngine *inner, HttpDownloadManager *http, QObject *parent)
    : IEngine(parent), m_inner(inner), m_http(http)
{
    // Forward the inner engine's reactive surface 1:1 so the UI connects to us
    // and still sees every torrent event unchanged.
    connect(m_inner, &IEngine::torrentsUpdated,       this, &IEngine::torrentsUpdated);
    connect(m_inner, &IEngine::torrentFinished,       this, &IEngine::torrentFinished);
    connect(m_inner, &IEngine::extractionCompleted,   this, &IEngine::extractionCompleted);
    connect(m_inner, &IEngine::altSpeedsActiveChanged,this, &IEngine::altSpeedsActiveChanged);
    connect(m_inner, &IEngine::portStatusChanged,     this, &IEngine::portStatusChanged);
    connect(m_inner, &IEngine::torrentRemoved,        this, &IEngine::torrentRemoved);
    connect(m_inner, &IEngine::torrentError,          this, &IEngine::torrentError);
    connect(m_inner, &IEngine::suspiciousFilesDetected,this, &IEngine::suspiciousFilesDetected);
    connect(m_inner, &IEngine::killSwitchTriggered,   this, &IEngine::killSwitchTriggered);
    connect(m_inner, &IEngine::torrentAddedInfo,      this, &IEngine::torrentAddedInfo);

    // HTTP progress/state moves the same grid; a completed download rides the
    // same completion event as a finished torrent (toast, Play loop, disk-fit).
    connect(m_http, &HttpDownloadManager::changed, this, &IEngine::torrentsUpdated);
    connect(m_http, &HttpDownloadManager::downloadFinished, this,
            [this](const QString &id, const QString &name, bool ok) {
                if (ok) emit torrentFinished(name, bat::httpRowHash(id));
            });
}

int HttpMergeEngine::base() const { return m_inner->torrentCount(); }
bool HttpMergeEngine::isHttp(int index) const { return index >= base(); }
HttpDownload *HttpMergeEngine::httpAt(int index) const { return m_http->at(index - base()); }

int HttpMergeEngine::torrentCount() const { return base() + m_http->count(); }

TorrentInfo HttpMergeEngine::torrentAt(int index) const
{
    if (!isHttp(index)) return m_inner->torrentAt(index);

    TorrentInfo info{};
    HttpDownload *d = httpAt(index);
    if (!d) return info;

    info.name = d->fileName();
    info.savePath = QFileInfo(d->finalPath()).absolutePath();
    info.totalSize = d->totalBytes() > 0 ? d->totalBytes() : 0;
    info.totalDone = d->receivedBytes();
    info.addedTime = 0;

    switch (d->state()) {
    case HttpDownload::State::Completed:
        info.completed = true;
        info.progress = 1.0f;
        info.stateString = tr_("state_completed");
        break;
    case HttpDownload::State::Paused:
        info.paused = true;
        info.progress = float(d->progress());
        info.stateString = tr_("state_paused");
        break;
    case HttpDownload::State::Failed:
        info.paused = true;                          // reads as a stopped row
        info.progress = float(d->progress());
        info.stateString = tr_("state_failed");
        info.stateDetail = d->errorString();
        break;
    case HttpDownload::State::Probing:
        info.progress = float(d->progress());
        info.stateString = tr_("state_connecting");
        break;
    default:                                         // Downloading / Queued
        info.progress = float(d->progress());
        info.downloadRate = d->downloadRate();
        info.stateString = tr_("state_downloading");
        break;
    }
    return info;
}

QString HttpMergeEngine::torrentHashAt(int index) const
{
    if (!isHttp(index)) return m_inner->torrentHashAt(index);
    HttpDownload *d = httpAt(index);
    return d ? bat::httpRowHash(d->id()) : QString();
}

int HttpMergeEngine::torrentIndexByInfoHash(const QString &infoHash) const
{
    if (bat::isHttpRowHash(infoHash)) {
        const QString id = bat::httpRowId(infoHash);
        for (int i = 0; i < m_http->count(); ++i)
            if (m_http->at(i)->id() == id) return base() + i;
        return -1;
    }
    return m_inner->torrentIndexByInfoHash(infoHash);
}

std::vector<FileInfo> HttpMergeEngine::filesAt(int index) const
{
    if (!isHttp(index)) return m_inner->filesAt(index);
    HttpDownload *d = httpAt(index);
    if (!d) return {};
    FileInfo f;
    f.path = d->fileName();
    f.size = d->totalBytes() > 0 ? d->totalBytes() : 0;
    f.progress = float(d->progress());
    f.priority = 1;
    return { f };
}

void HttpMergeEngine::pauseTorrent(int index)
{
    if (isHttp(index)) { if (HttpDownload *d = httpAt(index)) m_http->pause(d->id()); return; }
    m_inner->pauseTorrent(index);
}

void HttpMergeEngine::resumeTorrent(int index)
{
    if (isHttp(index)) { if (HttpDownload *d = httpAt(index)) m_http->resume(d->id()); return; }
    m_inner->resumeTorrent(index);
}

void HttpMergeEngine::removeTorrent(int index, bool deleteFiles, bool permanent)
{
    if (isHttp(index)) { if (HttpDownload *d = httpAt(index)) m_http->remove(d->id(), deleteFiles); return; }
    m_inner->removeTorrent(index, deleteFiles, permanent);
}

QString HttpMergeEngine::torrentRootPath(int index) const
{
    if (isHttp(index)) { HttpDownload *d = httpAt(index); return d ? d->finalPath() : QString(); }
    return m_inner->torrentRootPath(index);
}

bool HttpMergeEngine::torrentHasVideo(int index) const
{
    if (isHttp(index)) {
        static const QStringList v = {".mp4",".mkv",".avi",".mov",".wmv",".flv",".webm",".m4v",".ts",".mpg",".mpeg",".m2ts"};
        HttpDownload *d = httpAt(index);
        return d && hasExt(d->fileName(), v);
    }
    return m_inner->torrentHasVideo(index);
}

bool HttpMergeEngine::torrentHasArchives(int index) const
{
    if (isHttp(index)) {
        static const QStringList a = {".zip",".rar",".7z"};
        HttpDownload *d = httpAt(index);
        return d && hasExt(d->fileName(), a);
    }
    return m_inner->torrentHasArchives(index);
}

QString HttpMergeEngine::torrentMagnetUri(int index) const
{
    return isHttp(index) ? QString() : m_inner->torrentMagnetUri(index);
}

QStringList HttpMergeEngine::torrentFileNames(int index) const
{
    if (isHttp(index)) { HttpDownload *d = httpAt(index); return d ? QStringList{d->fileName()} : QStringList{}; }
    return m_inner->torrentFileNames(index);
}

QString HttpMergeEngine::streamFilePath(int torrentIndex, int fileIndex) const
{
    if (isHttp(torrentIndex)) {
        HttpDownload *d = httpAt(torrentIndex);
        return (d && d->state() == HttpDownload::State::Completed) ? d->finalPath() : QString();
    }
    return m_inner->streamFilePath(torrentIndex, fileIndex);
}

qint64 HttpMergeEngine::streamFileSize(int torrentIndex, int fileIndex) const
{
    if (isHttp(torrentIndex)) { HttpDownload *d = httpAt(torrentIndex); return d ? d->totalBytes() : 0; }
    return m_inner->streamFileSize(torrentIndex, fileIndex);
}

// ---------------------------------------------------------------------------
//  Torrent-only per-row surface: no-op / empty for an HTTP row, else delegate.
// ---------------------------------------------------------------------------
void HttpMergeEngine::setFilePriority(int t, int f, int p) { if (!isHttp(t)) m_inner->setFilePriority(t, f, p); }
void HttpMergeEngine::setTorrentQueuePosition(int i, int p) { if (!isHttp(i)) m_inner->setTorrentQueuePosition(i, p); }
void HttpMergeEngine::setSequentialDownload(int i, bool s) { if (!isHttp(i)) m_inner->setSequentialDownload(i, s); }
void HttpMergeEngine::prioritizeFilePieceBoundaries(int t, int f) { if (!isHttp(t)) m_inner->prioritizeFilePieceBoundaries(t, f); }
void HttpMergeEngine::addTorrent(const QString &fp, const QString &sp) { m_inner->addTorrent(fp, sp); }
std::vector<TrackerInfo> HttpMergeEngine::trackersAt(int i) const { return isHttp(i) ? std::vector<TrackerInfo>{} : m_inner->trackersAt(i); }
int HttpMergeEngine::torrentDownloadLimit(int i) const { return isHttp(i) ? 0 : m_inner->torrentDownloadLimit(i); }
void HttpMergeEngine::renameFile(int t, int f, const QString &n) { if (!isHttp(t)) m_inner->renameFile(t, f, n); }
void HttpMergeEngine::extractTorrent(int i, const QString &p) { if (!isHttp(i)) m_inner->extractTorrent(i, p); }
void HttpMergeEngine::unmarkCompleted(int i) { if (!isHttp(i)) m_inner->unmarkCompleted(i); }
int HttpMergeEngine::torrentUploadLimit(int i) const { return isHttp(i) ? 0 : m_inner->torrentUploadLimit(i); }
QStringList HttpMergeEngine::torrentTags(int i) const { return isHttp(i) ? QStringList{} : m_inner->torrentTags(i); }
int HttpMergeEngine::torrentStopAfterDownload(int i) const { return isHttp(i) ? 0 : m_inner->torrentStopAfterDownload(i); }
qint64 HttpMergeEngine::torrentMaxSeedSeconds(int i) const { return isHttp(i) ? 0 : m_inner->torrentMaxSeedSeconds(i); }
QVariantMap HttpMergeEngine::streamFileStats(int t, int f) const { return isHttp(t) ? QVariantMap{} : m_inner->streamFileStats(t, f); }
void HttpMergeEngine::stopSeedingTorrent(int i) { if (!isHttp(i)) m_inner->stopSeedingTorrent(i); }
void HttpMergeEngine::setTorrentUploadLimit(int i, int k) { if (!isHttp(i)) m_inner->setTorrentUploadLimit(i, k); }
void HttpMergeEngine::setTorrentTags(int i, const QStringList &t) { if (!isHttp(i)) m_inner->setTorrentTags(i, t); }
void HttpMergeEngine::setTorrentStopAfterDownload(int i, int v) { if (!isHttp(i)) m_inner->setTorrentStopAfterDownload(i, v); }
void HttpMergeEngine::setTorrentMaxSeedSeconds(int i, qint64 s) { if (!isHttp(i)) m_inner->setTorrentMaxSeedSeconds(i, s); }
void HttpMergeEngine::setTorrentDownloadLimit(int i, int k) { if (!isHttp(i)) m_inner->setTorrentDownloadLimit(i, k); }
void HttpMergeEngine::setTorrentCategory(int i, const QString &c) { if (!isHttp(i)) m_inner->setTorrentCategory(i, c); }
void HttpMergeEngine::setSuperSeeding(int i, bool o) { if (!isHttp(i)) m_inner->setSuperSeeding(i, o); }
void HttpMergeEngine::setForceStart(int i, bool o) { if (!isHttp(i)) m_inner->setForceStart(i, o); }
bool HttpMergeEngine::restoreRemoved(const QString &h) { return m_inner->restoreRemoved(h); }
void HttpMergeEngine::replaceTrackers(int t, const QStringList &u) { if (!isHttp(t)) m_inner->replaceTrackers(t, u); }
std::vector<bool> HttpMergeEngine::piecesAt(int i) const { return isHttp(i) ? std::vector<bool>{} : m_inner->piecesAt(i); }
std::vector<PeerInfo> HttpMergeEngine::peersAt(int i, int m) const { return isHttp(i) ? std::vector<PeerInfo>{} : m_inner->peersAt(i, m); }
void HttpMergeEngine::moveStorage(int t, const QString &p) { if (!isHttp(t)) m_inner->moveStorage(t, p); }
void HttpMergeEngine::markCompleted(int i) { if (!isHttp(i)) m_inner->markCompleted(i); }
bool HttpMergeEngine::isSuperSeeding(int i) const { return isHttp(i) ? false : m_inner->isSuperSeeding(i); }
bool HttpMergeEngine::isSequentialDownload(int i) const { return isHttp(i) ? false : m_inner->isSequentialDownload(i); }
bool HttpMergeEngine::isForceStart(int i) const { return isHttp(i) ? false : m_inner->isForceStart(i); }
void HttpMergeEngine::forceRecheck(int i) { if (!isHttp(i)) m_inner->forceRecheck(i); }
void HttpMergeEngine::forceReannounce(int i) { if (!isHttp(i)) m_inner->forceReannounce(i); }
bool HttpMergeEngine::exportTorrent(int i, const QString &d) { return isHttp(i) ? false : m_inner->exportTorrent(i, d); }
void HttpMergeEngine::addTracker(int i, const QString &u) { if (!isHttp(i)) m_inner->addTracker(i, u); }
void HttpMergeEngine::addTorrentWithPriorities(const QString &fp, const QString &sp, const std::vector<int> &pr) { m_inner->addTorrentWithPriorities(fp, sp, pr); }
void HttpMergeEngine::addMagnet(const QString &u, const QString &sp, const QString &ch, int ct) { m_inner->addMagnet(u, sp, ch, ct); }
void HttpMergeEngine::streamSetDeadlineWindow(int t, int f, qint64 s, int w) { if (!isHttp(t)) m_inner->streamSetDeadlineWindow(t, f, s, w); }
qint64 HttpMergeEngine::streamContiguousAvailableBytes(int t, int f, qint64 from, qint64 cap) const { return isHttp(t) ? 0 : m_inner->streamContiguousAvailableBytes(t, f, from, cap); }

// ---------------------------------------------------------------------------
//  Global / session surface: pure delegation.
// ---------------------------------------------------------------------------
qint64 HttpMergeEngine::globalUploaded() const { return m_inner->globalUploaded(); }
float HttpMergeEngine::globalRatio() const { return m_inner->globalRatio(); }
qint64 HttpMergeEngine::globalDownloaded() const { return m_inner->globalDownloaded(); }
int HttpMergeEngine::totalTorrentsAdded() const { return m_inner->totalTorrentsAdded(); }
QVariantMap HttpMergeEngine::statsWrapped(int y) const { return m_inner->statsWrapped(y); }
void HttpMergeEngine::setAltSpeedsActive(bool a) { m_inner->setAltSpeedsActive(a); }
qint64 HttpMergeEngine::sessionUploaded() const { return m_inner->sessionUploaded(); }
qint64 HttpMergeEngine::sessionDownloaded() const { return m_inner->sessionDownloaded(); }
void HttpMergeEngine::saveResumeData() { m_inner->saveResumeData(); }
void HttpMergeEngine::resumeAll() { m_inner->resumeAll(); }
QList<RemovedEntry> HttpMergeEngine::recentlyRemoved() const { return m_inner->recentlyRemoved(); }
int HttpMergeEngine::portStatus() const { return m_inner->portStatus(); }
void HttpMergeEngine::pauseAll() { m_inner->pauseAll(); }
int HttpMergeEngine::listenPort() const { return m_inner->listenPort(); }
int HttpMergeEngine::downloadLimit() const { return m_inner->downloadLimit(); }
int HttpMergeEngine::uploadLimit() const { return m_inner->uploadLimit(); }
int HttpMergeEngine::importFromQBittorrent(const QString &d) { return m_inner->importFromQBittorrent(d); }
bool HttpMergeEngine::dhtEnabled() const { return m_inner->dhtEnabled(); }
DetailedStats HttpMergeEngine::detailedStats() const { return m_inner->detailedStats(); }
void HttpMergeEngine::clearRemovedHistory() { m_inner->clearRemovedHistory(); }
QStringList HttpMergeEngine::categories() const { return m_inner->categories(); }
bool HttpMergeEngine::altSpeedsActive() const { return m_inner->altSpeedsActive(); }
QMap<QString, QString> HttpMergeEngine::allCategorySavePaths() const { return m_inner->allCategorySavePaths(); }
void HttpMergeEngine::setProxySettings(int t, const QString &h, int p, const QString &u, const QString &pw) { m_inner->setProxySettings(t, h, p, u, pw); }
int HttpMergeEngine::proxyType() const { return m_inner->proxyType(); }
QString HttpMergeEngine::proxyHost() const { return m_inner->proxyHost(); }
int HttpMergeEngine::proxyPort() const { return m_inner->proxyPort(); }
QString HttpMergeEngine::proxyUser() const { return m_inner->proxyUser(); }
QString HttpMergeEngine::proxyPass() const { return m_inner->proxyPass(); }
AdvancedSettings HttpMergeEngine::advancedSettings() const { return m_inner->advancedSettings(); }
void HttpMergeEngine::setAdvancedSettings(const AdvancedSettings &s) { m_inner->setAdvancedSettings(s); }
bool HttpMergeEngine::applySetting(const QString &k, const QVariant &v) { return m_inner->applySetting(k, v); }
