// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include <QApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QSplashScreen>
#include <QPixmap>
#include <QStandardPaths>
#include <cstdlib>

#ifdef BAT_HAVE_SENTRY
#include <sentry.h>
#endif

#include "app/appruntime.h"
#include "app/appservices.h"
#include "app/boothealth.h"
#include "app/enginechild.h"
#include "app/qmlboot.h"
#include "app/qmlcontextwiring.h"
#include "app/singleinstance.h"
#include "app/macfileopen.h"
#include "bridges/session/qmlsessionbridge.h"
#include "bridges/qmlupdaterbridge.h"
#include "services/platform/logger.h"
#include "services/security/crashhandler.h"

int main(int argc, char *argv[])
{
    int childExit = 0;
    if (EngineChild::tryRun(argc, argv, &childExit))
        return childExit;

    AppRuntime::applyGraphicsApiPreference();

    QApplication app(argc, argv);
    app.setOrganizationName("BATorrent");
    app.setApplicationName("BATorrent");
    app.setApplicationVersion(APP_VERSION);
#ifndef Q_OS_MACOS
    app.setWindowIcon(QIcon(":/images/logo1.png"));
#endif
    app.setQuitOnLastWindowClosed(false);

    const bool debugFlag = app.arguments().contains("--debug")
                        || app.arguments().contains("-d");
    Logger::instance().init();
    CrashHandler::install(
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/crashes",
        APP_VERSION);
#ifdef BAT_HAVE_SENTRY
    AppRuntime::initSentry(QStringLiteral("ui"));
    if (qEnvironmentVariableIsSet("BAT_SENTRY_TEST"))
        sentry_capture_event(sentry_value_new_message_event(
            SENTRY_LEVEL_INFO, "custom", "BATorrent sentry test event"));
#endif
    if (debugFlag) {
        Logger::instance().setLevel(Logger::Trace);
        qputenv("QSG_INFO", "1");
    }
    AppRuntime::installQtMessageHandler();
    AppRuntime::runStartupMigrations();

    if (SingleInstance::forwardToRunning(SingleInstance::collectTorrentArgs(app.arguments())))
        return 0;

    SingleInstance instance;
    instance.claim(&app);
    // Before anything slow: macOS posts the open-file event early, and the
    // filter has to be up to catch it.
    MacFileOpen::install(&app, &instance);

    QSplashScreen splash(QPixmap(QStringLiteral(":/images/logo1.png"))
                             .scaled(160, 160, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    splash.show();
    app.processEvents();

    bool safeMode = false;
    switch (BootHealth::checkCrashSentinel()) {
    case BootHealth::CheckResult::ExitZero: return 0;
    case BootHealth::CheckResult::ExitOne:  return 1;
    case BootHealth::CheckResult::ContinueSafeMode: safeMode = true; break;
    case BootHealth::CheckResult::Continue: break;
    }
    switch (BootHealth::checkLibtorrentAbi()) {
    case BootHealth::CheckResult::ExitZero: return 0;
    case BootHealth::CheckResult::ExitOne:  return 1;
    default: break;
    }

    AppRuntime::loadFonts(app);
    AppRuntime::logDependencyVersions();
    QQuickStyle::setStyle("Basic");

    AppServices svc = AppServices::create(app);

    QQmlApplicationEngine engine;
    engine.addImageProvider(QStringLiteral("applogo"), AppRuntime::createAppLogoProvider());

    QmlContextObjects ctx;
    ctx.torrentFilter = svc.filterProxy;
    ctx.themeBridge = svc.themeBridge;
    ctx.session = svc.sessionBridge;
    ctx.rss = svc.rssBridge;
    ctx.settings = svc.settingsBridge;
    ctx.addons = svc.addonBridge;
    ctx.search = svc.searchBridge;
    ctx.discovery = svc.discoveryService;
    ctx.logs = svc.logBridge;
    ctx.subsearch = svc.subtitleBridge;
    ctx.pairing = svc.pairingBridge;
    ctx.debrid = svc.debrid;
    ctx.notifications = svc.notificationBridge;
    ctx.i18n = svc.i18nBridge;
#ifndef BAT_STORE_BUILD
    ctx.updater = svc.updaterBridge;
    ctx.vpn = svc.vpnManager;
#endif
    QmlContextWiring::registerProperties(engine.rootContext(), ctx);

    const auto loaded = QmlBoot::loadMain(&engine, &app, &splash);
    if (!loaded.ok)
        return 1;

    instance.setSessionBridge(svc.sessionBridge);
    instance.setRootWindow(engine.rootObjects().first());
    instance.flushPending();

    QmlBoot::startHotReloadIfDev(&engine, QmlBoot::rootUrl(), &app);

    if (loaded.window) {
        splash.finish(nullptr);
        QmlBoot::attachFirstFrameHealth(loaded.window, svc.themeBridge, &app);
    } else {
        splash.finish(nullptr);
    }

#ifndef BAT_STORE_BUILD
    if (!safeMode)
        svc.updaterBridge->check(true);
#endif

    QmlBoot::armMacDockReopen(engine.rootObjects().first(), &app);

    for (int i = 1; i < app.arguments().size(); ++i) {
        const QString &arg = app.arguments().at(i);
        if (arg.endsWith(".torrent")) svc.sessionBridge->requestAddTorrentFile(arg);
        else if (arg.startsWith("magnet:") || arg.startsWith("bittorrent:"))
            svc.sessionBridge->addMagnetUri(arg);
    }

    const int rc = app.exec();
    qInstallMessageHandler(nullptr);
    return rc;
}
