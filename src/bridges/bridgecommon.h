#ifndef BRIDGECOMMON_H
#define BRIDGECOMMON_H

#include <QAbstractListModel>
#include <QColor>
#include <QIcon>
#include <QPixmap>
#include <QSortFilterProxyModel>
#include <QString>
#include <QSet>
#include <QTimer>
#include <QVariantList>
#include <QVector>
#include <QSettings>

#include "services/discovery/addonmanager.h"   // CatalogItem / StreamResult / TorrentSearchResult
#include "services/discovery/gamesourcemanager.h"   // GameDownload
#include "services/platform/translator.h"

// forward declarations used across the bridge headers
class SessionManager;
class IEngine;
class MetadataResolver;
class DiscoveryService;
class GeoIpResolver;
class TelegramNotifier;
class WebServer;
class QWindow;
class QEvent;
class SubtitleSearch;
class DiscordRPC;
class Updater;

class QmlPosterModel;
class QmlTorrentFilterProxy;
class QmlSessionBridge;
class QmlThemeBridge;
class QmlRssBridge;
class QmlSearchBridge;
class QmlAddonBridge;
class QmlPairingBridge;
class QmlSubtitleBridge;
class QmlLogBridge;
class QmlSettingsBridge;
class QmlI18nBridge;
class QmlNotificationBridge;
class DiscordRpcBridge;
class QmlUpdaterBridge;

#endif // BRIDGECOMMON_H
