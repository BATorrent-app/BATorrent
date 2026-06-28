// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// The session API contract shared by the UI and the engine. SessionManager
// implements it in-process today; an IpcEngine will implement it over a local
// socket once the engine runs in its own process, so an engine crash can't take
// the UI down. The UI talks ONLY to IEngine* — never to SessionManager directly.
// See internal/ENGINE_SPLIT_PLAN.md.
#ifndef BATORRENT_IENGINE_H
#define BATORRENT_IENGINE_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantMap>
#include <QMap>
#include <QList>
#include <vector>
#include "types.h"

class IEngine : public QObject
{
    Q_OBJECT
public:
    explicit IEngine(QObject *parent = nullptr) : QObject(parent) {}
    ~IEngine() override = default;

    // --- command/query surface (74 methods used by the UI bridge) ---
    virtual TorrentInfo torrentAt(int index) const = 0;
    virtual QString torrentHashAt(int index) const = 0;
    virtual int torrentIndexByInfoHash(const QString &infoHash) const = 0;
    virtual int torrentCount() const = 0;
    virtual std::vector<FileInfo> filesAt(int index) const = 0;
    virtual void setFilePriority(int torrentIndex, int fileIndex, int priority) = 0;
    virtual void setTorrentQueuePosition(int index, int position) = 0;
    virtual void resumeTorrent(int index) = 0;
    virtual void setSequentialDownload(int index, bool sequential) = 0;
    virtual void prioritizeFilePieceBoundaries(int torrentIndex, int fileIndex) = 0;
    virtual void addTorrent(const QString &filePath, const QString &savePath) = 0;
    virtual std::vector<TrackerInfo> trackersAt(int index) const = 0;
    virtual QString torrentRootPath(int index) const = 0;
    virtual bool torrentHasVideo(int index) const = 0;
    virtual bool torrentHasArchives(int index) const = 0;
    virtual int torrentDownloadLimit(int index) const = 0;
    virtual QString streamFilePath(int torrentIndex, int fileIndex) const = 0;
    virtual void renameFile(int torrentIndex, int fileIndex, const QString &newRelativePath) = 0;
    virtual void pauseTorrent(int index) = 0;
    virtual qint64 globalUploaded() const = 0;
    virtual float globalRatio() const = 0;
    virtual qint64 globalDownloaded() const = 0;
    virtual void extractTorrent(int index, const QString &password) = 0;
    virtual void unmarkCompleted(int index) = 0;
    virtual int totalTorrentsAdded() const = 0;
    virtual int torrentUploadLimit(int index) const = 0;
    virtual QStringList torrentTags(int index) const = 0;
    virtual int torrentStopAfterDownload(int index) const = 0;
    virtual qint64 torrentMaxSeedSeconds(int index) const = 0;
    virtual QString torrentMagnetUri(int index) const = 0;
    virtual QStringList torrentFileNames(int index) const = 0;
    virtual QVariantMap streamFileStats(int torrentIndex, int fileIndex) const = 0;
    virtual void stopSeedingTorrent(int index) = 0;
    virtual QVariantMap statsWrapped(int year) const = 0;
    virtual void setTorrentUploadLimit(int index, int kbps) = 0;
    virtual void setTorrentTags(int index, const QStringList &tags) = 0;
    virtual void setTorrentStopAfterDownload(int index, int value) = 0;
    virtual void setTorrentMaxSeedSeconds(int index, qint64 seconds) = 0;
    virtual void setTorrentDownloadLimit(int index, int kbps) = 0;
    virtual void setTorrentCategory(int index, const QString &category) = 0;
    virtual void setSuperSeeding(int index, bool on) = 0;
    virtual void setForceStart(int index, bool on) = 0;
    virtual void setAltSpeedsActive(bool active) = 0;
    virtual qint64 sessionUploaded() const = 0;
    virtual qint64 sessionDownloaded() const = 0;
    virtual void saveResumeData() = 0;
    virtual void resumeAll() = 0;
    virtual bool restoreRemoved(const QString &hash) = 0;
    virtual void replaceTrackers(int torrentIndex, const QStringList &urls) = 0;
    virtual void removeTorrent(int index, bool deleteFiles = false, bool permanent = false) = 0;
    virtual QList<RemovedEntry> recentlyRemoved() const = 0;
    virtual int portStatus() const = 0;
    virtual std::vector<bool> piecesAt(int index) const = 0;
    virtual std::vector<PeerInfo> peersAt(int index, int maxPeers = 0) const = 0;
    virtual void pauseAll() = 0;
    virtual void moveStorage(int torrentIndex, const QString &newSavePath) = 0;
    virtual void markCompleted(int index) = 0;
    virtual int listenPort() const = 0;
    virtual int downloadLimit() const = 0;   // global rate caps (WebUI reads these)
    virtual int uploadLimit() const = 0;
    virtual bool isSuperSeeding(int index) const = 0;
    virtual bool isSequentialDownload(int index) const = 0;
    virtual bool isForceStart(int index) const = 0;
    virtual int importFromQBittorrent(const QString &defaultSavePath) = 0;
    virtual void forceRecheck(int index) = 0;
    virtual void forceReannounce(int index) = 0;
    virtual bool exportTorrent(int index, const QString &destPath) = 0;
    virtual bool dhtEnabled() const = 0;
    virtual DetailedStats detailedStats() const = 0;
    virtual void clearRemovedHistory() = 0;
    virtual QStringList categories() const = 0;
    virtual bool altSpeedsActive() const = 0;
    virtual QMap<QString, QString> allCategorySavePaths() const = 0;
    virtual void addTracker(int index, const QString &url) = 0;
    virtual void addTorrentWithPriorities(const QString &filePath, const QString &savePath, const std::vector<int> &filePriorities) = 0;
    virtual void addMagnet(const QString &uri, const QString &savePath, const QString &coverHint = QString(), int coverType = -1) = 0;

    // --- proxy (Settings) ---
    virtual void setProxySettings(int type, const QString &host, int port, const QString &user, const QString &pass) = 0;
    virtual int proxyType() const = 0;
    virtual QString proxyHost() const = 0;
    virtual int proxyPort() const = 0;
    virtual QString proxyUser() const = 0;
    virtual QString proxyPass() const = 0;

    // --- advanced libtorrent tuning (Settings) ---
    virtual AdvancedSettings advancedSettings() const = 0;
    virtual void setAdvancedSettings(const AdvancedSettings &s) = 0;

    // --- streaming server hooks (embedded player) ---
    virtual qint64 streamFileSize(int torrentIndex, int fileIndex) const = 0;
    virtual qint64 streamContiguousAvailableBytes(int torrentIndex, int fileIndex, qint64 fromByte, qint64 cap = 8 * 1024 * 1024) const = 0;
    virtual void streamSetDeadlineWindow(int torrentIndex, int fileIndex, qint64 startByte, int windowPieces = 24) = 0;

signals:
    void torrentsUpdated();
    void torrentFinished(const QString &name, const QString &infoHash);
    void extractionCompleted(const QString &infoHash, bool success);
    void altSpeedsActiveChanged(bool active);
    void portStatusChanged(int status);
    // Reactive surface forwarded over IPC in split mode (hoisted from
    // SessionManager so the UI connects to the same signals in both modes).
    void torrentRemoved(int index);
    void torrentError(const QString &message);
    void suspiciousFilesDetected(const QString &name, const QStringList &files);
    void killSwitchTriggered();
    // Self-contained add notification for the cover-resolve / toast / auto-tracker
    // flow. Emitted only by IpcEngine (split mode) from the forwarded event; the
    // in-process path drives the same flow off SessionManager::torrentAdded(int).
    void torrentAddedInfo(int index, const QString &hash, const QString &name,
                          qint64 totalSize, const QStringList &fileNames,
                          const QString &hintTitle, int hintType);
};

#endif
