// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "mainwindow.h"
#include "../app/logger.h"
#include "torrentmodel.h"
#include "torrentfilter.h"
#include "progressdelegate.h"
#include "detailspanel.h"
#include "settingsdialog.h"
#include "welcomedialog.h"
#include "createtorrentdialog.h"
#include "addtorrentdialog.h"
#include "speedgraph.h"
#include "batwidget.h"
#include "thememanager.h"
#include "toast.h"
#include "../torrent/sessionmanager.h"
#include "../webui/webserver.h"
#include "../app/translator.h"
#include "../app/updater.h"
#include "../app/utils.h"
#include "../app/addonmanager.h"
#include "../app/secretstore.h"
#include "../app/notifier.h"
#include "../app/discordrpc.h"
#include "addondialog.h"
#include "searchdialog.h"
#include "rssdialog.h"
#include "releasenotesdialog.h"
#include "statisticsdialog.h"
#include "shortcutsdialog.h"
#include "logviewerdialog.h"
#include "diagnosticsdialog.h"
#include "removedhistorydialog.h"
#include "torrentinspectordialog.h"
#include "traypopup.h"
#include "../app/rssmanager.h"

#include <QMenuBar>
#include <QToolBar>
#include <QToolButton>
#include <QStatusBar>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QTableView>
#include <QHeaderView>
#include <QLabel>
#include <QIcon>
#include <QSplitter>
#include <QPixmap>
#include <QSystemTrayIcon>
#include <QCursor>
#include <QMenu>
#include <QCloseEvent>
#include <QSettings>
#include <QMimeData>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QResizeEvent>
#include <QApplication>
#include <QClipboard>
#include <QColor>
#include <QPalette>
#include <QLocale>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStackedWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QShortcut>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QProgressDialog>
#include <QProcess>
#include <QRandomGenerator>
#include <QScrollArea>
#include <QScrollBar>
#include <QDesktopServices>
#include <QStandardPaths>
#include <QUrl>
#include <QNetworkAccessManager>
#include <QNetworkInformation>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>

MainWindow::MainWindow(SessionManager *session, QWidget *parent)
    : QMainWindow(parent), m_session(session)
{
    setWindowTitle("BATorrent");
    setWindowIcon(QIcon(":/images/logo.svg"));
    setAcceptDrops(true);
    // Default size targets the comfortable layout: 8-column table + 3-column
    // details panel + bandwidth pill all visible without crowding. Used only
    // on first launch — loadSettings restores the user's last geometry if
    // there is one.
    resize(1280, 820);
    setMinimumSize(960, 600);

    m_model = new TorrentModel(session, this);

    loadSettings();
    setupMenuBar();
    setupCentralWidget();
    // applyTheme() owns toolbar + status-bar construction so a later theme
    // switch can rebuild them without leaking widgets or running setup twice.
    applyTheme();
    // Defer tray-icon creation until after the event loop is running.
    // NSStatusItem registration during the MainWindow ctor (before
    // QApplication::exec()) can land in a state where macOS reports
    // the item as height 0 / position bottom-of-screen — the slot is
    // never allocated. Deferring to the next event-loop tick lets
    // AppKit finish its initialization first.
    QTimer::singleShot(0, this, &MainWindow::setupTrayIcon);

    // Restore table layout
    {
        QSettings settings("BATorrent", "BATorrent");
        if (settings.contains("tableHeaderState"))
            m_tableView->horizontalHeader()->restoreState(settings.value("tableHeaderState").toByteArray());
        if (settings.contains("tableSortColumn")) {
            int col = settings.value("tableSortColumn").toInt();
            auto order = static_cast<Qt::SortOrder>(settings.value("tableSortOrder", 0).toInt());
            m_tableView->sortByColumn(col, order);
        }
    }

    connect(m_session, &SessionManager::torrentsUpdated, m_model, &TorrentModel::refresh);
    connect(m_session, &SessionManager::torrentsUpdated, this, &MainWindow::updateStatusBar);
    connect(m_session, &SessionManager::torrentAdded, m_model, &TorrentModel::refresh);
    connect(m_session, &SessionManager::torrentAdded, this, [this](int index) {
        TorrentInfo info = m_session->torrentAt(index);
        QString body = info.totalSize > 0
            ? QStringLiteral("%1 · %2").arg(info.name, formatSize(info.totalSize))
            : info.name;
        Toast::notify(tr_("notif_torrent_added"), body,
                      Toast::Info, this, SLOT(trayActivated()));
        if (m_notifSoundEnabled)
            QApplication::beep();
    });
    connect(m_session, &SessionManager::torrentRemoved, m_model, &TorrentModel::refresh);
    // Removing a torrent shifts every higher index down by one. If the user
    // had a higher-indexed torrent selected, the details pane would silently
    // start showing some other torrent's data. Clear the selection-driven
    // panel state on remove and let the user re-select.
    connect(m_session, &SessionManager::torrentRemoved, this, [this]() {
        m_detailsPanel->showTorrent(-1);
    });
    connect(m_session, &SessionManager::torrentFinished, this, &MainWindow::onTorrentFinished);
    connect(m_session, &SessionManager::torrentError, this, &MainWindow::onTorrentError);

    // Kill switch notifications
    connect(m_session, &SessionManager::killSwitchTriggered, this, [this]() {
        int active = 0;
        for (int i = 0; i < m_session->torrentCount(); ++i) {
            TorrentInfo info = m_session->torrentAt(i);
            if (!info.paused && (info.downloadRate > 0 || info.uploadRate > 0))
                ++active;
        }
        const QString iface = m_session->outgoingInterface();
        const QString body = !iface.isEmpty()
            ? QStringLiteral("%1 active torrents paused · %2 disconnected")
                  .arg(active).arg(iface)
            : tr_("killswitch_triggered");
        Toast::notify(tr_("killswitch_title"), body, Toast::Warning);
    });
    connect(m_session, &SessionManager::interfaceRestored, this, [this]() {
        const QString iface = m_session->outgoingInterface();
        const QString body = !iface.isEmpty()
            ? QStringLiteral("%1 reconnected").arg(iface)
            : tr_("killswitch_restored");
        Toast::notify(tr_("killswitch_title"), body, Toast::Success);
    });

    // Auto-updater — disabled in Store builds (Microsoft handles updates).
    m_updater = new Updater(this);
#ifndef BAT_STORE_BUILD
    checkForUpdate(true); // silent check on startup
#endif

    // Telegram notifier — mirrors the in-app Toasts into a chat. Reload-on-
    // settings-save is wired below in openSettings(). Subscribe to the same
    // event surfaces the Toast notify() calls use.
    m_telegramNotifier = new TelegramNotifier(this);
    connect(m_session, &SessionManager::torrentFinished,
            m_telegramNotifier, &TelegramNotifier::onTorrentFinished);
    connect(m_session, &SessionManager::killSwitchTriggered,
            m_telegramNotifier, &TelegramNotifier::onKillSwitchTriggered);
    connect(m_session, &SessionManager::torrentError,
            m_telegramNotifier, &TelegramNotifier::onTorrentError);
    connect(&RssManager::instance(), &RssManager::itemAutoDownloaded,
            m_telegramNotifier, &TelegramNotifier::onRssAutoDownloaded);

    // Discord Rich Presence — empty client ID at startup is fine (the RPC
    // is silent until configured in Settings → Integrations). Refresh every
    // 15 s so the activity tracks the current download set.
    m_discordRpc = new DiscordRPC(this);
    m_discordSessionStart = QDateTime::currentSecsSinceEpoch();
    {
        QSettings s("BATorrent", "BATorrent");
        // Enabled by default; user can toggle off in Settings → Integrations.
        // Application ID is hardcoded so it works for everyone out of the box.
        if (s.value("discordEnabled", true).toBool())
            m_discordRpc->setClientId(QStringLiteral("1508208411282640956"));
    }
    m_discordRefreshTimer = new QTimer(this);
    connect(m_discordRefreshTimer, &QTimer::timeout, this, &MainWindow::refreshDiscordPresence);
    m_discordRefreshTimer->start(15000);
    refreshDiscordPresence();

    // Auto tracker list: fetch on startup, add trackers to new torrents
    AddonManager::instance().fetchTrackerList();
    connect(m_session, &SessionManager::torrentAdded, this, [this](int index) {
        if (!AddonManager::instance().autoTrackersEnabled()) return;
        auto trackers = AddonManager::instance().trackerList();
        for (const auto &tracker : trackers)
            m_session->addTracker(index, tracker);
    });

    // RSS Manager: set session and check feeds on startup
    RssManager::instance().setSession(m_session, m_lastSavePath);
    RssManager::instance().checkAllFeeds();
    connect(&RssManager::instance(), &RssManager::itemAutoDownloaded, this,
        [this](const QString &feedName, const QString &itemTitle) {
            Toast::notify(tr_("rss_auto_downloaded"),
                          QStringLiteral("%1 · %2").arg(feedName, itemTitle),
                          Toast::Success, this, SLOT(trayActivated()));
        });

    // Periodic resume data save (every 5 minutes)
    auto *resumeTimer = new QTimer(this);
    connect(resumeTimer, &QTimer::timeout, this, [this]() {
        m_session->saveResumeData();
    });
    resumeTimer->start(5 * 60 * 1000);

    // Extra keyboard shortcuts
    auto *pauseShortcut = new QShortcut(Qt::Key_Space, this);
    connect(pauseShortcut, &QShortcut::activated, this, [this]() {
        auto rows = selectedRows();
        if (rows.isEmpty()) return;
        // Toggle: if first selected is paused, resume all; otherwise pause all
        TorrentInfo info = m_session->torrentAt(rows.first());
        if (info.paused) {
            for (int r : rows) m_session->resumeTorrent(r);
        } else {
            for (int r : rows) m_session->pauseTorrent(r);
        }
    });

    auto *selectAllShortcut = new QShortcut(QKeySequence::SelectAll, this);
    connect(selectAllShortcut, &QShortcut::activated, m_tableView, &QTableView::selectAll);

    // Ctrl+V anywhere in the window: if the clipboard holds a magnet link,
    // a 40-char info-hash, or a .torrent URL, offer to add it. Saves a trip
    // through File → Add Magnet for the most common workflow.
    auto *smartPasteShortcut = new QShortcut(QKeySequence(QKeySequence::Paste), this);
    smartPasteShortcut->setContext(Qt::ApplicationShortcut);
    connect(smartPasteShortcut, &QShortcut::activated, this, [this]() {
        QString clip = QApplication::clipboard()->text().trimmed();
        if (clip.isEmpty()) return;
        // Heuristic: magnet:?... (any length); .torrent ending URL; or a
        // bare 40-char hex info-hash that we wrap into a magnet.
        QString magnet;
        if (clip.startsWith("magnet:", Qt::CaseInsensitive)) {
            magnet = clip;
        } else if (clip.startsWith("thunder://", Qt::CaseInsensitive)) {
            // Xunlei thunder:// links are base64("AA" + real_url + "ZZ").
            // Extremely common on Chinese forums — without this decode,
            // CN users have to manually convert before adding.
            QByteArray encoded = clip.mid(10).toUtf8();
            QByteArray decoded = QByteArray::fromBase64(encoded);
            if (decoded.startsWith("AA") && decoded.endsWith("ZZ"))
                decoded = decoded.mid(2, decoded.size() - 4);
            QString inner = QString::fromUtf8(decoded);
            if (inner.startsWith("magnet:", Qt::CaseInsensitive))
                magnet = inner;
            else
                return; // HTTP/FTP thunder links not supported yet
        } else if (clip.size() == 40
                   && std::all_of(clip.begin(), clip.end(),
                       [](QChar c){ return c.isLetterOrNumber(); })) {
            magnet = QStringLiteral("magnet:?xt=urn:btih:") + clip;
        } else {
            return;
        }
        auto reply = QMessageBox::question(this, tr_("smartpaste_title"),
            tr_("smartpaste_body").arg(magnet.left(80) + (magnet.size() > 80 ? "..." : "")),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
        if (reply == QMessageBox::Yes)
            m_session->addMagnet(magnet, chooseSavePath());
    });

    // F2 on selected row — matches universal file-manager rename convention.
    auto *renameShortcut = new QShortcut(QKeySequence(Qt::Key_F2), m_tableView);
    renameShortcut->setContext(Qt::WidgetShortcut);
    connect(renameShortcut, &QShortcut::activated, this, [this]() {
        auto rows = selectedRows();
        if (rows.size() != 1) return;
        const int row = rows.first();
        TorrentInfo info = m_session->torrentAt(row);
        bool ok = false;
        QString name = QInputDialog::getText(this, tr_("ctx_rename"),
            tr_("ctx_rename_prompt"), QLineEdit::Normal, info.name, &ok);
        if (!ok || name.trimmed().isEmpty() || name == info.name) return;
        m_session->renameFile(row, 0, name);
    });

    // Network reachability watcher — when the system reports it just came
    // back online, kick libtorrent to re-announce immediately. Otherwise
    // the next tracker probe waits for the next scheduled interval (minutes).
    // loadDefaultBackend() was added in Qt 6.3 — older Qt needs explicit
    // per-platform backend names, which isn't worth the build matrix
    // complexity. On Qt 6.2 (Ubuntu 22.04 CI default) we just skip and
    // rely on libtorrent's own reconnect logic.
#if QT_VERSION >= QT_VERSION_CHECK(6, 3, 0)
    if (QNetworkInformation::loadDefaultBackend()) {
        if (auto *ni = QNetworkInformation::instance()) {
            connect(ni, &QNetworkInformation::reachabilityChanged, this,
                    [this](QNetworkInformation::Reachability r) {
                if (r == QNetworkInformation::Reachability::Online) {
                    Logger::instance().log(Logger::Info,
                        "Network reachable again — re-announcing all trackers");
                    for (int i = 0; i < m_session->torrentCount(); ++i)
                        m_session->forceReannounce(i);
                }
            });
        }
    }
#endif

    // Show welcome on first launch + offer to set as default app
    QSettings settings("BATorrent", "BATorrent");

    // Auto-open release notes after a version bump. We compare against the
    // version the user last saw and skip the welcome dialog if both fire
    // (release notes is the more relevant "what changed" surface).
    const QString lastChangelog = settings.value("lastChangelogVersion").toString();
    const bool firstLaunch = lastChangelog.isEmpty();
    if (!firstLaunch && lastChangelog != QApplication::applicationVersion()) {
        QTimer::singleShot(300, this, [this]() {
            ReleaseNotesDialog dlg(this);
            dlg.exec();
        });
    }
    settings.setValue("lastChangelogVersion", QApplication::applicationVersion());

    if (!settings.value("welcomeShown", false).toBool())
        showWelcome();

    if (!settings.value("askedDefaultApp", false).toBool()) {
        settings.setValue("askedDefaultApp", true);
        QTimer::singleShot(500, this, [this]() {
            auto reply = QMessageBox::question(this, tr_("dlg_set_default_title"),
                tr_("dlg_set_default_msg"),
                QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
            if (reply == QMessageBox::Yes) {
                SettingsDialog dlg(this);
                dlg.setAsDefaultApp();
            }
        });
    }

    // Show release notes on version change
    {
        QString lastVer = settings.value("lastVersion").toString();
        QString curVer = QApplication::applicationVersion();
        if (!lastVer.isEmpty() && lastVer != curVer) {
            QTimer::singleShot(600, this, [this]() {
                ReleaseNotesDialog dlg(this);
                dlg.exec();
            });
        }
        settings.setValue("lastVersion", curVer);
    }

}

MainWindow::~MainWindow()
{
    saveSettings();
}

void MainWindow::applyTheme()
{
    qDebug() << "[ui] applyTheme:" << ThemeManager::instance().theme();
    const auto &tm = ThemeManager::instance();
    // Override the application QPalette so plain QWidgets (filter bar
    // container, splitters, scroll areas, details-panel inner widgets) pick
    // up our theme colors instead of inheriting macOS / Fusion defaults.
    // Without this the body looks correctly themed but the unstyled
    // containers around it stay in the system appearance (e.g. dark filter
    // row on top of a light table when our theme is set to Light).
    QPalette pal;
    pal.setColor(QPalette::Window,          QColor(tm.bgColor()));
    pal.setColor(QPalette::WindowText,      QColor(tm.textColor()));
    pal.setColor(QPalette::Base,            QColor(tm.surfaceColor()));
    pal.setColor(QPalette::AlternateBase,   QColor(tm.surfaceAltColor()));
    pal.setColor(QPalette::Text,            QColor(tm.textColor()));
    pal.setColor(QPalette::PlaceholderText, QColor(tm.mutedColor()));
    pal.setColor(QPalette::Button,          QColor(tm.surfaceColor()));
    pal.setColor(QPalette::ButtonText,      QColor(tm.textColor()));
    pal.setColor(QPalette::ToolTipBase,     QColor(tm.panelColor()));
    pal.setColor(QPalette::ToolTipText,     QColor(tm.textColor()));
    pal.setColor(QPalette::Highlight,       QColor(tm.accentColor()));
    pal.setColor(QPalette::HighlightedText, QColor("#ffffff"));
    qApp->setPalette(pal);
    // Re-apply the popup QSS (QMessageBox, QInputDialog, QToolTip). Without
    // this the About dialog and other ad-hoc popups keep startup-time colors
    // — e.g. dark bg + dark text in light mode after a theme switch.
    qApp->setStyleSheet(tm.appPopupStyleSheet());

    setStyleSheet(tm.styleSheet());
    // Toolbar widgets bake theme colors (tm.bgColor(), tm.surfaceColor(),
    // per-button QSS) into their own setStyleSheet at creation time, so the
    // MainWindow stylesheet alone doesn't refresh them. Without this rebuild:
    // header would freeze on the previous theme (e.g. light bg behind dark
    // body), and repeated setStyleSheet passes cascaded an ever-smaller font
    // onto the bake-in-place toolbar buttons.
    setupToolBar();
    // Filter-row widgets (search edit, filter pills, category combo) also
    // bake-in their colors during setupCentralWidget. Re-apply their styles
    // in place rather than rebuilding the central widget (which would drop
    // selection / scroll state).
    restyleFilterRow();
    if (m_detailsPanel) m_detailsPanel->restyle();
    // Status-bar labels bake mutedColor()/stateSeedingColor() in the same
    // way. Delete the specific labels we own, then rebuild — wholesale
    // clearing direct children would also kill QStatusBar's internal
    // QSizeGrip / message label.
    if (m_statusLabel) { delete m_statusLabel; m_statusLabel = nullptr; }
    if (m_statusSpeedLabel) { delete m_statusSpeedLabel; m_statusSpeedLabel = nullptr; }
    if (m_vpnLabel) { delete m_vpnLabel; m_vpnLabel = nullptr; }
    if (m_globalStatsLabel) { delete m_globalStatsLabel; m_globalStatsLabel = nullptr; }
    setupStatusBar();
}

void MainWindow::restyleFilterRow()
{
    const auto &tm = ThemeManager::instance();
    if (m_searchEdit) {
        m_searchEdit->setStyleSheet(QString(
            "QLineEdit { background-color: %1; color: %2; border: 1px solid %3;"
            "border-radius: 6px; padding: 6px 10px; font-size: 12px; }"
            "QLineEdit:focus { border-color: %4; }")
            .arg(tm.surfaceColor(), tm.textColor(), tm.borderColor(), tm.accentColor()));
    }
    const QString pillQss = QString(
        "QPushButton {"
        "  background: transparent; color: %1;"
        "  border: 1px solid %2; border-radius: 14px;"
        "  padding: 6px 14px; font-size: 11px; font-weight: 600;"
        "}"
        "QPushButton:hover { color: %3; }"
        "QPushButton:checked {"
        "  background: %4; color: %3; border-color: %5;"
        "}")
        .arg(tm.mutedColor(), tm.borderColor(), tm.textColor(),
             tm.accentTintColor(), tm.accentColor());
    for (QPushButton *pill : m_filterPills)
        pill->setStyleSheet(pillQss);
    if (m_categoryCombo) {
        m_categoryCombo->setStyleSheet(QString(
            "QComboBox { background-color: %1; color: %2; border: 1px solid %3;"
            "border-radius: 6px; padding: 5px 10px; font-size: 11px; }"
            "QComboBox:focus { border-color: %4; }"
            "QComboBox::drop-down { border: none; }")
            .arg(tm.surfaceColor(), tm.textColor(), tm.borderColor(), tm.accentColor()));
    }
}

void MainWindow::setupMenuBar()
{
    menuBar()->clear();

    QMenu *fileMenu = menuBar()->addMenu(tr_("menu_file"));
    auto *openAction = fileMenu->addAction(QIcon(":/icons/open.svg"), tr_("action_open"));
    openAction->setShortcut(QKeySequence::Open);
    connect(openAction, &QAction::triggered, this, &MainWindow::openTorrent);
    auto *magnetAction = fileMenu->addAction(QIcon(":/icons/magnet.svg"),
                                              tr_("action_magnet"));
    magnetAction->setShortcut(QKeySequence("Ctrl+M"));
    connect(magnetAction, &QAction::triggered, this, &MainWindow::openMagnet);
    fileMenu->addAction(tr_("action_create"), this, &MainWindow::createTorrent);
    fileMenu->addAction(tr_("action_import_qbt"), this, &MainWindow::importQBittorrent);
    fileMenu->addAction(tr_("action_recently_removed"), this, [this]() {
        RemovedHistoryDialog dlg(m_session, this);
        dlg.exec();
    });
    fileMenu->addAction(tr_("action_inspect"), this, [this]() {
        QString path = QFileDialog::getOpenFileName(this, tr_("action_inspect"),
            m_lastSavePath, tr_("dlg_torrent_filter"));
        if (path.isEmpty()) return;
        TorrentInspectorDialog dlg(path, m_session, this);
        dlg.exec();
        if (dlg.addRequested()) addTorrentFile(path);
    });
    fileMenu->addSeparator();
    auto *quitAction = fileMenu->addAction(tr_("action_quit"));
    quitAction->setShortcut(QKeySequence::Quit);
    connect(quitAction, &QAction::triggered, this, &QWidget::close);

    QMenu *torrentMenu = menuBar()->addMenu(tr_("menu_torrent"));
    torrentMenu->addAction(QIcon(":/icons/pause.svg"), tr_("action_pause"),
                           this, &MainWindow::pauseSelected);
    torrentMenu->addAction(QIcon(":/icons/play.svg"), tr_("action_resume"),
                           this, &MainWindow::resumeSelected);
    torrentMenu->addSeparator();
    torrentMenu->addAction(tr_("action_pause_all"), this, &MainWindow::pauseAll);
    torrentMenu->addAction(tr_("action_resume_all"), this, &MainWindow::resumeAll);
    torrentMenu->addSeparator();
    auto *removeAction = torrentMenu->addAction(QIcon(":/icons/trash.svg"), tr_("action_remove"));
    removeAction->setShortcut(QKeySequence::Delete);
    connect(removeAction, &QAction::triggered, this, &MainWindow::removeSelected);
    torrentMenu->addAction(tr_("action_remove_files"), this, &MainWindow::removeSelectedWithFiles);
    torrentMenu->addSeparator();
    auto *shutdownAction = torrentMenu->addAction(tr_("action_auto_shutdown"));
    shutdownAction->setCheckable(true);
    shutdownAction->setChecked(m_autoShutdown);
    connect(shutdownAction, &QAction::toggled, this, [this](bool checked) {
        m_autoShutdown = checked;
        QSettings settings("BATorrent", "BATorrent");
        settings.setValue("autoShutdown", checked);
    });

    QMenu *settingsMenu = menuBar()->addMenu(tr_("menu_settings"));
    auto *settingsAction = settingsMenu->addAction(QIcon(":/icons/settings.svg"),
                                                    tr_("action_settings"));
    settingsAction->setShortcut(QKeySequence::Preferences); // Ctrl+, on most platforms
    connect(settingsAction, &QAction::triggered, this, &MainWindow::openSettings);
    settingsMenu->addAction(tr_("action_addons"), this, &MainWindow::openAddons);
    settingsMenu->addAction(tr_("action_rss"), this, &MainWindow::openRssManager);
    settingsMenu->addSeparator();
    settingsMenu->addAction(tr_("action_search_addons"), this, &MainWindow::openSearch);
    settingsMenu->addSeparator();
    settingsMenu->addAction(tr_("action_speedtest"), this, []() {
        QDesktopServices::openUrl(QUrl(QStringLiteral("https://fast.com")));
    });
    settingsMenu->addAction(tr_("action_statistics"), this, [this]() {
        StatisticsDialog dlg(m_session, this);
        dlg.exec();
    });
    settingsMenu->addSeparator();
    settingsMenu->addAction(tr_("action_export_settings"), this, [this]() {
        QString path = QFileDialog::getSaveFileName(this, tr_("action_export_settings"),
            QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/batorrent_settings.json",
            "JSON (*.json)");
        if (path.isEmpty()) return;
        QSettings settings("BATorrent", "BATorrent");
        // Exclude credentials so users can share/attach the export without
        // leaking proxy passwords, media-server tokens, or the WebUI hash.
        static const QStringList kSecretKeys = {
            "proxyPass", "plexToken", "jellyfinApiKey", "webUiPasswordHash"
        };
        QJsonObject obj;
        for (const auto &key : settings.allKeys()) {
            if (kSecretKeys.contains(key)) continue;
            obj[key] = QJsonValue::fromVariant(settings.value(key));
        }
        QJsonDocument doc(obj);
        QFile file(path);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(doc.toJson());
            QMessageBox::information(this, "BATorrent", tr_("export_success"));
        }
    });
    settingsMenu->addAction(tr_("action_import_settings"), this, [this]() {
        QString path = QFileDialog::getOpenFileName(this, tr_("action_import_settings"),
            QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
            "JSON (*.json)");
        if (path.isEmpty()) return;
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) return;
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        if (!doc.isObject()) return;
        QSettings settings("BATorrent", "BATorrent");
        QJsonObject obj = doc.object();
        for (auto it = obj.begin(); it != obj.end(); ++it)
            settings.setValue(it.key(), it.value().toVariant());
        QMessageBox::information(this, "BATorrent", tr_("import_success") + "\n" + tr_("import_restart"));
    });
    settingsMenu->addSeparator();
    settingsMenu->addAction(tr_("action_full_backup"), this, [this]() {
        // Full backup: settings JSON + every .resume file in a single
        // self-describing archive. Custom format because shipping a zip
        // implementation just for this would dwarf the feature.
        // Layout: "BATBACKUP1\n" + count(u32 LE) + repeat{ nameLen(u32) +
        // name + dataLen(u64) + data }.
        QString out = QFileDialog::getSaveFileName(this, tr_("action_full_backup"),
            QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
                + QStringLiteral("/batorrent-backup-%1.bat")
                    .arg(QDate::currentDate().toString("yyyy-MM-dd")),
            "BATorrent backup (*.bat)");
        if (out.isEmpty()) return;
        QFile f(out);
        if (!f.open(QIODevice::WriteOnly)) return;

        QByteArray archive("BATBACKUP1\n");
        QList<QPair<QString, QByteArray>> entries;

        // Settings as JSON (re-using the export logic, but secrets included
        // since this is a private backup not a shareable export).
        QSettings settings("BATorrent", "BATorrent");
        QJsonObject obj;
        for (const auto &k : settings.allKeys())
            obj[k] = QJsonValue::fromVariant(settings.value(k));
        entries.append({"settings.json", QJsonDocument(obj).toJson()});

        // Resume + removed dirs
        QDir resumeDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                       + "/resume");
        if (resumeDir.exists()) {
            for (const auto &name : resumeDir.entryList({"*.resume"}, QDir::Files)) {
                QFile rf(resumeDir.filePath(name));
                if (rf.open(QIODevice::ReadOnly))
                    entries.append({"resume/" + name, rf.readAll()});
            }
        }

        quint32 count = entries.size();
        archive.append(reinterpret_cast<const char *>(&count), 4);
        for (const auto &e : entries) {
            QByteArray nameBytes = e.first.toUtf8();
            quint32 nameLen = nameBytes.size();
            quint64 dataLen = e.second.size();
            archive.append(reinterpret_cast<const char *>(&nameLen), 4);
            archive.append(nameBytes);
            archive.append(reinterpret_cast<const char *>(&dataLen), 8);
            archive.append(e.second);
        }
        f.write(archive);
        QMessageBox::information(this, "BATorrent",
            tr_("full_backup_done").arg(entries.size()).arg(formatSize(archive.size())));
    });
    settingsMenu->addAction(tr_("action_full_restore"), this, [this]() {
        QString path = QFileDialog::getOpenFileName(this, tr_("action_full_restore"),
            QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
            "BATorrent backup (*.bat)");
        if (path.isEmpty()) return;
        QFile f(path);
        if (!f.open(QIODevice::ReadOnly)) {
            QMessageBox::warning(this, "BATorrent", tr_("full_restore_failed"));
            return;
        }
        QByteArray data = f.readAll();
        if (!data.startsWith("BATBACKUP1\n")) {
            QMessageBox::warning(this, "BATorrent", tr_("full_restore_bad_format"));
            return;
        }
        const char *p = data.constData() + 11; // past magic
        const char *end = data.constData() + data.size();
        if (p + 4 > end) return;
        quint32 count;
        memcpy(&count, p, 4); p += 4;
        QString resumeBase = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QDir().mkpath(resumeBase + "/resume");
        int restored = 0;
        for (quint32 i = 0; i < count; ++i) {
            if (p + 4 > end) break;
            quint32 nameLen;
            memcpy(&nameLen, p, 4); p += 4;
            if (p + nameLen > end) break;
            QString name = QString::fromUtf8(p, nameLen); p += nameLen;
            if (p + 8 > end) break;
            quint64 dataLen;
            memcpy(&dataLen, p, 8); p += 8;
            if (p + dataLen > end) break;
            QByteArray payload(p, dataLen); p += dataLen;
            if (name == "settings.json") {
                auto obj = QJsonDocument::fromJson(payload).object();
                QSettings s("BATorrent", "BATorrent");
                for (auto it = obj.begin(); it != obj.end(); ++it)
                    s.setValue(it.key(), it.value().toVariant());
                ++restored;
            } else if (name.startsWith("resume/")) {
                QFile rf(resumeBase + "/" + name);
                if (rf.open(QIODevice::WriteOnly)) {
                    rf.write(payload);
                    ++restored;
                }
            }
        }
        QMessageBox::information(this, "BATorrent",
            tr_("full_restore_done").arg(restored) + "\n" + tr_("import_restart"));
    });

    QMenu *helpMenu = menuBar()->addMenu(tr_("menu_help"));
    helpMenu->addAction(tr_("action_welcome"), this, &MainWindow::showWelcome);
    helpMenu->addAction(tr_("action_release_notes"), this, [this]() {
        ReleaseNotesDialog dlg(this);
        dlg.exec();
    });
    helpMenu->addAction(tr_("action_shortcuts"), this, [this]() {
        ShortcutsDialog dlg(this);
        dlg.exec();
    });
#ifndef BAT_STORE_BUILD
    helpMenu->addAction(tr_("action_check_update"), this, [this]() { checkForUpdate(false); });
#endif
    helpMenu->addAction(tr_("action_log_viewer"), this, [this]() {
        LogViewerDialog dlg(this);
        dlg.exec();
    });
    helpMenu->addAction(tr_("action_diagnostics"), this, [this]() {
        DiagnosticsDialog dlg(m_session, this);
        dlg.exec();
    });
    helpMenu->addSeparator();
    helpMenu->addAction(tr_("action_donate"), this, []() {
        QDesktopServices::openUrl(QUrl(QStringLiteral("https://github.com/sponsors/Mateuscruz19")));
    });
    helpMenu->addAction(tr_("action_about"), this, &MainWindow::showAbout);
}

void MainWindow::setupToolBar()
{
    for (auto *tb : findChildren<QToolBar *>())
        delete tb;

    const auto &tm = ThemeManager::instance();

    // Values transcribed from canvas/main.jsx MainToolbar:
    //   height 64, padding '0 16px', gap 4 between children.
    QToolBar *toolbar = addToolBar("Main");
    toolbar->setMovable(false);
    toolbar->setFixedHeight(64);
    toolbar->setIconSize(QSize(18, 18));
    toolbar->setContentsMargins(0, 0, 0, 0);
    toolbar->setStyleSheet(QString(
        "QToolBar { background: %1; border: none; spacing: 2px; padding: 0 16px; }"
        ).arg(tm.bgColor()));

    // Brand cluster: bat 26px + stacked "BATorrent" / version. From JSX:
    //   gap 10, paddingRight 14, height 36, name font 13/700 letterSpacing -0.2,
    //   version eyebrow size 8 textDim.
    auto *brand = new QWidget;
    auto *brandRow = new QHBoxLayout(brand);
    brandRow->setContentsMargins(0, 0, 14, 0);
    brandRow->setSpacing(10);

    auto *logoLabel = new QLabel;
    logoLabel->setPixmap(tm.themedLogo(26, 2.0));
    logoLabel->setFixedSize(26, 26);
    brandRow->addWidget(logoLabel);

    auto *brandStack = new QWidget;
    auto *brandCol = new QVBoxLayout(brandStack);
    brandCol->setContentsMargins(0, 0, 0, 0);
    brandCol->setSpacing(0);
    auto *name = new QLabel(QStringLiteral("BATorrent"));
    {
        QFont f; f.setPointSize(12); f.setWeight(QFont::Bold);
        name->setFont(f);
        name->setStyleSheet(QString("color: %1;").arg(tm.textColor()));
        name->setContentsMargins(0, 0, 0, -2);
    }
    brandCol->addWidget(name);
    auto *ver = new QLabel(QStringLiteral("V") + QStringLiteral(APP_VERSION).toUpper());
    {
        QFont f; f.setPointSize(7); f.setWeight(QFont::Black);
        f.setLetterSpacing(QFont::AbsoluteSpacing, 1.0);
        ver->setFont(f);
        ver->setStyleSheet(QString("color: %1;").arg(tm.dimColor()));
        ver->setContentsMargins(0, -4, 0, 0);
    }
    brandCol->addWidget(ver);
    brandRow->addWidget(brandStack);
    toolbar->addWidget(brand);

    // Action buttons. ToolbarBtn spec (primitives.jsx):
    //   padding 6×14, minWidth 60, gap 4 icon↔label, fontSize 11, weight 500,
    //   icon 18, color textMuted (active/hover textColor), no border.
    const QString btnQss = QString(
        "QToolButton {"
        "  color: %1; background: transparent;"
        "  border: none; border-radius: 6px;"
        "  min-width: 60px;"
        "  padding: 4px 12px;"
        "}"
        "QToolButton:hover { background: %2; color: %3; }"
        "QToolButton:pressed { background: %4; color: #ffffff; }"
        ).arg(tm.mutedColor(), tm.surfaceColor(), tm.textColor(), tm.accentTintColor());

    auto addBtn = [&](const QString &icon, const QString &textKey, auto slot, bool danger = false) {
        auto *btn = new QToolButton;
        btn->setIcon(QIcon(icon));
        btn->setIconSize(QSize(18, 18));
        btn->setText(tr_(textKey));
        btn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setAutoRaise(true);
        btn->setMaximumHeight(56);
        btn->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        if (danger) {
            btn->setStyleSheet(QString(
                "QToolButton {"
                "  color: %1; background: transparent;"
                "  border: none; border-radius: 6px;"
                "  min-width: 60px; padding: 4px 12px;"
                "}"
                "QToolButton:hover { background: %2; color: #ffffff; }"
                ).arg(tm.accentLightColor(), tm.accentTintColor()));
        } else {
            btn->setStyleSheet(btnQss);
        }
        QFont f = btn->font();
        f.setPointSize(9);
        f.setWeight(QFont::Medium);
        btn->setFont(f);
        connect(btn, &QToolButton::clicked, this, slot);
        toolbar->addWidget(btn);
        return btn;
    };

    addBtn(":/icons/open.svg",     "tb_open",     &MainWindow::openTorrent);
    addBtn(":/icons/magnet.svg",   "tb_magnet",   &MainWindow::openMagnet);
    addBtn(":/icons/pause.svg",    "tb_pause",    &MainWindow::pauseSelected);
    addBtn(":/icons/play.svg",     "tb_resume",   &MainWindow::resumeSelected);
    addBtn(":/icons/stop.svg",     "tb_stop",     &MainWindow::stopSelected);
    addBtn(":/icons/trash.svg",    "tb_remove",   &MainWindow::removeSelected);
    addBtn(":/icons/search.svg",   "tb_search",   &MainWindow::openSearch);
    addBtn(":/icons/rss.svg",      "tb_rss",      &MainWindow::openRssManager);
    addBtn(":/icons/settings.svg", "tb_settings", &MainWindow::openSettings);

    auto *spacer = new QWidget;
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    toolbar->addWidget(spacer);

    // Bandwidth pill (live speeds). Per JSX:
    //   padding 6×12, borderRadius 6, bg surface, gap 12;
    //   each side: icon 12 + value 11px muted + unit 9px dim;
    //   middle vertical separator 1×14 hairline.
    auto *pill = new QFrame;
    pill->setStyleSheet(QString(
        "QFrame { background: %1; border-radius: 6px; }").arg(tm.surfaceColor()));
    pill->setFixedHeight(30);
    auto *pillRow = new QHBoxLayout(pill);
    pillRow->setContentsMargins(12, 4, 12, 4);
    pillRow->setSpacing(12);

    auto makeSpeedCluster = [&](const QString &iconPath, const QString &valueObjName,
                                const QString &unitObjName, const QString &numColor) {
        auto *box = new QWidget;
        auto *l = new QHBoxLayout(box);
        l->setContentsMargins(0, 0, 0, 0);
        l->setSpacing(6);
        auto *ic = new QLabel;
        QIcon icon(iconPath);
        ic->setPixmap(icon.pixmap(12, 12));
        l->addWidget(ic);
        auto *val = new QLabel(QStringLiteral("0"));
        val->setObjectName(valueObjName);
        QFont vf; vf.setPointSize(10); vf.setWeight(QFont::Medium);
        val->setFont(vf);
        val->setStyleSheet(QString("color: %1;").arg(numColor));
        l->addWidget(val);
        auto *unit = new QLabel(QStringLiteral("B/s"));
        unit->setObjectName(unitObjName);
        QFont uf; uf.setPointSize(8);
        unit->setFont(uf);
        unit->setStyleSheet(QString("color: %1;").arg(tm.dimColor()));
        l->addWidget(unit);
        return box;
    };

    pillRow->addWidget(makeSpeedCluster(":/icons/download.svg",
                                        QStringLiteral("pillDownVal"),
                                        QStringLiteral("pillDownUnit"),
                                        tm.textColor()));
    auto *sep = new QWidget;
    sep->setFixedSize(1, 14);
    sep->setStyleSheet(QStringLiteral("background: rgba(255,255,255,0.08);"));
    pillRow->addWidget(sep);
    pillRow->addWidget(makeSpeedCluster(":/icons/upload.svg",
                                        QStringLiteral("pillUpVal"),
                                        QStringLiteral("pillUpUnit"),
                                        tm.textColor()));

    toolbar->addWidget(pill);

    // m_bandwidthPill kept for the updateStatusBar setter; we now point it at
    // the down-value label and pull the up label dynamically when refreshing.
    m_bandwidthPill = pill->findChild<QLabel *>(QStringLiteral("pillDownVal"));
}

void MainWindow::setupCentralWidget()
{
    m_proxyModel = new TorrentFilter(this);
    m_proxyModel->setSourceModel(m_model);

    m_tableView = new QTableView;
    m_tableView->setModel(m_proxyModel);
    m_tableView->setSortingEnabled(true);
    m_tableView->sortByColumn(TorrentModel::Name, Qt::AscendingOrder);
    m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_tableView->horizontalHeader()->setStretchLastSection(true);
    m_tableView->horizontalHeader()->setSortIndicatorShown(true);
    m_tableView->verticalHeader()->hide();
    m_tableView->setShowGrid(false);
    m_tableView->setAlternatingRowColors(true);
    m_tableView->verticalHeader()->setDefaultSectionSize(36);
    m_tableView->setItemDelegateForColumn(TorrentModel::Progress,
                                          new ProgressDelegate(m_tableView));
    m_tableView->setDragEnabled(true);
    m_tableView->setAcceptDrops(true);
    m_tableView->setDragDropMode(QAbstractItemView::InternalMove);
    m_tableView->setDefaultDropAction(Qt::MoveAction);
    m_tableView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_tableView, &QWidget::customContextMenuRequested,
            this, &MainWindow::showContextMenu);

    connect(m_tableView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &MainWindow::onSelectionChanged);

    // Double-click on torrent row → reveal what this torrent put on disk
    // inside its save folder. For a single-file torrent that's the file
    // itself; for a multi-file torrent it's the root folder libtorrent
    // created. In both cases the user lands in the save dir with the right
    // item highlighted, instead of staring at a Downloads folder with 500
    // unrelated files.
    connect(m_tableView, &QTableView::doubleClicked, this, [this](const QModelIndex &index) {
        QModelIndex srcIdx = m_proxyModel->mapToSource(index);
        int row = srcIdx.row();
        if (row < 0 || row >= m_session->torrentCount()) return;
        QString root = m_session->torrentRootPath(row);
        if (!root.isEmpty()) revealInFileManager(root);
    });

    // Filter bar (wrapped in a horizontal scroll area so it overflows
    // gracefully on narrow windows instead of squeezing the table).
    auto *filterBar = new QWidget;
    auto *filterLayout = new QHBoxLayout(filterBar);
    const auto &tm = ThemeManager::instance();
    filterLayout->setContentsMargins(8, 6, 8, 6);
    filterLayout->setSpacing(6);
    filterBar->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    m_searchEdit = new QLineEdit;
    m_searchEdit->setPlaceholderText(tr_("filter_search"));
    m_searchEdit->setClearButtonEnabled(true);
    m_searchEdit->setMaximumWidth(260);
    m_searchEdit->setStyleSheet(QString(
        "QLineEdit { background-color: %1; color: %2; border: 1px solid %3;"
        "border-radius: 6px; padding: 6px 10px; font-size: 12px; }"
        "QLineEdit:focus { border-color: %4; }")
        .arg(tm.surfaceColor(), tm.textColor(), tm.borderColor(), tm.accentColor()));
    connect(m_searchEdit, &QLineEdit::textChanged, m_proxyModel, &TorrentFilter::setNameFilter);
    filterLayout->addWidget(m_searchEdit);

    filterLayout->addSpacing(10);

    // Filter pills with live count badges. Pills are tracked in m_filterPills
    // so updateStatusBar() can refresh badge counts each tick.
    m_filterPills.clear();

    auto addFilterBtn = [&](const QString &label, const QString &state) {
        auto *btn = new QPushButton;
        btn->setCheckable(true);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setProperty("filterState", state);
        btn->setProperty("filterLabel", label);
        btn->setText(QStringLiteral("%1   0").arg(label));
        btn->setStyleSheet(QString(
            "QPushButton {"
            "  background: transparent; color: %1;"
            "  border: 1px solid %2; border-radius: 14px;"
            "  padding: 6px 14px; font-size: 11px; font-weight: 600;"
            "}"
            "QPushButton:hover { color: %3; }"
            "QPushButton:checked {"
            "  background: %4; color: %3; border-color: %5;"
            "}")
            .arg(tm.mutedColor(), tm.borderColor(), tm.textColor(),
                 tm.accentTintColor(), tm.accentColor()));
        connect(btn, &QPushButton::toggled, this, [this, btn, state](bool checked) {
            if (checked) {
                for (auto *other : btn->parentWidget()->findChildren<QPushButton *>()) {
                    if (other != btn && other->property("filterState").isValid())
                        other->setChecked(false);
                }
                filterByState(state);
            } else {
                filterByState("");
            }
        });
        filterLayout->addWidget(btn);
        m_filterPills.append(btn);
    };

    addFilterBtn(tr_("filter_all"),            "");
    addFilterBtn(tr_("filter_all_active"),    "all_active");
    addFilterBtn(tr_("filter_downloading"), "downloading");
    addFilterBtn(tr_("filter_seeding"),     "seeding");
    addFilterBtn(tr_("filter_completed"), "completed");
    addFilterBtn(tr_("filter_paused"),      "paused");
    addFilterBtn(tr_("filter_finished"),    "finished");
    addFilterBtn(tr_("filter_queued"),      "queued");

    if (!m_filterPills.isEmpty())
        m_filterPills.first()->setChecked(true);

    filterLayout->addSpacing(10);

    m_categoryCombo = new QComboBox;
    m_categoryCombo->addItem(tr_("filter_all_categories"));
    for (const auto &cat : m_session->categories())
        m_categoryCombo->addItem(cat);
    m_categoryCombo->setStyleSheet(QString(
        "QComboBox { background-color: %1; color: %2; border: 1px solid %3;"
        "border-radius: 6px; padding: 5px 10px; font-size: 11px; }"
        "QComboBox:focus { border-color: %4; }"
        "QComboBox::drop-down { border: none; }")
        .arg(tm.surfaceColor(), tm.textColor(), tm.borderColor(), tm.accentColor()));
    m_categoryCombo->setMaximumWidth(160);
    connect(m_categoryCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int idx) {
        if (idx <= 0)
            m_proxyModel->setCategoryFilter("");
        else
            m_proxyModel->setCategoryFilter(m_categoryCombo->currentText());
    });
    filterLayout->addWidget(m_categoryCombo);

    filterLayout->addStretch();

    // Empty-state widget (shown when there are no torrents). Its three CTAs
    // route to the same MainWindow actions the toolbar uses.
    m_batWidget = new BatWidget;
    connect(m_batWidget, &BatWidget::openFileRequested, this, &MainWindow::openTorrent);
    connect(m_batWidget, &BatWidget::pasteMagnetRequested, this, &MainWindow::openMagnet);
    connect(m_batWidget, &BatWidget::openSearchRequested, this, &MainWindow::openSearch);

    // Top section: filter bar (in horizontal scroll) + table or bat widget
    auto *filterScroll = new QScrollArea;
    filterScroll->setWidget(filterBar);
    filterScroll->setWidgetResizable(true);
    filterScroll->setFrameShape(QFrame::NoFrame);
    filterScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    filterScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    filterScroll->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    filterScroll->setFixedHeight(filterBar->sizeHint().height());

    auto *tableContainer = new QWidget;
    auto *tableLayout = new QVBoxLayout(tableContainer);
    tableLayout->setContentsMargins(0, 0, 0, 0);
    tableLayout->setSpacing(0);
    tableLayout->addWidget(filterScroll);
    tableLayout->addWidget(m_tableView);

    m_topStack = new QStackedWidget;
    m_topStack->addWidget(tableContainer);  // index 0 = table
    m_topStack->addWidget(m_batWidget);     // index 1 = bat
    // Pick the right view up-front based on what loadResumeData already
    // populated. Defaulting to the bat widget caused a ~1 s flash on startup
    // because updateStatusBar only swaps after its first timer tick.
    m_topStack->setCurrentIndex(m_session->torrentCount() > 0 ? 0 : 1);

    m_speedGraph = new SpeedGraph;
    m_speedGraph->setMinimumHeight(30);
    m_detailsPanel = new DetailsPanel(m_session);
    m_detailsPanel->setMinimumHeight(60);

    // Inner splitter: graph ↔ details. Collapsible so the user can hide the
    // graph completely by dragging the handle to the edge.
    auto *bottomSplitter = new QSplitter(Qt::Vertical);
    bottomSplitter->addWidget(m_speedGraph);
    bottomSplitter->addWidget(m_detailsPanel);
    bottomSplitter->setStretchFactor(0, 1);
    bottomSplitter->setStretchFactor(1, 3);
    bottomSplitter->setSizes({120, 360});

    // Outer splitter: table ↔ (graph + details). Also collapsible — useful
    // when the user wants to focus on either the list or the details.
    auto *splitter = new QSplitter(Qt::Vertical);
    splitter->addWidget(m_topStack);
    splitter->addWidget(bottomSplitter);
    splitter->setStretchFactor(0, 2);
    splitter->setStretchFactor(1, 3);

    setCentralWidget(splitter);
}

void MainWindow::setupStatusBar()
{
    const auto &tm = ThemeManager::instance();

    m_statusLabel = new QLabel(tr_("status_no_torrents"));
    m_statusLabel->setStyleSheet(QString(
        "color: %1; font-size: 11px;").arg(tm.mutedColor()));
    statusBar()->addWidget(m_statusLabel);

    m_statusSpeedLabel = new QLabel;
    m_statusSpeedLabel->setStyleSheet(QString(
        "color: %1; font-size: 11px;").arg(tm.mutedColor()));
    statusBar()->addWidget(m_statusSpeedLabel);

    // VPN/interface indicator on the right side, before the global totals.
    // Hidden when no outgoing interface is bound.
    m_vpnLabel = new QLabel;
    m_vpnLabel->setStyleSheet(QString(
        "QLabel { color: %1; font-size: 11px; font-weight: 500; }"
        ).arg(tm.stateSeedingColor()));
    m_vpnLabel->setVisible(false);
    statusBar()->addPermanentWidget(m_vpnLabel);

    m_globalStatsLabel = new QLabel;
    m_globalStatsLabel->setStyleSheet(QString(
        "color: %1; font-size: 11px;").arg(tm.mutedColor()));
    statusBar()->addPermanentWidget(m_globalStatsLabel);
}

void MainWindow::setupTrayIcon()
{
    // Pre-rasterize the SVG at the tray sizes macOS / Windows / Linux are
    // likely to ask for. Letting Qt pick from a multi-size QIcon avoids a
    // single 1024→22 downsample that can otherwise produce a 0-height
    // NSStatusItem on macOS. Sourcing from the SVG keeps every size sharp.
    QIcon ic;
    for (int sz : {16, 22, 32, 44, 64, 128, 256}) {
        ic.addPixmap(ThemeManager::instance().themedLogo(sz, 1.0));
    }
    m_trayIcon = new QSystemTrayIcon(ic, this);

#ifndef Q_OS_MAC
    // On Windows/Linux, right-click on the tray icon shows a native context
    // menu (users expect it; it's also keyboard-navigable). Left-click opens
    // the rich custom popup. On macOS we DON'T set a context menu: NSStatusItem
    // fires both the menu AND the activated() signal on click, which would
    // pop the popup on top of the native menu.
    auto *trayMenu = new QMenu(this);
    trayMenu->addAction(tr_("tray_show"), this, &MainWindow::trayActivated);
    trayMenu->addSeparator();
    trayMenu->addAction(tr_("action_pause_all"), this, &MainWindow::pauseAll);
    trayMenu->addAction(tr_("action_resume_all"), this, &MainWindow::resumeAll);
    trayMenu->addSeparator();
    trayMenu->addAction(tr_("tray_quit"), this, [this]() {
        saveSettings();
        m_session->saveResumeData();
        QApplication::quit();
    });
    m_trayIcon->setContextMenu(trayMenu);
#endif

    m_trayPopup = new TrayPopup(m_session);
    m_trayPopup->setVpnInterface(m_session->outgoingInterface());
    m_trayPopup->setAutoShutdown(m_autoShutdown);
    connect(m_trayPopup, &TrayPopup::showWindowRequested,
            this, &MainWindow::trayActivated);
    connect(m_trayPopup, &TrayPopup::openFileRequested,
            this, &MainWindow::openTorrent);
    connect(m_trayPopup, &TrayPopup::pasteMagnetRequested,
            this, &MainWindow::openMagnet);
    connect(m_trayPopup, &TrayPopup::pauseAllRequested,
            this, &MainWindow::pauseAll);
    connect(m_trayPopup, &TrayPopup::resumeAllRequested,
            this, &MainWindow::resumeAll);
    connect(m_trayPopup, &TrayPopup::openSettingsRequested,
            this, &MainWindow::openSettings);
    connect(m_trayPopup, &TrayPopup::autoShutdownToggled,
            this, [this](bool on) {
        m_autoShutdown = on;
        saveSettings();
    });
    connect(m_trayPopup, &TrayPopup::quitRequested, this, [this]() {
        saveSettings();
        m_session->saveResumeData();
        QApplication::quit();
    });
    connect(m_session, &SessionManager::killSwitchTriggered, this, [this]() {
        if (m_trayPopup) m_trayPopup->setKillSwitchActive(true);
    });
    connect(m_session, &SessionManager::interfaceRestored, this, [this]() {
        if (m_trayPopup) m_trayPopup->setKillSwitchActive(false);
    });

    connect(m_trayIcon, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger) {
            if (m_trayPopup) {
                m_trayPopup->setAutoShutdown(m_autoShutdown);
                m_trayPopup->setVpnInterface(m_session->outgoingInterface());
                m_trayPopup->showAt(QCursor::pos());
            } else {
                trayActivated();
            }
        }
    });
    // Clicking the balloon brings the window forward AND highlights the
    // torrent that fired the notification, so the user lands directly on
    // what they were notified about instead of having to find it.
    connect(m_trayIcon, &QSystemTrayIcon::messageClicked, this, [this]() {
        trayActivated();
        if (m_lastNotifiedHash.isEmpty()) return;
        for (int i = 0; i < m_session->torrentCount(); ++i) {
            if (m_session->torrentHashAt(i) != m_lastNotifiedHash) continue;
            QModelIndex src = m_model->index(i, 0);
            QModelIndex proxy = m_proxyModel->mapFromSource(src);
            if (proxy.isValid()) {
                m_tableView->setCurrentIndex(proxy);
                m_tableView->scrollTo(proxy);
            }
            break;
        }
    });

    m_trayIcon->show();
}

void MainWindow::saveSettings()
{
    QSettings settings("BATorrent", "BATorrent");
    settings.setValue("geometry", saveGeometry());
    settings.setValue("lastSavePath", m_lastSavePath);
    settings.setValue("language", static_cast<int>(Translator::instance().language()));
    settings.setValue("maxDownload", m_session->downloadLimit());
    settings.setValue("maxUpload", m_session->uploadLimit());
    settings.setValue("startMinimized", m_startMinimized);
    settings.setValue("closeToTray", m_closeToTray);
    settings.setValue("useDefaultPath", m_useDefaultPath);
    settings.setValue("theme", static_cast<int>(ThemeManager::instance().theme()));
    settings.setValue("dhtEnabled", m_session->dhtEnabled());
    settings.setValue("encryptionMode", m_session->encryptionMode());
    settings.setValue("maxConnections", m_session->maxConnections());
    settings.setValue("seedRatioLimit", static_cast<double>(m_session->seedRatioLimit()));
    settings.setValue("tableHeaderState", m_tableView->horizontalHeader()->saveState());
    settings.setValue("tableSortColumn", m_tableView->horizontalHeader()->sortIndicatorSection());
    settings.setValue("tableSortOrder", static_cast<int>(m_tableView->horizontalHeader()->sortIndicatorOrder()));
    settings.setValue("outgoingInterface", m_session->outgoingInterface());
    settings.setValue("killSwitch", m_session->killSwitchEnabled());
    settings.setValue("autoResumeOnReconnect", m_session->autoResumeOnReconnect());
    settings.setValue("autoShutdown", m_autoShutdown);
    settings.setValue("notifSound", m_notifSoundEnabled);

    // Auto-move
    settings.setValue("autoMoveEnabled", m_session->autoMoveEnabled());
    settings.setValue("autoMovePath", m_session->autoMovePath());

    // Download queue
    settings.setValue("maxActiveDownloads", m_session->maxActiveDownloads());

    // Proxy
    settings.setValue("proxyType", m_session->proxyType());
    settings.setValue("proxyHost", m_session->proxyHost());
    settings.setValue("proxyPort", m_session->proxyPort());
    settings.setValue("proxyUser", m_session->proxyUser());
    // Stored via SecretStore instead of QSettings — see saveSettings's tail.

    // IP Filter
    settings.setValue("ipFilterPath", m_session->ipFilterPath());

    // Bandwidth Scheduler
    settings.setValue("schedulerEnabled", m_session->schedulerEnabled());
    settings.setValue("altDownLimit", m_session->altDownloadLimit());
    settings.setValue("altUpLimit", m_session->altUploadLimit());
    settings.setValue("scheduleFromHour", m_session->scheduleFromHour());
    settings.setValue("scheduleToHour", m_session->scheduleToHour());
    settings.setValue("scheduleDays", m_session->scheduleDays());

    // Credentials live in the OS keyring (via SecretStore), not QSettings.
    SecretStore::instance().set("proxyPass", m_session->proxyPass());
}

void MainWindow::loadSettings()
{
    QSettings settings("BATorrent", "BATorrent");
    if (settings.contains("geometry")) {
        restoreGeometry(settings.value("geometry").toByteArray());
        // Clamp to a visible screen — if the user undocks an external monitor
        // the saved coords can land off-screen and the window becomes
        // unreachable except via the tray icon.
        const QRect avail = screen() ? screen()->availableGeometry()
                                     : QGuiApplication::primaryScreen()->availableGeometry();
        if (!avail.intersects(geometry())) {
            QRect g = geometry();
            g.moveCenter(avail.center());
            if (g.width() > avail.width()) g.setWidth(avail.width());
            if (g.height() > avail.height()) g.setHeight(avail.height());
            setGeometry(g);
        }
    }
    m_lastSavePath = settings.value("lastSavePath",
        QStandardPaths::writableLocation(QStandardPaths::DownloadLocation)).toString();

    int lang;
    if (settings.contains("language")) {
        lang = settings.value("language").toInt();
    } else {
        // Auto-detect from system locale on first launch. Covers all 7
        // supported languages so international users see their language
        // immediately without visiting Settings first.
        const QString sys = QLocale::system().name().toLower();
        if      (sys.startsWith("pt")) lang = 1; // Portuguese
        else if (sys.startsWith("zh")) lang = 2; // Chinese
        else if (sys.startsWith("ja")) lang = 3; // Japanese
        else if (sys.startsWith("ru")) lang = 4; // Russian
        else if (sys.startsWith("es")) lang = 5; // Spanish
        else if (sys.startsWith("de")) lang = 6; // German
        else                           lang = 0; // English
    }
    Translator::instance().setLanguage(static_cast<Translator::Language>(lang));

    int maxDown = settings.value("maxDownload", 0).toInt();
    int maxUp = settings.value("maxUpload", 0).toInt();
    m_session->setDownloadLimit(maxDown);
    m_session->setUploadLimit(maxUp);

    m_startMinimized = settings.value("startMinimized", false).toBool();
    m_closeToTray = settings.value("closeToTray", true).toBool();
    m_useDefaultPath = settings.value("useDefaultPath", false).toBool();

    int theme = settings.value("theme", 0).toInt();
    ThemeManager::instance().setTheme(static_cast<ThemeManager::Theme>(theme));

    // Network settings
    bool dht = settings.value("dhtEnabled", true).toBool();
    m_session->setDhtEnabled(dht);
    int enc = settings.value("encryptionMode", 0).toInt();
    m_session->setEncryptionMode(enc);
    bool utp = settings.value("utpEnabled", true).toBool();
    m_session->setUtpEnabled(utp);
    int maxConn = settings.value("maxConnections", 200).toInt();
    m_session->setMaxConnections(maxConn);
    float seedRatio = settings.value("seedRatioLimit", 0.0).toFloat();
    m_session->setSeedRatioLimit(seedRatio);

    // Listening port: optionally re-randomize on each launch so trackers
    // and ISPs can't trivially fingerprint the client across sessions.
    bool randomizePort = settings.value("randomizePort", false).toBool();
    if (randomizePort) {
        // Use ephemeral port range so we never collide with well-known
        // services. 49152-65535 is the IANA ephemeral block.
        const int port = static_cast<int>(QRandomGenerator::system()
                                              ->bounded(49152, 65536));
        m_session->setListenPort(port);
        settings.setValue("listenPort", port);
    } else if (settings.contains("listenPort")) {
        m_session->setListenPort(settings.value("listenPort").toInt());
    }

    // VPN settings
    QString iface = settings.value("outgoingInterface", "").toString();
    m_session->setOutgoingInterface(iface);
    bool killSwitch = settings.value("killSwitch", false).toBool();
    m_session->setKillSwitchEnabled(killSwitch);
    bool autoResume = settings.value("autoResumeOnReconnect", false).toBool();
    m_session->setAutoResumeOnReconnect(autoResume);

    // Auto-shutdown
    m_autoShutdown = settings.value("autoShutdown", false).toBool();
    m_notifSoundEnabled = settings.value("notifSound", true).toBool();

    // Auto-move
    m_session->setAutoMove(
        settings.value("autoMoveEnabled", false).toBool(),
        settings.value("autoMovePath").toString());

    // Download queue
    m_session->setMaxActiveDownloads(settings.value("maxActiveDownloads", 0).toInt());

    // Proxy
    int proxyType = settings.value("proxyType", 0).toInt();
    m_session->setProxySettings(proxyType,
        settings.value("proxyHost").toString(),
        settings.value("proxyPort", 0).toInt(),
        settings.value("proxyUser").toString(),
        SecretStore::instance().get("proxyPass"));

    // IP Filter
    QString ipFilter = settings.value("ipFilterPath").toString();
    if (!ipFilter.isEmpty())
        m_session->loadIpFilter(ipFilter);

    // Bandwidth Scheduler
    m_session->setAltSpeedLimits(
        settings.value("altDownLimit", 0).toInt(),
        settings.value("altUpLimit", 0).toInt());
    m_session->setScheduleFromHour(settings.value("scheduleFromHour", 1).toInt());
    m_session->setScheduleToHour(settings.value("scheduleToHour", 7).toInt());
    m_session->setScheduleDays(settings.value("scheduleDays", 0x7F).toInt());
    m_session->setSchedulerEnabled(settings.value("schedulerEnabled", false).toBool());

    // WebUI
    startWebServer();
}

void MainWindow::startWebServer()
{
    QSettings settings("BATorrent", "BATorrent");
    bool webUiEnabled = settings.value("webUiEnabled", false).toBool();

    if (m_webServer) {
        m_webServer->stop();
        delete m_webServer;
        m_webServer = nullptr;
    }

    if (webUiEnabled) {
        m_webServer = new WebServer(m_session, this);
        int port = settings.value("webUiPort", 8080).toInt();
        bool remote = settings.value("webUiRemoteAccess", false).toBool();
        QString user = settings.value("webUiUser", "admin").toString();
        QString passHash = SecretStore::instance().get("webUiPasswordHash");
        if (!user.isEmpty() && !passHash.isEmpty())
            m_webServer->setCredentials(user, passHash);
        m_webServer->start(static_cast<quint16>(port), remote);
    }
}

void MainWindow::openTorrent()
{
    QString file = QFileDialog::getOpenFileName(this, tr_("dlg_open_torrent"),
                                                 QString(), tr_("dlg_torrent_filter"));
    if (file.isEmpty())
        return;

    addTorrentFile(file);
}

QString MainWindow::chooseSavePath()
{
    if (m_useDefaultPath) {
        // The user enabled "always use default path" but the default may now
        // be unreachable (external drive unplugged, network share offline,
        // user moved their folder). Fall back to the system Downloads dir
        // and warn so they know we changed targets — better than adding the
        // torrent and watching every write fail later with file_error_alert.
        if (QDir(m_lastSavePath).exists())
            return m_lastSavePath;

        QString fallback = QStandardPaths::writableLocation(
            QStandardPaths::DownloadLocation);
        QMessageBox::warning(this, tr_("dlg_error"),
            tr_("dlg_save_path_missing").arg(m_lastSavePath, fallback));
        return fallback;
    }

    QString path = QFileDialog::getExistingDirectory(this, tr_("dlg_choose_folder"), m_lastSavePath);
    if (path.isEmpty())
        return {};

    m_lastSavePath = path;
    return path;
}

void MainWindow::addTorrentFile(const QString &filePath)
{
    // Show the confirmation dialog so the user can change the save path,
    // pick which files to download, and choose start-immediately. The path
    // input + file tree live in the same dialog so they don't need to go
    // through two separate steps.
    AddTorrentDialog dlg(filePath, QString(), m_lastSavePath, this);
    if (dlg.exec() != QDialog::Accepted) return;
    const QString savePath = dlg.savePath();
    if (savePath.isEmpty()) return;
    m_lastSavePath = savePath;

    const auto priorities = dlg.filePriorities();
    if (priorities.empty())
        m_session->addTorrent(filePath, savePath);
    else
        m_session->addTorrentWithPriorities(filePath, savePath, priorities);
    if (!dlg.startImmediately())
        m_session->pauseTorrent(m_session->torrentCount() - 1);
}

void MainWindow::addTorrentFromCli(const QString &filePath)
{
    addTorrentFile(filePath);
}

void MainWindow::addMagnetFromCli(const QString &uri)
{
    AddTorrentDialog dlg(QString(), uri, m_lastSavePath, this);
    if (dlg.exec() != QDialog::Accepted) return;
    const QString savePath = dlg.savePath();
    if (savePath.isEmpty()) return;
    m_lastSavePath = savePath;
    m_session->addMagnet(uri, savePath);
    if (!dlg.startImmediately())
        m_session->pauseTorrent(m_session->torrentCount() - 1);
}

void MainWindow::openMagnet()
{
    QString magnet = QInputDialog::getText(this, tr_("dlg_add_magnet"),
                                            tr_("dlg_paste_magnet"));
    if (magnet.isEmpty() || !magnet.startsWith("magnet:"))
        return;

    AddTorrentDialog dlg(QString(), magnet, m_lastSavePath, this);
    if (dlg.exec() != QDialog::Accepted) return;
    const QString savePath = dlg.savePath();
    if (savePath.isEmpty()) return;
    m_lastSavePath = savePath;
    m_session->addMagnet(magnet, savePath);
    if (!dlg.startImmediately())
        m_session->pauseTorrent(m_session->torrentCount() - 1);
}

void MainWindow::removeSelected()
{
    auto rows = selectedRows();
    if (rows.isEmpty()) return;
    // Snapshot the .resume bytes BEFORE removal so the toast undo button has
    // something to restore from. Order doesn't matter for the capture; we
    // sort descending for the actual remove call to keep indices valid.
    QList<QByteArray> snapshots;
    for (int r : rows) {
        QByteArray data = m_session->captureResumeData(r);
        if (!data.isEmpty()) snapshots.append(data);
    }
    std::sort(rows.begin(), rows.end(), std::greater<int>());
    for (int r : rows)
        m_session->removeTorrent(r, false);
    if (snapshots.isEmpty()) return;
    const QString body = snapshots.size() == 1
        ? tr_("toast_removed_one")
        : tr_("toast_removed_many").arg(snapshots.size());
    // Stash snapshots on the toast click; on a 5 s dismiss they're discarded.
    m_pendingUndoRemove = snapshots;
    Toast::notify(tr_("toast_removed_title"), body, Toast::Info,
                  this, SLOT(undoLastRemove()));
}

void MainWindow::removeSelectedWithFiles()
{
    auto rows = selectedRows();
    if (rows.isEmpty()) return;

    auto reply = QMessageBox::warning(this, tr_("dlg_confirm_delete"),
        tr_("dlg_confirm_delete_msg"),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (reply != QMessageBox::Yes) return;

    std::sort(rows.begin(), rows.end(), std::greater<int>());
    for (int r : rows)
        m_session->removeTorrent(r, true);
}

void MainWindow::pauseSelected()
{
    for (int r : selectedRows())
        m_session->pauseTorrent(r);
}

void MainWindow::resumeSelected()
{
    for (int r : selectedRows())
        m_session->resumeTorrent(r);
}

void MainWindow::stopSelected()
{
    auto rows = selectedRows();
    if (rows.isEmpty()) return;
    // Stop only acts on torrents that finished downloading — "completed" is
    // meaningless for an in-flight torrent. For in-progress rows the button
    // is a no-op; the regular Pause action covers that intent.
    for (int r : rows) {
        TorrentInfo info = m_session->torrentAt(r);
        if (info.progress >= 1.0f)
            m_session->markCompleted(r);
    }
}

void MainWindow::createTorrent()
{
    CreateTorrentDialog dlg(this);
    dlg.exec();
}

void MainWindow::importQBittorrent()
{
    int count = m_session->importFromQBittorrent(m_lastSavePath);
    if (count > 0)
        QMessageBox::information(this, "Import", tr_("import_qbt_success").arg(count));
    else
        QMessageBox::information(this, "Import", tr_("import_qbt_none"));
}

void MainWindow::pauseAll()
{
    // Count active (non-paused, non-completed) torrents — pausing while
    // nothing is running isn't worth a dialog, but pausing 20 seeding
    // torrents by an accidental hotkey definitely is.
    int active = 0;
    for (int i = 0; i < m_session->torrentCount(); ++i) {
        TorrentInfo info = m_session->torrentAt(i);
        if (!info.paused && !info.completed) ++active;
    }
    if (active >= 2) {
        auto reply = QMessageBox::question(this, tr_("dlg_pause_all_title"),
            tr_("dlg_pause_all_msg").arg(active),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
        if (reply != QMessageBox::Yes) return;
    }
    m_session->pauseAll();
}

void MainWindow::diagnoseSlow(int row)
{
    if (row < 0 || row >= m_session->torrentCount()) return;
    TorrentInfo info = m_session->torrentAt(row);
    QStringList lines;
    // Single-finding mode: tell the user the most likely reason first;
    // others are listed beneath for transparency. Bias toward concrete
    // fixes ("reannounce", "check VPN") over generic descriptions.
    if (info.paused) {
        lines << "★ " + tr_("diag_paused");
    } else if (info.completed) {
        lines << "★ " + tr_("diag_completed");
    } else if (info.progress >= 1.0f) {
        if (info.uploadRate == 0)
            lines << "★ " + tr_("diag_seeding_no_uploaders");
        else
            lines << "★ " + tr_("diag_seeding_ok");
    } else {
        // Active download diagnostics.
        if (info.numPeers == 0) {
            lines << "★ " + tr_("diag_no_peers");
        } else if (info.numSeeds == 0) {
            lines << "★ " + tr_("diag_no_seeds");
        } else if (info.downloadRate == 0) {
            lines << "★ " + tr_("diag_choked");
        } else {
            const int dlimit = m_session->torrentDownloadLimit(row);
            if (dlimit > 0 && info.downloadRate >= dlimit * 1024 * 0.9) {
                lines << "★ " + tr_("diag_at_local_limit").arg(dlimit);
            } else {
                lines << "★ " + tr_("diag_throughput_normal")
                              .arg(formatSpeed(info.downloadRate));
            }
        }
    }
    // Always show the supporting facts so the user can verify.
    lines << "";
    lines << tr_("diag_facts");
    lines << QStringLiteral("    • %1: %2").arg(tr_("col_peers"), QString::number(info.numPeers));
    lines << QStringLiteral("    • %1: %2").arg(tr_("detail_seeds"), QString::number(info.numSeeds));
    lines << QStringLiteral("    • %1: %2").arg(tr_("col_down"), formatSpeed(info.downloadRate));
    lines << QStringLiteral("    • %1: %2").arg(tr_("col_up"), formatSpeed(info.uploadRate));
    lines << QStringLiteral("    • %1: %2").arg(tr_("col_state"), info.stateString);

    QMessageBox box(QMessageBox::NoIcon, tr_("ctx_why_slow"), lines.join('\n'),
                    QMessageBox::Ok, this);
    box.exec();
}

void MainWindow::undoLastRemove()
{
    if (m_pendingUndoRemove.isEmpty()) return;
    int restored = 0;
    for (const auto &snapshot : m_pendingUndoRemove)
        if (m_session->restoreFromResumeData(snapshot)) ++restored;
    m_pendingUndoRemove.clear();
    if (restored > 0)
        Toast::notify(tr_("toast_restored_title"),
                      tr_("toast_restored_body").arg(restored),
                      Toast::Success);
}

void MainWindow::resumeAll()
{
    m_session->resumeAll();
}

void MainWindow::updateStatusBar()
{
    int count = m_session->torrentCount();
    int totalDown = 0, totalUp = 0;
    float totalProgress = 0;

    for (int i = 0; i < count; ++i) {
        TorrentInfo info = m_session->torrentAt(i);
        totalDown += info.downloadRate;
        totalUp += info.uploadRate;
        totalProgress += info.progress;
    }

    // Feed speed graph
    m_speedGraph->addDataPoint(totalDown, totalUp);

    // Toggle bat widget vs table with fade transition
    int target = (count == 0) ? 1 : 0;
    if (m_topStack->currentIndex() != target) {
        auto *incoming = m_topStack->widget(target);
        auto *effect = new QGraphicsOpacityEffect(incoming);
        incoming->setGraphicsEffect(effect);
        m_topStack->setCurrentIndex(target);

        auto *anim = new QPropertyAnimation(effect, "opacity", this);
        anim->setDuration(300);
        anim->setStartValue(0.0);
        anim->setEndValue(1.0);
        connect(anim, &QPropertyAnimation::finished, this, [incoming]() {
            incoming->setGraphicsEffect(nullptr); // clean up
        });
        anim->start(QAbstractAnimation::DeleteWhenStopped);
    }

    // Toolbar bandwidth pill — written to the per-direction labels found by
    // objectName so the pill keeps its icon+value+unit structure rather than
    // collapsing into a single text run.
    auto splitSpeed = [](int bps) -> std::pair<QString, QString> {
        if (bps < 1024)        return {QString::number(bps), "B/s"};
        if (bps < 1024 * 1024) return {QString::number(bps / 1024.0, 'f', 1), "KB/s"};
        return {QString::number(bps / (1024.0 * 1024.0), 'f', 1), "MB/s"};
    };
    if (auto *root = findChild<QFrame *>()) {
        if (auto *dv = findChild<QLabel *>(QStringLiteral("pillDownVal"))) {
            auto [v, u] = splitSpeed(totalDown);
            dv->setText(v);
            if (auto *du = findChild<QLabel *>(QStringLiteral("pillDownUnit")))
                du->setText(u);
        }
        if (auto *uv = findChild<QLabel *>(QStringLiteral("pillUpVal"))) {
            auto [v, u] = splitSpeed(totalUp);
            uv->setText(v);
            if (auto *uu = findChild<QLabel *>(QStringLiteral("pillUpUnit")))
                uu->setText(u);
        }
        Q_UNUSED(root);
    }

    // VPN/interface indicator — show only when we've bound an outgoing
    // interface in Settings; otherwise the pill in the status bar is hidden.
    if (m_vpnLabel) {
        const QString iface = m_session->outgoingInterface();
        if (iface.isEmpty()) {
            m_vpnLabel->setVisible(false);
        } else {
            m_vpnLabel->setText(QStringLiteral("● VPN %1").arg(iface));
            m_vpnLabel->setVisible(true);
        }
    }

    if (count == 0) {
        m_statusLabel->setText(tr_("status_no_torrents"));
        return;
    }

    int activeCount = 0, downloadingCount = 0, seedingCount = 0,
        pausedCount = 0, finishedCount = 0, completedCount = 0, queuedCount = 0;
    for (int i = 0; i < count; ++i) {
        TorrentInfo info = m_session->torrentAt(i);
        if (info.completed) ++completedCount;
        else if (info.paused) ++pausedCount;
        else if (info.downloadRate > 0 || info.uploadRate > 0) ++activeCount;
        const QString &st = info.stateString;
        if (st == tr_("state_downloading")) ++downloadingCount;
        else if (st == tr_("state_seeding")) ++seedingCount;
        else if (st == tr_("state_finished")) ++finishedCount;
        else if (st == tr_("state_queued"))   ++queuedCount;
    }
    m_statusLabel->setText(QStringLiteral("%1 torrents · %2 active")
                               .arg(count).arg(activeCount));

    if (m_statusSpeedLabel) {
        m_statusSpeedLabel->setText(QStringLiteral("   ↓ %1   ↑ %2")
                                        .arg(formatSpeed(totalDown),
                                             formatSpeed(totalUp)));
    }

    // Refresh filter pill counts.
    for (QPushButton *pill : m_filterPills) {
        const QString state = pill->property("filterState").toString();
        const QString label = pill->property("filterLabel").toString();
        int n = 0;
        if (state.isEmpty())                 n = count;
        else if (state == "all_active")      n = activeCount;
        else if (state == "downloading")     n = downloadingCount;
        else if (state == "seeding")         n = seedingCount;
        else if (state == "paused")          n = pausedCount;
        else if (state == "finished")        n = finishedCount;
        else if (state == "completed")       n = completedCount;
        else if (state == "queued")          n = queuedCount;
        pill->setText(QStringLiteral("%1   %2").arg(label).arg(n));
    }

    float avgProgress = totalProgress / count;
    // Tray icon is created via a deferred singleShot(0) (NSStatusItem timing
    // workaround), so the first updateStatusBar after a fast first tick can
    // hit it before setupTrayIcon runs. Guard.
    if (m_trayIcon)
        m_trayIcon->setToolTip(QString("BATorrent - %1%").arg(static_cast<int>(avgProgress * 100)));

    if (m_globalStatsLabel)
        m_globalStatsLabel->setText(QStringLiteral("Total: %1 down · %2 up   · Ratio %3")
            .arg(formatSize(m_session->globalDownloaded()),
                 formatSize(m_session->globalUploaded()),
                 QString::number(static_cast<double>(m_session->globalRatio()), 'f', 2)));
}

void MainWindow::onSelectionChanged()
{
    auto rows = selectedRows();
    m_detailsPanel->showTorrent(rows.isEmpty() ? -1 : rows.first());
}

void MainWindow::onTorrentFinished(const QString &name, const QString &infoHash)
{
    qDebug() << "[ui] torrent finished:" << name;
    QString completedBody = name;
    for (int i = 0; i < m_session->torrentCount(); ++i) {
        if (m_session->torrentHashAt(i) == infoHash) {
            const qint64 totalSize = m_session->torrentAt(i).totalSize;
            if (totalSize > 0)
                completedBody = QStringLiteral("%1 · %2")
                                    .arg(name, formatSize(totalSize));
            break;
        }
    }
    // Click on a "download complete" toast jumps the user straight to the
    // file in Finder / Explorer instead of just raising the app — the more
    // common follow-up action than "look at the torrent row I just saw".
    Toast::notify(tr_("dlg_download_complete"), completedBody,
                  Toast::Success, this, SLOT(revealLastNotified()));
    // Also fire a native OS notification so it lands in Notification Center
    // (macOS) / Action Center (Windows) / D-Bus org.freedesktop.Notifications
    // (Linux) even if the user is on a different desktop / display.
    if (m_trayIcon && m_trayIcon->isVisible() && m_trayIcon->supportsMessages()) {
        m_trayIcon->showMessage(tr_("dlg_download_complete"), completedBody,
                                QSystemTrayIcon::Information, 5000);
    }
    if (m_notifSoundEnabled)
        QApplication::beep();
    // Remember the hash so clicking the balloon focuses the matching row
    // rather than just raising the window.
    m_lastNotifiedHash = infoHash;
    m_model->flashRow(infoHash);
    notifyMediaServers();
    checkAutoShutdown();
}

void MainWindow::notifyMediaServers()
{
    QSettings settings("BATorrent", "BATorrent");

    if (!m_mediaServerNam)
        m_mediaServerNam = new QNetworkAccessManager(this);

    // Plex library scan
    if (settings.value("plexEnabled", false).toBool()) {
        QString url = settings.value("plexUrl").toString();
        QString token = SecretStore::instance().get("plexToken");
        if (!url.isEmpty() && !token.isEmpty()) {
            // Refresh all sections
            QNetworkRequest req(QUrl(url + "/library/sections/all/refresh?X-Plex-Token=" + token));
            req.setHeader(QNetworkRequest::UserAgentHeader, "BATorrent");
            auto *reply = m_mediaServerNam->get(req);
            connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
        }
    }

    // Jellyfin/Emby library scan
    if (settings.value("jellyfinEnabled", false).toBool()) {
        QString url = settings.value("jellyfinUrl").toString();
        QString apiKey = SecretStore::instance().get("jellyfinApiKey");
        if (!url.isEmpty() && !apiKey.isEmpty()) {
            QNetworkRequest req(QUrl(url + "/Library/Refresh?api_key=" + apiKey));
            req.setHeader(QNetworkRequest::UserAgentHeader, "BATorrent");
            auto *reply = m_mediaServerNam->post(req, QByteArray());
            connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
        }
    }
}

void MainWindow::onTorrentError(const QString &message)
{
    QMessageBox::warning(this, tr_("dlg_error"), message);
}

void MainWindow::trayActivated()
{
    // showNormal unminimizes — plain show() is a no-op on an already-visible
    // but minimized window, which is exactly the state a tray-only app sits in.
    if (isMinimized() || !isVisible())
        showNormal();
    else
        show();
    raise();
    activateWindow();
}

void MainWindow::refreshDiscordPresence()
{
    if (!m_discordRpc || m_discordRpc->clientId().isEmpty()) return;
    // Find the most interesting torrent to feature — bias toward the largest
    // active download. If nothing's downloading, mention seeding count.
    int activeDownloads = 0, seeding = 0;
    int featured = -1;
    int featuredRate = 0;
    for (int i = 0; i < m_session->torrentCount(); ++i) {
        TorrentInfo info = m_session->torrentAt(i);
        if (info.paused || info.completed) continue;
        if (info.progress >= 1.0f) { ++seeding; continue; }
        ++activeDownloads;
        if (info.downloadRate > featuredRate) {
            featuredRate = info.downloadRate;
            featured = i;
        }
    }
    if (featured >= 0) {
        TorrentInfo info = m_session->torrentAt(featured);
        const QString name = info.name.left(64);
        const QString state = QStringLiteral("%1% · ↓ %2")
            .arg(static_cast<int>(info.progress * 100))
            .arg(formatSpeed(info.downloadRate));
        m_discordRpc->setActivity(name, state, m_discordSessionStart);
    } else if (seeding > 0) {
        m_discordRpc->setActivity(
            tr_("discord_seeding").arg(seeding),
            tr_("discord_seeding_state"),
            m_discordSessionStart);
    } else {
        m_discordRpc->setActivity(tr_("discord_idle"), QString(), m_discordSessionStart);
    }
}

void MainWindow::revealLastNotified()
{
    if (m_lastNotifiedHash.isEmpty()) { trayActivated(); return; }
    for (int i = 0; i < m_session->torrentCount(); ++i) {
        if (m_session->torrentHashAt(i) != m_lastNotifiedHash) continue;
        QString root = m_session->torrentRootPath(i);
        if (!root.isEmpty()) {
            revealInFileManager(root);
            return;
        }
    }
    trayActivated();
}

void MainWindow::openSettings()
{
    qDebug() << "[ui] openSettings";
    SettingsDialog dlg(this);
    dlg.setDefaultSavePath(m_lastSavePath);
    dlg.setMaxDownloadSpeed(m_session->downloadLimit());
    dlg.setMaxUploadSpeed(m_session->uploadLimit());
    dlg.setLanguageIndex(static_cast<int>(Translator::instance().language()));
    dlg.setStartMinimized(m_startMinimized);
    dlg.setCloseToTray(m_closeToTray);
    dlg.setUseDefaultPath(m_useDefaultPath);
    dlg.setThemeIndex(static_cast<int>(ThemeManager::instance().theme()));
    dlg.setDhtEnabled(m_session->dhtEnabled());
    dlg.setEncryptionMode(m_session->encryptionMode());
    dlg.setMaxConnections(m_session->maxConnections());
    dlg.setSeedRatioLimit(m_session->seedRatioLimit());
    dlg.setOutgoingInterface(m_session->outgoingInterface());
    dlg.setKillSwitchEnabled(m_session->killSwitchEnabled());
    dlg.setAutoResumeOnReconnect(m_session->autoResumeOnReconnect());
    dlg.setAutoShutdown(m_autoShutdown);
    dlg.setNotifSoundEnabled(m_notifSoundEnabled);
    dlg.setVerboseLogEnabled(Logger::instance().level() <= Logger::Debug);
    {
        QSettings s("BATorrent", "BATorrent");
        dlg.setSpeedUnit(s.value("speedUnit", 0).toInt());
        dlg.setUpdateChannel(s.value("updateChannel", "github").toString());
        dlg.setTelegramToken(SecretStore::instance().get("telegramBotToken"));
        dlg.setTelegramChatId(s.value("telegramChatId").toString());
        dlg.setTelegramEvents(s.value("telegramEvents", 0xF).toInt());
        dlg.setDiscordEnabled(s.value("discordEnabled", true).toBool());
        dlg.loadAdvancedSettings(m_session->advancedSettings());
    }
    dlg.setAutoMoveEnabled(m_session->autoMoveEnabled());
    dlg.setRunOnComplete(m_session->runOnComplete());
    dlg.setWatchedFolder(m_session->watchedFolder());
    dlg.setAutoMovePath(m_session->autoMovePath());
    dlg.setMaxActiveDownloads(m_session->maxActiveDownloads());
    dlg.setStopAfterDownload(m_session->stopAfterDownload());
    dlg.setMaxSeedDays(static_cast<int>(m_session->maxSeedSeconds() / 86400));
    dlg.setAutoCompleteSeconds(m_session->autoCompleteSeconds());
    {
        QSettings s("BATorrent", "BATorrent");
        dlg.setUtpEnabled(m_session->utpEnabled());
        dlg.setAnonymousMode(m_session->anonymousMode());
        dlg.setForceIpv4(m_session->forceIpv4());
        dlg.setPtMode(m_session->ptMode());
        dlg.setBlockLeechers(m_session->blockLeecherClients());
        dlg.setRandomizePort(s.value("randomizePort", false).toBool());
        dlg.setListenPort(s.value("listenPort", 6881).toInt());
    }

    {
        QSettings settings("BATorrent", "BATorrent");
        dlg.setWebUiEnabled(settings.value("webUiEnabled", false).toBool());
        dlg.setWebUiPort(settings.value("webUiPort", 8080).toInt());
        dlg.setWebUiUser(settings.value("webUiUser", "admin").toString());
        dlg.setWebUiPasswordHash(SecretStore::instance().get("webUiPasswordHash"));
        dlg.setWebUiRemoteAccess(settings.value("webUiRemoteAccess", false).toBool());

        // Proxy
        dlg.setProxyType(m_session->proxyType());
        dlg.setProxyHost(m_session->proxyHost());
        dlg.setProxyPort(m_session->proxyPort());
        dlg.setProxyUser(m_session->proxyUser());
        dlg.setProxyPass(m_session->proxyPass());

        // IP Filter
        dlg.setIpFilterPath(m_session->ipFilterPath());

        // Bandwidth Scheduler
        dlg.setSchedulerEnabled(m_session->schedulerEnabled());
        dlg.setAltDownloadSpeed(m_session->altDownloadLimit());
        dlg.setAltUploadSpeed(m_session->altUploadLimit());
        dlg.setScheduleFromHour(m_session->scheduleFromHour());
        dlg.setScheduleToHour(m_session->scheduleToHour());
        dlg.setScheduleDays(m_session->scheduleDays());

        // Media Server
        dlg.setPlexEnabled(settings.value("plexEnabled", false).toBool());
        dlg.setPlexUrl(settings.value("plexUrl").toString());
        dlg.setPlexToken(SecretStore::instance().get("plexToken"));
        dlg.setJellyfinEnabled(settings.value("jellyfinEnabled", false).toBool());
        dlg.setJellyfinUrl(settings.value("jellyfinUrl").toString());
        dlg.setJellyfinApiKey(SecretStore::instance().get("jellyfinApiKey"));
    }

    if (dlg.exec() == QDialog::Accepted) {
        m_lastSavePath = dlg.defaultSavePath();
        m_session->setDownloadLimit(dlg.maxDownloadSpeed());
        m_session->setUploadLimit(dlg.maxUploadSpeed());
        m_startMinimized = dlg.startMinimized();
        m_closeToTray = dlg.closeToTray();
        m_useDefaultPath = dlg.useDefaultPath();

        int newLang = dlg.languageIndex();
        if (newLang != static_cast<int>(Translator::instance().language())) {
            Translator::instance().setLanguage(static_cast<Translator::Language>(newLang));
            retranslateUi();
        }

        int newTheme = dlg.themeIndex();
        if (newTheme != static_cast<int>(ThemeManager::instance().theme())) {
            ThemeManager::instance().setTheme(static_cast<ThemeManager::Theme>(newTheme));
            applyTheme();
        }

        // Network settings
        m_session->setDhtEnabled(dlg.dhtEnabled());
        m_session->setEncryptionMode(dlg.encryptionMode());
        m_session->setMaxConnections(dlg.maxConnections());
        m_session->setSeedRatioLimit(dlg.seedRatioLimit());

        // VPN settings
        m_session->setOutgoingInterface(dlg.outgoingInterface());
        m_session->setKillSwitchEnabled(dlg.killSwitchEnabled());
        m_session->setAutoResumeOnReconnect(dlg.autoResumeOnReconnect());

        // Auto-shutdown & notifications
        m_autoShutdown = dlg.autoShutdown();
        m_notifSoundEnabled = dlg.notifSoundEnabled();
        Logger::instance().setLevel(dlg.verboseLogEnabled() ? Logger::Debug : Logger::Info);
        {
            QSettings s("BATorrent", "BATorrent");
            s.setValue("speedUnit", dlg.speedUnit());
            setSpeedUnit(dlg.speedUnit());
            s.setValue("updateChannel", dlg.updateChannel());
            SecretStore::instance().set("telegramBotToken", dlg.telegramToken());
            s.setValue("telegramChatId", dlg.telegramChatId());
            s.setValue("telegramEvents", dlg.telegramEvents());
            if (m_telegramNotifier) m_telegramNotifier->reload();
            s.setValue("discordEnabled", dlg.discordEnabled());
            if (m_discordRpc) {
                if (dlg.discordEnabled())
                    m_discordRpc->setClientId(QStringLiteral("1508208411282640956"));
                else
                    m_discordRpc->setClientId(QString());
            }
            m_session->setAdvancedSettings(dlg.collectAdvancedSettings());
        }
        // Auto-move
        m_session->setAutoMove(dlg.autoMoveEnabled(), dlg.autoMovePath());
        m_session->setRunOnComplete(dlg.runOnComplete());
        m_session->setWatchedFolder(dlg.watchedFolder());

        // Download queue
        m_session->setMaxActiveDownloads(dlg.maxActiveDownloads());

        // Stop-seeding rules
        m_session->setStopAfterDownload(dlg.stopAfterDownload());
        m_session->setMaxSeedSeconds(static_cast<qint64>(dlg.maxSeedDays()) * 86400);
        m_session->setAutoCompleteSeconds(dlg.autoCompleteSeconds());

        // Transport + port
        m_session->setUtpEnabled(dlg.utpEnabled());
        m_session->setAnonymousMode(dlg.anonymousMode());
        m_session->setForceIpv4(dlg.forceIpv4());
        m_session->setPtMode(dlg.ptMode());
        m_session->setBlockLeecherClients(dlg.blockLeechers());
        {
            QSettings s("BATorrent", "BATorrent");
            s.setValue("utpEnabled", dlg.utpEnabled());
            s.setValue("randomizePort", dlg.randomizePort());
            if (!dlg.randomizePort()) {
                s.setValue("listenPort", dlg.listenPort());
                m_session->setListenPort(dlg.listenPort());
            }
        }

        // Proxy
        m_session->setProxySettings(dlg.proxyType(), dlg.proxyHost(),
                                     dlg.proxyPort(), dlg.proxyUser(), dlg.proxyPass());

        // IP Filter
        QString ipFilterPath = dlg.ipFilterPath();
        if (ipFilterPath != m_session->ipFilterPath()) {
            if (ipFilterPath.isEmpty())
                m_session->clearIpFilter();
            else
                m_session->loadIpFilter(ipFilterPath);
        }

        // Bandwidth Scheduler
        m_session->setAltSpeedLimits(dlg.altDownloadSpeed(), dlg.altUploadSpeed());
        m_session->setScheduleFromHour(dlg.scheduleFromHour());
        m_session->setScheduleToHour(dlg.scheduleToHour());
        m_session->setScheduleDays(dlg.scheduleDays());
        m_session->setSchedulerEnabled(dlg.schedulerEnabled());

        // WebUI settings
        {
            QSettings settings("BATorrent", "BATorrent");
            settings.setValue("webUiEnabled", dlg.webUiEnabled());
            settings.setValue("webUiPort", dlg.webUiPort());
            settings.setValue("webUiUser", dlg.webUiUser());
            settings.setValue("webUiRemoteAccess", dlg.webUiRemoteAccess());
            QString passHash = dlg.webUiPasswordHash();
            if (!passHash.isEmpty())
                SecretStore::instance().set("webUiPasswordHash", passHash);
            startWebServer();

            // Media Server settings — secrets live in the OS keyring, the
            // rest in QSettings.
            settings.setValue("plexEnabled", dlg.plexEnabled());
            settings.setValue("plexUrl", dlg.plexUrl());
            SecretStore::instance().set("plexToken", dlg.plexToken());
            settings.setValue("jellyfinEnabled", dlg.jellyfinEnabled());
            settings.setValue("jellyfinUrl", dlg.jellyfinUrl());
            SecretStore::instance().set("jellyfinApiKey", dlg.jellyfinApiKey());
        }
    }
}

void MainWindow::showWelcome()
{
    WelcomeDialog dlg(this);
    connect(&dlg, &WelcomeDialog::openFileRequested,   this, &MainWindow::openTorrent);
    connect(&dlg, &WelcomeDialog::pasteMagnetRequested, this, &MainWindow::openMagnet);
    connect(&dlg, &WelcomeDialog::openSearchRequested, this, &MainWindow::openSearch);
    connect(&dlg, &WelcomeDialog::openRssRequested,    this, &MainWindow::openRssManager);
    dlg.exec();

    if (dlg.dontShowAgain()) {
        QSettings settings("BATorrent", "BATorrent");
        settings.setValue("welcomeShown", true);
    }
}

void MainWindow::showAbout()
{
    QString text = QString(
        "<h2>BATorrent v%1</h2>"
        "<p>%2</p>"
        "<p>🔒 <b>%6</b></p>"
        "<p><b>%3:</b><br>"
        "libtorrent-rasterbar, Qt 6, OpenSSL</p>"
        "<p>%4 MIT</p>"
        "<p>%5</p>"
        "<p>💬 <a href=\"https://discord.com/users/241995362057977856\">%7</a></p>")
        .arg(QApplication::applicationVersion(),
             tr_("about_description"),
             tr_("about_libraries"),
             tr_("about_license"),
             tr_("about_donate_blurb"),
             tr_("about_no_telemetry"),
             tr_("about_discord"));
    QMessageBox box(QMessageBox::NoIcon, tr_("action_about"), text, QMessageBox::NoButton, this);
    box.setTextFormat(Qt::RichText);
    auto *donateBtn = box.addButton(tr_("action_donate"), QMessageBox::ActionRole);
    box.addButton(QMessageBox::Ok);
    box.exec();
    if (box.clickedButton() == donateBtn)
        QDesktopServices::openUrl(QUrl(QStringLiteral("https://github.com/sponsors/Mateuscruz19")));
}

void MainWindow::retranslateUi()
{
    setupMenuBar();
    setupToolBar();
    m_statusLabel->setText(tr_("status_no_torrents"));
    m_detailsPanel->retranslate();
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);

    // Adaptive toolbar: show icon labels only when there's room for them.
    // Below ~820 px the labelled buttons would push the toolbar past the
    // window width.
    for (auto *tb : findChildren<QToolBar *>()) {
        const Qt::ToolButtonStyle desired = (width() < 820)
            ? Qt::ToolButtonIconOnly
            : Qt::ToolButtonTextBesideIcon;
        if (tb->toolButtonStyle() != desired)
            tb->setToolButtonStyle(desired);
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    saveSettings();
    m_session->saveResumeData();
    if (m_closeToTray) {
        hide();
        event->ignore();
    } else {
        qApp->quit();
    }
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        for (const auto &url : event->mimeData()->urls()) {
            if (url.toLocalFile().endsWith(".torrent")) {
                event->acceptProposedAction();
                return;
            }
        }
    }
    if (event->mimeData()->hasText()) {
        QString text = event->mimeData()->text();
        if (text.startsWith("magnet:"))
            event->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        for (const auto &url : event->mimeData()->urls()) {
            QString path = url.toLocalFile();
            if (path.endsWith(".torrent"))
                addTorrentFile(path);
        }
    } else if (event->mimeData()->hasText()) {
        QString text = event->mimeData()->text();
        if (text.startsWith("magnet:"))
            m_session->addMagnet(text, m_lastSavePath);
    }
}

void MainWindow::filterByState(const QString &state)
{
    if (state.isEmpty()) {
        m_proxyModel->setStateFilter("");
        if (m_batWidget) m_batWidget->setCustomMessage("", "");
        return;
    }

    // Map button key to translated state string(s)
    static const QMap<QString, QString> keyMap = {
        {"downloading", "state_downloading"},
        {"seeding", "state_seeding"},
        {"paused", "state_paused"},
        {"finished", "state_finished"},
        {"completed", "state_completed"},
    };

    if (state == "all_active") {
        // Show downloading + seeding: use special "active" filter
        m_proxyModel->setStateFilter("__active__");
    } else {
        QString trKey = keyMap.value(state);
        m_proxyModel->setStateFilter(trKey.isEmpty() ? state : tr_(trKey));
    }

    // Per-filter empty state copy — saves the user from the generic bat
    // widget when they're filtering for "Completed" and see "no torrents
    // yet" with three CTAs that aren't even relevant to that view.
    if (m_batWidget) {
        QString title, body;
        if (state == "all_active")        { title = tr_("empty_active_t"); body = tr_("empty_active_b"); }
        else if (state == "downloading")  { title = tr_("empty_downloading_t"); body = tr_("empty_downloading_b"); }
        else if (state == "seeding")      { title = tr_("empty_seeding_t"); body = tr_("empty_seeding_b"); }
        else if (state == "completed")    { title = tr_("empty_completed_t"); body = tr_("empty_completed_b"); }
        else if (state == "paused")       { title = tr_("empty_paused_t"); body = tr_("empty_paused_b"); }
        else if (state == "finished")     { title = tr_("empty_finished_t"); body = tr_("empty_finished_b"); }
        else if (state == "queued")       { title = tr_("empty_queued_t"); body = tr_("empty_queued_b"); }
        m_batWidget->setCustomMessage(title, body);
    }
}

void MainWindow::showContextMenu(const QPoint &pos)
{
    auto rows = selectedRows();
    if (rows.isEmpty()) return;

    QMenu menu(this);
    menu.addAction(QIcon(":/icons/play.svg"), tr_("action_resume"), this, &MainWindow::resumeSelected);
    menu.addAction(QIcon(":/icons/pause.svg"), tr_("action_pause"), this, &MainWindow::pauseSelected);
    menu.addSeparator();

    // Sequential download toggle (only for single selection)
    if (rows.size() == 1) {
        TorrentInfo info = m_session->torrentAt(rows.first());
        auto *seqAction = menu.addAction(tr_("ctx_sequential"));
        seqAction->setCheckable(true);
        seqAction->setChecked((info.handle.flags() & lt::torrent_flags::sequential_download) != lt::torrent_flags_t{});
        connect(seqAction, &QAction::toggled, this, [this, row = rows.first()](bool checked) {
            m_session->setSequentialDownload(row, checked);
        });

        auto *forceAction = menu.addAction(tr_("ctx_force_start"));
        forceAction->setCheckable(true);
        forceAction->setChecked(m_session->isForceStart(rows.first()));
        forceAction->setToolTip(tr_("tip_force_start"));
        connect(forceAction, &QAction::toggled, this, [this, row = rows.first()](bool checked) {
            m_session->setForceStart(row, checked);
        });

        // Super seeding toggle
        if (info.progress >= 1.0f) {
            auto *superSeedAction = menu.addAction(tr_("ctx_super_seeding"));
            superSeedAction->setCheckable(true);
            superSeedAction->setChecked(m_session->isSuperSeeding(rows.first()));
            superSeedAction->setToolTip("Send each piece only once to maximize initial distribution");
            connect(superSeedAction, &QAction::toggled, this, [this, row = rows.first()](bool checked) {
                m_session->setSuperSeeding(row, checked);
            });
        }

        // Tags — comma-separated quick editor. Multi-tag support without
        // building a full tag picker UI: most users want to type "anime, 2024"
        // and move on.
        auto *tagsAction = menu.addAction(tr_("ctx_tags"));
        connect(tagsAction, &QAction::triggered, this, [this, row = rows.first()]() {
            QStringList current = m_session->torrentTags(row);
            bool ok = false;
            QString text = QInputDialog::getText(this, tr_("ctx_tags"),
                tr_("ctx_tags_prompt"), QLineEdit::Normal,
                current.join(", "), &ok);
            if (!ok) return;
            m_session->setTorrentTags(row, text.split(',', Qt::SkipEmptyParts));
        });

        menu.addSeparator();
    }

    // Per-torrent rate caps. Submenu with presets + Custom entry. Applies
    // to whichever rows are selected (bulk-friendly).
    {
        const QList<QPair<QString, int>> presets = {
            {tr_("ctx_speed_unlimited"), 0},
            {QStringLiteral("50 KB/s"),   50},
            {QStringLiteral("100 KB/s"),  100},
            {QStringLiteral("500 KB/s"),  500},
            {QStringLiteral("1 MB/s"),    1024},
            {QStringLiteral("5 MB/s"),    5120},
            {QStringLiteral("10 MB/s"),  10240},
        };
        auto buildLimitMenu = [&](const QString &title, bool isDownload) {
            QMenu *sub = menu.addMenu(title);
            int current = 0;
            if (rows.size() == 1) {
                current = isDownload
                    ? m_session->torrentDownloadLimit(rows.first())
                    : m_session->torrentUploadLimit(rows.first());
            }
            for (const auto &p : presets) {
                QAction *a = sub->addAction(p.first);
                a->setCheckable(true);
                a->setChecked(current == p.second);
                connect(a, &QAction::triggered, this, [this, rows, isDownload, kbps = p.second]() {
                    for (int r : rows) {
                        if (isDownload) m_session->setTorrentDownloadLimit(r, kbps);
                        else            m_session->setTorrentUploadLimit(r, kbps);
                    }
                });
            }
            sub->addSeparator();
            QAction *custom = sub->addAction(tr_("ctx_speed_custom"));
            connect(custom, &QAction::triggered, this, [this, rows, isDownload, current]() {
                bool ok = false;
                int v = QInputDialog::getInt(this, tr_("ctx_speed_custom"),
                    tr_("ctx_speed_prompt"), current, 0, 1000000, 50, &ok);
                if (!ok) return;
                for (int r : rows) {
                    if (isDownload) m_session->setTorrentDownloadLimit(r, v);
                    else            m_session->setTorrentUploadLimit(r, v);
                }
            });
        };
        buildLimitMenu(tr_("ctx_speed_down"), true);
        buildLimitMenu(tr_("ctx_speed_up"), false);
        menu.addSeparator();
    }

    // Copy magnet / info-hash for the first selected torrent (clipboard
    // bulk-copy of multiple torrents is rarely what users want — they want
    // to share one).
    if (!rows.isEmpty()) {
        const int row = rows.first();
        menu.addAction(tr_("ctx_copy_magnet"), this, [this, row]() {
            QString uri = m_session->torrentMagnetUri(row);
            if (!uri.isEmpty()) QApplication::clipboard()->setText(uri);
        });
        menu.addAction(tr_("ctx_copy_hash"), this, [this, row]() {
            QString hash = m_session->torrentHashAt(row);
            if (!hash.isEmpty()) QApplication::clipboard()->setText(hash);
        });
        menu.addAction(tr_("ctx_why_slow"), this, [this, row]() {
            diagnoseSlow(row);
        });
        menu.addSeparator();
    }

    // Mark/Unmark Completed. Acts only on 100%+ rows; mid-download selection
    // gets the regular pause/resume path instead.
    {
        bool anyCompletable = false, anyCompleted = false;
        for (int r : rows) {
            TorrentInfo info = m_session->torrentAt(r);
            if (info.completed) anyCompleted = true;
            else if (info.progress >= 1.0f) anyCompletable = true;
        }
        if (anyCompletable) {
            menu.addAction(QIcon(":/icons/stop.svg"), tr_("ctx_mark_completed"),
                this, [this, rows]() {
                    for (int r : rows) {
                        TorrentInfo info = m_session->torrentAt(r);
                        if (!info.completed && info.progress >= 1.0f)
                            m_session->markCompleted(r);
                    }
                });
        }
        if (anyCompleted) {
            menu.addAction(QIcon(":/icons/play.svg"), tr_("ctx_unmark_completed"),
                this, [this, rows]() {
                    for (int r : rows) m_session->unmarkCompleted(r);
                });
        }
    }

    // Per-torrent stop-after-download + max-seed-time overrides (single selection)
    if (rows.size() == 1) {
        int row = rows.first();
        QMenu *seedMenu = menu.addMenu(tr_("ctx_seed_rules"));

        // Stop after download toggle
        auto *stopAfter = seedMenu->addAction(tr_("ctx_stop_after_download"));
        stopAfter->setCheckable(true);
        int override_ = m_session->torrentStopAfterDownload(row);
        bool effective = (override_ >= 0)
            ? (override_ == 1)
            : m_session->stopAfterDownload();
        stopAfter->setChecked(effective);
        connect(stopAfter, &QAction::toggled, this, [this, row](bool checked) {
            // Toggling sets an explicit per-torrent override.
            m_session->setTorrentStopAfterDownload(row, checked ? 1 : 0);
        });

        // Max seed time
        seedMenu->addAction(tr_("ctx_max_seed_time"), this, [this, row]() {
            qint64 current = m_session->torrentMaxSeedSeconds(row);
            if (current < 0) current = m_session->maxSeedSeconds();
            int curDays = static_cast<int>(current / 86400);
            bool ok = false;
            int days = QInputDialog::getInt(this, tr_("ctx_max_seed_time"),
                tr_("ctx_max_seed_prompt"), curDays, 0, 365, 1, &ok);
            if (!ok) return;
            m_session->setTorrentMaxSeedSeconds(row,
                static_cast<qint64>(days) * 86400);
        });

        // Reset to global default
        seedMenu->addAction(tr_("ctx_seed_use_default"), this, [this, row]() {
            m_session->setTorrentStopAfterDownload(row, -1);
            m_session->setTorrentMaxSeedSeconds(row, -1);
        });

        menu.addSeparator();
    }

    // Category submenu
    {
        QMenu *catMenu = menu.addMenu(tr_("ctx_category"));
        auto *noneAction = catMenu->addAction(tr_("category_none"));
        connect(noneAction, &QAction::triggered, this, [this, rows]() {
            for (int r : rows)
                m_session->setTorrentCategory(r, "");
        });
        catMenu->addSeparator();
        for (const auto &cat : m_session->categories()) {
            auto *catAction = catMenu->addAction(cat);
            connect(catAction, &QAction::triggered, this, [this, rows, cat]() {
                for (int r : rows)
                    m_session->setTorrentCategory(r, cat);
            });
        }
        menu.addSeparator();
    }

    // Open folder + Stream (only for single selection)
    if (rows.size() == 1) {
        TorrentInfo info = m_session->torrentAt(rows.first());
        // Reveal the torrent's root in its save folder — the single file
        // for one-file torrents, or the root folder libtorrent created for
        // multi-file torrents. The user lands on exactly what this torrent
        // produced, with that item highlighted in the file manager.
        menu.addAction(tr_("ctx_open_folder"), this, [this, row = rows.first()]() {
            QString root = m_session->torrentRootPath(row);
            if (!root.isEmpty()) revealInFileManager(root);
        });
        if (info.progress < 1.0f && !info.paused) {
            menu.addAction(QIcon(":/icons/play.svg"), tr_("ctx_stream"), this, [this, row = rows.first()]() {
                streamTorrent(row);
            });
        }
        menu.addSeparator();
    }

    // Queue position (single selection only)
    if (rows.size() == 1) {
        menu.addSeparator();
        int row = rows.first();
        menu.addAction(tr_("ctx_queue_up"), this, [this, row]() {
            if (row > 0) {
                m_session->setTorrentQueuePosition(row, row - 1);
                m_model->refresh();
            }
        });
        menu.addAction(tr_("ctx_queue_down"), this, [this, row]() {
            if (row < m_session->torrentCount() - 1) {
                m_session->setTorrentQueuePosition(row, row + 1);
                m_model->refresh();
            }
        });
    }

    menu.addSeparator();
    menu.addAction(tr_("ctx_force_recheck"), this, [this, rows]() {
        for (int r : rows) m_session->forceRecheck(r);
    });
    menu.addAction(tr_("ctx_force_reannounce"), this, [this, rows]() {
        for (int r : rows) m_session->forceReannounce(r);
    });

    // Rename + move storage on a single selection (these need the user to
    // pick a new name / path so they don't make sense for bulk operations).
    if (rows.size() == 1) {
        int row = rows.first();
        TorrentInfo info = m_session->torrentAt(row);
        menu.addSeparator();
        menu.addAction(tr_("ctx_rename"), this, [this, row, info]() {
            bool ok = false;
            QString name = QInputDialog::getText(this, tr_("ctx_rename"),
                tr_("ctx_rename_prompt"), QLineEdit::Normal, info.name, &ok);
            if (!ok || name.trimmed().isEmpty()) return;
            // libtorrent's rename_file with file_index 0 + new name renames
            // the single top-level file (for single-file torrents) or the
            // root folder (for multi-file torrents).
            m_session->renameFile(row, 0, name);
        });
        menu.addAction(tr_("ctx_move_storage"), this, [this, row, info]() {
            QString dir = QFileDialog::getExistingDirectory(this,
                tr_("ctx_move_storage"), info.savePath);
            if (dir.isEmpty() || dir == info.savePath) return;
            m_session->moveStorage(row, dir);
        });
    }

    menu.addSeparator();
    menu.addAction(QIcon(":/icons/trash.svg"), tr_("action_remove"), this, &MainWindow::removeSelected);
    menu.addAction(tr_("action_remove_files"), this, &MainWindow::removeSelectedWithFiles);

    menu.exec(m_tableView->viewport()->mapToGlobal(pos));
}

QList<int> MainWindow::selectedRows() const
{
    QList<int> rows;
    QModelIndexList sel = m_tableView->selectionModel()->selectedRows();
    for (const auto &idx : sel)
        rows.append(m_proxyModel->mapToSource(idx).row());
    std::sort(rows.begin(), rows.end());
    return rows;
}

void MainWindow::openAddons()
{
    AddonDialog dlg(this);
    dlg.exec();
}

void MainWindow::openRssManager()
{
    RssDialog dlg(this);
    dlg.exec();
}

void MainWindow::openSearch()
{
    if (AddonManager::instance().addons().isEmpty()) {
        QMessageBox::information(this, tr_("search_title"), tr_("search_no_addons"));
        return;
    }
    SearchDialog dlg(m_session, m_lastSavePath, this);
    dlg.exec();
}

void MainWindow::streamTorrent(int row)
{
    static const QStringList videoExts = {".mp4", ".mkv", ".avi", ".mov", ".wmv", ".flv", ".webm", ".m4v", ".ts"};

    auto files = m_session->filesAt(row);
    TorrentInfo info = m_session->torrentAt(row);
    int bestIdx = -1;
    qint64 bestSize = 0;
    // In-progress files carry a ".!bt" suffix; match against the stripped
    // form so an incomplete Movie.mp4.!bt still detects as video.
    auto stripBt = [](const QString &p) {
        return p.endsWith(QStringLiteral(".!bt")) ? p.chopped(4) : p;
    };
    for (int i = 0; i < static_cast<int>(files.size()); ++i) {
        const auto &f = files[i];
        const QString matchPath = stripBt(f.path);
        bool isVideo = false;
        for (const auto &ext : videoExts) {
            if (matchPath.endsWith(ext, Qt::CaseInsensitive)) { isVideo = true; break; }
        }
        if (isVideo && f.size > bestSize) {
            bestSize = f.size;
            bestIdx = i;
        }
    }

    if (bestIdx < 0) {
        QMessageBox::information(this, "Stream", tr_("stream_no_video"));
        return;
    }

    // Enable sequential download
    m_session->setSequentialDownload(row, true);

    // Skip all non-video files so pieces focus on the video from the start
    for (int i = 0; i < static_cast<int>(files.size()); ++i) {
        if (i != bestIdx)
            m_session->setFilePriority(row, i, 0);
    }

    // Set video file to high priority
    m_session->setFilePriority(row, bestIdx, 7);

    // Boost piece boundaries (first 4 + last 2 pieces) so the player can
    // open the container header + trailing index before the body fills in.
    m_session->prioritizeFilePieceBoundaries(row, bestIdx);

    m_streamTorrentIndex = row;
    // libtorrent reports the on-disk path (suffixed for incomplete files), so
    // this points at the actual byte stream the player can open right now.
    m_streamFilePath = info.savePath + "/" + files[bestIdx].path;

    // Poll until enough data is buffered (at least 2% or 5MB)
    if (m_streamPollTimer) {
        m_streamPollTimer->stop();
        m_streamPollTimer->deleteLater();
    }
    m_streamPollTimer = new QTimer(this);
    connect(m_streamPollTimer, &QTimer::timeout, this, [this, bestIdx]() {
        if (m_streamTorrentIndex < 0 || m_streamTorrentIndex >= m_session->torrentCount()) {
            m_streamPollTimer->stop();
            return;
        }
        auto files = m_session->filesAt(m_streamTorrentIndex);
        TorrentInfo info = m_session->torrentAt(m_streamTorrentIndex);

        // Check if the video file has enough data (2% of the file or 5MB minimum)
        float fileProgress = (bestIdx < static_cast<int>(files.size())) ? files[bestIdx].progress : 0.0f;
        qint64 fileDone = (bestIdx < static_cast<int>(files.size()))
            ? static_cast<qint64>(fileProgress * files[bestIdx].size) : 0;
        bool ready = QFile::exists(m_streamFilePath) && (fileProgress >= 0.02f || fileDone > 5 * 1024 * 1024);
        if (ready) {
            m_streamPollTimer->stop();
            bool opened = false;
#ifdef Q_OS_MACOS
            // macOS: try VLC, IINA, then QuickTime as fallbacks
            for (const QString &app : {"VLC", "IINA", "QuickTime Player"}) {
                if (QProcess::startDetached("open", {"-a", app, m_streamFilePath})) {
                    opened = true;
                    break;
                }
            }
            if (!opened) {
                // Last resort: let macOS pick
                opened = QProcess::startDetached("open", {m_streamFilePath});
            }
#else
            opened = QDesktopServices::openUrl(QUrl::fromLocalFile(m_streamFilePath));
#endif
            if (opened) {
                Toast::notify(tr_("ctx_stream"),
                              tr_("stream_started").arg(info.name));
            } else {
                QMessageBox::warning(this, tr_("ctx_stream"), tr_("stream_no_player"));
            }
        }
    });
    m_streamPollTimer->start(2000);
}

void MainWindow::checkAutoShutdown()
{
    if (!m_autoShutdown) return;
    if (m_shutdownDialog) return; // already showing countdown

    // Check if any torrent is still downloading
    int count = m_session->torrentCount();
    for (int i = 0; i < count; ++i) {
        TorrentInfo info = m_session->torrentAt(i);
        if (!info.paused && info.progress < 1.0f)
            return; // still downloading
    }

    // All downloads complete — start 60-second countdown
    m_shutdownCountdown = 60;
    m_shutdownDialog = new QMessageBox(this);
    m_shutdownDialog->setWindowTitle(tr_("shutdown_title"));
    m_shutdownDialog->setText(tr_("shutdown_msg").arg(m_shutdownCountdown));
    m_shutdownDialog->setIcon(QMessageBox::Information);
    m_shutdownDialog->setStandardButtons(QMessageBox::Cancel);

    m_shutdownTimer = new QTimer(this);
    connect(m_shutdownTimer, &QTimer::timeout, this, [this]() {
        m_shutdownCountdown--;
        if (m_shutdownCountdown <= 0) {
            m_shutdownTimer->stop();
            m_shutdownDialog->close();
            saveSettings();
            m_session->saveResumeData();

#ifdef Q_OS_WIN
            QProcess::startDetached("shutdown", {"/s", "/t", "0"});
#elif defined(Q_OS_MACOS)
            QProcess::startDetached("osascript", {"-e", "tell app \"System Events\" to shut down"});
#else
            QProcess::startDetached("shutdown", {"-h", "now"});
#endif
            QApplication::quit();
        } else {
            m_shutdownDialog->setText(tr_("shutdown_msg").arg(m_shutdownCountdown));
        }
    });

    connect(m_shutdownDialog, &QMessageBox::rejected, this, [this]() {
        m_shutdownTimer->stop();
        m_shutdownTimer->deleteLater();
        m_shutdownTimer = nullptr;
        m_shutdownDialog->deleteLater();
        m_shutdownDialog = nullptr;
    });

    m_shutdownTimer->start(1000);
    m_shutdownDialog->show();
}

void MainWindow::checkForUpdate(bool silent)
{
    connect(m_updater, &Updater::updateAvailable, this,
            [this, silent](const QString &version, const QString &url, const QString &assetName) {
        // Honor "skip this version": only suppress on silent (startup) checks
        // so the user can still trigger the prompt via Help → Check for updates.
        if (silent) {
            QSettings settings("BATorrent", "BATorrent");
            QString skipped = settings.value("skippedUpdateVersion").toString();
            if (skipped == version)
                return;
        }

        QString msg = tr_("update_available").arg(version);
        QMessageBox box(QMessageBox::Question, tr_("update_title"), msg,
                        QMessageBox::NoButton, this);
        auto *yesBtn = box.addButton(QMessageBox::Yes);
        box.addButton(QMessageBox::No);
        auto *skipBtn = box.addButton(tr_("update_skip"), QMessageBox::ActionRole);
        box.setDefaultButton(yesBtn);
        box.exec();

        if (box.clickedButton() == skipBtn) {
            QSettings settings("BATorrent", "BATorrent");
            settings.setValue("skippedUpdateVersion", version);
            return;
        }
        if (box.clickedButton() != yesBtn)
            return;

        auto *progress = new QProgressDialog(tr_("update_downloading"), tr_("btn_cancel"), 0, 100, this);
        progress->setWindowModality(Qt::WindowModal);
        progress->setAutoClose(false);
        progress->show();

        connect(m_updater, &Updater::downloadProgress, progress,
                [progress](qint64 received, qint64 total) {
            if (total > 0)
                progress->setValue(static_cast<int>(received * 100 / total));
        });

        connect(m_updater, &Updater::updateReady, progress, &QProgressDialog::close);
        connect(m_updater, &Updater::errorOccurred, this,
                [this, progress](const QString &err) {
            progress->close();
            QMessageBox::warning(this, tr_("dlg_error"), err);
        });

        m_updater->downloadAndInstall(url, assetName);
    }, Qt::SingleShotConnection);

    connect(m_updater, &Updater::noUpdateAvailable, this, [this, silent]() {
        if (!silent)
            QMessageBox::information(this, tr_("update_title"), tr_("update_none"));
    }, Qt::SingleShotConnection);

    connect(m_updater, &Updater::errorOccurred, this, [this, silent](const QString &err) {
        if (!silent)
            QMessageBox::warning(this, tr_("dlg_error"), err);
    }, Qt::SingleShotConnection);

    m_updater->checkForUpdate();
}
