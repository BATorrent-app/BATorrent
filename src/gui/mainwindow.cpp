#include "mainwindow.h"
#include "torrentmodel.h"
#include "torrentfilter.h"
#include "progressdelegate.h"
#include "detailspanel.h"
#include "settingsdialog.h"
#include "welcomedialog.h"
#include "createtorrentdialog.h"
#include "speedgraph.h"
#include "thememanager.h"
#include "../core/sessionmanager.h"
#include "../core/translator.h"

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
#include <QApplication>
#include <QLocale>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QShortcut>

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
    connect(m_session, &SessionManager::torrentRemoved, m_model, &TorrentModel::refresh);
    connect(m_session, &SessionManager::torrentFinished, this, &MainWindow::onTorrentFinished);
    connect(m_session, &SessionManager::torrentError, this, &MainWindow::onTorrentError);

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
    fileMenu->addAction(QIcon(":/icons/open.svg"), tr_("action_open"),
                        QKeySequence::Open, this, &MainWindow::openTorrent);
    fileMenu->addAction(QIcon(":/icons/magnet.svg"), tr_("action_magnet"),
                        this, &MainWindow::openMagnet);
    fileMenu->addAction(tr_("action_create"), this, &MainWindow::createTorrent);
    fileMenu->addAction(tr_("action_import_qbt"), this, &MainWindow::importQBittorrent);
    fileMenu->addSeparator();
    fileMenu->addAction(tr_("action_quit"), QKeySequence::Quit, this, &QWidget::close);

    QMenu *torrentMenu = menuBar()->addMenu(tr_("menu_torrent"));
    torrentMenu->addAction(QIcon(":/icons/pause.svg"), tr_("action_pause"),
                           this, &MainWindow::pauseSelected);
    torrentMenu->addAction(QIcon(":/icons/play.svg"), tr_("action_resume"),
                           this, &MainWindow::resumeSelected);
    torrentMenu->addSeparator();
    torrentMenu->addAction(tr_("action_pause_all"), this, &MainWindow::pauseAll);
    torrentMenu->addAction(tr_("action_resume_all"), this, &MainWindow::resumeAll);
    torrentMenu->addSeparator();
    torrentMenu->addAction(QIcon(":/icons/trash.svg"), tr_("action_remove"),
                           QKeySequence::Delete, this, &MainWindow::removeSelected);
    torrentMenu->addAction(tr_("action_remove_files"), this, &MainWindow::removeSelectedWithFiles);

    QMenu *settingsMenu = menuBar()->addMenu(tr_("menu_settings"));
    settingsMenu->addAction(QIcon(":/icons/settings.svg"), tr_("action_settings"),
                            this, &MainWindow::openSettings);

    QMenu *helpMenu = menuBar()->addMenu(tr_("menu_help"));
    helpMenu->addAction(tr_("action_welcome"), this, &MainWindow::showWelcome);
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
    logoLabel->setContentsMargins(4, 0, 6, 0);
    toolbar->addWidget(logoLabel);

    toolbar->addSeparator();

    toolbar->addAction(QIcon(":/icons/open.svg"), tr_("tb_open"), this, &MainWindow::openTorrent);
    toolbar->addAction(QIcon(":/icons/magnet.svg"), tr_("tb_magnet"), this, &MainWindow::openMagnet);
    toolbar->addSeparator();
    toolbar->addAction(QIcon(":/icons/pause.svg"), tr_("tb_pause"), this, &MainWindow::pauseSelected);
    toolbar->addAction(QIcon(":/icons/play.svg"), tr_("tb_resume"), this, &MainWindow::resumeSelected);
    toolbar->addAction(QIcon(":/icons/trash.svg"), tr_("tb_remove"), this, &MainWindow::removeSelected);
    toolbar->addSeparator();
    toolbar->addAction(QIcon(":/icons/settings.svg"), tr_("tb_settings"), this, &MainWindow::openSettings);
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
    m_tableView->setItemDelegateForColumn(TorrentModel::Progress,
                                          new ProgressDelegate(m_tableView));
    m_tableView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_tableView, &QWidget::customContextMenuRequested,
            this, &MainWindow::showContextMenu);

    connect(m_tableView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &MainWindow::onSelectionChanged);

    // Filter bar
    auto *filterBar = new QWidget;
    auto *filterLayout = new QHBoxLayout(filterBar);
    filterLayout->setContentsMargins(6, 4, 6, 4);
    filterLayout->setSpacing(4);

    m_searchEdit = new QLineEdit;
    m_searchEdit->setPlaceholderText(tr_("filter_search"));
    m_searchEdit->setClearButtonEnabled(true);
    m_searchEdit->setMaximumWidth(250);
    m_searchEdit->setStyleSheet(
        "QLineEdit { background-color: #1a1a1a; color: #b0b0b0; border: 1px solid #252525;"
        "border-radius: 4px; padding: 4px 8px; font-size: 11px; }"
        "QLineEdit:focus { border-color: #6b2020; }");
    connect(m_searchEdit, &QLineEdit::textChanged, m_proxyModel, &TorrentFilter::setNameFilter);
    filterLayout->addWidget(m_searchEdit);

    filterLayout->addSpacing(8);

    auto addFilterBtn = [&](const QString &key, const QString &state) {
        auto *btn = new QPushButton(tr_(key));
        btn->setCheckable(true);
        btn->setStyleSheet(
            "QPushButton { background: transparent; color: #707070; border: 1px solid #252525;"
            "border-radius: 3px; padding: 3px 10px; font-size: 11px; font-weight: bold; }"
            "QPushButton:hover { background-color: #2a1015; color: #c43030; }"
            "QPushButton:checked { background-color: #2a1015; color: #c43030; border-color: #6b2020; }");
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

    // Top section: filter bar + table
    auto *topWidget = new QWidget;
    auto *topLayout = new QVBoxLayout(topWidget);
    topLayout->setContentsMargins(0, 0, 0, 0);
    topLayout->setSpacing(0);
    topLayout->addWidget(filterBar);
    topLayout->addWidget(m_tableView);

    m_speedGraph = new SpeedGraph;
    m_detailsPanel = new DetailsPanel(m_session);

    auto *bottomWidget = new QWidget;
    auto *bottomLayout = new QVBoxLayout(bottomWidget);
    bottomLayout->setContentsMargins(0, 0, 0, 0);
    bottomLayout->setSpacing(0);
    bottomLayout->addWidget(m_speedGraph);
    bottomLayout->addWidget(m_detailsPanel);

    auto *splitter = new QSplitter(Qt::Vertical);
    splitter->addWidget(topWidget);
    splitter->addWidget(bottomWidget);
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 2);

    setCentralWidget(splitter);
}

void MainWindow::setupStatusBar()
{
    m_statusLabel = new QLabel(tr_("status_no_torrents"));
    statusBar()->addWidget(m_statusLabel);
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
    trayMenu->addAction(tr_("tray_quit"), this, &QApplication::quit);
    m_trayIcon->setContextMenu(trayMenu);

    connect(m_trayIcon, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger)
            trayActivated();
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
    settings.setValue("useDefaultPath", m_useDefaultPath);
    settings.setValue("theme", static_cast<int>(ThemeManager::instance().theme()));
    settings.setValue("dhtEnabled", m_session->dhtEnabled());
    settings.setValue("encryptionMode", m_session->encryptionMode());
    settings.setValue("maxConnections", m_session->maxConnections());
    settings.setValue("seedRatioLimit", static_cast<double>(m_session->seedRatioLimit()));
    settings.setValue("tableHeaderState", m_tableView->horizontalHeader()->saveState());
    settings.setValue("tableSortColumn", m_tableView->horizontalHeader()->sortIndicatorSection());
    settings.setValue("tableSortOrder", static_cast<int>(m_tableView->horizontalHeader()->sortIndicatorOrder()));
}

void MainWindow::loadSettings()
{
    QSettings settings("BATorrent", "BATorrent");
    if (settings.contains("geometry"))
        restoreGeometry(settings.value("geometry").toByteArray());
    m_lastSavePath = settings.value("lastSavePath",
        QDir::homePath() + "/Downloads").toString();

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

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (m_trayIcon->isVisible()) {
        hide();
        event->ignore();
    } else {
        event->accept();
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
        seqAction->setChecked(info.handle.flags() & lt::torrent_flags::sequential_download);
        connect(seqAction, &QAction::toggled, this, [this, row = rows.first()](bool checked) {
            m_session->setSequentialDownload(row, checked);
        });
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
