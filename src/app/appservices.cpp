// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "app/appservices.h"

#include "app/gamecatalogseed.h"
#include "app/mediarefresh.h"
#include "app/vpnwiring.h"
#include "bridges/qmlposterbridge.h"
#include "services/discovery/addonmanager.h"
#include "services/discovery/discoveryservice.h"
#include "services/downloads/httpdownloadmanager.h"
#include "services/downloads/httpmergeengine.h"
#include "services/integrations/debridmanager.h"
#include "services/integrations/notifier.h"
#include "services/integrations/rssmanager.h"
#include "services/metadata/metadataresolver.h"
#include "services/platform/translator.h"
#include "services/platform/utils.h"
#include "services/security/blocklistupdater.h"
#include "services/vpn/vpnmanager.h"
#include "services/vpn/wgtunnelfactory.h"
#include "ipc/ipcengine.h"
#include "torrent/sessionmanager.h"
#include "webui/streamserver.h"

#include <QApplication>
#include <QCoreApplication>
#include <QFileInfo>
#include <QLocale>
#include <QSettings>

static void applyLanguage()
{
    QSettings s;
    int lang = 0;
    if (s.contains("language")) {
        lang = s.value("language").toInt();
    } else {
        const QString sys = QLocale::system().name().toLower();
        if      (sys.startsWith("pt")) lang = 1;
        else if (sys.startsWith("zh")) lang = 2;
        else if (sys.startsWith("ja")) lang = 3;
        else if (sys.startsWith("ru")) lang = 4;
        else if (sys.startsWith("es")) lang = 5;
        else if (sys.startsWith("de")) lang = 6;
        else if (sys.startsWith("uk")) lang = 7;
        else if (sys.startsWith("tr")) lang = 8;
        else                           lang = 0;
    }
    Translator::instance().setLanguage(static_cast<Translator::Language>(lang));
}

AppServices AppServices::create(QApplication &app)
{
    AppServices svc;

    {
        const bool wantIpc = QSettings().value(QStringLiteral("engineMode")).toString()
                             == QLatin1String("ipc");
        if (wantIpc) {
            svc.ipcEngine = new IpcEngine(QCoreApplication::applicationFilePath(), &app);
            if (!svc.ipcEngine->start()) {
                qWarning() << "[engine] IPC engine failed to start — falling back to in-process";
                svc.ipcEngine->deleteLater();
                svc.ipcEngine = nullptr;
            } else {
                qInfo() << "[engine] running split (engine in child process)";
            }
        }
        if (!svc.ipcEngine) svc.localSession = new SessionManager(&app);
    }
    IEngine *baseEng = svc.localSession ? static_cast<IEngine *>(svc.localSession)
                                        : static_cast<IEngine *>(svc.ipcEngine);

    svc.httpDownloads = new HttpDownloadManager(&app);
    svc.eng = new HttpMergeEngine(baseEng, svc.httpDownloads, &app);

    svc.vpnManager = new VpnManager(makeWgTunnel(&app), &app);
    VpnWiring::wire(&app, svc.vpnManager, svc.eng);

    svc.resolver = new MetadataResolver(&app);
    svc.posterModel = new QmlPosterModel(svc.eng, svc.resolver, &app);
    svc.themeBridge = new QmlThemeBridge(&app);
    svc.sessionBridge = new QmlSessionBridge(svc.eng, svc.resolver, &app);
    svc.sessionBridge->setHttpDownloads(svc.httpDownloads);
    svc.httpDownloads->setDefaultDir(svc.sessionBridge->defaultSavePath());

    auto *streamServer = new StreamServer(svc.eng, &app);
    if (streamServer->start()) {
        svc.sessionBridge->setStreamPort(streamServer->port());
        qInfo() << "[stream] listening on 127.0.0.1:" << streamServer->port();
    } else {
        qWarning() << "[stream] failed to start local stream server";
    }

    RssManager::instance().setSession(svc.eng, svc.sessionBridge->defaultSavePath());
    svc.rssBridge = new QmlRssBridge(&app);
    svc.settingsBridge = new QmlSettingsBridge(svc.localSession, svc.eng, &app);

    auto *blocklist = new BlocklistUpdater(&app);
    QObject::connect(blocklist, &BlocklistUpdater::ready, &app,
                     [eng = svc.eng](const QString &path, int) { eng->applySetting("autoBlocklistFile", path); });
    QObject::connect(svc.settingsBridge, &QmlSettingsBridge::blockBadPeersToggled, &app,
                     [eng = svc.eng, blocklist](bool on) {
        if (on) blocklist->update();
        else    eng->applySetting("autoBlocklistFile", QString());
    });
    if (QSettings().value("blockBadPeers", false).toBool()) {
        if (QFileInfo::exists(BlocklistUpdater::cachePath()))
            svc.eng->applySetting("autoBlocklistFile", BlocklistUpdater::cachePath());
        if (BlocklistUpdater::cacheStale())
            blocklist->update();
    }

    svc.addonBridge = new QmlAddonBridge(&app);
    svc.searchBridge = new QmlSearchBridge(svc.eng, &app);
    svc.searchBridge->setResolver(svc.resolver);
    svc.searchBridge->setHttpDownloads(svc.httpDownloads);
    svc.discoveryService = new DiscoveryService(&app);
    svc.searchBridge->setDiscovery(svc.discoveryService);

#ifndef BAT_STORE_BUILD
    GameCatalogSeed::apply();
#endif

    svc.logBridge = new QmlLogBridge(&app);
    svc.subtitleBridge = new QmlSubtitleBridge(svc.eng, &app);
    svc.subtitleBridge->setResolver(svc.resolver);
    svc.pairingBridge = new QmlPairingBridge(&app);
    svc.debrid = new DebridManager(&app);
    svc.notificationBridge = new QmlNotificationBridge(&app);
    svc.notificationBridge->setSession(svc.eng);

    QObject::connect(svc.eng, &IEngine::torrentFinished,
                     svc.notificationBridge, &QmlNotificationBridge::onTorrentFinished);
    auto *telegram = new TelegramNotifier(&app);
    QObject::connect(svc.eng, &IEngine::torrentFinished,
                     telegram, &TelegramNotifier::onTorrentFinished);
    QObject::connect(svc.eng, &IEngine::torrentError,
                     svc.notificationBridge, &QmlNotificationBridge::onTorrentError);
    QObject::connect(svc.eng, &IEngine::killSwitchTriggered,
                     svc.notificationBridge, &QmlNotificationBridge::onKillSwitchTriggered);
    QObject::connect(svc.eng, &IEngine::suspiciousFilesDetected,
                     svc.notificationBridge, &QmlNotificationBridge::onSuspiciousFilesDetected);
    QObject::connect(svc.eng, &IEngine::killSwitchTriggered,
                     telegram, &TelegramNotifier::onKillSwitchTriggered);
    QObject::connect(svc.eng, &IEngine::torrentError,
                     telegram, &TelegramNotifier::onTorrentError);

    if (svc.ipcEngine) {
        QObject::connect(svc.ipcEngine, &IpcEngine::engineStatusChanged, svc.notificationBridge,
                         [nb = svc.notificationBridge](bool up) {
            nb->onTorrentError(up ? QStringLiteral("Torrent engine reconnected")
                                  : QStringLiteral("Torrent engine restarting…"));
        });
    }
    QObject::connect(&RssManager::instance(), &RssManager::itemAutoDownloaded,
                     svc.notificationBridge, &QmlNotificationBridge::onRssAutoDownloaded);
    QObject::connect(&RssManager::instance(), &RssManager::itemAutoDownloaded,
                     telegram, &TelegramNotifier::onRssAutoDownloaded);
    svc.settingsBridge->setTelegramNotifier(telegram);

    MediaRefresh::install(&app, svc.eng);

    svc.discordBridge = new DiscordRpcBridge(svc.eng, &app);
    QObject::connect(svc.eng, &IEngine::torrentsUpdated,
                     svc.discordBridge, &DiscordRpcBridge::refresh);
#ifndef BAT_STORE_BUILD
    svc.updaterBridge = new QmlUpdaterBridge(&app);
#endif
    QObject::connect(svc.eng, &IEngine::torrentsUpdated,
                     svc.sessionBridge, &QmlSessionBridge::emitStats);
    QObject::connect(svc.resolver, &MetadataResolver::metadataReady,
                     svc.sessionBridge, &QmlSessionBridge::emitStats);
    QObject::connect(svc.eng, &IEngine::torrentsUpdated,
                     svc.posterModel, &QmlPosterModel::refresh);
    QObject::connect(svc.eng, &IEngine::torrentRemoved,
                     svc.posterModel, &QmlPosterModel::removeRow);
    QObject::connect(svc.eng, &IEngine::torrentRemoved,
                     svc.sessionBridge, &QmlSessionBridge::onTorrentRemoved);
    QObject::connect(svc.resolver, &MetadataResolver::metadataReady,
                     svc.posterModel, &QmlPosterModel::posterResolved);
    QObject::connect(svc.sessionBridge, &QmlSessionBridge::queueRefreshNeeded,
                     svc.posterModel, &QmlPosterModel::refreshFull);
    QObject::connect(svc.sessionBridge, &QmlSessionBridge::queueMoved,
                     svc.posterModel, &QmlPosterModel::moveRow);

    auto handleAdded = [resolver = svc.resolver, notificationBridge = svc.notificationBridge,
                        eng = svc.eng](
            int index, const QString &hash, const QString &name, qint64 totalSize,
            const QStringList &fileNames, const QString &hintTitle, int hintType) {
        if (!hash.isEmpty()) {
            if (!hintTitle.isEmpty() && hintType >= 0)
                resolver->resolveManual(hash, hintTitle, static_cast<ContentType>(hintType));
            else
                resolver->resolve(hash, name, fileNames);
        }
        notificationBridge->onTorrentAdded(
            totalSize > 0 ? name + " · " + formatSize(totalSize) : name);
        if (AddonManager::instance().autoTrackersEnabled())
            for (const QString &tr : AddonManager::instance().trackerList())
                eng->addTracker(index, tr);
    };
    if (svc.localSession) {
        QObject::connect(svc.localSession, &SessionManager::torrentAdded,
                         &app, [localSession = svc.localSession, handleAdded](int index) {
            const auto info = localSession->torrentAt(index);
            const QString hash = localSession->torrentHashAt(index);
            const auto hint = localSession->takeCoverHint(hash);
            handleAdded(index, hash, info.name, info.totalSize,
                        localSession->torrentFileNames(index), hint.title, hint.type);
        });
    }
    QObject::connect(svc.eng, &IEngine::torrentAddedInfo, &app,
                     [handleAdded](int index, const QString &hash, const QString &name,
                                   qint64 totalSize, const QStringList &fileNames,
                                   const QString &hintTitle, int hintType) {
        handleAdded(index, hash, name, totalSize, fileNames, hintTitle, hintType);
    });
    AddonManager::instance().fetchTrackerList();

    {
        QStringList hashes, names;
        for (int i = 0; i < svc.eng->torrentCount(); ++i) {
            QString h = svc.eng->torrentHashAt(i);
            if (!h.isEmpty() && !svc.resolver->hasCached(h)) {
                hashes << h;
                names << svc.eng->torrentAt(i).name;
            }
        }
        if (!hashes.isEmpty())
            svc.resolver->batchResolve(hashes, names);
    }

    svc.filterProxy = new QmlTorrentFilterProxy(&app);
    svc.filterProxy->setSourceModel(svc.posterModel);

    applyLanguage();
    svc.i18nBridge = new QmlI18nBridge(&app);

#ifndef Q_OS_MACOS
    app.setWindowIcon(svc.themeBridge->trayIcon());
#endif
    svc.themeBridge->applySavedAppIcon();

    return svc;
}
