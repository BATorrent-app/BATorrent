// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef BATORRENT_APPSERVICES_H
#define BATORRENT_APPSERVICES_H

// Complete types: main/callers invoke methods on session + updater pointers.
#include "bridges/session/qmlsessionbridge.h"
#include "bridges/qmlupdaterbridge.h"

class QApplication;
class SessionManager;
class IpcEngine;
class IEngine;
class HttpDownloadManager;
class VpnManager;
class MetadataResolver;
class QmlPosterModel;
class QmlThemeBridge;
class QmlRssBridge;
class QmlSettingsBridge;
class QmlAddonBridge;
class QmlSearchBridge;
class DiscoveryService;
class QmlLogBridge;
class QmlSubtitleBridge;
class QmlPairingBridge;
class DebridManager;
class QmlNotificationBridge;
class DiscordRpcBridge;
class QmlI18nBridge;
class QmlTorrentFilterProxy;

// Engine selection + bridge graph + signal wiring that feeds QML context props.
struct AppServices {
    SessionManager *localSession = nullptr;
    IpcEngine *ipcEngine = nullptr;
    IEngine *eng = nullptr;
    HttpDownloadManager *httpDownloads = nullptr;
    VpnManager *vpnManager = nullptr;
    MetadataResolver *resolver = nullptr;
    QmlPosterModel *posterModel = nullptr;
    QmlThemeBridge *themeBridge = nullptr;
    QmlSessionBridge *sessionBridge = nullptr;
    QmlRssBridge *rssBridge = nullptr;
    QmlSettingsBridge *settingsBridge = nullptr;
    QmlAddonBridge *addonBridge = nullptr;
    QmlSearchBridge *searchBridge = nullptr;
    DiscoveryService *discoveryService = nullptr;
    QmlLogBridge *logBridge = nullptr;
    QmlSubtitleBridge *subtitleBridge = nullptr;
    QmlPairingBridge *pairingBridge = nullptr;
    DebridManager *debrid = nullptr;
    QmlNotificationBridge *notificationBridge = nullptr;
    DiscordRpcBridge *discordBridge = nullptr;
    QmlUpdaterBridge *updaterBridge = nullptr;
    QmlI18nBridge *i18nBridge = nullptr;
    QmlTorrentFilterProxy *filterProxy = nullptr;

    static AppServices create(QApplication &app);
};

#endif
