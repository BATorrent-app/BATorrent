// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef SERVICES_DOWNLOADS_HTTPMERGEENGINE_H
#define SERVICES_DOWNLOADS_HTTPMERGEENGINE_H

// A thin IEngine decorator that presents direct-HTTP downloads as extra rows
// after the real torrents. The whole UI keys off IEngine::torrentCount/torrentAt
// + info-hash, so wrapping the real engine here makes HTTP downloads flow through
// the Downloads grid, completion toast, disk-fit and Play — with zero changes to
// SessionManager or the UI. Torrent rows delegate to the inner engine untouched;
// HTTP rows are served from the HttpDownloadManager and pseudo-hashed "http:<id>".

#include "torrent/iengine.h"

class HttpDownloadManager;
class HttpDownload;

class HttpMergeEngine : public IEngine
{
    Q_OBJECT
public:
    // Does not take ownership of `inner` or `http` (both owned by main()).
    HttpMergeEngine(IEngine *inner, HttpDownloadManager *http, QObject *parent = nullptr);

    // --- row space: torrents [0, base) then HTTP rows [base, base+httpCount) ---
    int torrentCount() const override;
    TorrentInfo torrentAt(int index) const override;
    QString torrentHashAt(int index) const override;
    int torrentIndexByInfoHash(const QString &infoHash) const override;
    std::vector<FileInfo> filesAt(int index) const override;

    // --- per-row commands that make sense for an HTTP download ---
    void pauseTorrent(int index) override;
    void resumeTorrent(int index) override;
    void removeTorrent(int index, bool deleteFiles = false, bool permanent = false) override;

    // --- per-row queries the grid / details / player read ---
    QString torrentRootPath(int index) const override;
    bool torrentHasVideo(int index) const override;
    bool torrentHasArchives(int index) const override;
    QString torrentMagnetUri(int index) const override;
    QStringList torrentFileNames(int index) const override;
    QString streamFilePath(int torrentIndex, int fileIndex) const override;
    qint64 streamFileSize(int torrentIndex, int fileIndex) const override;

    // ---------------------------------------------------------------------
    // Everything below is torrent-only. For an HTTP row it is a safe no-op /
    // empty; for a torrent row it delegates unchanged to the inner engine.
    // ---------------------------------------------------------------------
    void setFilePriority(int torrentIndex, int fileIndex, int priority) override;
    void setTorrentQueuePosition(int index, int position) override;
    void setSequentialDownload(int index, bool sequential) override;
    void prioritizeFilePieceBoundaries(int torrentIndex, int fileIndex) override;
    void addTorrent(const QString &filePath, const QString &savePath) override;
    std::vector<TrackerInfo> trackersAt(int index) const override;
    int torrentDownloadLimit(int index) const override;
    void renameFile(int torrentIndex, int fileIndex, const QString &newRelativePath) override;
    void extractTorrent(int index, const QString &password) override;
    void unmarkCompleted(int index) override;
    int torrentUploadLimit(int index) const override;
    QStringList torrentTags(int index) const override;
    int torrentStopAfterDownload(int index) const override;
    qint64 torrentMaxSeedSeconds(int index) const override;
    QVariantMap streamFileStats(int torrentIndex, int fileIndex) const override;
    void stopSeedingTorrent(int index) override;
    void setTorrentUploadLimit(int index, int kbps) override;
    void setTorrentTags(int index, const QStringList &tags) override;
    void setTorrentStopAfterDownload(int index, int value) override;
    void setTorrentMaxSeedSeconds(int index, qint64 seconds) override;
    void setTorrentDownloadLimit(int index, int kbps) override;
    void setTorrentCategory(int index, const QString &category) override;
    void setSuperSeeding(int index, bool on) override;
    void setForceStart(int index, bool on) override;
    bool restoreRemoved(const QString &hash) override;
    void replaceTrackers(int torrentIndex, const QStringList &urls) override;
    std::vector<bool> piecesAt(int index) const override;
    std::vector<PeerInfo> peersAt(int index, int maxPeers = 0) const override;
    void moveStorage(int torrentIndex, const QString &newSavePath) override;
    void markCompleted(int index) override;
    bool isSuperSeeding(int index) const override;
    bool isSequentialDownload(int index) const override;
    bool isForceStart(int index) const override;
    void forceRecheck(int index) override;
    void forceReannounce(int index) override;
    bool exportTorrent(int index, const QString &destPath) override;
    void addTracker(int index, const QString &url) override;
    void addTorrentWithPriorities(const QString &filePath, const QString &savePath, const std::vector<int> &filePriorities) override;
    void addMagnet(const QString &uri, const QString &savePath, const QString &coverHint = QString(), int coverType = -1) override;
    void streamSetDeadlineWindow(int torrentIndex, int fileIndex, qint64 startByte, int windowPieces = 24) override;
    qint64 streamContiguousAvailableBytes(int torrentIndex, int fileIndex, qint64 fromByte, qint64 cap = 8 * 1024 * 1024) const override;

    // ---------------------------------------------------------------------
    // Global / session-wide surface — no row involved, pure delegation.
    // ---------------------------------------------------------------------
    qint64 globalUploaded() const override;
    float globalRatio() const override;
    qint64 globalDownloaded() const override;
    int totalTorrentsAdded() const override;
    QVariantMap statsWrapped(int year) const override;
    void setAltSpeedsActive(bool active) override;
    qint64 sessionUploaded() const override;
    qint64 sessionDownloaded() const override;
    void saveResumeData() override;
    void resumeAll() override;
    QList<RemovedEntry> recentlyRemoved() const override;
    int portStatus() const override;
    void pauseAll() override;
    int listenPort() const override;
    int downloadLimit() const override;
    int uploadLimit() const override;
    int importFromQBittorrent(const QString &defaultSavePath) override;
    bool dhtEnabled() const override;
    DetailedStats detailedStats() const override;
    void clearRemovedHistory() override;
    QStringList categories() const override;
    bool altSpeedsActive() const override;
    QMap<QString, QString> allCategorySavePaths() const override;
    void setProxySettings(int type, const QString &host, int port, const QString &user, const QString &pass) override;
    int proxyType() const override;
    QString proxyHost() const override;
    int proxyPort() const override;
    QString proxyUser() const override;
    QString proxyPass() const override;
    AdvancedSettings advancedSettings() const override;
    void setAdvancedSettings(const AdvancedSettings &s) override;
    bool applySetting(const QString &key, const QVariant &v) override;

private:
    int base() const;                       // inner->torrentCount()
    bool isHttp(int index) const;           // index refers to an HTTP row?
    HttpDownload *httpAt(int index) const;  // the HTTP download for a merged index

    IEngine *m_inner;
    HttpDownloadManager *m_http;
};

#endif
