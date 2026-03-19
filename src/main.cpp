// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include <QApplication>
#include <QIcon>
#include <QStringList>
#include <QFont>
#include <QFontDatabase>
#include "torrent/sessionmanager.h"
#include "gui/mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("BATorrent");
    app.setApplicationVersion("1.8.0");
    app.setWindowIcon(QIcon(":/images/logo1.png"));
    app.setQuitOnLastWindowClosed(false); // keep running in tray when window is closed

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

    // Handle CLI arguments (torrent files or magnet links)
    QStringList args = app.arguments();
    for (int i = 1; i < args.size(); ++i) {
        const QString &arg = args[i];
        if (arg.endsWith(".torrent"))
            window.addTorrentFromCli(arg);
        else if (arg.startsWith("magnet:"))
            window.addMagnetFromCli(arg);
    }

    return app.exec();
}
