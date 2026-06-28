// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef QMLSESSIONBRIDGE_H
#define QMLSESSIONBRIDGE_H

#include "bridges/bridgecommon.h"

class QmlSessionBridge : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList watchlist READ watchlist NOTIFY watchlistChanged)   // saved titles ("My List")
    Q_PROPERTY(int torrentCount READ torrentCount NOTIFY statsChanged)
    Q_PROPERTY(int activeCount READ activeCount NOTIFY statsChanged)
    Q_PROPERTY(int downloadingCount READ downloadingCount NOTIFY statsChanged)
    Q_PROPERTY(int seedingCount READ seedingCount NOTIFY statsChanged)
    Q_PROPERTY(int pausedCount READ pausedCount NOTIFY statsChanged)
    Q_PROPERTY(int completedCount READ completedCount NOTIFY statsChanged)
    Q_PROPERTY(QString totalDownSpeed READ totalDownSpeed NOTIFY statsChanged)
    Q_PROPERTY(QString totalUpSpeed READ totalUpSpeed NOTIFY statsChanged)
    Q_PROPERTY(QString freeDiskSpace READ freeDiskSpace NOTIFY statsChanged)
    Q_PROPERTY(double diskUsedFraction READ diskUsedFraction NOTIFY statsChanged)
    // The distinct volumes torrents actually save to (default + per-category
    // paths) — each { name, free, usedFraction }. Multi-HD users see all of them.
    Q_PROPERTY(QVariantList diskVolumes READ diskVolumes NOTIFY statsChanged)
    // currently-downloading torrents (cover/name/%/speed) for the nav-rail mini card
    Q_PROPERTY(QVariantList activeDownloads READ activeDownloads NOTIFY statsChanged)
    // seeding torrents (cover/name/↑speed/ratio) — nav-rail card fallback when nothing's downloading
    Q_PROPERTY(QVariantList seedingTransfers READ seedingTransfers NOTIFY statsChanged)
    // continue watching/playing (resume) — the nav-rail slot's content on the Downloads tab
    Q_PROPERTY(QVariantList resumeItems READ resumeItems NOTIFY statsChanged)
    Q_PROPERTY(QString totalDownloaded READ totalDownloaded NOTIFY statsChanged)
    Q_PROPERTY(QString totalUploaded READ totalUploaded NOTIFY statsChanged)
    Q_PROPERTY(QString globalRatio READ globalRatio NOTIFY statsChanged)
    Q_PROPERTY(QVariantList downloadHistory READ downloadHistory NOTIFY historyChanged)
    Q_PROPERTY(QVariantList uploadHistory READ uploadHistory NOTIFY historyChanged)
    // per-torrent speed history for the SELECTED torrent (in-memory, fills over
    // time) — drives the details-panel background graph.
    Q_PROPERTY(QVariantList selectedDownHistory READ selectedDownHistory NOTIFY historyChanged)
    Q_PROPERTY(QVariantList selectedUpHistory READ selectedUpHistory NOTIFY historyChanged)

    Q_PROPERTY(QString selectedName READ selectedName NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedSize READ selectedSize NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedHash READ selectedHash NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedDownloaded READ selectedDownloaded NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedDownSpeed READ selectedDownSpeed NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedUpSpeed READ selectedUpSpeed NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedEta READ selectedEta NOTIFY selectionChanged)
    Q_PROPERTY(int selectedSeeds READ selectedSeeds NOTIFY selectionChanged)
    Q_PROPERTY(int selectedPeers READ selectedPeers NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedRatio READ selectedRatio NOTIFY selectionChanged)
    Q_PROPERTY(double selectedProgress READ selectedProgress NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedUploaded READ selectedUploaded NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedAvailability READ selectedAvailability NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedAdded READ selectedAdded NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedPath READ selectedPath NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedState READ selectedState NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedPoster READ selectedPoster NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedDescription READ selectedDescription NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedMetaTitle READ selectedMetaTitle NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedMetaInfo READ selectedMetaInfo NOTIFY selectionChanged)
    Q_PROPERTY(bool selectedForceStart READ selectedForceStart NOTIFY selectionChanged)
    Q_PROPERTY(bool selectedSuperSeeding READ selectedSuperSeeding NOTIFY selectionChanged)
    Q_PROPERTY(bool selectedCompleted READ selectedCompleted NOTIFY selectionChanged)
    Q_PROPERTY(bool selectedPaused READ selectedPaused NOTIFY selectionChanged)
    Q_PROPERTY(bool selectedHasArchives READ selectedHasArchives NOTIFY selectionChanged)
    Q_PROPERTY(bool selectedHasVideo READ selectedHasVideo NOTIFY selectionChanged)
    Q_PROPERTY(QVariantList selectedPeerList READ selectedPeerList NOTIFY selectionListsChanged)
    Q_PROPERTY(bool peersLoading READ peersLoading NOTIFY selectionListsChanged)  // true while the first peer build is pending
    Q_PROPERTY(QVariantList selectedFiles READ selectedFiles NOTIFY selectionListsChanged)
    Q_PROPERTY(QVariantList selectedTrackers READ selectedTrackers NOTIFY selectionListsChanged)
    Q_PROPERTY(QVariantMap selectedPieces READ selectedPieces NOTIFY selectionListsChanged)
    Q_PROPERTY(bool hasSelection READ hasSelection NOTIFY selectionChanged)
    Q_PROPERTY(bool altSpeedsActive READ altSpeedsActive NOTIFY altSpeedsActiveChanged)
    Q_PROPERTY(int portStatus READ portStatus NOTIFY portStatusChanged)

public:
    explicit QmlSessionBridge(IEngine *session, MetadataResolver *resolver, QObject *parent = nullptr);

    int torrentCount() const;
    int activeCount() const;
    int downloadingCount() const;
    int seedingCount() const;
    int pausedCount() const;
    int completedCount() const;
    QString totalDownSpeed() const;
    QString totalUpSpeed() const;
    QString freeDiskSpace() const;
    Q_INVOKABLE qint64 freeSaveBytes() const;   // single source of truth: free bytes on the default save volume
    double diskUsedFraction() const;
    QVariantList diskVolumes() const;
    QVariantList activeDownloads() const;
    QVariantList seedingTransfers() const;
    QVariantList resumeItems() const;
    QString totalDownloaded() const;
    QString totalUploaded() const;
    QString globalRatio() const;
    QVariantList downloadHistory() const;
    QVariantList uploadHistory() const;
    QVariantList selectedDownHistory() const;
    QVariantList selectedUpHistory() const;

    Q_INVOKABLE void setSelectedRows(const QList<int> &rows);
    // Select a torrent by its info hash (source row). Returns false if it isn't
    // in the model yet — used to focus a torrent just added from Search.
    Q_INVOKABLE bool selectByInfoHash(const QString &infoHash);
    // Keep the stored selection (source-model indices) in sync when a torrent is
    // removed, so a later batch action doesn't hit a different torrent that
    // shifted into the old index.
    void onTorrentRemoved(int index);
    Q_INVOKABLE QList<int> selectedRows() const;
    Q_INVOKABLE void pauseSelected();
    Q_INVOKABLE void resumeSelected();
    Q_INVOKABLE void removeSelected();
    Q_INVOKABLE void removeSelectedWithFiles();
    Q_INVOKABLE void removeSelectedWithFilesPermanent();   // hard delete, skips Trash
    void removeSelectedRows(bool deleteFiles, bool permanent = false);
    Q_INVOKABLE void pauseAll();
    Q_INVOKABLE void resumeAll();
    Q_INVOKABLE void addTorrentFile(const QString &filePath);
    // Ask the UI to add a .torrent via the add dialog (used by the file-
    // association / CLI / single-instance paths in main.cpp).
    void requestAddTorrentFile(const QString &filePath);
    Q_INVOKABLE void addMagnetUri(const QString &uri, const QString &savePath = QString());
    // Fetch a .torrent from an http(s) URL, then route it through the add flow.
    Q_INVOKABLE void addTorrentUrl(const QString &url);
    bool altSpeedsActive() const;
    int portStatus() const;
    Q_INVOKABLE void setAltSpeedsActive(bool active);
    Q_INVOKABLE QVariantMap previewTorrent(const QString &filePath) const;
    Q_INVOKABLE void resolvePreview(const QString &infoHash, const QString &name);
    Q_INVOKABLE void addTorrentWithPrefs(const QString &filePath, const QString &savePath,
                                         const QVariantList &priorities);
    Q_INVOKABLE void copyMagnetLink();
    Q_INVOKABLE void copyInfoHash();
    Q_INVOKABLE void openSaveFolder();
    Q_INVOKABLE bool excludeTorrentFromDefender(int row);   // Windows: exclude this torrent's folder
    Q_INVOKABLE void extractSelected(const QString &password);   // manual extract (empty pw allowed)
    Q_INVOKABLE QString defaultSavePath() const;
    Q_INVOKABLE void setSelectedForceStart(bool on);
    Q_INVOKABLE void setSelectedSuperSeeding(bool on);
    Q_INVOKABLE void markSelectedCompleted();
    Q_INVOKABLE void unmarkSelectedCompleted();
    Q_INVOKABLE void forceRecheckSelected();
    Q_INVOKABLE bool exportSelectedTorrent(const QString &destPath);
    Q_INVOKABLE void forceReannounceSelected();
    Q_INVOKABLE void queueUpSelected();
    Q_INVOKABLE void queueDownSelected();
    Q_INVOKABLE void queueTopSelected();
    Q_INVOKABLE void queueBottomSelected();
    Q_INVOKABLE void stopSeedingSelected();
    Q_INVOKABLE void smartPaste();

    // Convert a FileDialog url ("file:///C:/x" / "file:///x") to a native local
    // path. QML's naive `replace(/^file:\/\//,"")` left a leading slash before
    // the Windows drive letter ("/C:/x"), breaking every picker on Windows.
    Q_INVOKABLE QString urlToLocalPath(const QString &url) const;

    // per-torrent edits (wrap SessionManager against the current selection)
    Q_INVOKABLE void moveSelectedStorage(const QString &path);
    Q_INVOKABLE void setSelectedDownloadLimit(int kbps);
    Q_INVOKABLE void setSelectedUploadLimit(int kbps);
    Q_INVOKABLE void setSelectedSequential(bool on);
    Q_INVOKABLE bool selectedSequential() const;
    // per-torrent seed rules: stopAfter -1=use default,0=off,1=on; maxSeedDays -1=default,0=unlimited
    Q_INVOKABLE void setSelectedStopAfter(int mode);
    Q_INVOKABLE int  selectedStopAfter() const;
    Q_INVOKABLE void setSelectedMaxSeedDays(int days);
    Q_INVOKABLE int  selectedMaxSeedDays() const;
    Q_INVOKABLE void renameSelected(const QString &name);   // rename the torrent (file 0)
    Q_INVOKABLE QString diagnoseSelectedSlow() const;       // "why is this slow" report
    Q_INVOKABLE void streamSelected();                      // prioritize video + open EXTERNAL player
    Q_INVOKABLE void playSelected();                        // prioritize video + open the embedded player
    // Local-stream URL for a torrent's best video file (preps priorities too);
    // empty if no video / no stream server / metadata not ready.
    Q_INVOKABLE QString streamUrl(int row);
    // Hand off to the OS media player for the exact file being watched.
    Q_INVOKABLE void openExternalForHash(const QString &infoHash, int fileIndex);
    // HUB (movies): the library of watchable video torrents (completed or with
    // enough buffered to start), each with its best video file, cover, download
    // progress and resume/watched position.
    Q_INVOKABLE QVariantList movieLibrary() const;
    // Open a library item in the embedded player (resolves row from info hash).
    Q_INVOKABLE void playByHash(const QString &infoHash);
    Q_INVOKABLE void playByHashFile(const QString &infoHash, int fileIndex);   // resume a specific episode
    // Next episode's file index after fileIndex (season/episode order), or -1.
    Q_INVOKABLE int nextEpisode(const QString &infoHash, int fileIndex) const;
    // Play a specific video file (episode) of a torrent in the embedded player.
    Q_INVOKABLE void playFile(const QString &infoHash, int fileIndex);
    // Forget a movie's resume position (drops it from HUB "Continue watching").
    Q_INVOKABLE void clearResume(const QString &infoHash, int fileIndex);
    // Player buffer bar + download badge: { totalBytes, downloadedBytes, progress, buffered }.
    Q_INVOKABLE QVariantMap streamFileStats(const QString &infoHash, int fileIndex) const;
    // Raw video file name (has the quality/audio tags the player badges parse).
    Q_INVOKABLE QString streamFileName(const QString &infoHash, int fileIndex) const;
    // Watchlist ("My List") — saved titles (not torrents), persisted in QSettings.
    QVariantList watchlist() const;
    Q_INVOKABLE bool inWatchlist(const QString &title, const QString &type) const;
    Q_INVOKABLE void toggleWatchlist(const QVariantMap &item);   // {title,type,poster,year}
    // HUB (games) — minimal: list game torrents (cover/progress) and launch them
    // via a user-set executable (no auto-detection yet — improved gradually).
    Q_INVOKABLE QVariantList gameLibrary() const;
    Q_INVOKABLE QString gameExe(const QString &infoHash) const;        // saved exe path, "" if unset
    Q_INVOKABLE void setGameExe(const QString &infoHash, const QString &fileUrl);
    Q_INVOKABLE QString gameFolder(const QString &infoHash) const;     // for the picker's start dir
    Q_INVOKABLE void launchGame(const QString &infoHash);              // run saved exe, else open folder
    // One-click install: extract (if needed) → detect exe → register, or drive the
    // installer (silent where the engine allows, guided otherwise). Drives the
    // card's install-state machine surfaced via gameLibrary()'s "installState".
    Q_INVOKABLE void installGame(const QString &infoHash);
    // Downloads-list surface: the same install/play action on the selected row.
    Q_INVOKABLE bool selectedIsGame() const;
    Q_INVOKABLE int selectedGameState() const;       // GameInstallState, -1 if not a game
    Q_INVOKABLE void installSelectedGame();
    Q_INVOKABLE void playSelectedGame();
    void setStreamPort(quint16 port) { m_streamPort = port; }
    Q_INVOKABLE void setSelectedCategory(const QString &category);
    Q_INVOKABLE void setSelectedTags(const QStringList &tags);
    Q_INVOKABLE void addTrackerToSelected(const QString &url);
    Q_INVOKABLE void removeTrackerFromSelected(const QString &url);
    Q_INVOKABLE void renameSelectedFile(int fileIndex, const QString &newName);
    Q_INVOKABLE void setSelectedFilePriority(int fileIndex, int priority);
    Q_INVOKABLE void copySelectedName();
    Q_INVOKABLE void openSelectedFile();
    Q_INVOKABLE void copySelectedContentPath();
    Q_INVOKABLE QVariantList torrentPalette() const;
    Q_INVOKABLE QVariantList loadSubtitleFile(const QString &path);
    Q_INVOKABLE QString findSidecarSubtitle(const QString &infoHash, int fileIndex);
    // Manual cover/title fix for the selected torrent when the auto match was
    // wrong. typeStr = "movie" | "series" | "game".
    Q_INVOKABLE void relinkSelectedCover(const QString &query, const QString &typeStr);
    Q_INVOKABLE void clearSelectedCover();
    Q_INVOKABLE void importQbittorrent(const QString &savePath);
    // Create a .torrent (mirrors the validated synchronous logic from
    // createtorrentdialog.cpp). Returns "" on success, else an error message.
    // opts keys: source, output, trackers, pieceSize(bytes,0=auto), comment, priv, startSeeding.
    Q_INVOKABLE QString createTorrent(const QVariantMap &opts);
    Q_INVOKABLE QString suggestTorrentOutput(const QString &source) const;

    // Statistics window
    Q_INVOKABLE QVariantMap statistics() const;
    Q_INVOKABLE QVariantMap wrapped(int year) const;   // "Year in Torrents"
    // Diagnostics window (snapshot of listen/DHT/NAT/port reachability)
    Q_INVOKABLE QVariantMap diagnostics() const;
    // Removed-history window
    Q_INVOKABLE QVariantList recentlyRemoved() const;
    Q_INVOKABLE bool restoreRemoved(const QString &hash);
    Q_INVOKABLE void clearRemovedHistory();
    // Generic clipboard copy (inspector full hash, etc.)
    Q_INVOKABLE void copyToClipboard(const QString &text);

    // selection-state + global lists for the context menu
    Q_INVOKABLE int selectedDownloadLimit() const;
    Q_INVOKABLE int selectedUploadLimit() const;
    Q_INVOKABLE QString selectedCategory() const;
    Q_INVOKABLE QStringList selectedTagList() const;
    Q_INVOKABLE QStringList categories() const;

    QString selectedName() const;
    QString selectedSize() const;
    QString selectedHash() const;
    QString selectedDownloaded() const;
    QString selectedDownSpeed() const;
    QString selectedUpSpeed() const;
    QString selectedEta() const;
    int selectedSeeds() const;
    int selectedPeers() const;
    QString selectedRatio() const;
    double selectedProgress() const;
    QString selectedUploaded() const;
    QString selectedAvailability() const;
    QString selectedAdded() const;
    QString selectedPath() const;
    QString selectedState() const;
    QString selectedPoster() const;
    QString selectedDescription() const;
    QString selectedMetaTitle() const;
    QString selectedMetaInfo() const;
    bool selectedForceStart() const;
    bool selectedSuperSeeding() const;
    bool selectedCompleted() const;
    bool selectedPaused() const;
    bool selectedHasArchives() const;
    bool selectedHasVideo() const;
    QVariantList selectedPeerList() const;
    bool peersLoading() const { return m_peersLoading; }
    QVariantList selectedFiles() const;
    QVariantList selectedTrackers() const;
    QVariantMap selectedPieces() const;
    bool hasSelection() const;

    // The Peers detail tab is expensive (pulls every peer from libtorrent), so the
    // peer list only refreshes per tick while that tab is actually open.
    Q_INVOKABLE void setDetailPeersActive(bool active);

    void emitStats();

    // Auto-shutdown: power off the machine after all downloads complete (the QML
    // side shows a cancelable countdown, then calls performShutdown()).
    Q_INVOKABLE void performShutdown();

    // Get & Watch: after a release is added, watch its hash and auto-open the
    // player once it's buffered enough. cancelWatch drops a pending request.
    Q_INVOKABLE void watchWhenReady(const QString &infoHash, const QString &title);
    Q_INVOKABLE void cancelWatch(const QString &infoHash);

signals:
    void statsChanged();
    void watchlistChanged();
    void selectionChanged();
    // Heavy per-selection lists (peers/files/trackers/pieces) notify on this
    // instead of selectionChanged, which fires every ~1s from emitStats() and
    // would otherwise rebuild those big lists every second.
    void selectionListsChanged();
    void historyChanged();
    void queueRefreshNeeded();
    void queueMoved(int from, int to);
    void previewPosterReady(const QString &infoHash, const QString &posterPath);
    void allDownloadsComplete();   // fired once when the last active download finishes
    void toast(const QString &title, const QString &body);   // in-app toast (stream feedback, etc.)
    void openPlayer(const QString &url, const QString &title, const QString &infoHash, int fileIndex);
    void watchBuffering(const QString &title);   // Get&Watch: added, downloading until playable
    void watchProgress(const QString &infoHash, double percent);   // 0..1, each tick while pending
    void watchFailed(const QString &title);      // Get&Watch: gave up (no seeds / no metadata)
    void gamesChanged();   // a game started/stopped → refresh the game library
    void movieReady(const QString &infoHash, const QString &name);   // a movie/series finished → offer Play now
    // A .torrent arrived from outside the UI (file association, CLI, second
    // instance). QML routes it through the same add dialog as a drag-drop so
    // the user always picks save path / files — never a silent auto-download.
    void openTorrentRequested(const QString &path);
    void altSpeedsActiveChanged();
    void portStatusChanged();

private slots:
    void sampleSpeeds();
    void onWatchTick();   // poll pending Get&Watch hashes; open the player when buffered
    void onExtractionCompleted(const QString &infoHash, bool success);  // chain → detect/install
    void onGameTorrentFinished(const QString &name, const QString &infoHash);  // auto-install hook

private:
    // States surfaced to the game card as "installState" (int). Downloading/Ready/
    // Playing are derived from torrent + run state; the rest are transient overlays
    // held in m_gameInstallState while an operation is in flight.
    enum GameInstallState {
        GIS_Downloading = 0,   // still downloading
        GIS_ReadyToInstall,    // download done, not installed yet → "Install"
        GIS_Extracting,        // unpacking archives
        GIS_Installing,        // installer running (silent) or awaiting user ("finish setup")
        GIS_Ready,             // exe known → "Play"
        GIS_Playing,           // process alive
        GIS_NeedsSetup,        // couldn't find an exe → "Set up game"
        GIS_Failed,            // extraction failed → "Retry"
    };
    int gameInstallState(const QString &infoHash, bool completed) const;
    bool isGameTorrent(int row) const;               // game detection (metadata → parser → exe/no-video)
    void finalizeInstall(const QString &infoHash);   // post-extract: detect exe or run installer
    void runInstaller(const QString &infoHash, const QString &installerExe, const QString &folder);
    void pollInstallWatch();   // guided installs: watch the folder for a produced exe

    IEngine *m_session;   // the session API — SessionManager in-process today, IpcEngine after the split
    QHash<QString, QPair<QString, qint64>> m_pendingWatch;   // infoHash → {title, startedAtSec}
    QHash<QString, qint64> m_runningGames;   // infoHash → pid of a launched (detached) game
    QHash<QString, qint64> m_gameStartMs;    // infoHash → launch timestamp (ms)
    QHash<QString, int> m_gameInstallState;  // infoHash → transient GameInstallState overlay
    QHash<QString, qint64> m_installWatch;   // infoHash → pid of a guided installer being watched
    QSet<QString> m_extracted;               // infoHashes already extracted (avoid re-extracting)
    void pollRunningGames();   // detect game exit → record playtime
    MetadataResolver *m_resolver;
    quint16 m_streamPort = 0;
    int m_selectedIndex = -1;
    QList<int> m_selectedRows;
    GeoIpResolver *m_geoIp = nullptr;
    QTimer m_peerListThrottle;      // coalesce geo-lookup results into ≤1 peer-list rebuild/sec
    bool m_detailPeersActive = false;   // true only while the Peers detail tab is open
    bool m_peersLoading = false;        // first peer build pending (show placeholder)
    QVariantList m_peerCache;           // built off the click / per tick, not on the QML binding path
    void rebuildPeerCache();            // heavy peersAt build → cache (deferred, never on the click)
    bool m_shutdownArmed = false;   // debounce so the countdown fires once per drain
    QTimer *m_streamTimer = nullptr;   // polls until a streamed file is buffered
    QString m_streamFilePath;
    int m_streamIndex = -1;
    int m_streamFileIdx = -1;
    int m_streamTries = 0;          // stop polling for buffer after a while (dead torrent)

    QTimer m_sampleTimer;
    QVector<int> m_downloadHistory;
    QVector<int> m_uploadHistory;
    QHash<QString, QVector<int>> m_torrentDownHist;   // per-torrent rate rings (by hash)
    QHash<QString, QVector<int>> m_torrentUpHist;

    // Counts/totals recomputed in one pass per stats tick (recomputeAggregates)
    // so the seven count/speed getters don't each re-loop the whole library on
    // every QML read (~10 O(torrents) passes/sec → 1).
    void recomputeAggregates();
    int m_activeCount = 0, m_downloadingCount = 0, m_seedingCount = 0,
        m_pausedCount = 0, m_completedCount = 0;
    int m_totalDownRate = 0, m_totalUpRate = 0;
    bool m_anyDownloading = false;
    static constexpr int HistoryMaxPoints = 60;
};

#endif // QMLSESSIONBRIDGE_H
