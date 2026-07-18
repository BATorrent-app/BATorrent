// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include <QApplication>
#include <QIcon>
#include <QStringList>
#include <QFont>
#include <QFontDatabase>
#include <QLocalServer>
#include <QLocalSocket>
#include <QWindow>
#include <QSettings>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickImageProvider>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QSGRendererInterface>
#include <QLocale>
#include <QMessageBox>
#include <QPushButton>
#include <QDesktopServices>
#include <QProcess>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QUrl>
#include <cstdlib>
#ifdef BAT_HAVE_SENTRY
#include <sentry.h>
#endif
#include "services/metadata/metadataresolver.h"
#include "services/discovery/discoveryservice.h"
#include "services/platform/translator.h"
#include "bridges/qmlposterbridge.h"
#include "webui/streamserver.h"
#include "services/discovery/gamesourcemanager.h"
#include "services/integrations/rssmanager.h"
#include "services/discovery/addonmanager.h"
#include "services/integrations/notifier.h"
#include "torrent/sessionmanager.h"
#include "ipc/enginehost.h"
#include "ipc/ipcengine.h"
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QEventLoop>
#include <cstring>
#include <QThread>
#include "services/security/secretstore.h"
#include "services/integrations/debridmanager.h"
#include "services/platform/logger.h"
#include "services/security/crashhandler.h"
#include "services/platform/utils.h"

#include <libtorrent/version.hpp>

// Serves the app logo recolored for the OS scheme to QML (the system tray
// icon.source wants a URL, not a QIcon). URL id is "light"/"dark"; the body
// is swapped to dark text for the "dark"-id (light-background) request.
class AppLogoImageProvider : public QQuickImageProvider
{
public:
    AppLogoImageProvider() : QQuickImageProvider(QQuickImageProvider::Pixmap) {}
    QPixmap requestPixmap(const QString &id, QSize *size, const QSize &requested) override
    {
        const int sz = requested.width() > 0 ? requested.width() : 256;
        const bool darkBody = id.startsWith("dark");   // dark logo for light OS bar
        QPixmap pm = QmlThemeBridge::renderLogo(darkBody, sz, 1.0);
        if (size) *size = pm.size();
        return pm;
    }
};

static const QString kServerName = QStringLiteral("BATorrent-SingleInstance");

static QString collectArgs(const QStringList &args)
{
    QStringList relevant;
    for (int i = 1; i < args.size(); ++i) {
        const QString &a = args[i];
        if (a.endsWith(".torrent") || a.startsWith("magnet:") || a.startsWith("bittorrent:"))
            relevant << a;
    }
    return relevant.join('\n');
}

static bool sendToRunningInstance(const QString &message)
{
    QLocalSocket socket;
    socket.connectToServer(kServerName);
    if (!socket.waitForConnected(1000))
        return false;
    socket.write(message.toUtf8());
    socket.waitForBytesWritten(1000);
    socket.disconnectFromServer();
    return true;
}

// A QML/JS runtime diagnostic (vs a generic Qt warning): carries a .qml/qrc
// source location AND an error phrase. These are the silent broken-binding /
// TypeError messages that keep running until they cascade into a crash.
static bool isQmlError(const QString &m)
{
    if (!m.contains(QLatin1String(".qml:")) && !m.contains(QLatin1String("qrc:")))
        return false;
    static const char *markers[] = {
        "TypeError", "ReferenceError", "is not a function", "is not defined",
        "Cannot read property", "Unable to assign", "Cannot assign",
        "Binding loop detected", "Error:"
    };
    for (const char *mk : markers)
        if (m.contains(QLatin1String(mk))) return true;
    return false;
}

#ifdef BAT_HAVE_SENTRY
// Bring up Crashpad before anything heavy runs, so a crash from the very first
// torrent/QML interaction is captured. No-op without a compiled-in DSN.
static void initSentry(const QString &role)
{
    sentry_options_t *o = sentry_options_new();
#ifdef BAT_SENTRY_DSN
    sentry_options_set_dsn(o, BAT_SENTRY_DSN);
#endif
    // Prefer the crashpad_handler shipped next to the executable (the packaged
    // case); fall back to the build-time path (local dev against brew).
    {
        QString handler = QCoreApplication::applicationDirPath()
                          + QStringLiteral("/crashpad_handler");
#ifdef Q_OS_WIN
        handler += QStringLiteral(".exe");
#endif
#ifdef BAT_SENTRY_HANDLER
        if (!QFileInfo::exists(handler)) handler = QStringLiteral(BAT_SENTRY_HANDLER);
#endif
        if (QFileInfo::exists(handler))
            sentry_options_set_handler_path(o, handler.toUtf8().constData());
    }
    const QString db = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                       + QStringLiteral("/sentry-") + role;
    sentry_options_set_database_path(o, db.toUtf8().constData());
    sentry_options_set_release(o, "batorrent@" APP_VERSION);
#ifdef QT_DEBUG
    sentry_options_set_environment(o, "development");
#else
    sentry_options_set_environment(o, "production");
#endif
    if (qEnvironmentVariableIsSet("BAT_SENTRY_TEST"))
        sentry_options_set_debug(o, 1);   // verbose transport logs for local validation
    sentry_init(o);
    qAddPostRoutine([]{ sentry_close(); });
}
#endif

// Maps Qt's runtime log categories (QtDebugMsg / QtInfoMsg / etc) to our
// internal Logger levels so every existing qDebug() / qWarning() in the
// codebase ends up in the same file the user can export.
static void qtMessageHandler(QtMsgType type, const QMessageLogContext &ctx,
                             const QString &msg)
{
    Logger::Level lvl = Logger::Debug;
    switch (type) {
    case QtDebugMsg:    lvl = Logger::Debug;    break;
    case QtInfoMsg:     lvl = Logger::Info;     break;
    case QtWarningMsg:  lvl = Logger::Warning;  break;
    case QtCriticalMsg: lvl = Logger::Error;    break;
    case QtFatalMsg:    lvl = Logger::Critical; break;
    }
    QString prefix;
    if (ctx.category && qstrcmp(ctx.category, "default") != 0)
        prefix = QStringLiteral("[%1] ").arg(QString::fromUtf8(ctx.category));
    Logger::instance().log(lvl, prefix + msg);
    // Keep stderr live for `--debug` console use.
    fprintf(stderr, "%s\n", qPrintable(prefix + msg));

    // Dev-only: stop QML runtime errors from hiding in the log until they
    // cascade into a crash. BAT_QML_STRICT=warn prints a loud banner; =fatal
    // aborts at the broken binding so a debugger / the crash handler catches it.
    // Unset in production → zero effect.
    static const QByteArray qmlStrict = qgetenv("BAT_QML_STRICT");
    if (!qmlStrict.isEmpty() && type == QtWarningMsg && isQmlError(msg)) {
        fprintf(stderr, "\n‼️  [QML ERROR] %s\n\n", qPrintable(msg));
        if (qmlStrict == "fatal") { fflush(stderr); abort(); }
    }
}

int main(int argc, char *argv[])
{
    // --- engine/UI split (internal/ENGINE_SPLIT_PLAN.md) ---
    // These branch before QApplication so the engine child stays headless.
    for (int i = 1; i < argc; ++i) {
        // Engine child: headless host of the libtorrent session over a local socket.
        if (std::strcmp(argv[i], "--engine") == 0 && i + 1 < argc) {
            QCoreApplication eapp(argc, argv);
            eapp.setOrganizationName("BATorrent");
            eapp.setApplicationName("BATorrent");
            eapp.setApplicationVersion(APP_VERSION);
            Logger::instance().init();
#ifdef BAT_HAVE_SENTRY
            initSentry(QStringLiteral("engine"));   // the split's whole point: report engine crashes
#endif
            SessionManager session;   // loadResumeData() runs in the ctor
            EngineHost host(&session, QString::fromLocal8Bit(argv[i + 1]));
            if (!host.listen()) return 1;
            return eapp.exec();
        }
        // Proof-of-life: spawn our own binary as the engine, round-trip a few
        // calls, exit. Validates the channel + process supervision end-to-end.
        if (std::strcmp(argv[i], "--engine-selftest") == 0) {
            QCoreApplication eapp(argc, argv);
            eapp.setOrganizationName("BATorrent");
            eapp.setApplicationName("BATorrent");
            Logger::instance().init();
            IpcEngine engine(QCoreApplication::applicationFilePath());
            QObject::connect(&engine, &IpcEngine::engineStatusChanged, [](bool up) {
                qInfo() << "[selftest] engine status:" << (up ? "UP" : "DOWN");
            });
            if (!engine.start()) { qWarning() << "[selftest] engine failed to start"; return 1; }
            // Pump events ~3s so the engine finishes loading resume data and
            // pushes a full snapshot; reads are served from it (no blocking).
            QElapsedTimer pump; pump.start();
            while (pump.elapsed() < 3000) eapp.processEvents(QEventLoop::AllEvents, 100);
            qInfo() << "[selftest] connected. snapshot torrentCount =" << engine.torrentCount();
            if (engine.torrentCount() > 0) {
                const TorrentInfo t = engine.torrentAt(0);
                qInfo() << "[selftest] torrentAt(0):" << t.name << "progress" << t.progress
                        << "hash" << engine.torrentHashAt(0);
                qInfo() << "[selftest] filesAt(0) count =" << int(engine.filesAt(0).size());
            }
            qInfo() << "[selftest] pauseAll()"; engine.pauseAll();
            qInfo() << "[selftest] round-trip OK";
            return 0;
        }
    }

    QApplication app(argc, argv);
    // Set the org name so default-constructed QSettings() resolves to the same
    // store as the explicit QSettings("BATorrent","BATorrent") used elsewhere —
    // otherwise the QML UI's settings (language, theme, …) fork into a separate
    // store the legacy UI can't see.
    app.setOrganizationName("BATorrent");
    app.setApplicationName("BATorrent");
    app.setApplicationVersion(APP_VERSION);
#ifndef Q_OS_MACOS
    app.setWindowIcon(QIcon(":/images/logo1.png"));   // macOS Dock uses the bundled .icns (issue #14)
#endif
    app.setQuitOnLastWindowClosed(false); // keep running in tray when window is closed

    // CLI flag: --debug / -d forces verbose logging for this session (without
    // mutating the persisted setting). Comes before init() so the lowered
    // level takes effect immediately.
    const bool debugFlag = app.arguments().contains("--debug")
                        || app.arguments().contains("-d");
    Logger::instance().init();
    // Capture a backtrace on a fatal crash (the MS-Store crashes have no repro on
    // the dev box). Installed right after the log is up so startup crashes count too.
    CrashHandler::install(
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/crashes",
        APP_VERSION);
#ifdef BAT_HAVE_SENTRY
    initSentry(QStringLiteral("ui"));   // field crash reporting (release builds)
    if (qEnvironmentVariableIsSet("BAT_SENTRY_TEST"))
        sentry_capture_event(sentry_value_new_message_event(
            SENTRY_LEVEL_INFO, "custom", "BATorrent sentry test event"));
#endif
    if (debugFlag) {
        Logger::instance().setLevel(Logger::Trace);
        // Make Qt dump the scene-graph/RHI init (which GPU backend it picked and
        // why it fell back, if it did) — captured into our log via the handler.
        qputenv("QSG_INFO", "1");
    }
    qInstallMessageHandler(qtMessageHandler);

    // Speed unit display preference (0 = bytes/sec, 1 = bits/sec). Read once
    // here so formatSpeed() doesn't hit QSettings on every call (UI refresh
    // rate would otherwise translate to ~10 QSettings opens/sec).
    setSpeedUnit(QSettings("BATorrent", "BATorrent").value("speedUnit", 0).toInt());

    // One-time migration of plaintext secrets from QSettings into the OS
    // keyring (no-op on subsequent runs and on builds without QtKeychain).
    SecretStore::instance().migrateFromSettings({
        "proxyPass", "plexToken", "jellyfinApiKey"
    });

    // One-time migration: "autoShutdown" (bool) -> "postDownloadAction" (index,
    // 6 = shut down). A user who had it on keeps getting a shutdown, not
    // silently nothing, once the setting becomes a multi-action picker.
    {
        QSettings st;
        if (!st.contains("postDownloadAction") && st.value("autoShutdown", false).toBool())
            st.setValue("postDownloadAction", 6);
    }

    // webUiPasswordHash used to live in the keychain, but on unsigned macOS
    // builds reading it at every cold start pops a login-keychain prompt. It's
    // only a SHA-256 hash (not a usable credential), so it now lives in
    // QSettings; pull any existing one back out of the keychain (one-time — the
    // plaintext webUiPassword stays in the keychain). Reading a missing item
    // doesn't prompt, so users who never enabled the WebUI see nothing.
    {
        QSettings st;
        if (!st.contains("webUiPasswordHash")) {
            const QString h = SecretStore::instance().get("webUiPasswordHash");
            if (!h.isEmpty()) {
                st.setValue("webUiPasswordHash", h);
                SecretStore::instance().set("webUiPasswordHash", QString());
            }
        }
    }

    // Single-instance check: if another instance is running, forward args and quit
    QString argsPayload = collectArgs(app.arguments());
    if (sendToRunningInstance(argsPayload))
        return 0;

    // --- boot-health sentinel: catch a crash during startup (e.g. corrupt resume
    // data, or a bad auto-update) and offer recovery BEFORE the risky init runs
    // again. markBootHealthy() (called from QML once the UI is up) clears it.
    bool safeMode = false;
    {
        QSettings st;
        int crashes = st.value("bootCrashes", 0).toInt();
        if (st.value("bootInProgress", false).toBool())
            ++crashes;                         // last boot never reported healthy
        st.setValue("bootCrashes", crashes);
        st.setValue("bootInProgress", true);
        st.sync();

        if (crashes >= 2) {
            QMessageBox box;
            box.setIcon(QMessageBox::Warning);
            box.setWindowTitle("BATorrent — Recovery");
            box.setText("BATorrent didn't start properly the last couple of times.");
            box.setInformativeText("You can reset settings, get the latest version, or try starting normally.");
            QPushButton *resetBtn = box.addButton("Reset settings & restart", QMessageBox::DestructiveRole);
            QPushButton *dlBtn    = box.addButton("Download latest", QMessageBox::ActionRole);
            QPushButton *contBtn  = box.addButton("Continue anyway", QMessageBox::AcceptRole);
            box.setDefaultButton(contBtn);
            box.exec();
            if (box.clickedButton() == resetBtn) {
                const QString resumeDir =
                    QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/resume";
                QSettings().clear();
                QSettings().sync();
                QDir(resumeDir).removeRecursively();
                QProcess::startDetached(QApplication::applicationFilePath(), {});
                return 0;
            }
            if (box.clickedButton() == dlBtn) {
                QDesktopServices::openUrl(QUrl("https://github.com/BATorrent-app/BATorrent/releases/latest"));
                return 0;
            }
            safeMode = true;   // continue, but skip the auto update-check this run
        }
    }

    // --- engine ABI guard: a portable update can swap the exe while a stale
    // process still holds the old torrent-rasterbar DLL, leaving a version-skewed
    // install that dies as an access violation inside session construction
    // (the Sentry NATIVE-QT-4 / GetProcAddress signature). lt::version() comes
    // from the DLL, LIBTORRENT_VERSION from our headers — skew → clear dialog.
    if (!QString::fromLatin1(lt::version()).startsWith(QStringLiteral(LIBTORRENT_VERSION))) {
        QMessageBox box;
        box.setIcon(QMessageBox::Critical);
        box.setWindowTitle("BATorrent — Recovery");
        box.setText("This installation is broken: the torrent engine on disk is from a different version.");
        box.setInformativeText("This usually happens when an update is interrupted. Please download the latest version again.");
        QPushButton *dlBtn = box.addButton("Download latest", QMessageBox::AcceptRole);
        box.addButton("Quit", QMessageBox::RejectRole);
        box.setDefaultButton(dlBtn);
        box.exec();
        if (box.clickedButton() == dlBtn)
            QDesktopServices::openUrl(QUrl("https://github.com/BATorrent-app/BATorrent/releases/latest"));
        return 1;
    }

    // Load Inter font family
    QFontDatabase::addApplicationFont(":/fonts/Inter-Regular.ttf");
    QFontDatabase::addApplicationFont(":/fonts/Inter-Medium.ttf");
    QFontDatabase::addApplicationFont(":/fonts/Inter-SemiBold.ttf");
    QFontDatabase::addApplicationFont(":/fonts/Inter-Bold.ttf");
    QFontDatabase::addApplicationFont(":/fonts/NewRocker-Regular.ttf");   // brand wordmark

    QFont defaultFont("Inter", 10);
    defaultFont.setStyleStrategy(QFont::PreferAntialias);
    app.setFont(defaultFont);

    // Use the Qt Quick default (distance-field) text rendering on every
    // platform so Windows matches macOS. Native Windows rendering was crisper
    // but rendered the Inter weights noticeably thinner than the Mac reference.

    QQuickStyle::setStyle("Basic");
    {

        // Engine selection (internal/ENGINE_SPLIT_PLAN.md). Opt-in: when
        // engineMode == "ipc" the libtorrent session runs in a separate child
        // process so an engine crash can't take the UI down; otherwise it runs
        // in-process as before. The UI talks only to IEngine* either way.
        SessionManager *localSession = nullptr;   // non-null only in in-process mode
        IpcEngine *ipcEngine = nullptr;
        {
            const bool wantIpc = QSettings().value(QStringLiteral("engineMode")).toString()
                                 == QLatin1String("ipc");
            if (wantIpc) {
                ipcEngine = new IpcEngine(QCoreApplication::applicationFilePath(), &app);
                if (!ipcEngine->start()) {
                    qWarning() << "[engine] IPC engine failed to start — falling back to in-process";
                    ipcEngine->deleteLater();
                    ipcEngine = nullptr;
                } else {
                    qInfo() << "[engine] running split (engine in child process)";
                }
            }
            if (!ipcEngine) localSession = new SessionManager(&app);
        }
        IEngine *eng = localSession ? static_cast<IEngine *>(localSession)
                                    : static_cast<IEngine *>(ipcEngine);

        auto *resolver = new MetadataResolver(&app);
        auto *posterModel = new QmlPosterModel(eng, resolver, &app);
        auto *themeBridge = new QmlThemeBridge(&app);
        auto *sessionBridge = new QmlSessionBridge(eng, resolver, &app);
        // Local stream server for the embedded player (4.0 step ④).
        auto *streamServer = new StreamServer(eng, &app);
        if (streamServer->start()) {
            sessionBridge->setStreamPort(streamServer->port());
            qInfo() << "[stream] listening on 127.0.0.1:" << streamServer->port();
        } else {
            qWarning() << "[stream] failed to start local stream server";
        }
        RssManager::instance().setSession(eng, sessionBridge->defaultSavePath());
        auto *rssBridge = new QmlRssBridge(&app);
        // Settings/WebUI stay on the in-process session (config control plane);
        // null in IPC mode, where the bridge falls back to QSettings.
        auto *settingsBridge = new QmlSettingsBridge(localSession, eng, &app);
        auto *addonBridge = new QmlAddonBridge(&app);
        auto *searchBridge = new QmlSearchBridge(eng, &app);
        searchBridge->setResolver(resolver);
        auto *discoveryService = new DiscoveryService(&app);
        searchBridge->setDiscovery(discoveryService);

#ifndef BAT_STORE_BUILD
        // Seed a default community game catalog once so "Jogos" works out of the
        // box (open → search → download). Store builds stay clean. The user can
        // remove it; the seed flag keeps it from coming back.
        {
            QSettings gs;
            if (!gs.value(QStringLiteral("gameSourcesSeeded"), false).toBool()) {
                gs.setValue(QStringLiteral("gameSourcesSeeded"), true);
                auto &gsm = GameSourceManager::instance();
                if (gsm.sources().isEmpty())
                    gsm.addSource(QStringLiteral("RuTracker (Combined)"),
                                  QStringLiteral("https://raw.githubusercontent.com/Jdjsjjqq/rutracker-hydra/main/combined_torrents.json"));
            }
        }
#endif
        auto *logBridge = new QmlLogBridge(&app);
        auto *subtitleBridge = new QmlSubtitleBridge(eng, &app);
        subtitleBridge->setResolver(resolver);
        auto *pairingBridge = new QmlPairingBridge(&app);
        auto *debrid = new DebridManager(&app);
        auto *notificationBridge = new QmlNotificationBridge(&app);
        notificationBridge->setSession(eng);
        QObject::connect(eng, &IEngine::torrentFinished,
                         notificationBridge, &QmlNotificationBridge::onTorrentFinished);
        // Telegram webhook notifications (same event surfaces as the toasts above).
        auto *telegram = new TelegramNotifier(&app);
        QObject::connect(eng, &IEngine::torrentFinished,
                         telegram, &TelegramNotifier::onTorrentFinished);
        // Engine-only alert signals (error/kill-switch/suspicious) live on
        // SessionManager; in IPC mode they aren't proxied yet, so wire them only
        // in-process. The IPC banner below covers the engine-down case instead.
        // Forwarded over IPC in split mode (events re-emitted by IpcEngine), so
        // connect on the IEngine interface — works in-process and split alike.
        QObject::connect(eng, &IEngine::torrentError,
                         notificationBridge, &QmlNotificationBridge::onTorrentError);
        QObject::connect(eng, &IEngine::killSwitchTriggered,
                         notificationBridge, &QmlNotificationBridge::onKillSwitchTriggered);
        QObject::connect(eng, &IEngine::suspiciousFilesDetected,
                         notificationBridge, &QmlNotificationBridge::onSuspiciousFilesDetected);
        QObject::connect(eng, &IEngine::killSwitchTriggered,
                         telegram, &TelegramNotifier::onKillSwitchTriggered);
        QObject::connect(eng, &IEngine::torrentError,
                         telegram, &TelegramNotifier::onTorrentError);
        // IPC supervision: surface engine respawns to the user as a toast.
        if (ipcEngine) {
            QObject::connect(ipcEngine, &IpcEngine::engineStatusChanged, notificationBridge,
                             [notificationBridge](bool up) {
                notificationBridge->onTorrentError(up ? QStringLiteral("Torrent engine reconnected")
                                                      : QStringLiteral("Torrent engine restarting…"));
            });
        }
        QObject::connect(&RssManager::instance(), &RssManager::itemAutoDownloaded,
                         notificationBridge, &QmlNotificationBridge::onRssAutoDownloaded);
        QObject::connect(&RssManager::instance(), &RssManager::itemAutoDownloaded,
                         telegram, &TelegramNotifier::onRssAutoDownloaded);
        settingsBridge->setTelegramNotifier(telegram);

        // Media-server library refresh: ping Plex/Jellyfin when a download finishes.
        auto *mediaNam = new QNetworkAccessManager(&app);
        QObject::connect(eng, &IEngine::torrentFinished, &app, [mediaNam](const QString &, const QString &) {
            QSettings st;
            if (st.value("plexEnabled", false).toBool()) {
                const QString url = st.value("plexUrl").toString();
                const QString token = SecretStore::instance().get("plexToken");
                if (!url.isEmpty() && !token.isEmpty()) {
                    QNetworkRequest req(QUrl(url + "/library/sections/all/refresh?X-Plex-Token=" + token));
                    req.setHeader(QNetworkRequest::UserAgentHeader, "BATorrent");
                    auto *r = mediaNam->get(req);
                    QObject::connect(r, &QNetworkReply::finished, r, &QNetworkReply::deleteLater);
                }
            }
            if (st.value("jellyfinEnabled", false).toBool()) {
                const QString url = st.value("jellyfinUrl").toString();
                const QString key = SecretStore::instance().get("jellyfinApiKey");
                if (!url.isEmpty() && !key.isEmpty()) {
                    QNetworkRequest req(QUrl(url + "/Library/Refresh?api_key=" + key));
                    req.setHeader(QNetworkRequest::UserAgentHeader, "BATorrent");
                    auto *r = mediaNam->post(req, QByteArray());
                    QObject::connect(r, &QNetworkReply::finished, r, &QNetworkReply::deleteLater);
                }
            }
        });

        auto *discordBridge = new DiscordRpcBridge(eng, &app);
        QObject::connect(eng, &IEngine::torrentsUpdated,
                         discordBridge, &DiscordRpcBridge::refresh);
#ifndef BAT_STORE_BUILD
        auto *updaterBridge = new QmlUpdaterBridge(&app);
#endif
        QObject::connect(eng, &IEngine::torrentsUpdated,
                         sessionBridge, &QmlSessionBridge::emitStats);
        QObject::connect(resolver, &MetadataResolver::metadataReady,
                         sessionBridge, &QmlSessionBridge::emitStats);

        QObject::connect(eng, &IEngine::torrentsUpdated,
                         posterModel, &QmlPosterModel::refresh);
        // Index-aware removal — beginRemoveRows for the exact row instead of a
        // full model reset, so the grid doesn't flash and jump to the top.
        // torrentRemoved is forwarded over IPC (the snapshot refresh is the
        // safety net if the index races), so connect on the IEngine interface.
        QObject::connect(eng, &IEngine::torrentRemoved,
                         posterModel, &QmlPosterModel::removeRow);
        // Keep the bridge's stored selection indices valid across a removal.
        QObject::connect(eng, &IEngine::torrentRemoved,
                         sessionBridge, &QmlSessionBridge::onTorrentRemoved);
        // A resolved poster only touches one row's poster/title roles.
        QObject::connect(resolver, &MetadataResolver::metadataReady,
                         posterModel, &QmlPosterModel::posterResolved);
        // Explicit edits (rename/category/restore/import) need every role.
        QObject::connect(sessionBridge, &QmlSessionBridge::queueRefreshNeeded,
                         posterModel, &QmlPosterModel::refreshFull);
        QObject::connect(sessionBridge, &QmlSessionBridge::queueMoved,
                         posterModel, &QmlPosterModel::moveRow);
        // The per-add cover-hint resolve + add-toast + auto-trackers. Both modes
        // funnel into one handler: in-process off SessionManager::torrentAdded(int)
        // (gathering the data + consuming the hint by index), and in split mode off
        // IEngine::torrentAddedInfo, whose event carries the same data resolved
        // engine-side (no racing index query). Resume-loaded torrents don't reach
        // here — loadResumeData() runs in the SessionManager ctor, before this.
        auto handleAdded = [resolver, notificationBridge, eng](
                int index, const QString &hash, const QString &name, qint64 totalSize,
                const QStringList &fileNames, const QString &hintTitle, int hintType) {
            if (!hash.isEmpty()) {
                // A catalog add carries a clean title + known type (game catalog →
                // Game, Stremio → Movie/Series). Query the API directly with it
                // instead of guessing from the messy torrent/metadata name, which
                // mismatched (e.g. GoW Ragnarök → wrong game).
                if (!hintTitle.isEmpty() && hintType >= 0)
                    resolver->resolveManual(hash, hintTitle, static_cast<ContentType>(hintType));
                else
                    resolver->resolve(hash, name, fileNames);
            }
            notificationBridge->onTorrentAdded(
                totalSize > 0 ? name + " · " + formatSize(totalSize) : name);
            if (AddonManager::instance().autoTrackersEnabled())
                for (const QString &tr : AddonManager::instance().trackerList())
                    eng->addTracker(index, tr);
        };
        if (localSession) {
            QObject::connect(localSession, &SessionManager::torrentAdded,
                             &app, [localSession, handleAdded](int index) {
                const auto info = localSession->torrentAt(index);
                const QString hash = localSession->torrentHashAt(index);
                const auto hint = localSession->takeCoverHint(hash);
                handleAdded(index, hash, info.name, info.totalSize,
                            localSession->torrentFileNames(index), hint.title, hint.type);
            });
        }
        // Split mode: IpcEngine re-emits this from the forwarded event (never fires
        // in-process, so it can connect unconditionally).
        QObject::connect(eng, &IEngine::torrentAddedInfo, &app,
                         [handleAdded](int index, const QString &hash, const QString &name,
                                       qint64 totalSize, const QStringList &fileNames,
                                       const QString &hintTitle, int hintType) {
            handleAdded(index, hash, name, totalSize, fileNames, hintTitle, hintType);
        });
        AddonManager::instance().fetchTrackerList();   // refresh the list on startup

        {
            QStringList hashes, names;
            for (int i = 0; i < eng->torrentCount(); ++i) {
                QString h = eng->torrentHashAt(i);
                if (!h.isEmpty() && !resolver->hasCached(h)) {
                    hashes << h;
                    names << eng->torrentAt(i).name;
                }
            }
            if (!hashes.isEmpty())
                resolver->batchResolve(hashes, names);
        }

        auto *filterProxy = new QmlTorrentFilterProxy(&app);
        filterProxy->setSourceModel(posterModel);

        {
            QSettings s;
            int lang = 0;
            if (s.contains("language")) {
                lang = s.value("language").toInt();
            } else {
                const QString sys = QLocale::system().name().toLower();
                if      (sys.startsWith("pt")) lang = 1;
                else if (sys.startsWith("zh")) lang = 2;
                else if (sys.startsWith("ja")) lang = 3;
                else if (sys.startsWith("ru")) lang = 4;
                else if (sys.startsWith("es")) lang = 5;
                else if (sys.startsWith("de")) lang = 6;
                else if (sys.startsWith("uk")) lang = 7;
                else if (sys.startsWith("tr")) lang = 8;
                else                           lang = 0;
            }
            Translator::instance().setLanguage(static_cast<Translator::Language>(lang));
        }
        auto *i18nBridge = new QmlI18nBridge(&app);

        // OS-scheme-aware app/window icon (so the white logo isn't invisible on
        // a light Windows taskbar). The bridge keeps it live on scheme changes.
        // Skipped on macOS: setWindowIcon hijacks the Dock tile, overriding the
        // bundled .icns and any user customization (issue #14).
#ifndef Q_OS_MACOS
        app.setWindowIcon(themeBridge->trayIcon());
#endif
        // A user-chosen custom app icon overrides the above on all platforms
        // (incl. the macOS Dock — intended; the default keeps the bundled .icns).
        themeBridge->applySavedAppIcon();

        QQmlApplicationEngine engine;
        engine.addImageProvider(QStringLiteral("applogo"), new AppLogoImageProvider());
        engine.rootContext()->setContextProperty("torrentModel", filterProxy);
        engine.rootContext()->setContextProperty("torrentFilter", filterProxy);
        engine.rootContext()->setContextProperty("themeBridge", themeBridge);
        engine.rootContext()->setContextProperty("session", sessionBridge);
        engine.rootContext()->setContextProperty("rss", rssBridge);
        engine.rootContext()->setContextProperty("settings", settingsBridge);
        engine.rootContext()->setContextProperty("addons", addonBridge);
        engine.rootContext()->setContextProperty("search", searchBridge);
        engine.rootContext()->setContextProperty("discovery", discoveryService);
        // Store builds stay neutral: the Find page drops the curated catalog
        // (browse surface) and falls back to plain search. Everything else is identical.
#ifdef BAT_STORE_BUILD
        engine.rootContext()->setContextProperty("isStoreBuild", true);
#else
        engine.rootContext()->setContextProperty("isStoreBuild", false);
#endif
        engine.rootContext()->setContextProperty("logs", logBridge);
        engine.rootContext()->setContextProperty("subsearch", subtitleBridge);
        engine.rootContext()->setContextProperty("pairing", pairingBridge);
        engine.rootContext()->setContextProperty("debrid", debrid);
        engine.rootContext()->setContextProperty("notifications", notificationBridge);
        engine.rootContext()->setContextProperty("i18n", i18nBridge);
#ifndef BAT_STORE_BUILD
        engine.rootContext()->setContextProperty("updater", updaterBridge);
#else
        engine.rootContext()->setContextProperty("updater", nullptr);
#endif
        engine.load(QUrl("qrc:/src/qml/Main.qml"));
        if (engine.rootObjects().isEmpty())
            return -1;

        // Record which scene-graph backend is actually in use. If a machine
        // falls back to the Software renderer (no GPU), the whole UI stutters
        // like a game compiling shaders even on a fast card — this line in the
        // log tells us that immediately instead of guessing.
        if (auto *qw = qobject_cast<QQuickWindow *>(engine.rootObjects().first())) {
            const char *backend = "Unknown";
            switch (qw->rendererInterface()->graphicsApi()) {
            case QSGRendererInterface::Software:   backend = "Software (NO GPU — expect heavy stutter)"; break;
            case QSGRendererInterface::OpenGL:     backend = "OpenGL"; break;
            case QSGRendererInterface::Direct3D11: backend = "Direct3D11"; break;
            case QSGRendererInterface::Direct3D12: backend = "Direct3D12"; break;
            case QSGRendererInterface::Vulkan:     backend = "Vulkan"; break;
            case QSGRendererInterface::Metal:      backend = "Metal"; break;
            default: break;
            }
            Logger::instance().log(Logger::Info,
                QStringLiteral("[render] scene graph backend: %1").arg(QLatin1String(backend)));
        }
#ifndef BAT_STORE_BUILD
        if (!safeMode)                // in safe mode, don't re-trigger a possibly-bad auto-update
            updaterBridge->check(true);   // silent check on startup
#endif

        // Single-instance server (the legacy path had this; the QML path didn't,
        // so launching again opened a second copy). Listen for forwarded args
        // from a second launch, raise our window, and add any .torrent/magnet.
        QObject *rootObj = engine.rootObjects().first();
        QLocalServer::removeServer(kServerName);
        auto *instanceServer = new QLocalServer(&app);
        instanceServer->listen(kServerName);
        QObject::connect(instanceServer, &QLocalServer::newConnection, &app,
                         [instanceServer, rootObj, sessionBridge]() {
            QLocalSocket *client = instanceServer->nextPendingConnection();
            if (auto *w = qobject_cast<QWindow *>(rootObj)) {
                // show() demotes a maximized window to normal (reported: a
                // browser magnet click un-maximized the app). Only touch
                // visibility when actually hidden/minimized.
                if (w->visibility() == QWindow::Hidden) w->show();
                else if (w->windowStates() & Qt::WindowMinimized)
                    w->setWindowStates(w->windowStates() & ~Qt::WindowMinimized);
                w->raise(); w->requestActivate();
            }
            QObject::connect(client, &QLocalSocket::readyRead, client, [client, sessionBridge]() {
                const QStringList lines = QString::fromUtf8(client->readAll()).split('\n', Qt::SkipEmptyParts);
                for (const QString &line : lines) {
                    if (line.endsWith(".torrent")) sessionBridge->requestAddTorrentFile(line);
                    else if (line.startsWith("magnet:") || line.startsWith("bittorrent:")) sessionBridge->addMagnetUri(line);
                }
                client->deleteLater();
            });
        });

        // First-instance CLI args (.torrent / magnet passed on launch)
        for (int i = 1; i < app.arguments().size(); ++i) {
            const QString &arg = app.arguments().at(i);
            if (arg.endsWith(".torrent")) sessionBridge->requestAddTorrentFile(arg);
            else if (arg.startsWith("magnet:") || arg.startsWith("bittorrent:")) sessionBridge->addMagnetUri(arg);
        }

        const int rc = app.exec();
        // `app`'s dtor (Qt/plugin DLL teardown) runs after this, during which a
        // late Qt log message used to crash inside our handler (Logger touched
        // mid-DllMain/FreeLibrary at exit — Sentry NATIVE-QT-9); revert to Qt's
        // default handler first so nothing that late reaches our machinery.
        qInstallMessageHandler(nullptr);
        return rc;
    }
}
