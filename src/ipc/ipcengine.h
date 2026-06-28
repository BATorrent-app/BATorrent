// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// UI-side IEngine that proxies to the engine child process over a QLocalSocket,
// supervising it (respawn on crash). The UI never knows whether it's talking to
// an in-process SessionManager or this. See internal/ENGINE_SPLIT_PLAN.md.
//
// NOTE (Phase 2 scaffolding): the channel, process supervision and a few
// representative methods are wired; the remaining methods are stubs returning
// defaults, filled in next. engineMode defaults to in-process, so this is not
// yet on the shipping path.
#ifndef BATORRENT_IPCENGINE_H
#define BATORRENT_IPCENGINE_H

#include "../torrent/iengine.h"
#include <QProcess>
#include <QByteArray>

class QLocalSocket;

class IpcEngine : public IEngine
{
    Q_OBJECT
public:
    explicit IpcEngine(const QString &exePath, QObject *parent = nullptr);
    ~IpcEngine() override;

    bool start();             // spawn the engine child + connect
    bool connected() const;

    // ---- IEngine surface (generated overrides) ----
    TorrentInfo torrentAt(int index) const override;
    QString torrentHashAt(int index) const override;
    int torrentIndexByInfoHash(const QString &infoHash) const override;
    int torrentCount() const override;
    std::vector<FileInfo> filesAt(int index) const override;
    void setFilePriority(int torrentIndex, int fileIndex, int priority) override;
    void setTorrentQueuePosition(int index, int position) override;
    void resumeTorrent(int index) override;
    void setSequentialDownload(int index, bool sequential) override;
    void prioritizeFilePieceBoundaries(int torrentIndex, int fileIndex) override;
    void addTorrent(const QString &filePath, const QString &savePath) override;
    std::vector<TrackerInfo> trackersAt(int index) const override;
    QString torrentRootPath(int index) const override;
    bool torrentHasVideo(int index) const override;
    bool torrentHasArchives(int index) const override;
    int torrentDownloadLimit(int index) const override;
    QString streamFilePath(int torrentIndex, int fileIndex) const override;
    void renameFile(int torrentIndex, int fileIndex, const QString &newRelativePath) override;
    void pauseTorrent(int index) override;
    qint64 globalUploaded() const override;
    float globalRatio() const override;
    qint64 globalDownloaded() const override;
    void extractTorrent(int index, const QString &password) override;
    void unmarkCompleted(int index) override;
    int totalTorrentsAdded() const override;
    int torrentUploadLimit(int index) const override;
    QStringList torrentTags(int index) const override;
    int torrentStopAfterDownload(int index) const override;
    qint64 torrentMaxSeedSeconds(int index) const override;
    QString torrentMagnetUri(int index) const override;
    QStringList torrentFileNames(int index) const override;
    QVariantMap streamFileStats(int torrentIndex, int fileIndex) const override;
    void stopSeedingTorrent(int index) override;
    QVariantMap statsWrapped(int year) const override;
    void setTorrentUploadLimit(int index, int kbps) override;
    void setTorrentTags(int index, const QStringList &tags) override;
    void setTorrentStopAfterDownload(int index, int value) override;
    void setTorrentMaxSeedSeconds(int index, qint64 seconds) override;
    void setTorrentDownloadLimit(int index, int kbps) override;
    void setTorrentCategory(int index, const QString &category) override;
    void setSuperSeeding(int index, bool on) override;
    void setForceStart(int index, bool on) override;
    void setAltSpeedsActive(bool active) override;
    qint64 sessionUploaded() const override;
    qint64 sessionDownloaded() const override;
    void saveResumeData() override;
    void resumeAll() override;
    bool restoreRemoved(const QString &hash) override;
    void replaceTrackers(int torrentIndex, const QStringList &urls) override;
    void removeTorrent(int index, bool deleteFiles = false, bool permanent = false) override;
    QList<RemovedEntry> recentlyRemoved() const override;
    int portStatus() const override;
    std::vector<bool> piecesAt(int index) const override;
    std::vector<PeerInfo> peersAt(int index, int maxPeers = 0) const override;
    void pauseAll() override;
    void moveStorage(int torrentIndex, const QString &newSavePath) override;
    void markCompleted(int index) override;
    int listenPort() const override;
    bool isSuperSeeding(int index) const override;
    bool isSequentialDownload(int index) const override;
    bool isForceStart(int index) const override;
    int importFromQBittorrent(const QString &defaultSavePath) override;
    void forceRecheck(int index) override;
    void forceReannounce(int index) override;
    bool exportTorrent(int index, const QString &destPath) override;
    bool dhtEnabled() const override;
    DetailedStats detailedStats() const override;
    void clearRemovedHistory() override;
    QStringList categories() const override;
    bool altSpeedsActive() const override;
    QMap<QString, QString> allCategorySavePaths() const override;
    void addTracker(int index, const QString &url) override;
    void addTorrentWithPriorities(const QString &filePath, const QString &savePath, const std::vector<int> &filePriorities) override;
    void addMagnet(const QString &uri, const QString &savePath, const QString &coverHint = QString(), int coverType = -1) override;

signals:
    void engineStatusChanged(bool up);   // drives the UI "engine restarting" banner

private slots:
    void onProcFinished(int exitCode, QProcess::ExitStatus status);
    void onSocketReadyRead();

private:
    void spawnEngine();
    QByteArray request(const QString &method, const QByteArray &args = {}) const;   // blocking req→reply
    void call(const QString &method, const QByteArray &args = {}) const { request(method, args); }

    QString m_exePath;
    QString m_serverName;
    QProcess *m_proc = nullptr;
    QLocalSocket *m_sock = nullptr;
    mutable QByteArray m_buf;
    mutable quint32 m_nextId = 1;
    bool m_shuttingDown = false;
};

#endif
