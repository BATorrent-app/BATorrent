// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "mainwindow.h"
#include "torrentmodel.h"
#include "torrentfilter.h"
#include "progressdelegate.h"
#include "detailspanel.h"
#include "settingsdialog.h"
#include "welcomedialog.h"
#include "createtorrentdialog.h"
#include "speedgraph.h"
#include "batwidget.h"
#include "splashwidget.h"
#include "thememanager.h"
#include "../torrent/sessionmanager.h"
#include "../webui/webserver.h"
#include "../app/translator.h"
#include "../app/updater.h"
#include "../app/utils.h"
#include "../app/addonmanager.h"
#include "addondialog.h"
#include "searchdialog.h"
#include "rssdialog.h"
#include "../app/rssmanager.h"

#include <QMenuBar>
#include <QToolBar>
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
#include <QMenu>
#include <QCloseEvent>
#include <QSettings>
#include <QMimeData>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QResizeEvent>
#include <QApplication>
#include <QLocale>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStackedWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QShortcut>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QProgressDialog>
#include <QProcess>
#include <QDesktopServices>
#include <QStandardPaths>
#include <QUrl>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>

MainWindow::MainWindow(SessionManager *session, QWidget *parent)
    : QMainWindow(parent), m_session(session)
{
    setWindowTitle("BATorrent");
    setWindowIcon(QIcon(":/images/logo1.png"));
    setAcceptDrops(true);
    resize(1000, 650);

    m_model = new TorrentModel(session, this);

    loadSettings();
    applyTheme();
    setupMenuBar();
    setupToolBar();
    setupCentralWidget();
    setupStatusBar();
    setupTrayIcon();

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
        m_trayIcon->showMessage(tr_("notif_torrent_added"), info.name,
                                QSystemTrayIcon::Information, 3000);
        if (m_notifSoundEnabled)
            QApplication::beep();
    });
    connect(m_session, &SessionManager::torrentRemoved, m_model, &TorrentModel::refresh);
    connect(m_session, &SessionManager::torrentFinished, this, &MainWindow::onTorrentFinished);
    connect(m_session, &SessionManager::torrentError, this, &MainWindow::onTorrentError);

    // Kill switch notifications
    connect(m_session, &SessionManager::killSwitchTriggered, this, [this]() {
        m_trayIcon->showMessage(tr_("killswitch_title"), tr_("killswitch_triggered"),
                                QSystemTrayIcon::Warning, 5000);
    });
    connect(m_session, &SessionManager::interfaceRestored, this, [this]() {
        m_trayIcon->showMessage(tr_("killswitch_title"), tr_("killswitch_restored"),
                                QSystemTrayIcon::Information, 5000);
    });

    // Auto-updater
    m_updater = new Updater(this);
    checkForUpdate(true); // silent check on startup

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
            m_trayIcon->showMessage(tr_("rss_auto_downloaded"),
                QString("%1: %2").arg(feedName, itemTitle));
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

    // Show welcome on first launch
    QSettings settings("BATorrent", "BATorrent");
    if (!settings.value("welcomeShown", false).toBool())
        showWelcome();

    // Splash animation
    m_splash = new SplashWidget(this);
    m_splash->setGeometry(0, 0, width(), height());
    m_splash->raise();
    m_splash->show();
    m_splash->start();
    connect(m_splash, &SplashWidget::finished, this, [this]() {
        m_splash->hide();
        m_splash->deleteLater();
        m_splash = nullptr;
    });
}

MainWindow::~MainWindow()
{
    saveSettings();
}

void MainWindow::applyTheme()
{
    setStyleSheet(ThemeManager::instance().styleSheet());
}

void MainWindow::setupMenuBar()
{
    menuBar()->clear();

    QMenu *fileMenu = menuBar()->addMenu(tr_("menu_file"));
    auto *openAction = fileMenu->addAction(QIcon(":/icons/open.svg"), tr_("action_open"));
    openAction->setShortcut(QKeySequence::Open);
    connect(openAction, &QAction::triggered, this, &MainWindow::openTorrent);
    fileMenu->addAction(QIcon(":/icons/magnet.svg"), tr_("action_magnet"),
                        this, &MainWindow::openMagnet);
    fileMenu->addAction(tr_("action_create"), this, &MainWindow::createTorrent);
    fileMenu->addAction(tr_("action_import_qbt"), this, &MainWindow::importQBittorrent);
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
    settingsMenu->addAction(QIcon(":/icons/settings.svg"), tr_("action_settings"),
                            this, &MainWindow::openSettings);
    settingsMenu->addAction(tr_("action_addons"), this, &MainWindow::openAddons);
    settingsMenu->addAction(tr_("action_rss"), this, &MainWindow::openRssManager);
    settingsMenu->addSeparator();
    settingsMenu->addAction(tr_("action_search_addons"), this, &MainWindow::openSearch);

    QMenu *helpMenu = menuBar()->addMenu(tr_("menu_help"));
    helpMenu->addAction(tr_("action_welcome"), this, &MainWindow::showWelcome);
    helpMenu->addAction(tr_("action_check_update"), this, [this]() { checkForUpdate(false); });
    helpMenu->addSeparator();
    helpMenu->addAction(tr_("action_about"), this, &MainWindow::showAbout);
}

void MainWindow::setupToolBar()
{
    // Remove existing toolbars
    for (auto *tb : findChildren<QToolBar *>())
        delete tb;

    QToolBar *toolbar = addToolBar("Main");
    toolbar->setMovable(false);
    toolbar->setIconSize(QSize(20, 20));
    toolbar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    auto *logoLabel = new QLabel;
    QPixmap logo(":/images/logo1.png");
    logoLabel->setPixmap(logo.scaled(28, 28, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    logoLabel->setContentsMargins(4, 0, 8, 0);
    toolbar->addWidget(logoLabel);

    toolbar->addSeparator();

    // Prepend space to text for icon-text gap (Qt has no CSS property for this)
    auto addTbAction = [&](const QString &icon, const QString &textKey, auto slot) {
        toolbar->addAction(QIcon(icon), "  " + tr_(textKey), this, slot);
    };
    addTbAction(":/icons/open.svg", "tb_open", &MainWindow::openTorrent);
    addTbAction(":/icons/magnet.svg", "tb_magnet", &MainWindow::openMagnet);
    toolbar->addSeparator();
    addTbAction(":/icons/pause.svg", "tb_pause", &MainWindow::pauseSelected);
    addTbAction(":/icons/play.svg", "tb_resume", &MainWindow::resumeSelected);
    addTbAction(":/icons/trash.svg", "tb_remove", &MainWindow::removeSelected);
    toolbar->addSeparator();
    addTbAction(":/icons/settings.svg", "tb_settings", &MainWindow::openSettings);
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

    // Filter bar
    auto *filterBar = new QWidget;
    auto *filterLayout = new QHBoxLayout(filterBar);
    const auto &tm = ThemeManager::instance();
    filterLayout->setContentsMargins(8, 6, 8, 6);
    filterLayout->setSpacing(6);

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

    auto addFilterBtn = [&](const QString &key, const QString &state) {
        auto *btn = new QPushButton(tr_(key));
        btn->setCheckable(true);
        btn->setStyleSheet(QString(
            "QPushButton { background: transparent; color: %1; border: 1px solid %2;"
            "border-radius: 14px; padding: 6px 14px; font-size: 11px; font-weight: 600; }"
            "QPushButton:hover { border-color: %3; color: %3; }"
            "QPushButton:checked { background-color: %3; color: #ffffff; border-color: %3; }")
            .arg(tm.mutedColor(), tm.borderColor(), tm.accentColor()));
        connect(btn, &QPushButton::toggled, this, [this, btn, state](bool checked) {
            // Uncheck other filter buttons
            if (checked) {
                for (auto *other : btn->parentWidget()->findChildren<QPushButton *>()) {
                    if (other != btn) other->setChecked(false);
                }
                filterByState(state);
            } else {
                filterByState("");
            }
        });
        filterLayout->addWidget(btn);
    };

    addFilterBtn("filter_all_active", "all_active");
    addFilterBtn("filter_downloading", "downloading");
    addFilterBtn("filter_seeding", "seeding");
    addFilterBtn("filter_paused", "paused");
    addFilterBtn("filter_finished", "finished");

    filterLayout->addStretch();

    // Bat animation (shown when no torrents)
    m_batWidget = new BatWidget;

    // Top section: filter bar + table or bat widget
    auto *tableContainer = new QWidget;
    auto *tableLayout = new QVBoxLayout(tableContainer);
    tableLayout->setContentsMargins(0, 0, 0, 0);
    tableLayout->setSpacing(0);
    tableLayout->addWidget(filterBar);
    tableLayout->addWidget(m_tableView);

    m_topStack = new QStackedWidget;
    m_topStack->addWidget(tableContainer);  // index 0 = table
    m_topStack->addWidget(m_batWidget);     // index 1 = bat
    m_topStack->setCurrentIndex(1);         // start with bat (no torrents yet)

    m_speedGraph = new SpeedGraph;
    m_detailsPanel = new DetailsPanel(m_session);

    auto *bottomWidget = new QWidget;
    auto *bottomLayout = new QVBoxLayout(bottomWidget);
    bottomLayout->setContentsMargins(0, 0, 0, 0);
    bottomLayout->setSpacing(0);
    bottomLayout->addWidget(m_speedGraph);
    bottomLayout->addWidget(m_detailsPanel);

    auto *splitter = new QSplitter(Qt::Vertical);
    splitter->addWidget(m_topStack);
    splitter->addWidget(bottomWidget);
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 2);

    setCentralWidget(splitter);
}

void MainWindow::setupStatusBar()
{
    m_statusLabel = new QLabel(tr_("status_no_torrents"));
    statusBar()->addWidget(m_statusLabel);

    m_globalStatsLabel = new QLabel;
    statusBar()->addPermanentWidget(m_globalStatsLabel);
}

void MainWindow::setupTrayIcon()
{
    m_trayIcon = new QSystemTrayIcon(QIcon(":/images/logo1.png"), this);

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

    connect(m_trayIcon, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger)
            trayActivated();
    });
    connect(m_trayIcon, &QSystemTrayIcon::messageClicked, this, &MainWindow::trayActivated);

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

    // Proxy
    settings.setValue("proxyType", m_session->proxyType());
    settings.setValue("proxyHost", m_session->proxyHost());
    settings.setValue("proxyPort", m_session->proxyPort());
    settings.setValue("proxyUser", m_session->proxyUser());
    settings.setValue("proxyPass", m_session->proxyPass());

    // IP Filter
    settings.setValue("ipFilterPath", m_session->ipFilterPath());

    // Bandwidth Scheduler
    settings.setValue("schedulerEnabled", m_session->schedulerEnabled());
    settings.setValue("altDownLimit", m_session->altDownloadLimit());
    settings.setValue("altUpLimit", m_session->altUploadLimit());
    settings.setValue("scheduleFromHour", m_session->scheduleFromHour());
    settings.setValue("scheduleToHour", m_session->scheduleToHour());
    settings.setValue("scheduleDays", m_session->scheduleDays());
}

void MainWindow::loadSettings()
{
    QSettings settings("BATorrent", "BATorrent");
    if (settings.contains("geometry"))
        restoreGeometry(settings.value("geometry").toByteArray());
    m_lastSavePath = settings.value("lastSavePath",
        QStandardPaths::writableLocation(QStandardPaths::DownloadLocation)).toString();

    int lang;
    if (settings.contains("language")) {
        lang = settings.value("language").toInt();
    } else {
        QString sysLang = QLocale::system().name();
        lang = sysLang.startsWith("pt") ? 1 : 0;
    }
    Translator::instance().setLanguage(static_cast<Translator::Language>(lang));

    int maxDown = settings.value("maxDownload", 0).toInt();
    int maxUp = settings.value("maxUpload", 0).toInt();
    m_session->setDownloadLimit(maxDown);
    m_session->setUploadLimit(maxUp);

    m_startMinimized = settings.value("startMinimized", false).toBool();
    m_useDefaultPath = settings.value("useDefaultPath", false).toBool();

    int theme = settings.value("theme", 0).toInt();
    ThemeManager::instance().setTheme(static_cast<ThemeManager::Theme>(theme));

    // Network settings
    bool dht = settings.value("dhtEnabled", true).toBool();
    m_session->setDhtEnabled(dht);
    int enc = settings.value("encryptionMode", 0).toInt();
    m_session->setEncryptionMode(enc);
    int maxConn = settings.value("maxConnections", 200).toInt();
    m_session->setMaxConnections(maxConn);
    float seedRatio = settings.value("seedRatioLimit", 0.0).toFloat();
    m_session->setSeedRatioLimit(seedRatio);

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

    // Proxy
    int proxyType = settings.value("proxyType", 0).toInt();
    m_session->setProxySettings(proxyType,
        settings.value("proxyHost").toString(),
        settings.value("proxyPort", 0).toInt(),
        settings.value("proxyUser").toString(),
        settings.value("proxyPass").toString());

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
        QString passHash = settings.value("webUiPasswordHash").toString();
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

void MainWindow::addTorrentFile(const QString &filePath)
{
    m_session->addTorrent(filePath, m_lastSavePath);
}

void MainWindow::addTorrentFromCli(const QString &filePath)
{
    m_session->addTorrent(filePath, m_lastSavePath);
}

void MainWindow::addMagnetFromCli(const QString &uri)
{
    m_session->addMagnet(uri, m_lastSavePath);
}

void MainWindow::openMagnet()
{
    QString magnet = QInputDialog::getText(this, tr_("dlg_add_magnet"),
                                            tr_("dlg_paste_magnet"));
    if (magnet.isEmpty() || !magnet.startsWith("magnet:"))
        return;

    m_session->addMagnet(magnet, m_lastSavePath);
}

void MainWindow::removeSelected()
{
    auto rows = selectedRows();
    // Remove in reverse order so indices stay valid
    std::sort(rows.begin(), rows.end(), std::greater<int>());
    for (int r : rows)
        m_session->removeTorrent(r, false);
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
    m_session->pauseAll();
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

    if (count == 0) {
        m_statusLabel->setText(tr_("status_no_torrents"));
        return;
    }

    m_statusLabel->setText(tr_("status_format")
                               .arg(count)
                               .arg(totalDown / 1024.0, 0, 'f', 1)
                               .arg(totalUp / 1024.0, 0, 'f', 1));

    // Update taskbar progress via tray icon tooltip
    float avgProgress = totalProgress / count;
    m_trayIcon->setToolTip(QString("BATorrent - %1%").arg(static_cast<int>(avgProgress * 100)));

    // Global stats (right side of status bar)
    m_globalStatsLabel->setText(tr_("status_global")
        .arg(formatSize(m_session->globalDownloaded()),
             formatSize(m_session->globalUploaded()),
             QString::number(static_cast<double>(m_session->globalRatio()), 'f', 2)));
}

void MainWindow::onSelectionChanged()
{
    auto rows = selectedRows();
    m_detailsPanel->showTorrent(rows.isEmpty() ? -1 : rows.first());
}

void MainWindow::onTorrentFinished(const QString &name)
{
    m_trayIcon->showMessage(tr_("dlg_download_complete"),
                            tr_("dlg_finished_msg").arg(name),
                            QSystemTrayIcon::Information, 5000);
    if (m_notifSoundEnabled)
        QApplication::beep();
    m_model->flashRow(name);
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
        QString token = settings.value("plexToken").toString();
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
        QString apiKey = settings.value("jellyfinApiKey").toString();
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
    show();
    raise();
    activateWindow();
}

void MainWindow::openSettings()
{
    SettingsDialog dlg(this);
    dlg.setDefaultSavePath(m_lastSavePath);
    dlg.setMaxDownloadSpeed(m_session->downloadLimit());
    dlg.setMaxUploadSpeed(m_session->uploadLimit());
    dlg.setLanguageIndex(static_cast<int>(Translator::instance().language()));
    dlg.setStartMinimized(m_startMinimized);
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

    {
        QSettings settings("BATorrent", "BATorrent");
        dlg.setWebUiEnabled(settings.value("webUiEnabled", false).toBool());
        dlg.setWebUiPort(settings.value("webUiPort", 8080).toInt());
        dlg.setWebUiUser(settings.value("webUiUser", "admin").toString());
        dlg.setWebUiPasswordHash(settings.value("webUiPasswordHash").toString());
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
        dlg.setPlexToken(settings.value("plexToken").toString());
        dlg.setJellyfinEnabled(settings.value("jellyfinEnabled", false).toBool());
        dlg.setJellyfinUrl(settings.value("jellyfinUrl").toString());
        dlg.setJellyfinApiKey(settings.value("jellyfinApiKey").toString());
    }

    if (dlg.exec() == QDialog::Accepted) {
        m_lastSavePath = dlg.defaultSavePath();
        m_session->setDownloadLimit(dlg.maxDownloadSpeed());
        m_session->setUploadLimit(dlg.maxUploadSpeed());
        m_startMinimized = dlg.startMinimized();
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
                settings.setValue("webUiPasswordHash", passHash);
            startWebServer();

            // Media Server settings
            settings.setValue("plexEnabled", dlg.plexEnabled());
            settings.setValue("plexUrl", dlg.plexUrl());
            settings.setValue("plexToken", dlg.plexToken());
            settings.setValue("jellyfinEnabled", dlg.jellyfinEnabled());
            settings.setValue("jellyfinUrl", dlg.jellyfinUrl());
            settings.setValue("jellyfinApiKey", dlg.jellyfinApiKey());
        }
    }
}

void MainWindow::showWelcome()
{
    WelcomeDialog dlg(this);
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
        "<p><b>%3:</b><br>"
        "libtorrent-rasterbar, Qt 6, OpenSSL</p>"
        "<p>%4 MIT</p>")
        .arg(QApplication::applicationVersion(),
             tr_("about_description"),
             tr_("about_libraries"),
             tr_("about_license"));
    QMessageBox::about(this, tr_("action_about"), text);
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
    if (m_splash)
        m_splash->setGeometry(0, 0, width(), height());
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    saveSettings();
    m_session->saveResumeData();
    hide();
    event->ignore();
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
        return;
    }

    // Map button key to translated state string(s)
    static const QMap<QString, QString> keyMap = {
        {"downloading", "state_downloading"},
        {"seeding", "state_seeding"},
        {"paused", "state_paused"},
        {"finished", "state_finished"},
    };

    if (state == "all_active") {
        // Show downloading + seeding: use special "active" filter
        m_proxyModel->setStateFilter("__active__");
    } else {
        QString trKey = keyMap.value(state);
        m_proxyModel->setStateFilter(trKey.isEmpty() ? state : tr_(trKey));
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
        menu.addSeparator();
    }

    // Open folder + Stream (only for single selection)
    if (rows.size() == 1) {
        TorrentInfo info = m_session->torrentAt(rows.first());
        menu.addAction(tr_("ctx_open_folder"), this, [savePath = info.savePath]() {
            QDesktopServices::openUrl(QUrl::fromLocalFile(savePath));
        });
        if (info.progress < 1.0f && !info.paused) {
            menu.addAction(QIcon(":/icons/play.svg"), tr_("ctx_stream"), this, [this, row = rows.first()]() {
                streamTorrent(row);
            });
        }
        menu.addSeparator();
    }

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

    // Enable sequential download
    m_session->setSequentialDownload(row, true);

    // Find the largest video file
    auto files = m_session->filesAt(row);
    TorrentInfo info = m_session->torrentAt(row);
    int bestIdx = -1;
    qint64 bestSize = 0;
    for (int i = 0; i < static_cast<int>(files.size()); ++i) {
        const auto &f = files[i];
        bool isVideo = false;
        for (const auto &ext : videoExts) {
            if (f.path.endsWith(ext, Qt::CaseInsensitive)) { isVideo = true; break; }
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

    // Set video file to high priority
    m_session->setFilePriority(row, bestIdx, 7);

    m_streamTorrentIndex = row;
    m_streamFilePath = info.savePath + "/" + files[bestIdx].path;

    // Poll until enough data is buffered (at least 2% or 5MB)
    if (m_streamPollTimer) {
        m_streamPollTimer->stop();
        m_streamPollTimer->deleteLater();
    }
    m_streamPollTimer = new QTimer(this);
    connect(m_streamPollTimer, &QTimer::timeout, this, [this]() {
        if (m_streamTorrentIndex < 0 || m_streamTorrentIndex >= m_session->torrentCount()) {
            m_streamPollTimer->stop();
            return;
        }
        auto files = m_session->filesAt(m_streamTorrentIndex);
        TorrentInfo info = m_session->torrentAt(m_streamTorrentIndex);

        // Check if file has enough data (2% or 5MB minimum)
        bool ready = QFile::exists(m_streamFilePath) && (info.progress >= 0.02f || info.totalDone > 5 * 1024 * 1024);
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
                m_trayIcon->showMessage(tr_("ctx_stream"), tr_("stream_started").arg(info.name),
                                        QSystemTrayIcon::Information, 3000);
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
            [this](const QString &version, const QString &url, const QString &assetName) {
        QString msg = tr_("update_available").arg(version);
        int ret = QMessageBox::question(this, tr_("update_title"), msg,
                                        QMessageBox::Yes | QMessageBox::No);
        if (ret != QMessageBox::Yes)
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
