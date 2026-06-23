// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef QMLPOSTERBRIDGE_H
#define QMLPOSTERBRIDGE_H

#include <QAbstractListModel>
#include <QColor>
#include <QIcon>
#include <QPixmap>
#include <QSortFilterProxyModel>
#include <QString>
#include <QSet>
#include <QTimer>
#include <QVariantList>
#include <QVector>
#include <QSettings>

#include "../app/addonmanager.h"   // CatalogItem / StreamResult / TorrentSearchResult
#include "../app/gamesourcemanager.h"   // GameDownload
#include "../app/translator.h"

class SessionManager;
class MetadataResolver;
class DiscoveryService;
class GeoIpResolver;
class TelegramNotifier;
class WebServer;
class QWindow;
class QEvent;

class QmlPosterModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Roles {
        NameRole = Qt::UserRole + 1,
        StateKeyRole,
        InfoHashRole,
        ProgressRole,
        PosterPathRole,
        MetaTitleRole,
        StateStringRole,
        StateDetailRole,
        DownSpeedRole,
        UpSpeedRole,
        SizeRole,
        CategoryRole,
        NumPeersRole,
        DownRateRole,
        UpRateRole,
        SizeBytesRole
    };

    explicit QmlPosterModel(SessionManager *session, MetadataResolver *resolver,
                            QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

public slots:
    void refresh();                         // periodic tick: volatile roles only
    void refreshFull();                     // explicit edits: all roles
    void removeRow(int index);              // index-aware delete (no full reset)
    void posterResolved(const QString &hash); // one row's poster/title only
    void moveRow(int from, int to);

private:
    void syncCount();                       // insert/remove rows to match session
    void emitRows(bool fullRoles);          // dataChanged over the whole list
    SessionManager *m_session;
    MetadataResolver *m_resolver;
    int m_lastCount = 0;
};

class QmlTorrentFilterProxy : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit QmlTorrentFilterProxy(QObject *parent = nullptr);

    Q_INVOKABLE void setFilterState(const QString &state);
    Q_INVOKABLE void setCategoryFilter(const QString &category);
    Q_INVOKABLE void setSearchText(const QString &text);
    Q_INVOKABLE void setSortColumn(const QString &column, bool ascending);
    Q_INVOKABLE void clearSort();
    Q_INVOKABLE int mapToSource(int proxyRow) const;
    Q_INVOKABLE int mapFromSource(int sourceRow) const;

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
    bool lessThan(const QModelIndex &l, const QModelIndex &r) const override;

private:
    QString m_filterState = QStringLiteral("all");
    QString m_categoryFilter;
    QString m_searchText;
    QString m_sortColumn;
};

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
    Q_PROPERTY(QString totalDownloaded READ totalDownloaded NOTIFY statsChanged)
    Q_PROPERTY(QString totalUploaded READ totalUploaded NOTIFY statsChanged)
    Q_PROPERTY(QString globalRatio READ globalRatio NOTIFY statsChanged)
    Q_PROPERTY(QVariantList downloadHistory READ downloadHistory NOTIFY historyChanged)
    Q_PROPERTY(QVariantList uploadHistory READ uploadHistory NOTIFY historyChanged)

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
    Q_PROPERTY(QVariantList selectedPieces READ selectedPieces NOTIFY selectionListsChanged)
    Q_PROPERTY(bool hasSelection READ hasSelection NOTIFY selectionChanged)
    Q_PROPERTY(bool altSpeedsActive READ altSpeedsActive NOTIFY altSpeedsActiveChanged)
    Q_PROPERTY(int portStatus READ portStatus NOTIFY portStatusChanged)

public:
    explicit QmlSessionBridge(SessionManager *session, MetadataResolver *resolver, QObject *parent = nullptr);

    int torrentCount() const;
    int activeCount() const;
    int downloadingCount() const;
    int seedingCount() const;
    int pausedCount() const;
    int completedCount() const;
    QString totalDownSpeed() const;
    QString totalUpSpeed() const;
    QString freeDiskSpace() const;
    QString totalDownloaded() const;
    QString totalUploaded() const;
    QString globalRatio() const;
    QVariantList downloadHistory() const;
    QVariantList uploadHistory() const;

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
    void removeSelectedRows(bool deleteFiles);
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
    // Play a specific video file (episode) of a torrent in the embedded player.
    Q_INVOKABLE void playFile(const QString &infoHash, int fileIndex);
    // Forget a movie's resume position (drops it from HUB "Continue watching").
    Q_INVOKABLE void clearResume(const QString &infoHash, int fileIndex);
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
    QVariantList selectedPieces() const;
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
    void watchFailed(const QString &title);      // Get&Watch: gave up (no seeds / no metadata)
    // A .torrent arrived from outside the UI (file association, CLI, second
    // instance). QML routes it through the same add dialog as a drag-drop so
    // the user always picks save path / files — never a silent auto-download.
    void openTorrentRequested(const QString &path);
    void altSpeedsActiveChanged();
    void portStatusChanged();

private slots:
    void sampleSpeeds();
    void onWatchTick();   // poll pending Get&Watch hashes; open the player when buffered

private:
    SessionManager *m_session;
    QHash<QString, QPair<QString, qint64>> m_pendingWatch;   // infoHash → {title, startedAtSec}
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

class QmlThemeBridge : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString themeName READ themeName WRITE setThemeName NOTIFY changed)
    Q_PROPERTY(bool followSystem READ followSystem WRITE setFollowSystem NOTIFY changed)
    Q_PROPERTY(bool anime READ anime WRITE setAnime NOTIFY changed)
    // Custom theme (name === "custom"): a list of user profiles, each a full
    // palette (6 colors + bg image + opacity). The active one drives Theme.qml.
    Q_PROPERTY(QVariantList customProfiles READ customProfiles NOTIFY profilesChanged)
    Q_PROPERTY(int activeProfile READ activeProfile WRITE setActiveProfile NOTIFY changed)
    // Active-profile colors, read by Theme.qml (no per-token signal needed —
    // changed() re-evaluates all of them).
    Q_PROPERTY(QString cBg        READ cBg        NOTIFY changed)
    Q_PROPERTY(QString cPanel     READ cPanel     NOTIFY changed)
    Q_PROPERTY(QString cText      READ cText      NOTIFY changed)
    Q_PROPERTY(QString cPrimary   READ cPrimary   NOTIFY changed)
    Q_PROPERTY(QString cSecondary READ cSecondary NOTIFY changed)
    Q_PROPERTY(QString cTertiary  READ cTertiary  NOTIFY changed)
    Q_PROPERTY(QString cBgImage   READ cBgImage   NOTIFY changed)
    Q_PROPERTY(int     cBgOpacity READ cBgOpacity NOTIFY changed)
    // True when the OS taskbar/tray is in light mode (for logo contrast).
    Q_PROPERTY(bool osLight READ osLight NOTIFY osSchemeChanged)
    Q_PROPERTY(QString appVersion READ appVersion CONSTANT)
    // App-icon picker — independent of the UI theme (issue #15). Swaps the live
    // Dock (macOS) / taskbar+window (Windows) icon; does NOT touch the signed
    // bundle's Finder/.exe icon.
    Q_PROPERTY(QString appIconChoice READ appIconChoice WRITE setAppIcon NOTIFY appIconChanged)

public:
    explicit QmlThemeBridge(QObject *parent = nullptr);

    QString themeName() const;
    void setThemeName(const QString &n);
    bool followSystem() const;
    void setFollowSystem(bool on);
    bool anime() const;
    void setAnime(bool on);

    // ---- custom profiles ----
    QVariantList customProfiles() const;       // list of {name,bg,panel,text,primary,secondary,tertiary,image,opacity}
    int activeProfile() const;
    void setActiveProfile(int i);
    Q_INVOKABLE int addProfile();              // appends a default profile, returns its index
    Q_INVOKABLE void removeProfile(int i);     // keeps at least one profile
    Q_INVOKABLE void renameProfile(int i, const QString &name);
    Q_INVOKABLE void setProfileColor(int i, const QString &role, const QString &hex);  // role: bg/panel/text/primary/secondary/tertiary
    Q_INVOKABLE void setProfileImage(int i, const QString &path);
    Q_INVOKABLE void setProfileOpacity(int i, int pct);

    // active-profile accessors (Theme.qml)
    QString cBg() const;
    QString cPanel() const;
    QString cText() const;
    QString cPrimary() const;
    QString cSecondary() const;
    QString cTertiary() const;
    QString cBgImage() const;
    int cBgOpacity() const;

    bool osLight() const;
    QString appVersion() const;
    Q_INVOKABLE QString releaseNotes() const;     // changelog HTML (shared with legacy dialog)
    Q_INVOKABLE QVariantList libraries() const;   // [{nm,v}] real linked-library versions
    Q_INVOKABLE void markBootHealthy() const;     // UI is up → clear the boot-crash sentinel
    QIcon trayIcon() const;   // logo recolored for the current OS scheme

    // ---- app-icon picker ----
    QString appIconChoice() const;
    Q_INVOKABLE void setAppIcon(const QString &key);   // persist + apply live
    Q_INVOKABLE QVariantList appIcons() const;         // [{key,label,preview}]
    void applySavedAppIcon();                          // startup: apply a saved non-default pick
    static QIcon iconForKey(const QString &key);       // multi-size icon from a bundled png

    // SVG-swap logo renderer (shared with the image provider). darkBody=true
    // swaps the off-white body to dark text for light backgrounds.
    static QPixmap renderLogo(bool darkBody, int size, qreal dpr = 1.0);

signals:
    void changed();
    void profilesChanged();
    void osSchemeChanged();
    void appIconChanged();

protected:
    // Catch each top-level window's Show to paint its native title bar dark/light
    // to match the app theme (Windows leaves it white otherwise).
    bool eventFilter(QObject *o, QEvent *e) override;

private:
    void loadProfiles();
    void saveProfiles();
    QVariantMap activeMap() const;
    static QVariantMap defaultProfile(const QString &name);
    bool darkTitleBar() const;                     // true unless a light-ish theme
    static void applyTitleBar(QWindow *w, bool dark);
    void refreshTitleBars();                       // re-apply to all open windows

    QString m_themeName;
    bool m_followSystem = false;
    bool m_anime = true;
    QVariantList m_profiles;
    int m_activeProfile = 0;
    bool m_osLight = false;
    QString m_appIconChoice;
    void applyAppIcon(const QString &key);   // sets QGuiApplication window icon
};

// Bridge for RSS feeds (wraps RssManager singleton).
class QmlRssBridge : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList feeds READ feeds NOTIFY feedsChanged)
public:
    explicit QmlRssBridge(QObject *parent = nullptr);

    QVariantList feeds() const;
    Q_INVOKABLE QVariantList itemsForFeed(int feedIndex) const;
    Q_INVOKABLE void addFeed(const QString &url);
    Q_INVOKABLE void removeFeed(int index);
    Q_INVOKABLE void setFeedEnabled(int index, bool enabled);
    Q_INVOKABLE void setAutoDownload(int index, bool on);
    Q_INVOKABLE void checkAllFeeds();
    Q_INVOKABLE void checkFeed(int index);
    Q_INVOKABLE void downloadItem(int feedIndex, int itemIndex);
    Q_INVOKABLE void updateFeedSettings(int index, const QString &filterPattern,
                                        const QString &savePath, int checkInterval,
                                        bool enabled, bool autoDownload);

signals:
    void feedsChanged();
    void errorOccurred(const QString &message);
    void autoDownloaded(const QString &feedName, const QString &itemTitle);
};

// Bridge for the Search window (wraps AddonManager's 4 search modes).
class QmlSearchBridge : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList sources READ sources NOTIFY sourcesChanged)
    Q_PROPERTY(QVariantList categories READ categories CONSTANT)
    Q_PROPERTY(QVariantList results READ results NOTIFY resultsChanged)
    Q_PROPERTY(QString activeQuery READ activeQuery NOTIFY resultsChanged) // for client-side relevance ranking
    Q_PROPERTY(QString mode READ mode NOTIFY modeChanged)        // titles|catalog|streams|torrent|games
    Q_PROPERTY(bool inStreams READ inStreams NOTIFY modeChanged)
    Q_PROPERTY(bool canGoBack READ canGoBack NOTIFY modeChanged) // sources view reachable from a title/catalog
    Q_PROPERTY(bool singleTitleView READ singleTitleView NOTIFY modeChanged) // list is one picked title → drop per-row covers
    Q_PROPERTY(bool searching READ searching NOTIFY searchingChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusChanged)
public:
    explicit QmlSearchBridge(SessionManager *session, QObject *parent = nullptr);

    QVariantList sources() const;
    QVariantList categories() const;
    QVariantList results() const;
    QString activeQuery() const;
    QString mode() const;
    bool inStreams() const;
    bool canGoBack() const;
    bool singleTitleView() const;
    bool searching() const;
    QString statusText() const;

    Q_INVOKABLE void refreshSources();
    Q_INVOKABLE void search(const QString &sourceKey, const QString &query, int categoryCode = 0);
    Q_INVOKABLE void activateResult(int index);   // catalog→streams; else add magnet
    Q_INVOKABLE void back();                       // streams → catalog

    // Hydra-format game catalogs the user adds (neutral infra — nothing bundled).
    Q_INVOKABLE QVariantList gameSources() const;          // [{name, url}]
    Q_INVOKABLE void addGameSource(const QString &name, const QString &url);
    Q_INVOKABLE void removeGameSource(const QString &url);
    Q_INVOKABLE void refreshGames();

    // Lazily resolve a TMDB cover for a torrent row (by its info hash) as it
    // scrolls into view — mirrors the Downloads grid's on-demand resolution.
    void setResolver(MetadataResolver *r);
    Q_INVOKABLE void resolveCover(int index);

    // Title-first search (default "Tudo"): resolve the query to real works via
    // DiscoveryService, then drill into a picked title's torrents.
    void setDiscovery(DiscoveryService *d);
    Q_INVOKABLE void searchRaw();   // escape hatch: flat aggregate over every source
    Q_INVOKABLE void copyMagnet(int index);   // copy a flat result's magnet to the clipboard
    // One-click "Get & Watch": search the title, auto-pick the best release, add
    // it, then hand the hash off (prepareAndWatch) so the player opens when it
    // buffers. Movie/series only — games are handled separately.
    Q_INVOKABLE void getAndWatch(const QString &title, const QString &year, const QString &type);

signals:
    void sourcesChanged();
    void resultsChanged();
    void modeChanged();
    void searchingChanged();
    void statusChanged();
    void gameSourcesChanged();
    void coverReady(const QString &infoHash, const QString &posterPath);
    void addedTorrent(const QString &infoHash);   // a magnet was added from Search
    void watchSearching(const QString &title);    // Get&Watch: started looking for a release
    void watchNoRelease(const QString &title);    // Get&Watch: nothing usable found
    void prepareAndWatch(const QString &infoHash, const QString &title);   // added → buffer & open

private:
    void setSearching(bool on);
    void setStatus(const QString &s);
    void setMode(const QString &m);
    void runGameSearch(const QString &query);
    QSet<QString> currentResultKeys() const;
    void appendGameRows(const QList<GameDownload> &games);
    void appendTorrentRows(const QList<TorrentSearchResult> &results);
    void finishAggregateSource();
    static QString detectRepacker(const QString &name);
    // Parse quality/source/codec/hdr/lang tokens out of a release name for filtering.
    static void fillMediaAttrs(QVariantMap &m, const QString &name);
    static QString appLangCode();   // the app UI language as a release-tag code (PT/EN/…)
    // Flat aggregate search over every enabled source (the old "Tudo" behavior).
    void rawAggregateSearch(const QString &q, int categoryCode);
    // Type-scoped drill-down for a picked title: games hit game catalogs + the
    // games category; movies/series hit video torrents only (no game catalogs).
    void searchSourcesForWork(const QString &title, const QString &year, const QString &type);
    int pickBestResult() const;       // index of the best release in m_results, or -1
    void gwResolve();                 // Get&Watch: pick + add once the search settles

    bool m_gwActive = false;          // a Get&Watch search is in flight
    QString m_gwTitle, m_gwType;

    MetadataResolver *m_resolver = nullptr;
    DiscoveryService *m_discovery = nullptr;
    QString m_streamHintPoster;         // activated catalog item's poster, for stream rows
    QVariantList m_titleCache;          // works grid, restored when leaving the sources view
    bool m_fromTitles = false;          // sources view was opened by picking a title
    bool m_titleSources = false;        // current flat list is one picked title's torrents
    QString m_titleQuery;               // the free-text query behind the current titles
    QString m_activeQuery;              // effective query of the current flat list (for relevance)

    SessionManager *m_session;
    QString m_mode;
    QString m_savePath;
    QString m_lastQuery;
    bool m_searching = false;
    bool m_isGameSearch = false;
    bool m_aggregate = false;          // "Tudo": merge every source into one list
    int m_pendingSources = 0;          // async sources still running in aggregate
    QString m_status;
    QVariantList m_results;
    QStringList m_resultMagnets;        // magnet per flat result row (add target)
    QStringList m_resultTitles;         // clean game title per row (cover hint), "" for torrents
    QList<CatalogItem> m_catalogCache;
    QList<StreamResult> m_streamCache;
    QList<TorrentSearchResult> m_torrentCache;
    QList<GameDownload> m_gameCache;
    QString m_pendingGameQuery;
    QString m_streamHintTitle;          // parent catalog title for a Stremio stream add
    int m_streamHintType = -1;          // its ContentType (Movie/Series) as int
    QVariantList m_catalogResultsSnapshot;
};

// Bridge for community addons (wraps AddonManager singleton).
class QmlAddonBridge : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList installed READ installed NOTIFY changed)
    Q_PROPERTY(QVariantList suggested READ suggested NOTIFY changed)
    Q_PROPERTY(bool autoTrackers READ autoTrackers WRITE setAutoTrackers NOTIFY changed)
    Q_PROPERTY(int trackerCount READ trackerCount NOTIFY changed)
    Q_PROPERTY(bool torrentSearchEnabled READ torrentSearchEnabled WRITE setTorrentSearchEnabled NOTIFY changed)
    Q_PROPERTY(QString torrentSearchUrl READ torrentSearchUrl WRITE setTorrentSearchUrl NOTIFY changed)
public:
    explicit QmlAddonBridge(QObject *parent = nullptr);

    QVariantList installed() const;
    QVariantList suggested() const;
    bool autoTrackers() const;
    void setAutoTrackers(bool on);
    int trackerCount() const;
    bool torrentSearchEnabled() const;
    void setTorrentSearchEnabled(bool on);
    QString torrentSearchUrl() const;
    void setTorrentSearchUrl(const QString &url);

    Q_INVOKABLE void addAddon(const QString &url);
    Q_INVOKABLE void removeAddon(int index);
    Q_INVOKABLE void setEnabled(int index, bool on);
    Q_INVOKABLE void refreshTrackers();
    Q_INVOKABLE bool isInstalled(const QString &url) const;

signals:
    void changed();
    void error(const QString &message);
};

// WebUI pairing backend. Presentational (mirrors pairingdialog.cpp): detect LAN
// IP, build the http URL for the configured WebUI port, render a QR matrix.
class QmlPairingBridge : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString url READ url NOTIFY changed)
    Q_PROPERTY(bool available READ available NOTIFY changed)
public:
    explicit QmlPairingBridge(QObject *parent = nullptr);

    QString url() const { return m_url; }
    bool available() const { return !m_url.isEmpty(); }

    // Recompute the URL from current settings (WebUI port + LAN IP). The dialog
    // calls this on open so enabling the WebUI / changing the port reflects
    // without a restart.
    Q_INVOKABLE void refresh();
    Q_INVOKABLE void copyUrl();
    Q_INVOKABLE void openBrowser();
    Q_INVOKABLE void copyText(const QString &t);   // copy an arbitrary string (the password)
    // QR as a list of row strings ("0101…"), one char per module. Empty on failure.
    Q_INVOKABLE QStringList qrRows() const;
    Q_INVOKABLE QStringList qrRowsForUrl(const QString &url) const;   // QR for a URL with creds

signals:
    void changed();

private:
    static QString detectLanIp();
    QString m_url;
};

// Log viewer backend. Logger has no signal, so we poll the file delta (1s)
// exactly like the old LogViewerDialog and re-emit a Qt signal to QML.
class SubtitleSearch;

// Player-facing subtitle search/download (OpenSubtitles + Gestdown)
class QmlSubtitleBridge : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool searching READ searching NOTIFY searchingChanged)
    Q_PROPERTY(QVariantList results READ results NOTIFY resultsChanged)
public:
    explicit QmlSubtitleBridge(SessionManager *session, QObject *parent = nullptr);

    bool searching() const { return m_searching; }
    QVariantList results() const;
    Q_INVOKABLE void searchFor(const QString &infoHash, int fileIndex, const QString &mediaTitle);
    Q_INVOKABLE void download(int index);
    Q_INVOKABLE bool hasOpenSubtitlesKey() const;

signals:
    void searchingChanged();
    void resultsChanged();
    void subtitleReady(const QString &path);
    void searchError(const QString &message);

private:
    SessionManager *m_session;
    SubtitleSearch *m_search;
    QString m_targetDir;
    QString m_baseName;
    bool m_searching = false;
};

class QmlLogBridge : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString text READ text NOTIFY textChanged)
    Q_PROPERTY(int level READ level WRITE setLevel NOTIFY levelChanged)
    Q_PROPERTY(QStringList levelNames READ levelNames CONSTANT)
    Q_PROPERTY(QString logsDir READ logsDir CONSTANT)
public:
    explicit QmlLogBridge(QObject *parent = nullptr);

    QString text() const { return m_text; }
    int level() const;
    void setLevel(int l);
    QStringList levelNames() const;
    QString logsDir() const;

    Q_INVOKABLE void start();
    Q_INVOKABLE void stop();
    Q_INVOKABLE void clearLog();
    Q_INVOKABLE void openLogsFolder();
    Q_INVOKABLE bool exportLogs(const QString &filePath);
    Q_INVOKABLE QString defaultExportName() const;
    Q_INVOKABLE bool previousSessionCrashed() const;
    Q_INVOKABLE QString crashReportUrl() const;

signals:
    void textChanged();
    void levelChanged();

private slots:
    void poll();

private:
    QString m_text;
    qint64 m_lastSize = 0;
    QTimer m_pollTimer;
};

// Settings window backend. Generic get(key)/set(key,value) keyed by setting
// name: most route to SessionManager (live + persisted); UI-only prefs and the
// media API keys fall through to QSettings.
class QmlSettingsBridge : public QObject
{
    Q_OBJECT
public:
    explicit QmlSettingsBridge(SessionManager *session, QObject *parent = nullptr);
    Q_INVOKABLE QVariant get(const QString &key) const;
    Q_INVOKABLE void set(const QString &key, const QVariant &v);
    // Register BATorrent as the default .torrent / magnet handler. Returns success.
    Q_INVOKABLE bool setAsDefaultApp();
    // Post a test message to the configured Telegram chat; result via telegramTestResult.
    Q_INVOKABLE void testTelegram();
    // Windows-only: add the save folder to Windows Defender's exclusion list (UAC prompt).
    Q_INVOKABLE bool excludeFromDefender();
    // Settings export/import (JSON, secrets excluded) + full backup/restore
    // (.bat archive: settings + resume data). Each returns a localized result.
    Q_INVOKABLE QString exportSettings(const QString &path);
    Q_INVOKABLE QString importSettings(const QString &path);
    Q_INVOKABLE QString fullBackup(const QString &path);
    Q_INVOKABLE QString fullRestore(const QString &path);
    // Up/active network interfaces by name (index 0 = "Any"), for the VPN bind select.
    Q_INVOKABLE QStringList networkInterfaces() const;
    // One-tap phone pairing (the recommended "scan and connect" flow): enable the
    // WebUI, generate a strong password, allow LAN access — secure but zero manual
    // setup. enablePairing returns the plaintext password to display.
    Q_INVOKABLE QString enablePairing();
    Q_INVOKABLE void disablePairing();
    Q_INVOKABLE bool pairingActive() const;
    Q_INVOKABLE QString webUiUser() const;
    Q_INVOKABLE QString webUiPassword() const;   // plaintext from the secure store, for display
    void setTelegramNotifier(TelegramNotifier *n) { m_telegram = n; }
signals:
    void changed();
    void telegramTestResult(bool ok, const QString &message);
private:
    static int telegramEventBit(const QString &key);   // toggle key → Events bit (0 if none)
    void applyWebUi();                                  // (re)start the WebUI server from settings
    SessionManager *m_session;
    TelegramNotifier *m_telegram = nullptr;
    WebServer *m_webServer = nullptr;
};

// Exposes the Translator to QML with a reactive `language` property so every
// i18n.t("key") binding re-evaluates when the user switches language.
class QmlI18nBridge : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int language READ language WRITE setLanguage NOTIFY languageChanged)
public:
    explicit QmlI18nBridge(QObject *parent = nullptr) : QObject(parent) {}
    Q_INVOKABLE QString t(const QString &key) const { return tr_(key); }
    int language() const { return static_cast<int>(Translator::instance().language()); }
    Q_INVOKABLE void setLanguage(int lang) {
        if (lang == language()) return;
        Translator::instance().setLanguage(static_cast<Translator::Language>(lang));
        QSettings().setValue("language", lang);
        emit languageChanged();
    }
signals:
    void languageChanged();
};

// Turns background SessionManager / RSS events into a single `notify` signal
// that QML hands to the system tray icon as a native OS notification.
class QmlNotificationBridge : public QObject
{
    Q_OBJECT
public:
    explicit QmlNotificationBridge(QObject *parent = nullptr) : QObject(parent) {}

public slots:
    void onTorrentAdded(const QString &name);
    void onTorrentFinished(const QString &name, const QString &infoHash);
    void onTorrentError(const QString &message);
    void onKillSwitchTriggered();
    void onRssAutoDownloaded(const QString &feedName, const QString &itemTitle);
    void onSuspiciousFilesDetected(const QString &name, const QStringList &files);

signals:
    // level: 0 = information, 1 = warning, 2 = critical
    void notify(const QString &title, const QString &body, int level);
};

// Owns a DiscordRPC instance and refreshes the user's Discord activity with
// the current download/seed state. Gated on the "discordRichPresence" setting.
class DiscordRPC;
class DiscordRpcBridge : public QObject
{
    Q_OBJECT
public:
    explicit DiscordRpcBridge(SessionManager *session, QObject *parent = nullptr);

public slots:
    void refresh();

private:
    SessionManager *m_session;
    DiscordRPC *m_rpc = nullptr;
    QTimer m_timer;
    qint64 m_sessionStart = 0;
    QString m_lastActivityKey;   // skip resending an identical presence each tick
};

// Exposes the GitHub-release auto-updater to QML. Disabled (not instantiated)
// in Microsoft Store builds — the Store handles updates there.
class Updater;
class QmlUpdaterBridge : public QObject
{
    Q_OBJECT
public:
    explicit QmlUpdaterBridge(QObject *parent = nullptr);

    Q_INVOKABLE void check(bool silent = false);
    Q_INVOKABLE void downloadAndInstall(const QString &url, const QString &assetName);
    Q_INVOKABLE void skipVersion(const QString &version);

signals:
    void updateFound(const QString &version, const QString &url, const QString &assetName);
    void noUpdate(bool silent);
    void progress(int percent);
    void ready();
    void failed(const QString &message, bool silent);

private:
    Updater *m_updater;
    bool m_silent = false;
};

#endif
