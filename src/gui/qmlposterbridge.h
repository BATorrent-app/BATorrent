// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef QMLPOSTERBRIDGE_H
#define QMLPOSTERBRIDGE_H

#include <QAbstractListModel>
#include <QColor>
#include <QSortFilterProxyModel>
#include <QString>
#include <QTimer>
#include <QVariantList>
#include <QVector>
#include <QSettings>

#include "../app/addonmanager.h"   // CatalogItem / StreamResult / TorrentSearchResult
#include "../app/translator.h"

class SessionManager;
class MetadataResolver;
class GeoIpResolver;

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
    void refresh();
    void moveRow(int from, int to);

private:
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
    Q_PROPERTY(int torrentCount READ torrentCount NOTIFY statsChanged)
    Q_PROPERTY(int activeCount READ activeCount NOTIFY statsChanged)
    Q_PROPERTY(int downloadingCount READ downloadingCount NOTIFY statsChanged)
    Q_PROPERTY(int seedingCount READ seedingCount NOTIFY statsChanged)
    Q_PROPERTY(int pausedCount READ pausedCount NOTIFY statsChanged)
    Q_PROPERTY(int completedCount READ completedCount NOTIFY statsChanged)
    Q_PROPERTY(QString totalDownSpeed READ totalDownSpeed NOTIFY statsChanged)
    Q_PROPERTY(QString totalUpSpeed READ totalUpSpeed NOTIFY statsChanged)
    Q_PROPERTY(QString totalDownloaded READ totalDownloaded NOTIFY statsChanged)
    Q_PROPERTY(QString totalUploaded READ totalUploaded NOTIFY statsChanged)
    Q_PROPERTY(QString globalRatio READ globalRatio NOTIFY statsChanged)
    Q_PROPERTY(QVariantList downloadHistory READ downloadHistory NOTIFY historyChanged)
    Q_PROPERTY(QVariantList uploadHistory READ uploadHistory NOTIFY historyChanged)
    Q_PROPERTY(int historyMaxBytes READ historyMaxBytes NOTIFY historyChanged)

    Q_PROPERTY(QString selectedName READ selectedName NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedSavePath READ selectedSavePath NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedSize READ selectedSize NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedHash READ selectedHash NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedDownloaded READ selectedDownloaded NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedUploaded READ selectedUploaded NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedSpeed READ selectedSpeed NOTIFY selectionChanged)
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
    Q_PROPERTY(bool selectedAtFullProgress READ selectedAtFullProgress NOTIFY selectionChanged)
    Q_PROPERTY(bool selectedPaused READ selectedPaused NOTIFY selectionChanged)
    Q_PROPERTY(QVariantList selectedPeerList READ selectedPeerList NOTIFY selectionChanged)
    Q_PROPERTY(QVariantList selectedFiles READ selectedFiles NOTIFY selectionChanged)
    Q_PROPERTY(QVariantList selectedTrackers READ selectedTrackers NOTIFY selectionChanged)
    Q_PROPERTY(QVariantList selectedPieces READ selectedPieces NOTIFY selectionChanged)
    Q_PROPERTY(bool hasSelection READ hasSelection NOTIFY selectionChanged)

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
    QString totalDownloaded() const;
    QString totalUploaded() const;
    QString globalRatio() const;
    QVariantList downloadHistory() const;
    QVariantList uploadHistory() const;
    int historyMaxBytes() const;

    Q_INVOKABLE void setSelectedIndex(int index);
    Q_INVOKABLE void setSelectedRows(const QList<int> &rows);
    Q_INVOKABLE QList<int> selectedRows() const;
    Q_INVOKABLE void pauseSelected();
    Q_INVOKABLE void resumeSelected();
    Q_INVOKABLE void removeSelected();
    Q_INVOKABLE void removeSelectedWithFiles();
    Q_INVOKABLE void pauseAll();
    Q_INVOKABLE void resumeAll();
    Q_INVOKABLE void addTorrentFile(const QString &filePath);
    Q_INVOKABLE void addMagnetUri(const QString &uri);
    Q_INVOKABLE QVariantMap previewTorrent(const QString &filePath) const;
    Q_INVOKABLE void resolvePreview(const QString &infoHash, const QString &name);
    Q_INVOKABLE void addTorrentWithPrefs(const QString &filePath, const QString &savePath,
                                         const QVariantList &priorities);
    Q_INVOKABLE void copyMagnetLink();
    Q_INVOKABLE void copyInfoHash();
    Q_INVOKABLE void openSaveFolder();
    Q_INVOKABLE QString defaultSavePath() const;
    Q_INVOKABLE void setSelectedForceStart(bool on);
    Q_INVOKABLE void setSelectedSuperSeeding(bool on);
    Q_INVOKABLE void markSelectedCompleted();
    Q_INVOKABLE void unmarkSelectedCompleted();
    Q_INVOKABLE void forceRecheckSelected();
    Q_INVOKABLE void forceReannounceSelected();
    Q_INVOKABLE void queueUpSelected();
    Q_INVOKABLE void queueDownSelected();
    Q_INVOKABLE void queueTopSelected();
    Q_INVOKABLE void queueBottomSelected();
    Q_INVOKABLE void toggleSelectedPause();
    Q_INVOKABLE void stopSeedingSelected();
    Q_INVOKABLE void smartPaste();

    // per-torrent edits (wrap SessionManager against the current selection)
    Q_INVOKABLE void moveSelectedStorage(const QString &path);
    Q_INVOKABLE void setSelectedDownloadLimit(int kbps);
    Q_INVOKABLE void setSelectedUploadLimit(int kbps);
    Q_INVOKABLE void setSelectedSequential(bool on);
    Q_INVOKABLE void setSelectedCategory(const QString &category);
    Q_INVOKABLE void setSelectedTags(const QStringList &tags);
    Q_INVOKABLE void addTrackerToSelected(const QString &url);
    Q_INVOKABLE void removeTrackerFromSelected(const QString &url);
    Q_INVOKABLE void renameSelectedFile(int fileIndex, const QString &newName);
    Q_INVOKABLE void setSelectedFilePriority(int fileIndex, int priority);
    Q_INVOKABLE void copySelectedName();
    Q_INVOKABLE void openSelectedFile();
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
    Q_INVOKABLE QStringList allTags() const;

    QString selectedName() const;
    QString selectedSavePath() const;
    QString selectedSize() const;
    QString selectedHash() const;
    QString selectedDownloaded() const;
    QString selectedUploaded() const;
    QString selectedSpeed() const;
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
    bool selectedAtFullProgress() const;
    bool selectedPaused() const;
    QVariantList selectedPeerList() const;
    QVariantList selectedFiles() const;
    QVariantList selectedTrackers() const;
    QVariantList selectedPieces() const;
    bool hasSelection() const;

    void emitStats();

signals:
    void statsChanged();
    void selectionChanged();
    void historyChanged();
    void queueRefreshNeeded();
    void queueMoved(int from, int to);
    void previewPosterReady(const QString &infoHash, const QString &posterPath);

private slots:
    void sampleSpeeds();

private:
    SessionManager *m_session;
    MetadataResolver *m_resolver;
    int m_selectedIndex = -1;
    QList<int> m_selectedRows;
    GeoIpResolver *m_geoIp = nullptr;

    QTimer m_sampleTimer;
    QVector<int> m_downloadHistory;
    QVector<int> m_uploadHistory;
    static constexpr int HistoryMaxPoints = 60;
};

class QmlThemeBridge : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString themeName READ themeName WRITE setThemeName NOTIFY changed)
    Q_PROPERTY(bool anime READ anime WRITE setAnime NOTIFY changed)

public:
    explicit QmlThemeBridge(QObject *parent = nullptr);

    QString themeName() const;
    void setThemeName(const QString &n);
    bool anime() const;
    void setAnime(bool on);

signals:
    void changed();

private:
    QString m_themeName;
    bool m_anime = true;
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
    Q_PROPERTY(QString mode READ mode NOTIFY modeChanged)        // catalog|streams|torrent|games
    Q_PROPERTY(bool inStreams READ inStreams NOTIFY modeChanged)
    Q_PROPERTY(bool searching READ searching NOTIFY searchingChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusChanged)
public:
    explicit QmlSearchBridge(SessionManager *session, QObject *parent = nullptr);

    QVariantList sources() const;
    QVariantList categories() const;
    QVariantList results() const;
    QString mode() const;
    bool inStreams() const;
    bool searching() const;
    QString statusText() const;

    Q_INVOKABLE void refreshSources();
    Q_INVOKABLE void search(const QString &sourceKey, const QString &query, int categoryCode = 0);
    Q_INVOKABLE void activateResult(int index);   // catalog→streams; else add magnet
    Q_INVOKABLE void back();                       // streams → catalog

signals:
    void sourcesChanged();
    void resultsChanged();
    void modeChanged();
    void searchingChanged();
    void statusChanged();

private:
    void setSearching(bool on);
    void setStatus(const QString &s);
    void setMode(const QString &m);
    static QString detectRepacker(const QString &name);

    SessionManager *m_session;
    QString m_mode;
    QString m_savePath;
    QString m_lastQuery;
    bool m_searching = false;
    bool m_isGameSearch = false;
    QString m_status;
    QVariantList m_results;
    QList<CatalogItem> m_catalogCache;
    QList<StreamResult> m_streamCache;
    QList<TorrentSearchResult> m_torrentCache;
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
    Q_PROPERTY(QString url READ url CONSTANT)
    Q_PROPERTY(bool available READ available CONSTANT)
public:
    explicit QmlPairingBridge(QObject *parent = nullptr);

    QString url() const { return m_url; }
    bool available() const { return !m_url.isEmpty(); }

    Q_INVOKABLE void copyUrl();
    Q_INVOKABLE void openBrowser();
    // QR as a list of row strings ("0101…"), one char per module. Empty on failure.
    Q_INVOKABLE QStringList qrRows() const;

private:
    static QString detectLanIp();
    QString m_url;
};

// Log viewer backend. Logger has no signal, so we poll the file delta (1s)
// exactly like the old LogViewerDialog and re-emit a Qt signal to QML.
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
signals:
    void changed();
private:
    SessionManager *m_session;
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
    void onTorrentFinished(const QString &name, const QString &infoHash);
    void onTorrentError(const QString &message);
    void onKillSwitchTriggered();
    void onRssAutoDownloaded(const QString &feedName, const QString &itemTitle);

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
    void failed(const QString &message);

private:
    Updater *m_updater;
    bool m_silent = false;
};

#endif
