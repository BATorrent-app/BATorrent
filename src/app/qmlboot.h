// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef BATORRENT_QMLBOOT_H
#define BATORRENT_QMLBOOT_H

#include <QUrl>

class QApplication;
class QObject;
class QQmlApplicationEngine;
class QQuickWindow;
class QmlThemeBridge;
class QSplashScreen;

// Main.qml load, first-frame boot-healthy, hot-reload, scene-graph fallback.
namespace QmlBoot {

struct LoadResult {
    bool ok = false;
    QQuickWindow *window = nullptr;
};

LoadResult loadMain(QQmlApplicationEngine *engine, QApplication *app, QSplashScreen *splash);

void attachFirstFrameHealth(QQuickWindow *qw, QmlThemeBridge *themeBridge, QApplication *app);
void startHotReloadIfDev(QQmlApplicationEngine *engine, const QUrl &rootUrl, QApplication *app);
void armMacDockReopen(QObject *rootObj, QApplication *app);

QUrl rootUrl();

} // namespace QmlBoot

#endif
