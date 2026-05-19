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
#include "torrent/sessionmanager.h"
#include "app/secretstore.h"
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

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("BATorrent");
    app.setApplicationVersion(APP_VERSION);
    app.setWindowIcon(QIcon(":/images/logo1.png"));
    app.setQuitOnLastWindowClosed(false); // keep running in tray when window is closed

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
