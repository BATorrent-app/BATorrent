// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "app/mediachild.h"

#include "app/appruntime.h"
#include "bridges/isolatedmediaplayer.h"
#include "ipc/mediahost.h"
#include "services/platform/logger.h"

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QMediaPlayer>
#include <QUrl>
#include <QVideoFrame>
#include <QVideoSink>
#include <cstring>

namespace {

// Plays `path` through a real --media child for a few seconds and reports what
// came back: the whole spawn/socket/ring path, no UI. A tester can run it as-is.
int selfTest(const QString &path)
{
    IsolatedMediaPlayer player;
    QVideoSink sink;
    int frames = 0;
    QObject::connect(&sink, &QVideoSink::videoFrameChanged, [&](const QVideoFrame &f) {
        if (f.isValid() && ++frames == 1)
            qInfo() << "[media-selftest] first frame" << f.size() << f.pixelFormat();
    });
    QObject::connect(&player, &IsolatedMediaPlayer::decoderCrashed, [] {
        qWarning() << "[media-selftest] decoder crashed";
    });
    player.setVideoOutput(&sink);
    player.setMuted(true);
    player.setSource(path.startsWith(QLatin1String("http")) ? QUrl(path) : QUrl::fromLocalFile(path));
    player.play();
    QElapsedTimer t;
    t.start();
    while (t.elapsed() < 6000 && player.error() == QMediaPlayer::NoError)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
    qInfo() << "[media-selftest] frames" << frames << "duration" << player.duration()
            << "position" << player.position() << "audio tracks" << player.audioTracks().size()
            << "subtitle tracks" << player.subtitleTracks().size() << "error" << player.errorString();
    return frames > 10 && player.error() == QMediaPlayer::NoError ? 0 : 1;
}

}

namespace MediaChild {

bool tryRun(int argc, char *argv[], int *exitCode)
{
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--media-selftest") == 0 && i + 1 < argc) {
            QCoreApplication mapp(argc, argv);
            mapp.setOrganizationName("BATorrent");
            mapp.setApplicationName("BATorrent");
            *exitCode = selfTest(QString::fromLocal8Bit(argv[i + 1]));
            return true;
        }
        if (std::strcmp(argv[i], "--media") != 0 || i + 1 >= argc) continue;
        QCoreApplication mapp(argc, argv);
        mapp.setOrganizationName("BATorrent");
        mapp.setApplicationName("BATorrent");
        mapp.setApplicationVersion(APP_VERSION);
        Logger::instance().init();
#ifdef BAT_HAVE_SENTRY
        AppRuntime::initSentry(QStringLiteral("media"));
#endif
        MediaHost host(QString::fromLocal8Bit(argv[i + 1]),
                       !qEnvironmentVariableIsSet("BAT_MEDIA_NO_SANDBOX"));
        *exitCode = host.listen() ? mapp.exec() : 1;
        return true;
    }
    return false;
}

} // namespace MediaChild
