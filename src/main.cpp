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
#include <QStyleFactory>
#include <QSettings>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include "app/metadataresolver.h"
#include "gui/qmlposterbridge.h"
#include "torrent/sessionmanager.h"
#include "app/secretstore.h"
#include "app/logger.h"
#include "app/utils.h"
#include "gui/mainwindow.h"
#include "gui/thememanager.h"

static const QString kServerName = QStringLiteral("BATorrent-SingleInstance");

static QString collectArgs(const QStringList &args)
{
    QStringList relevant;
    for (int i = 1; i < args.size(); ++i) {
        const QString &a = args[i];
        if (a.endsWith(".torrent") || a.startsWith("magnet:"))
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
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("BATorrent");
    app.setApplicationVersion(APP_VERSION);
    app.setWindowIcon(QIcon(":/images/logo1.png"));
    app.setQuitOnLastWindowClosed(false); // keep running in tray when window is closed

    // CLI flag: --debug / -d forces verbose logging for this session (without
    // mutating the persisted setting). Comes before init() so the lowered
    // level takes effect immediately.
    const bool debugFlag = app.arguments().contains("--debug")
                        || app.arguments().contains("-d");
    Logger::instance().init();
    if (debugFlag)
        Logger::instance().setLevel(Logger::Trace);
    qInstallMessageHandler(qtMessageHandler);

    // Speed unit display preference (0 = bytes/sec, 1 = bits/sec). Read once
    // here so formatSpeed() doesn't hit QSettings on every call (UI refresh
    // rate would otherwise translate to ~10 QSettings opens/sec).
    setSpeedUnit(QSettings("BATorrent", "BATorrent").value("speedUnit", 0).toInt());

    // Force the Fusion style application-wide. Native Windows styles
    // (WindowsVista / Windows11) ignore certain QPalette colors and overrule
    // stylesheet background-color on some user configurations — high-
    // contrast accessibility mode is the most common cause but it can also
    // happen with certain shell themes. Fusion paints purely from
    // QPalette + stylesheet, so the theme renders identically on every
    // Windows config and the previous "white/gray Settings dialog" reports
    // can't recur.
    if (QStyle *fusion = QStyleFactory::create("Fusion"))
        app.setStyle(fusion);

    // Initial popup QSS (QMessageBox / QInputDialog / QToolTip). Re-applied
    // by MainWindow::applyTheme on every theme change so popups stay in sync.
    app.setStyleSheet(ThemeManager::instance().appPopupStyleSheet());

    // One-time migration of plaintext secrets from QSettings into the OS
    // keyring (no-op on subsequent runs and on builds without QtKeychain).
    SecretStore::instance().migrateFromSettings({
        "proxyPass", "plexToken", "jellyfinApiKey", "webUiPasswordHash"
    });

    // Single-instance check: if another instance is running, forward args and quit
    QString argsPayload = collectArgs(app.arguments());
    if (sendToRunningInstance(argsPayload))
        return 0;

    // Load Inter font family
    QFontDatabase::addApplicationFont(":/fonts/Inter-Regular.ttf");
    QFontDatabase::addApplicationFont(":/fonts/Inter-Medium.ttf");
    QFontDatabase::addApplicationFont(":/fonts/Inter-SemiBold.ttf");
    QFontDatabase::addApplicationFont(":/fonts/Inter-Bold.ttf");

    QFont defaultFont("Inter", 10);
    defaultFont.setStyleStrategy(QFont::PreferAntialias);
    app.setFont(defaultFont);

    if (app.arguments().contains("--qml")) {
        QQuickStyle::setStyle("Basic");

        SessionManager session;

        auto *resolver = new MetadataResolver(&app);
        auto *posterModel = new QmlPosterModel(&session, resolver, &app);
        auto *themeBridge = new QmlThemeBridge(&app);
        auto *sessionBridge = new QmlSessionBridge(&session, resolver, &app);
        QObject::connect(&session, &SessionManager::torrentsUpdated,
                         sessionBridge, &QmlSessionBridge::emitStats);
        QObject::connect(resolver, &MetadataResolver::metadataReady,
                         sessionBridge, &QmlSessionBridge::emitStats);

        QObject::connect(&session, &SessionManager::torrentsUpdated,
                         posterModel, &QmlPosterModel::refresh);
        QObject::connect(resolver, &MetadataResolver::metadataReady,
                         posterModel, &QmlPosterModel::refresh);
        QObject::connect(sessionBridge, &QmlSessionBridge::queueRefreshNeeded,
                         posterModel, &QmlPosterModel::refresh);
        QObject::connect(sessionBridge, &QmlSessionBridge::queueMoved,
                         posterModel, &QmlPosterModel::moveRow);
        QObject::connect(&session, &SessionManager::torrentAdded,
                         &app, [&session, resolver](int index) {
            QString hash = session.torrentHashAt(index);
            if (!hash.isEmpty())
                resolver->resolve(hash, session.torrentAt(index).name);
        });

        {
            QStringList hashes, names;
            for (int i = 0; i < session.torrentCount(); ++i) {
                QString h = session.torrentHashAt(i);
                if (!h.isEmpty() && !resolver->hasCached(h)) {
                    hashes << h;
                    names << session.torrentAt(i).name;
                }
            }
            if (!hashes.isEmpty())
                resolver->batchResolve(hashes, names);
        }

        auto *filterProxy = new QmlTorrentFilterProxy(&app);
        filterProxy->setSourceModel(posterModel);

        QQmlApplicationEngine engine;
        engine.rootContext()->setContextProperty("torrentModel", filterProxy);
        engine.rootContext()->setContextProperty("torrentFilter", filterProxy);
        engine.rootContext()->setContextProperty("themeBridge", themeBridge);
        engine.rootContext()->setContextProperty("session", sessionBridge);
        engine.load(QUrl("qrc:/src/qml/Main.qml"));
        if (engine.rootObjects().isEmpty())
            return -1;
        return app.exec();
    }

    SessionManager session;
    MainWindow window(&session);
    window.show();

    // Start local server to receive args from new instances
    QLocalServer::removeServer(kServerName); // clean up stale socket
    QLocalServer server;
    server.listen(kServerName);
    QObject::connect(&server, &QLocalServer::newConnection, [&]() {
        QLocalSocket *client = server.nextPendingConnection();
        // Always bring window to front when another instance tries to open
        window.showNormal();
        window.raise();
        window.activateWindow();
        QObject::connect(client, &QLocalSocket::readyRead, [&window, client]() {
            QString data = QString::fromUtf8(client->readAll());
            const QStringList lines = data.split('\n', Qt::SkipEmptyParts);
            for (const QString &line : lines) {
                if (line.endsWith(".torrent"))
                    window.addTorrentFromCli(line);
                else if (line.startsWith("magnet:"))
                    window.addMagnetFromCli(line);
            }
            client->deleteLater();
        });
    });

    // Handle CLI arguments for the first instance
    const QStringList args = app.arguments();
    for (int i = 1; i < args.size(); ++i) {
        const QString &arg = args[i];
        if (arg.endsWith(".torrent"))
            window.addTorrentFromCli(arg);
        else if (arg.startsWith("magnet:"))
            window.addMagnetFromCli(arg);
    }

    return app.exec();
}
