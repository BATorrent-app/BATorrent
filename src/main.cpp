// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include <QApplication>
#include <memory>
#include <QIcon>
#include <QStringList>
#include <QFont>
#include <QFontDatabase>
#include <QLocalServer>
#include <QLocalSocket>
#include <QWindow>
#include <QSettings>
#include <QTimer>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickImageProvider>
#include <QQuickStyle>
#include <QGuiApplication>
#include <QSplashScreen>
#include <QPixmap>
#include <QQuickWindow>
#include <QSGRendererInterface>
#include <QLocale>
#include <QMessageBox>
#include <QPushButton>
#include <QDesktopServices>
#include <QProcess>
#include <QDir>
#include <QDirIterator>
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
#include "bridges/qmlsessionbridge.h"
#include "webui/streamserver.h"
#include "services/discovery/gamesourcemanager.h"
#include "services/integrations/rssmanager.h"
#include "services/discovery/addonmanager.h"
#include "services/integrations/notifier.h"
#include "torrent/sessionmanager.h"
#include "services/downloads/httpdownloadmanager.h"
#include "services/downloads/httpmergeengine.h"
#include "services/vpn/vpnmanager.h"
#include "services/vpn/wgtunnelfactory.h"
#include "ipc/enginehost.h"
#include "ipc/ipcengine.h"
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QEventLoop>
#include <cstring>
#include <QThread>
#include "services/security/secretstore.h"
#include "services/security/blocklistupdater.h"
#include "services/integrations/debridmanager.h"
#include "services/platform/logger.h"
#include "services/security/crashhandler.h"
#include "services/platform/utils.h"

#include <libtorrent/version.hpp>
#include <boost/version.hpp>

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

// Must run before QApplication constructs the first QQuickWindow. Index matches
// Settings → Advanced "graphicsApi" (0 Auto, 1 Software, 2 OpenGL, 3 D3D11).
static void applyGraphicsApiPreference()
{
    QString api = QString::fromLocal8Bit(qgetenv("BAT_GRAPHICS_API")).trimmed().toLower();
    if (api.isEmpty()) {
        const int idx = QSettings(QStringLiteral("BATorrent"), QStringLiteral("BATorrent"))
                            .value(QStringLiteral("graphicsApi"), 0).toInt();
        if (idx == 1) api = QStringLiteral("software");
        else if (idx == 2) api = QStringLiteral("opengl");
        else if (idx == 3) api = QStringLiteral("d3d11");
    }
    if (api == QLatin1String("software"))
        QQuickWindow::setGraphicsApi(QSGRendererInterface::Software);
    else if (api == QLatin1String("opengl"))
        QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);
    else if (api == QLatin1String("d3d11") || api == QLatin1String("direct3d11"))
        QQuickWindow::setGraphicsApi(QSGRendererInterface::Direct3D11);
}

static void showQmlLoadFailure(const QString &logHint)
{
    qInstallMessageHandler(nullptr);
    QMessageBox box;
    box.setIcon(QMessageBox::Critical);
    box.setWindowTitle(QStringLiteral("BATorrent"));
    box.setText(QStringLiteral("BATorrent couldn't load its interface."));
    box.setInformativeText(
        QStringLiteral("The log may explain why (%1). If the window opens blank or gray, "
                       "reinstall or set graphicsApi=software in settings and restart.")
            .arg(logHint));
    box.addButton(QMessageBox::Ok);
    box.exec();
}

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

    // Graphics API before QApplication — otherwise the first QQuickWindow locks
    // in D3D11/Metal and a gray client area has nowhere to fall back.
    applyGraphicsApiPreference();

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

    // Keychain migration runs after the first painted frame (see frameSwapped
    // below) so a stalled Credential Manager can't look like "won't open".

    // One-time migration: "autoShutdown" (bool) -> "postDownloadAction" (index,
    // 6 = shut down). A user who had it on keeps getting a shutdown, not
    // silently nothing, once the setting becomes a multi-action picker.
    {
        QSettings st;
        if (!st.contains("postDownloadAction") && st.value("autoShutdown", false).toBool())
            st.setValue("postDownloadAction", 6);
    }

    // Single-instance check: if another instance is running, forward args and quit
    QString argsPayload = collectArgs(app.arguments());
    if (sendToRunningInstance(argsPayload))
        return 0;

    // Claim the socket NOW — before SessionManager/resume — so a second launch
    // during a slow boot forwards here instead of fighting the same resume dir.
    QLocalServer::removeServer(kServerName);
    auto *instanceServer = new QLocalServer(&app);
    if (!instanceServer->listen(kServerName))
        qWarning() << "[instance] listen failed:" << instanceServer->errorString();
    auto sessionBridgeHolder = std::make_shared<QmlSessionBridge *>(nullptr);
    auto rootHolder = std::make_shared<QObject *>(nullptr);
    auto pendingForwarded = std::make_shared<QStringList>();
    QObject::connect(instanceServer, &QLocalServer::newConnection, &app,
                     [instanceServer, sessionBridgeHolder, rootHolder, pendingForwarded]() {
        QLocalSocket *client = instanceServer->nextPendingConnection();
        if (!client) return;
        if (auto *w = qobject_cast<QWindow *>(*rootHolder)) {
            if (w->visibility() == QWindow::Hidden) w->show();
            else if (w->windowStates() & Qt::WindowMinimized)
                w->setWindowStates(w->windowStates() & ~Qt::WindowMinimized);
            w->raise(); w->requestActivate();
        }
        QObject::connect(client, &QLocalSocket::readyRead, client,
                         [client, sessionBridgeHolder, pendingForwarded]() {
            const QStringList lines = QString::fromUtf8(client->readAll())
                                          .split('\n', Qt::SkipEmptyParts);
            if (QmlSessionBridge *bridge = *sessionBridgeHolder) {
                for (const QString &line : lines) {
                    if (line.endsWith(".torrent")) bridge->requestAddTorrentFile(line);
                    else if (line.startsWith("magnet:") || line.startsWith("bittorrent:"))
                        bridge->addMagnetUri(line);
                }
            } else {
                *pendingForwarded << lines;
            }
            client->deleteLater();
        });
    });

    // Native splash so a long resume parse isn't "I launched and nothing happened".
    QSplashScreen splash(QPixmap(QStringLiteral(":/images/logo1.png"))
                             .scaled(160, 160, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    splash.show();
    app.processEvents();

    // --- boot-health sentinel: catch a crash during startup (e.g. corrupt resume
    // data, or a bad auto-update) and offer recovery BEFORE the risky init runs
    // again. markBootHealthy() clears it after the first painted frame.
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

    // Load IBM Plex Sans family
    QFontDatabase::addApplicationFont(":/fonts/IBMPlexSans-Regular.ttf");
    QFontDatabase::addApplicationFont(":/fonts/IBMPlexSans-Medium.ttf");
    QFontDatabase::addApplicationFont(":/fonts/IBMPlexSans-SemiBold.ttf");
    QFontDatabase::addApplicationFont(":/fonts/IBMPlexSans-Bold.ttf");
    QFontDatabase::addApplicationFont(":/fonts/NewRocker-Regular.ttf");   // brand wordmark

    // A family Qt can't resolve falls back silently to the system font — the UI
    // still "works", just wrong everywhere. Say so in the log instead.
    if (!QFontDatabase::families().contains(QStringLiteral("IBM Plex Sans")))
        qWarning() << "[font] IBM Plex Sans failed to register — the UI is on a fallback family";

    QFont defaultFont("IBM Plex Sans", 10);
    defaultFont.setStyleStrategy(QFont::PreferAntialias);
    app.setFont(defaultFont);

    // Use the Qt Quick default (distance-field) text rendering on every
    // platform so Windows matches macOS. Native Windows rendering was crisper
    // but rendered the bundled weights noticeably thinner than the Mac reference.

    // Dependency versions in the log, not in the About box: a bug report ships
    // the log, and this is the first thing you need when a crash is library-specific.
    qInfo().nospace().noquote() << "[versions] BATorrent " << QCoreApplication::applicationVersion()
                      << " · Qt " << qVersion()
                      << " · libtorrent " << LIBTORRENT_VERSION
                      << " · Boost " << QString::fromLatin1(BOOST_LIB_VERSION).replace('_', '.');

    QQuickStyle::setStyle("Basic");
    {

#ifdef BAT_LIBTORRENT_FORK
        // Fork builds ship their exact torrent-rasterbar; a different library on
        // disk (in-place update gone wrong, loader picking a stray DLL) is ABI
        // poison that crashes as heap corruption deep inside session startup —
        // the shape of the top Sentry crasher (mtx_do_lock AV). Catch the
        // mismatch while it is still explainable.
        if (qstrcmp(lt::version(), LIBTORRENT_VERSION) != 0) {
#ifdef BAT_HAVE_SENTRY
            sentry_set_tag("lt.runtime", lt::version());
#endif
            QMessageBox::critical(nullptr, QStringLiteral("BATorrent"),
                QStringLiteral("The torrent engine on this install (%1) does not match "
                               "this version of BATorrent (%2).\n\nThe installation is "
                               "likely corrupted — please reinstall BATorrent.")
                    .arg(QString::fromLatin1(lt::version()),
                         QStringLiteral(LIBTORRENT_VERSION)));
            return 1;
        }
#endif

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
        IEngine *baseEng = localSession ? static_cast<IEngine *>(localSession)
                                        : static_cast<IEngine *>(ipcEngine);
        // Direct-HTTP downloads (file-host links, "Download from link") ride the
        // same Downloads UI by presenting as extra IEngine rows — the merge
        // decorator wraps the real engine so nothing downstream changes.
        auto *httpDownloads = new HttpDownloadManager(&app);
        IEngine *eng = new HttpMergeEngine(baseEng, httpDownloads, &app);

        // Integrated VPN (import a WireGuard config, connect through it). Once a
        // REAL tunnel is up, bind the torrent session to its interface (the stub
        // never binds, so it can't break connectivity). The pre-VPN binding is
        // remembered in "preVpnInterface" so a deliberate disconnect restores it;
        // a tunnel DROP with the kill switch on keeps the dead binding instead —
        // fail closed, the kill switch pauses everything until the VPN is back.
        auto *vpnManager = new VpnManager(makeWgTunnel(&app), &app);
        QObject::connect(vpnManager, &VpnManager::interfaceUp, &app, [vpnManager, eng](const QString &iface) {
            if (!vpnManager->tunnelIsReal()) return;
            QSettings s;
            if (!s.contains(QStringLiteral("preVpnInterface")))   // keep the original across reconnects
                s.setValue(QStringLiteral("preVpnInterface"), s.value(QStringLiteral("outgoingInterface")).toString());
            eng->applySetting(QStringLiteral("outgoingInterface"), iface);
        });
        QObject::connect(vpnManager, &VpnManager::interfaceDown, &app, [vpnManager, eng](bool deliberate) {
            if (!vpnManager->tunnelIsReal()) return;
            QSettings s;
            if (!s.contains(QStringLiteral("preVpnInterface"))) return;   // never got bound
            if (!deliberate && s.value(QStringLiteral("killSwitchEnabled"), false).toBool()) return;
            eng->applySetting(QStringLiteral("outgoingInterface"),
                              s.value(QStringLiteral("preVpnInterface")).toString());
            s.remove(QStringLiteral("preVpnInterface"));
        });
        // The tunnel outlives the process on every OS — re-attach to one a
        // previous run left up (re-emits interfaceUp → rebinds the engine).
        vpnManager->adoptRunningTunnel();
        if (vpnManager->state() != VpnManager::State::Connected) {
            // App quit (or crashed) while the VPN was connected and the tunnel
            // is gone: the persisted binding points at a dead interface. With
            // the kill switch off, restore the pre-VPN binding so torrents
            // aren't silently dead; with it on, stay bound = stay failed-closed.
            QSettings s;
            if (s.contains(QStringLiteral("preVpnInterface"))
                && !s.value(QStringLiteral("killSwitchEnabled"), false).toBool()) {
                eng->applySetting(QStringLiteral("outgoingInterface"),
                                  s.value(QStringLiteral("preVpnInterface")).toString());
                s.remove(QStringLiteral("preVpnInterface"));
            }
        }
        if (vpnManager->tunnelIsReal()
            && QSettings().value(QStringLiteral("vpnAutoConnect"), false).toBool()) {
            // Delayed so the elevation prompt appears over a visible window.
            QTimer::singleShot(1500, vpnManager, &VpnManager::connectLastUsed);
        }

        auto *resolver = new MetadataResolver(&app);
        auto *posterModel = new QmlPosterModel(eng, resolver, &app);
        auto *themeBridge = new QmlThemeBridge(&app);
        auto *sessionBridge = new QmlSessionBridge(eng, resolver, &app);
        sessionBridge->setHttpDownloads(httpDownloads);
        httpDownloads->setDefaultDir(sessionBridge->defaultSavePath());
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
        // "Block known bad peers": download/refresh a reputable IP blocklist and
        // feed it to the engine's ip_filter (dropped before the handshake), cutting
        // connections to flagged IPs that trip antivirus warnings. Opt-in.
        auto *blocklist = new BlocklistUpdater(&app);
        QObject::connect(blocklist, &BlocklistUpdater::ready, &app,
                         [eng](const QString &path, int) { eng->applySetting("autoBlocklistFile", path); });
        QObject::connect(settingsBridge, &QmlSettingsBridge::blockBadPeersToggled, &app,
                         [eng, blocklist](bool on) {
            if (on) blocklist->update();
            else    eng->applySetting("autoBlocklistFile", QString());
        });
        if (QSettings().value("blockBadPeers", false).toBool()) {
            if (QFileInfo::exists(BlocklistUpdater::cachePath()))
                eng->applySetting("autoBlocklistFile", BlocklistUpdater::cachePath());
            if (BlocklistUpdater::cacheStale())
                blocklist->update();
        }

        auto *addonBridge = new QmlAddonBridge(&app);
        auto *searchBridge = new QmlSearchBridge(eng, &app);
        searchBridge->setResolver(resolver);
        searchBridge->setHttpDownloads(httpDownloads);
        auto *discoveryService = new DiscoveryService(&app);
        searchBridge->setDiscovery(discoveryService);

#ifndef BAT_STORE_BUILD
        // Seed / migrate the default game catalog. Bump kCatalogSeedGen when the
        // default URL changes so existing installs pick up the BATorrent feed once
        // (users who removed all catalogs stay empty — we only add if missing).
        {
            static const int kCatalogSeedGen = 5;
            static const char *kCatalogName = "BATorrent Games";
            // Public Hydra JSON gist (neutral filename). Published by a private pipeline.
            static const char *kCatalogUrl =
                "https://gist.githubusercontent.com/Mateuscruz19/038beb9fef8681e191e3053b8a79c29b/raw/feed.json";
            static const char *kLegacyUrl =
                "https://raw.githubusercontent.com/Jdjsjjqq/rutracker-hydra/main/combined_torrents.json";
            static const char *kLegacyGistGames =
                "https://gist.githubusercontent.com/Mateuscruz19/038beb9fef8681e191e3053b8a79c29b/raw/games.json";
            static const char *kLegacyGistBat =
                "https://gist.githubusercontent.com/Mateuscruz19/038beb9fef8681e191e3053b8a79c29b/raw/batorrent-games.json";

            QSettings gs;
            const int gen = gs.value(QStringLiteral("gameCatalogSeedGen"), 0).toInt();
            auto &gsm = GameSourceManager::instance();
            if (gen < kCatalogSeedGen) {
                gs.setValue(QStringLiteral("gameCatalogSeedGen"), kCatalogSeedGen);
                gs.setValue(QStringLiteral("gameSourcesSeeded"), true);
                gsm.removeSource(QString::fromUtf8(kLegacyUrl));
                gsm.removeSource(QString::fromUtf8(kLegacyGistGames));
                gsm.removeSource(QString::fromUtf8(kLegacyGistBat));
                bool has = false;
                for (const auto &s : gsm.sources())
                    if (s.second == QLatin1String(kCatalogUrl)) { has = true; break; }
                if (!has)
                    gsm.addSource(QString::fromUtf8(kCatalogName), QString::fromUtf8(kCatalogUrl));
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
        // qml-smoke / CI: force-instantiate deferred window Loaders once.
        engine.rootContext()->setContextProperty(
            "batSmokeLoaders", qEnvironmentVariableIsSet("BAT_SMOKE_LOADERS"));
#ifndef BAT_STORE_BUILD
        engine.rootContext()->setContextProperty("updater", updaterBridge);
        engine.rootContext()->setContextProperty("vpn", vpnManager);
#else
        engine.rootContext()->setContextProperty("updater", nullptr);
#endif
        // Dev QML loop: `BAT_QML_DIR=$PWD/src/qml` loads the UI straight from the
        // source tree (not the compiled qrc), and hot-reloads the window whenever
        // a .qml is saved — so a colour/badge tweak shows in ~1s with no rebuild.
        // Empty in every shipped build → the normal qrc load below.
        const QString devQmlDir = qEnvironmentVariable("BAT_QML_DIR");
        const QUrl rootUrl = devQmlDir.isEmpty()
            ? QUrl(QStringLiteral("qrc:/src/qml/Main.qml"))
            : QUrl::fromLocalFile(QDir(devQmlDir).filePath(QStringLiteral("Main.qml")));

        bool mainObjectOk = false;
        QObject::connect(&engine, &QQmlApplicationEngine::objectCreated, &app,
                         [&mainObjectOk, rootUrl](QObject *obj, const QUrl &objUrl) {
            if (objUrl != rootUrl) return;
            mainObjectOk = (obj != nullptr);
        });
        engine.load(rootUrl);
        if (engine.rootObjects().isEmpty() || !mainObjectOk) {
            splash.finish(nullptr);
            const QString logPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                                    + QStringLiteral("/batorrent.log");
            showQmlLoadFailure(logPath);
            return 1;
        }

        *sessionBridgeHolder = sessionBridge;
        *rootHolder = engine.rootObjects().first();
        for (const QString &line : *pendingForwarded) {
            if (line.endsWith(".torrent")) sessionBridge->requestAddTorrentFile(line);
            else if (line.startsWith("magnet:") || line.startsWith("bittorrent:"))
                sessionBridge->addMagnetUri(line);
        }
        pendingForwarded->clear();

        if (!devQmlDir.isEmpty()) {
            // Poll .qml mtimes rather than QFileSystemWatcher: editors save by
            // atomic replace (write temp + rename), which swaps the inode and
            // makes the watcher miss the change on macOS. A 500 ms mtime scan is
            // dead-simple and catches every save however the editor writes it.
            auto snapshot = []( const QString &dir) {
                QHash<QString, qint64> m;
                QDirIterator it(dir, {QStringLiteral("*.qml"), QStringLiteral("qmldir")},
                                QDir::Files, QDirIterator::Subdirectories);
                while (it.hasNext()) {
                    const QString p = it.next();
                    m.insert(p, QFileInfo(p).lastModified().toMSecsSinceEpoch());
                }
                return m;
            };
            auto lastSeen = std::make_shared<QHash<QString, qint64>>(snapshot(devQmlDir));
            auto *poll = new QTimer(&app);
            poll->setInterval(500);
            QObject::connect(poll, &QTimer::timeout, &app,
                             [&engine, rootUrl, devQmlDir, snapshot, lastSeen]() {
                const QHash<QString, qint64> now = snapshot(devQmlDir);
                if (now == *lastSeen) return;   // nothing changed
                *lastSeen = now;
                const QList<QObject *> old = engine.rootObjects();
                engine.clearComponentCache();
                engine.load(rootUrl);
                for (QObject *o : old) {
                    if (auto *w = qobject_cast<QQuickWindow *>(o)) w->close();
                    o->deleteLater();
                }
                qInfo() << "[dev] QML hot-reloaded";
            });
            poll->start();
            qInfo() << "[dev] QML hot-reload watching" << devQmlDir;
        }

        // Record which scene-graph backend is actually in use. If a machine
        // falls back to the Software renderer (no GPU), the whole UI stutters
        // like a game compiling shaders even on a fast card — this line in the
        // log tells us that immediately instead of guessing.
        auto *qw = qobject_cast<QQuickWindow *>(engine.rootObjects().first());
        if (qw) {
            // QQuickWindow is not a QWidget — finish(nullptr) just dismisses the splash.
            splash.finish(nullptr);
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
            themeBridge->setSoftwareRenderer(
                qw->rendererInterface()->graphicsApi() == QSGRendererInterface::Software
                || QSettings(QStringLiteral("BATorrent"), QStringLiteral("BATorrent"))
                       .value(QStringLiteral("graphicsApi"), 0).toInt() == 1);
            Logger::instance().log(Logger::Info,
                QStringLiteral("[render] scene graph backend: %1").arg(QLatin1String(backend)));

            // Boot is healthy only after a real painted frame — a Timer firing
            // while the client area stays gray used to clear the crash sentinel.
            auto gotFrame = std::make_shared<bool>(false);
            auto frameConn = std::make_shared<QMetaObject::Connection>();
            *frameConn = QObject::connect(qw, &QQuickWindow::frameSwapped, themeBridge,
                             [themeBridge, gotFrame, frameConn]() {
                if (*gotFrame) return;
                *gotFrame = true;
                QObject::disconnect(*frameConn);
                themeBridge->markBootHealthy();
                // Deferred keychain I/O — Credential Manager stalls must not
                // block the first paint.
                SecretStore::instance().migrateFromSettings({
                    "proxyPass", "plexToken", "jellyfinApiKey"
                });
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
                qInfo() << "[boot] first frame presented — boot healthy";
                if (qEnvironmentVariableIsSet("BAT_SMOKE_EXIT_ON_FRAME"))
                    QCoreApplication::exit(0);
            });
            QObject::connect(qw, &QQuickWindow::sceneGraphError, qw,
                             [gotFrame](QQuickWindow::SceneGraphError, const QString &msg) {
                qCritical() << "[render] sceneGraphError:" << msg;
                QSettings st(QStringLiteral("BATorrent"), QStringLiteral("BATorrent"));
                st.setValue(QStringLiteral("graphicsApi"), 1);   // Software next launch
                st.sync();
                if (qEnvironmentVariableIsSet("BAT_SMOKE_EXIT_ON_FRAME"))
                    QCoreApplication::exit(3);
                QMessageBox::warning(nullptr, QStringLiteral("BATorrent"),
                    QStringLiteral("Graphics failed to start. Switching to Software renderer — "
                                   "please restart BATorrent.\n\n%1").arg(msg));
            });
            QTimer::singleShot(10000, &app, [gotFrame]() {
                if (*gotFrame) return;
                qCritical() << "[boot] no frame within 10s — treating as gray-screen failure";
                QSettings st(QStringLiteral("BATorrent"), QStringLiteral("BATorrent"));
                if (st.value(QStringLiteral("graphicsApi"), 0).toInt() != 1) {
                    st.setValue(QStringLiteral("graphicsApi"), 1);
                    st.sync();
                }
                if (qEnvironmentVariableIsSet("BAT_SMOKE_EXIT_ON_FRAME"))
                    QCoreApplication::exit(2);
            });
        } else {
            splash.finish(nullptr);
        }
#ifndef BAT_STORE_BUILD
        if (!safeMode)                // in safe mode, don't re-trigger a possibly-bad auto-update
            updaterBridge->check(true);   // silent check on startup
#endif

        // Instance server already listening (early claim). Raise/forward wired above.

#ifdef Q_OS_MACOS
        // Dock reopen: clicking the dock icon of a running app whose window is
        // hidden (close-to-tray) leaves nothing to bring back — Qt has no
        // cross-platform "reopen" event, so the window stayed lost until a full
        // relaunch. Restore it when the app becomes active while hidden, matching
        // the tray-click path. Armed after a delay so the launch-time activation
        // doesn't fight "start in tray".
        QObject *rootObj = engine.rootObjects().first();
        auto dockArmed = std::make_shared<bool>(false);
        QTimer::singleShot(2500, &app, [dockArmed]() { *dockArmed = true; });
        QObject::connect(&app, &QGuiApplication::applicationStateChanged, &app,
                         [rootObj, dockArmed](Qt::ApplicationState state) {
            if (!*dockArmed || state != Qt::ApplicationActive) return;
            if (auto *w = qobject_cast<QWindow *>(rootObj)) {
                if (w->visibility() == QWindow::Hidden) { w->show(); w->raise(); w->requestActivate(); }
            }
        });
#endif

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
