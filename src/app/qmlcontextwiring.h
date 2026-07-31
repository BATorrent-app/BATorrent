// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef BATORRENT_QMLCONTEXTWIRING_H
#define BATORRENT_QMLCONTEXTWIRING_H

class QQmlContext;
class QmlTorrentFilterProxy;
class QmlThemeBridge;
class QmlSessionBridge;
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
class QmlI18nBridge;
class QmlUpdaterBridge;
class VpnManager;

// Objects exposed to QML — names must stay stable (session, settings, …).
struct QmlContextObjects {
    QmlTorrentFilterProxy *torrentFilter = nullptr;
    QmlThemeBridge *themeBridge = nullptr;
    QmlSessionBridge *session = nullptr;
    QmlRssBridge *rss = nullptr;
    QmlSettingsBridge *settings = nullptr;
    QmlAddonBridge *addons = nullptr;
    QmlSearchBridge *search = nullptr;
    DiscoveryService *discovery = nullptr;
    QmlLogBridge *logs = nullptr;
    QmlSubtitleBridge *subsearch = nullptr;
    QmlPairingBridge *pairing = nullptr;
    DebridManager *debrid = nullptr;
    QmlNotificationBridge *notifications = nullptr;
    QmlI18nBridge *i18n = nullptr;
    QmlUpdaterBridge *updater = nullptr; // null in store builds
    VpnManager *vpn = nullptr;           // null in store builds
};

namespace QmlContextWiring {

void registerProperties(QQmlContext *ctx, const QmlContextObjects &o);

} // namespace QmlContextWiring

#endif
