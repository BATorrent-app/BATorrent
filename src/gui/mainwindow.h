// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class QTableView;
class QLabel;
class QLineEdit;
class QSplitter;
class QSystemTrayIcon;
class QMessageBox;
class SessionManager;
class TorrentModel;
class TorrentFilter;
class QStackedWidget;
class DetailsPanel;
class SpeedGraph;
class BatWidget;
class Updater;
class WebServer;
class AddonManager;
class QComboBox;
class QPushButton;
class QNetworkAccessManager;
class MetadataResolver;
class PosterView;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(SessionManager *session, QWidget *parent = nullptr);
    ~MainWindow();

    void addTorrentFromCli(const QString &filePath);
    void addMagnetFromCli(const QString &uri);

protected:
    void closeEvent(QCloseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private slots:
    void openTorrent();
    void openMagnet();
    void createTorrent();
    void importQBittorrent();
    void removeSelected();
    void removeSelectedWithFiles();
    void pauseSelected();
    void resumeSelected();
    void stopSelected();
    void pauseAll();
    void resumeAll();
    void updateStatusBar();
    void onSelectionChanged();
    void onTorrentFinished(const QString &name, const QString &infoHash);
    void onTorrentError(const QString &message);
    void trayActivated();
    // Click handler for a completion toast — opens the on-disk folder of the
    // last torrent we notified about, instead of just raising the window.
    void revealLastNotified();
    void undoLastRemove();
    void diagnoseSlow(int row);
    void openSettings();
    void showWelcome();
    void showAbout();
    void retranslateUi();
    void filterByState(const QString &state);
    void showContextMenu(const QPoint &pos);
    void checkForUpdate(bool silent = true);
    void checkAutoShutdown();
    void streamTorrent(int row);
    void openAddons();
    void openSearch();
    void openRssManager();

private:
    void applyTheme();
    void restyleFilterRow();
    void setupMenuBar();
    void setupToolBar();
    void setupCentralWidget();
    void setupStatusBar();
    void setupTrayIcon();
    void saveSettings();
    void loadSettings();
    void startWebServer();
    QList<int> selectedRows() const;
    QString chooseSavePath();
    void addTorrentFile(const QString &filePath);

    SessionManager *m_session = nullptr;
    TorrentModel *m_model = nullptr;
    TorrentFilter *m_proxyModel = nullptr;
    QTableView *m_tableView = nullptr;
    QLineEdit *m_searchEdit = nullptr;
    DetailsPanel *m_detailsPanel = nullptr;
    SpeedGraph *m_speedGraph = nullptr;
    BatWidget *m_batWidget = nullptr;
    QStackedWidget *m_topStack = nullptr;
    QLabel *m_statusLabel = nullptr;
    QLabel *m_statusSpeedLabel = nullptr;
    QLabel *m_bandwidthPill = nullptr;
    QLabel *m_vpnLabel = nullptr;
    QList<QPushButton *> m_filterPills;
    QSystemTrayIcon *m_trayIcon = nullptr;
    class TrayPopup *m_trayPopup = nullptr;
    QString m_lastSavePath;
    Updater *m_updater = nullptr;
    class TelegramNotifier *m_telegramNotifier = nullptr;
    class DiscordRPC *m_discordRpc = nullptr;
    QTimer *m_discordRefreshTimer = nullptr;
    qint64 m_discordSessionStart = 0;
    void refreshDiscordPresence();
    WebServer *m_webServer = nullptr;
    bool m_startMinimized = false;
    bool m_closeToTray = true;
    bool m_useDefaultPath = false;
    bool m_autoShutdown = false;
    bool m_notifSoundEnabled = true;
    QMessageBox *m_shutdownDialog = nullptr;
    QTimer *m_shutdownTimer = nullptr;
    int m_shutdownCountdown = 0;
    QLabel *m_globalStatsLabel = nullptr;
    QTimer *m_streamPollTimer = nullptr;
    int m_streamTorrentIndex = -1;
    QString m_streamFilePath;
    // The hash of the torrent that fired the most recent tray notification.
    // When the user clicks the balloon we use this to bring the matching
    // row into view instead of just raising the window.
    QString m_lastNotifiedHash;
    // Snapshot of .resume bytes captured by the most recent removeSelected()
    // call. Consumed by undoLastRemove() if the user clicks the toast within
    // its dismiss window; cleared (idempotently) after restore.
    QList<QByteArray> m_pendingUndoRemove;
    QComboBox *m_categoryCombo = nullptr;
    QNetworkAccessManager *m_mediaServerNam = nullptr;
    void notifyMediaServers();
    MetadataResolver *m_metadataResolver = nullptr;
    PosterView *m_posterView = nullptr;
    bool m_posterViewActive = false;
};

#endif
