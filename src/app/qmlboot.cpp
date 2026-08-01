// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "app/qmlboot.h"

#include "app/appruntime.h"
#include "bridges/qmlthemebridge.h"
#include "services/platform/logger.h"
#include "services/security/secretstore.h"

#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QGuiApplication>
#include <QHash>
#include <QPointer>
#include <QMessageBox>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QSGRendererInterface>
#include <QSettings>
#include <QSplashScreen>
#include <QStandardPaths>
#include <QTimer>
#include <QWindow>
#include <memory>

namespace QmlBoot {

QUrl rootUrl()
{
    const QString devQmlDir = qEnvironmentVariable("BAT_QML_DIR");
    if (devQmlDir.isEmpty())
        return QUrl(QStringLiteral("qrc:/src/qml/Main.qml"));
    return QUrl::fromLocalFile(QDir(devQmlDir).filePath(QStringLiteral("Main.qml")));
}

LoadResult loadMain(QQmlApplicationEngine *engine, QApplication *app, QSplashScreen *splash)
{
    const QUrl url = rootUrl();
    bool mainObjectOk = false;
    QObject::connect(engine, &QQmlApplicationEngine::objectCreated, app,
                     [&mainObjectOk, url](QObject *obj, const QUrl &objUrl) {
        if (objUrl != url) return;
        mainObjectOk = (obj != nullptr);
    });
    engine->load(url);
    if (engine->rootObjects().isEmpty() || !mainObjectOk) {
        if (splash) splash->finish(nullptr);
        const QString logPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                                + QStringLiteral("/batorrent.log");
        AppRuntime::showQmlLoadFailure(logPath);
        return {};
    }
    LoadResult r;
    r.ok = true;
    r.window = qobject_cast<QQuickWindow *>(engine->rootObjects().first());
    return r;
}

void startHotReloadIfDev(QQmlApplicationEngine *engine, const QUrl &url, QApplication *app)
{
    const QString devQmlDir = qEnvironmentVariable("BAT_QML_DIR");
    if (devQmlDir.isEmpty()) return;

    auto snapshot = [](const QString &dir) {
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
    auto *poll = new QTimer(app);
    poll->setInterval(500);
    QObject::connect(poll, &QTimer::timeout, app,
                     [engine, url, devQmlDir, snapshot, lastSeen]() {
        const QHash<QString, qint64> now = snapshot(devQmlDir);
        if (now == *lastSeen) return;
        *lastSeen = now;
        const QList<QObject *> old = engine->rootObjects();
        engine->clearComponentCache();
        engine->load(url);
        for (QObject *o : old) {
            if (auto *w = qobject_cast<QQuickWindow *>(o)) w->close();
            o->deleteLater();
        }
        qInfo() << "[dev] QML hot-reloaded";
    });
    poll->start();
    qInfo() << "[dev] QML hot-reload watching" << devQmlDir;
}

void attachFirstFrameHealth(QQuickWindow *qw, QmlThemeBridge *themeBridge, QApplication *app)
{
    if (!qw) return;

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

    auto gotFrame = std::make_shared<bool>(false);
    auto frameConn = std::make_shared<QMetaObject::Connection>();
    *frameConn = QObject::connect(qw, &QQuickWindow::frameSwapped, themeBridge,
                     [themeBridge, gotFrame, frameConn]() {
        if (*gotFrame) return;
        *gotFrame = true;
        QObject::disconnect(*frameConn);
        themeBridge->markBootHealthy();
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
        st.setValue(QStringLiteral("graphicsApi"), 1);
        st.sync();
        if (qEnvironmentVariableIsSet("BAT_SMOKE_EXIT_ON_FRAME"))
            QCoreApplication::exit(3);
        QMessageBox::warning(nullptr, QStringLiteral("BATorrent"),
            QStringLiteral("Graphics failed to start. Switching to Software renderer — "
                           "please restart BATorrent.\n\n%1").arg(msg));
    });
    QTimer::singleShot(10000, app, [gotFrame]() {
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
}

void armMacDockReopen(QObject *rootObj, QApplication *app)
{
#ifdef Q_OS_MACOS
    auto dockArmed = std::make_shared<bool>(false);
    QTimer::singleShot(2500, app, [dockArmed]() { *dockArmed = true; });
    // QPointer, not a raw capture: the context object is `app`, which outlives
    // the QML root, so on shutdown (or any engine teardown) this still fires
    // with a dangling pointer and qobject_cast reads freed memory — SIGSEGV
    // inside setApplicationState, i.e. a crash while the app is being brought
    // to the front.
    QPointer<QObject> root(rootObj);
    QObject::connect(app, &QGuiApplication::applicationStateChanged, app,
                     [root, dockArmed](Qt::ApplicationState state) {
        if (!*dockArmed || state != Qt::ApplicationActive || root.isNull()) return;
        if (auto *w = qobject_cast<QWindow *>(root.data())) {
            if (w->visibility() == QWindow::Hidden) { w->show(); w->raise(); w->requestActivate(); }
        }
    });
#else
    Q_UNUSED(rootObj);
    Q_UNUSED(app);
#endif
}

} // namespace QmlBoot
