#include <QApplication>
#include <QIcon>
#include <QStringList>
#include "core/sessionmanager.h"
#include "gui/mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("BATorrent");
    app.setApplicationVersion("1.4.0");
    app.setWindowIcon(QIcon(":/images/logo1.png"));

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
