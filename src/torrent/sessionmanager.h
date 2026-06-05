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
#include <QSet>
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
    // coverHint (optional): a clean title (e.g. from a game catalog) used as the
    // instant display name and as an accurate cover-lookup query, so magnet adds
    // don't sit nameless/placeholder until metadata arrives.
    void addMagnet(const QString &uri, const QString &savePath, const QString &coverHint = QString());
    QString takeCoverHint(const QString &hash);   // consumes the hint set at add time

    // Same as addTorrent but with up-front file priorities (0..7 per file).
    // Used by AddTorrentDialog so unchecked files never start downloading
    // — setting priorities only after add can leak a few KB before libtorrent
    // applies them.
    void addTorrentWithPriorities(const QString &filePath, const QString &savePath,
                                  const std::vector<int> &filePriorities);
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
    bool isSequentialDownload(int index) const;
    // Exposed for tests: one-time copy of pre-3.0 resume data into the new dir.
    void migrateLegacyResumeData();   // one-time pull from the pre-3.0 data dir
    QString resumeDataDir() const;
    // Boost piece priority for the first/last pieces of a specific file so
    // streaming players can read container headers (mp4 moov atom, mkv
    // cues) and trailing index data before the bulk download completes.
    void prioritizeFilePieceBoundaries(int torrentIndex, int fileIndex);
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

    // Categories — each can optionally map to a dedicated save path.
    // When a torrent is assigned a category that has a save path, new
    // downloads auto-use that path instead of the global default.
    void setTorrentCategory(int index, const QString &category);
    QStringList categories() const;
    void setCategorySavePath(const QString &category, const QString &path);
    QString categorySavePath(const QString &category) const;
    QMap<QString, QString> allCategorySavePaths() const;

    // Tags — multiple per torrent, free-form. Persisted under "torrentTags/{hash}"
    // as a comma-joined list. Used in addition to (not instead of) the single
    // category.
    QStringList torrentTags(int index) const;
    void setTorrentTags(int index, const QStringList &tags);
    QStringList allTags() const; // union of every torrent's tags, sorted

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

    // Transport-layer toggles. uTP is libtorrent's UDP-based reliable
    // transport — disabling it forces all peer traffic to TCP (useful on
    // misconfigured routers where uTP saturates the link with retransmits).
    // The toggle covers both incoming and outgoing.
    void setUtpEnabled(bool enabled);
    bool utpEnabled() const;

    // Anonymous mode hides the client version in the BitTorrent handshake
    // and disables uTP / NAT-PMP / UPnP advertisements. Trades discoverability
    // for less fingerprintable traffic — useful behind a VPN.
    void setAnonymousMode(bool enabled);
    bool anonymousMode() const;

    // Force IPv4-only outgoing connections. Useful on trackers that
    // misbehave with v6 (still common on RU/CN private trackers) and as a
    // simple leak-prevention when the v6 path isn't tunneled.
    void setForceIpv4(bool enabled);
    bool forceIpv4() const;

    // Private-tracker friendly mode. When on: DHT / PEX / LSD off, anonymous
    // handshake forced, announces hit every tracker tier instead of stopping
    // at the first responsive one. Exactly what private-tracker rules
    // (TorrentLeech, M-Team, RuTracker private, etc.) require to avoid bans.
    void setPtMode(bool enabled);
    bool ptMode() const;

    // Block known leecher clients (Xunlei/Thunder, QQDownload, Baidu
    // offline, etc.) that download from peers but never seed back. Detected
    // by peer_id prefix in the BitTorrent handshake. Popular in Chinese PT
    // communities where Xunlei's "vampire" behavior is universally hated.
    void setBlockLeecherClients(bool enabled);
    bool blockLeecherClients() const;
    int blockedLeecherCount() const;

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

    // Super seeding mode — only sends each piece once to maximize initial
    // distribution. Essential for first seeders of new content.
    void setSuperSeeding(int index, bool on);
    bool isSuperSeeding(int index) const;

    // Force start: exempt this torrent from the active-downloads queue cap.
    // It will keep downloading even when other torrents are queue-paused.
    void setForceStart(int index, bool on);
    bool isForceStart(int index) const;

    // Per-torrent rate caps (KB/s, 0 = unlimited). Persisted by info-hash so
    // limits survive restarts and re-applied on load. Override the global
    // session caps for that torrent only.
    void setTorrentDownloadLimit(int index, int kbps);
    void setTorrentUploadLimit(int index, int kbps);
    int torrentDownloadLimit(int index) const;
    int torrentUploadLimit(int index) const;

    // "Completed" — user-frozen state for torrents that are done and should
    // stop participating. Marked torrents are paused, persisted across
    // restarts, and surface in the UI with a distinct (green) state. Calling
    // resumeTorrent() on a completed torrent automatically un-marks it.
    void markCompleted(int index);
    void unmarkCompleted(int index);
    bool isTorrentCompleted(int index) const;

    // Auto-mark a seeding torrent as completed after this many seconds in
    // the seeding state (0 = disabled). Persisted across restarts.
    void setAutoCompleteSeconds(qint64 seconds);
    qint64 autoCompleteSeconds() const;

    // Manual tracker / disk operations
    void forceRecheck(int index);
    void forceReannounce(int index);

    // info-hash for a torrent at the given index, or empty string if metadata
    // isn't ready yet (magnets still resolving). Used by model/UI code that
    // needs a stable identifier across vector reorders.
    QString torrentHashAt(int index) const;

    // Build a magnet URI for the torrent at `index`. Returns empty if
    // metadata isn't loaded yet (still-resolving magnets, etc.).
    QString torrentMagnetUri(int index) const;

    // Snapshot the .resume bytes for a torrent so an undo path can recover
    // it after removal. Empty if the resume file isn't on disk yet (very
    // recent magnet add, etc.).
    QByteArray captureResumeData(int index) const;
    // Re-add a torrent from a previously captured .resume snapshot.
    // Returns true on success.
    bool restoreFromResumeData(const QByteArray &data);

    // Recently-removed history — persistent ring buffer of the last N removed
    // torrents' resume snapshots, so the user can re-add even after closing
    // the undo toast. Stored as files under <AppData>/removed/{hash}.resume.
    struct RemovedEntry {
        QString hash;
        QString name;
        qint64 totalSize;
        qint64 removedAt; // unix seconds
        QString resumePath; // absolute path to the snapshot file
    };
    QList<RemovedEntry> recentlyRemoved() const;
    bool restoreRemoved(const QString &hash);
    void clearRemovedHistory();

    // Absolute on-disk root path for the torrent (single file or root folder).
    // Resolves via libtorrent's file_path(0) rather than the torrent's display
    // name, which can drift from disk after rename/sanitization.
    QString torrentRootPath(int index) const;

    // Incomplete downloads path — download to a temp directory, move to the
    // real save path when the torrent finishes. Avoids media servers scanning
    // half-downloaded files and lets users put temp data on a faster drive.
    void setTempPath(const QString &path);
    QString tempPath() const;

    // Content layout: 0=Original, 1=Create subfolder, 2=No subfolder.
    // Controls how multi-file torrents are laid out on disk.
    void setContentLayout(int layout);
    int contentLayout() const;

    // Regex patterns for files to auto-skip (priority 0) on add. One pattern
    // per entry, matched against the file path inside the torrent.
    void setExcludedFilePatterns(const QStringList &patterns);
    QStringList excludedFilePatterns() const;

    // Auto-copy .torrent files to a backup directory when added. Essential
    // for private tracker users who want to keep an archive.
    void setTorrentExportDir(const QString &path);
    QString torrentExportDir() const;

    // Auto-extract archives (RAR/ZIP/7z) when a torrent completes.
    void setAutoExtract(bool enabled);
    bool autoExtract() const;
    void setAutoExtractDelete(bool deleteAfter);
    bool autoExtractDelete() const;
    void setExtractPasswords(const QStringList &passwords);
    QStringList extractPasswords() const;

    // Execute a command when a torrent finishes. Template variables:
    // %N = torrent name, %D = save path, %F = content path (first file),
    // %Z = total size, %H = info hash. Empty = disabled.
    void setRunOnComplete(const QString &command);
    QString runOnComplete() const;
    void executeOnComplete(const QString &name, const QString &savePath,
                           const QString &hash, qint64 totalSize);

    // Watch a directory for .torrent files — auto-add when detected.
    void setWatchedFolder(const QString &path);
    QString watchedFolder() const;

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

    // Advanced libtorrent tuning — exposed in Settings → Advanced.
    // Each setter applies immediately via settings_pack and persists to
    // QSettings so the value survives restarts.
    struct AdvancedSettings {
        int aioThreads = 10;
        int hashingThreads = 2;
        int filePoolSize = 100;
        int checkingMemUsage = 512;   // × 16KB = 8MB default
        int diskIOReadMode = 0;       // 0=EnableOSCache, 1=DisableOSCache
        int diskIOWriteMode = 0;      // 0=EnableOSCache, 1=DisableOSCache, 2=WriteThrough
        int connectionsLimit = 500;
        int connectionSpeed = 30;
        int maxUploadsPerTorrent = 4;
        int maxConnectionsPerTorrent = 100;
        int unchokeSlotsLimit = 20;
        int chokingAlgorithm = 0;     // 0=FixedSlots, 1=RateBased
        int seedChokingAlgorithm = 0; // 0=RoundRobin, 1=FastestUpload, 2=AntiLeech
        int sendBufferWatermark = 500; // KB
        int outgoingPortMin = 0;
        int outgoingPortMax = 0;
        bool rateLimitIpOverhead = false;
        bool ignoreLimitsOnLAN = true;
    };
    AdvancedSettings advancedSettings() const;
    void setAdvancedSettings(const AdvancedSettings &s);

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

    // Asynchronous "save everything" trigger. Each torrent gets a
    // save_resume_data() request; the writes happen in processAlerts when
    // libtorrent delivers save_resume_data_alert for each one. The GUI
    // thread doesn't block.
    void saveResumeData();
    // Synchronous flush used by the destructor / app shutdown. Requests
    // saves for every torrent and runs a bounded wait loop until either
    // every outstanding write returns or the timeout expires.
    void flushResumeDataBlocking(int timeoutMs = 5000);
    void loadResumeData();

    // Global statistics
    qint64 globalDownloaded() const;
    qint64 globalUploaded() const;
    float globalRatio() const;

    // Per-session statistics
    qint64 sessionDownloaded() const;
    qint64 sessionUploaded() const;

    // Detailed session metrics (from libtorrent session_stats)
    struct DetailedStats {
        int dhtNodes = 0;
        int peersCount = 0;
        qint64 totalWasted = 0;       // redundant + failed bytes
        int diskReadQueue = 0;
        int diskWriteQueue = 0;
        bool hasIncomingConnections = false;
    };
    DetailedStats detailedStats() const;

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
    // Rename every file in the torrent's add_torrent_params to have a ".!bt"
    // suffix so media servers ignore in-progress files. Stripped back as
    // each file completes (see file_completed_alert handling).
    void applyIncompleteSuffix(lt::add_torrent_params &atp);
    QString torrentHash(int index) const;
    // Fetch the cached torrent_status for a handle, falling back to a live
    // status() call only when the cache hasn't been populated yet (just after
    // add, before the first state_update_alert).
    lt::torrent_status cachedStatus(const lt::torrent_handle &h) const;
    // true if a torrent with this info-hash is already in m_torrents (used to
    // reject re-adding a duplicate; checks our list, not the lingering
    // libtorrent handle after an async removal).
    bool isDuplicate(const lt::info_hash_t &ih) const;
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
    bool m_utpEnabled = true;
    bool m_anonymousMode = false;
    bool m_forceIpv4 = false;
    bool m_ptMode = false;
    bool m_blockLeechers = false;
    int m_blockedLeecherCount = 0;
    void checkAndBlockLeechers();
    float m_seedRatioLimit = 0.0f; // 0 = no limit

    // Stop-seeding rules (globals)
    bool m_stopAfterDownload = false;
    qint64 m_maxSeedSeconds = 0; // 0 = unlimited

    // User-marked-as-completed info-hashes. Persisted to QSettings under
    // "completedTorrents". A torrent in this set is paused on resume and
    // displayed in the green "completed" state regardless of seeding flags.
    QSet<QString> m_completedTorrents;
    // Info-hashes that bypass the active-downloads queue cap.
    QSet<QString> m_forceStartHashes;
    qint64 m_autoCompleteSeconds = 0; // 0 = disabled
    void saveCompletedSet();
    void checkAutoComplete();

    // Per-torrent overrides (info_hash -> value; -1 = use global)
    QMap<QString, int> m_perTorrentStopAfter;
    QMap<QString, qint64> m_perTorrentMaxSeed;
    // Per-torrent rate caps (info_hash -> KB/s; 0 = unlimited)
    QMap<QString, int> m_perTorrentDownLimit;
    QMap<QString, int> m_perTorrentUpLimit;

    // Number of save_resume_data requests outstanding. processAlerts
    // decrements this as each save_resume_data_alert /
    // save_resume_data_failed_alert lands.
    int m_resumeOutstanding = 0;
    // Per-handle epoch-seconds of last "fast" tick (download_rate above
    // kSlowTorrentThresholdBps). Used by enforceDownloadQueue to skip
    // torrents that haven't transferred meaningfully for >60 s when
    // counting active downloads — a stalled torrent shouldn't permanently
    // hog one of the user's queue slots.
    std::map<lt::torrent_handle, qint64> m_lastFastAt;
    // Per-handle "epoch seconds of last resume save" so piece_finished_alert
    // can request a save without thrashing the disk on every piece. Saved
    // only if the last save was more than kMinResumeSaveIntervalSec ago.
    std::map<lt::torrent_handle, qint64> m_lastResumeSaveAt;
    // Info-hashes of recently removed torrents. Used to drop in-flight
    // save_resume_data_alerts that arrive after removeTorrent, which would
    // otherwise re-create the .resume file we just deleted.
    QSet<QString> m_removedHashes;

    // Handles loaded from resume data that still need a deferred ".!bt"
    // strip pass for files already 100% complete. Calling file_progress()
    // inline right after add_torrent throws "invalid torrent handle" —
    // libtorrent hasn't bound storage yet. We process them in the alert
    // loop the first time state_update_alert delivers a status with the
    // file storage attached, then drop them from the set.
    std::set<lt::torrent_handle> m_pendingResumeStripCheck;
    // Persist a save_resume_data_alert payload to its .resume file under
    // resumeDataDir(). Returns true on success.
    bool persistResumeAlert(const struct lt::save_resume_data_alert *rd);

    // Temp path for incomplete downloads
    QString m_tempPath;
    // Hash -> intended final save path (only when temp path is active)
    QMap<QString, QString> m_torrentIntendedPath;
    // Real info-hash (from the magnet URI) for magnets added this session, so a
    // row has a stable, unique key before metadata arrives — without it,
    // torrentHash() returns empty pre-metadata and the cover/name never resolve.
    std::map<lt::torrent_handle, QString> m_magnetHashes;
    // Hash -> clean-title cover hint (e.g. from a game catalog), consumed once.
    QMap<QString, QString> m_coverHints;

    // Content layout: 0=Original, 1=CreateSubfolder, 2=NoSubfolder
    int m_contentLayout = 0;
    void applyContentLayout(lt::add_torrent_params &atp);

    // Excluded file name patterns (regex)
    QStringList m_excludedFilePatterns;
    void applyExcludedPatterns(lt::add_torrent_params &atp);

    // Torrent export — auto-copy .torrent files to a backup directory
    QString m_torrentExportDir;
    // Run on complete
    QString m_runOnComplete;
    // Watched folder
    QString m_watchedFolder;
    QTimer *m_watchedFolderTimer = nullptr;
    void scanWatchedFolder();
    // Auto-extract
    bool m_autoExtract = false;
    bool m_autoExtractDelete = false;
    QStringList m_extractPasswords;
    void extractArchives(const QString &savePath, const QString &torrentName);

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
    // Per-category save paths (category name -> save path)
    QMap<QString, QString> m_categorySavePaths;
    // Tags per torrent (hash -> list of tag names)
    QMap<QString, QStringList> m_torrentTags;

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
