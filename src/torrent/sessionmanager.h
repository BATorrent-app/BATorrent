// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef SESSIONMANAGER_H
#define SESSIONMANAGER_H

#include "types.h"
#include <QObject>
#include <QTimer>
#include <QString>
#include <QStringList>
#include <libtorrent/session.hpp>
#include <libtorrent/torrent_handle.hpp>
#include <libtorrent/torrent_status.hpp>
#include <QMap>
#include <map>
#include <vector>
#include <set>

class SessionManager : public QObject
{
    Q_OBJECT
public:
    explicit SessionManager(QObject *parent = nullptr);
    ~SessionManager();

    void addTorrent(const QString &filePath, const QString &savePath);
    void addMagnet(const QString &uri, const QString &savePath);
    void removeTorrent(int index, bool deleteFiles = false);
    void pauseTorrent(int index);
    void resumeTorrent(int index);
    void pauseAll();
    void resumeAll();

    int torrentCount() const;
    TorrentInfo torrentAt(int index) const;
    std::vector<PeerInfo> peersAt(int index) const;
    std::vector<FileInfo> filesAt(int index) const;
    std::vector<TrackerInfo> trackersAt(int index) const;
    void addTracker(int index, const QString &url);

    void setFilePriority(int torrentIndex, int fileIndex, int priority);
    void setSequentialDownload(int index, bool sequential);
    // Rename one file in the torrent (libtorrent's rename_file). The path
    // is interpreted relative to the torrent's save_path.
    void renameFile(int torrentIndex, int fileIndex, const QString &newRelativePath);
    // Move the torrent's whole save folder to a new location. Triggers a
    // force_recheck after the move so libtorrent re-verifies pieces.
    void moveStorage(int torrentIndex, const QString &newSavePath);
    // Replace the tracker list with `urls`. Drops any tracker not in the
    // new list — used by "remove tracker" since libtorrent has no single-
    // tracker delete API.
    void replaceTrackers(int torrentIndex, const QStringList &urls);

    // Categories
    void setTorrentCategory(int index, const QString &category);
    QStringList categories() const;

    // Piece map
    std::vector<bool> piecesAt(int index) const;

    void setDownloadLimit(int kbps);
    void setUploadLimit(int kbps);
    int downloadLimit() const;
    int uploadLimit() const;

    // Network settings
    void setListenPort(int port);
    int listenPort() const;
    void setMaxConnections(int max);
    int maxConnections() const;
    void setDhtEnabled(bool enabled);
    bool dhtEnabled() const;
    void setEncryptionMode(int mode); // 0=enabled, 1=forced, 2=disabled
    int encryptionMode() const;

    // VPN / Interface binding
    void setOutgoingInterface(const QString &interfaceName); // "" = any
    QString outgoingInterface() const;
    void setKillSwitchEnabled(bool enabled);
    bool killSwitchEnabled() const;
    void setAutoResumeOnReconnect(bool enabled);
    bool autoResumeOnReconnect() const;

    // Seed limits
    void setSeedRatioLimit(float ratio);
    float seedRatioLimit() const;

    // Stop seeding when download completes (global default)
    void setStopAfterDownload(bool enabled);
    bool stopAfterDownload() const;

    // Maximum seeding time in seconds (global default, 0 = unlimited)
    void setMaxSeedSeconds(qint64 seconds);
    qint64 maxSeedSeconds() const;

    // Per-torrent overrides (-1 = use global default)
    void setTorrentStopAfterDownload(int index, int value); // -1, 0, or 1
    int torrentStopAfterDownload(int index) const;
    void setTorrentMaxSeedSeconds(int index, qint64 seconds); // -1 = use default
    qint64 torrentMaxSeedSeconds(int index) const;

    // Force pause regardless of state ("stop seeding now")
    void stopSeedingTorrent(int index);

    // Manual tracker / disk operations
    void forceRecheck(int index);
    void forceReannounce(int index);

    // info-hash for a torrent at the given index, or empty string if metadata
    // isn't ready yet (magnets still resolving). Used by model/UI code that
    // needs a stable identifier across vector reorders.
    QString torrentHashAt(int index) const;

    // Auto-move completed downloads
    void setAutoMove(bool enabled, const QString &path);
    bool autoMoveEnabled() const;
    QString autoMovePath() const;

    // Download queue
    void setMaxActiveDownloads(int max);
    int maxActiveDownloads() const;
    void setTorrentQueuePosition(int index, int position);

    // Proxy
    void setProxySettings(int type, const QString &host, int port,
                          const QString &user, const QString &pass);
    int proxyType() const;
    QString proxyHost() const;
    int proxyPort() const;
    QString proxyUser() const;
    QString proxyPass() const;

    // IP filter
    void loadIpFilter(const QString &filePath);
    void clearIpFilter();
    QString ipFilterPath() const;
    int ipFilterCount() const;

    // Bandwidth scheduler
    void setAltSpeedLimits(int downKbps, int upKbps);
    int altDownloadLimit() const;
    int altUploadLimit() const;
    void setSchedulerEnabled(bool enabled);
    bool schedulerEnabled() const;
    void setScheduleFromHour(int hour);
    void setScheduleToHour(int hour);
    int scheduleFromHour() const;
    int scheduleToHour() const;
    void setScheduleDays(int daysMask); // bit 0=Mon..6=Sun
    int scheduleDays() const;
    bool altSpeedsActive() const;

    void saveResumeData();
    void loadResumeData();

    // Global statistics
    qint64 globalDownloaded() const;
    qint64 globalUploaded() const;
    float globalRatio() const;

    // Per-session statistics
    qint64 sessionDownloaded() const;
    qint64 sessionUploaded() const;

    // Torrent count tracking
    void incrementTorrentCount();
    int totalTorrentsAdded() const;

    int importFromQBittorrent(const QString &defaultSavePath);

signals:
    void torrentAdded(int index);
    void torrentRemoved(int index);
    void torrentsUpdated();
    void torrentFinished(const QString &name, const QString &infoHash);
    void torrentError(const QString &message);
    void killSwitchTriggered();
    void interfaceRestored();

private slots:
    void updateStats();

private:
    static QString stateToString(lt::torrent_status::state_t state);
    void processAlerts();
    void checkSeedRatios();
    void checkSeedingLimits();
    void checkInterfaceStatus();
    void checkBandwidthSchedule();
    QString resumeDataDir() const;
    // Rename every file in the torrent's add_torrent_params to have a ".!bt"
    // suffix so media servers ignore in-progress files. Stripped back as
    // each file completes (see file_completed_alert handling).
    void applyIncompleteSuffix(lt::add_torrent_params &atp);
    QString torrentHash(int index) const;
    // Fetch the cached torrent_status for a handle, falling back to a live
    // status() call only when the cache hasn't been populated yet (just after
    // add, before the first state_update_alert).
    lt::torrent_status cachedStatus(const lt::torrent_handle &h) const;
    bool effectiveStopAfterDownload(const QString &hash) const;
    qint64 effectiveMaxSeedSeconds(const QString &hash) const;

    lt::session m_session;
    std::vector<lt::torrent_handle> m_torrents;
    // Snapshot of the most recent libtorrent state for each handle. Updated
    // from state_update_alert; consumed by every UI getter so we don't call
    // handle.status() — a synchronous cross-thread call — once per row per
    // tick on top of all the other periodic checks.
    mutable std::map<lt::torrent_handle, lt::torrent_status> m_statusCache;
    QTimer m_updateTimer;
    bool m_dhtEnabled = true;
    int m_encryptionMode = 0;
    float m_seedRatioLimit = 0.0f; // 0 = no limit

    // Stop-seeding rules (globals)
    bool m_stopAfterDownload = false;
    qint64 m_maxSeedSeconds = 0; // 0 = unlimited

    // Per-torrent overrides (info_hash -> value; -1 = use global)
    QMap<QString, int> m_perTorrentStopAfter;
    QMap<QString, qint64> m_perTorrentMaxSeed;

    // Auto-move
    bool m_autoMoveEnabled = false;
    QString m_autoMovePath;

    // Download queue
    int m_maxActiveDownloads = 0; // 0 = unlimited
    std::set<lt::torrent_handle> m_queuePaused; // torrents paused by queue logic
    void enforceDownloadQueue();

    // Global stats (accumulated from previous sessions)
    qint64 m_globalDownBase = 0;
    qint64 m_globalUpBase = 0;

    // Categories per torrent (hash -> category name)
    QMap<QString, QString> m_categories;

    // VPN / Interface binding
    QString m_outgoingInterface;
    bool m_killSwitchEnabled = false;
    bool m_autoResume = false;
    bool m_killSwitchActive = false;
    std::set<lt::torrent_handle> m_killSwitchPaused;

    // Proxy
    int m_proxyType = 0; // 0=none, 1=SOCKS5, 2=HTTP
    QString m_proxyHost;
    int m_proxyPort = 0;
    QString m_proxyUser;
    QString m_proxyPass;

    // IP filter
    QString m_ipFilterPath;
    int m_ipFilterCount = 0;

    // Magnets that haven't resolved metadata within this window get aborted
    // and emit a torrentError. 0 = no timeout.
    int m_magnetTimeoutSeconds = 300; // 5 min default
    // Tracks when each magnet was added, by info_hash, so we can age them
    // out after m_magnetTimeoutSeconds without metadata.
    std::map<lt::torrent_handle, qint64> m_magnetAddedAt;
    void checkMagnetTimeouts();

    // Bandwidth scheduler
    int m_altDownLimit = 0;
    int m_altUpLimit = 0;
    bool m_schedulerEnabled = false;
    int m_scheduleFromHour = 1;
    int m_scheduleToHour = 7;
    int m_scheduleDays = 0x7F; // all days
    bool m_altSpeedsActive = false;
    int m_normalDownLimit = 0;
    int m_normalUpLimit = 0;
};

#endif
