// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "app/enginechild.h"

#include "app/appruntime.h"
#include "ipc/enginehost.h"
#include "ipc/ipcengine.h"
#include "services/platform/logger.h"
#include "torrent/sessionmanager.h"
#include "torrent/types.h"

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QObject>
#include <cstring>

namespace EngineChild {

bool tryRun(int argc, char *argv[], int *exitCode)
{
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--engine") == 0 && i + 1 < argc) {
            QCoreApplication eapp(argc, argv);
            eapp.setOrganizationName("BATorrent");
            eapp.setApplicationName("BATorrent");
            eapp.setApplicationVersion(APP_VERSION);
            Logger::instance().init();
#ifdef BAT_HAVE_SENTRY
            AppRuntime::initSentry(QStringLiteral("engine"));
#endif
            SessionManager session;
            EngineHost host(&session, QString::fromLocal8Bit(argv[i + 1]));
            if (!host.listen()) {
                *exitCode = 1;
                return true;
            }
            *exitCode = eapp.exec();
            return true;
        }
        if (std::strcmp(argv[i], "--engine-selftest") == 0) {
            QCoreApplication eapp(argc, argv);
            eapp.setOrganizationName("BATorrent");
            eapp.setApplicationName("BATorrent");
            Logger::instance().init();
            IpcEngine engine(QCoreApplication::applicationFilePath());
            QObject::connect(&engine, &IpcEngine::engineStatusChanged, [](bool up) {
                qInfo() << "[selftest] engine status:" << (up ? "UP" : "DOWN");
            });
            if (!engine.start()) {
                qWarning() << "[selftest] engine failed to start";
                *exitCode = 1;
                return true;
            }
            QElapsedTimer pump;
            pump.start();
            while (pump.elapsed() < 3000)
                eapp.processEvents(QEventLoop::AllEvents, 100);
            qInfo() << "[selftest] connected. snapshot torrentCount =" << engine.torrentCount();
            if (engine.torrentCount() > 0) {
                const TorrentInfo t = engine.torrentAt(0);
                qInfo() << "[selftest] torrentAt(0):" << t.name << "progress" << t.progress
                        << "hash" << engine.torrentHashAt(0);
                qInfo() << "[selftest] filesAt(0) count =" << int(engine.filesAt(0).size());
            }
            qInfo() << "[selftest] pauseAll()";
            engine.pauseAll();
            qInfo() << "[selftest] round-trip OK";
            *exitCode = 0;
            return true;
        }
    }
    return false;
}

} // namespace EngineChild
