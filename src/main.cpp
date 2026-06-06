// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include <QApplication>
#include <QIcon>
#include <QStringList>
#include <QFont>
#include <QFontDatabase>
#include <QLocalServer>
#include <QLocalSocket>
#include <QWindow>
#include <QSettings>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickImageProvider>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QSGRendererInterface>
#include <QLocale>
#include "app/metadataresolver.h"
#include "app/discoveryservice.h"
#include "app/translator.h"
#include "gui/qmlposterbridge.h"
#include "app/gamesourcemanager.h"
#include "app/rssmanager.h"
#include "app/addonmanager.h"
#include "app/notifier.h"
#include "torrent/sessionmanager.h"
#include "app/secretstore.h"
#include "app/logger.h"
#include "app/utils.h"

// Serves the app logo recolored for the OS scheme to QML (the system tray
// icon.source wants a URL, not a QIcon). URL id is "light"/"dark"; the body
// is swapped to dark text for the "dark"-id (light-background) request.
class AppLogoImageProvider : public QQuickImageProvider
{
public:
    AppLogoImageProvider() : QQuickImageProvider(QQuickImageProvider::Pixmap) {}
    QPixmap requestPixmap(const QString &id, QSize *size, const QSize &requested) override
    {
        const int sz = requested.width() > 0 ? requested.width() : 256;
        const bool darkBody = id.startsWith("dark");   // dark logo for light OS bar
        QPixmap pm = QmlThemeBridge::renderLogo(darkBody, sz, 1.0);
        if (size) *size = pm.size();
        return pm;
    }
};

static const QString kServerName = QStringLiteral("BATorrent-SingleInstance");

static QString collectArgs(const QStringList &args)
{
    QStringList relevant;
    for (int i = 1; i < args.size(); ++i) {
        const QString &a = args[i];
        if (a.endsWith(".torrent") || a.startsWith("magnet:"))
            relevant << a;
    }
    return relevant.join('\n');
}

static bool sendToRunningInstance(const QString &message)
{
    QLocalSocket socket;
    socket.connectToServer(kServerName);
    if (!socket.waitForConnected(1000))
        return false;
    socket.write(message.toUtf8());
    socket.waitForBytesWritten(1000);
    socket.disconnectFromServer();
    return true;
}

// Maps Qt's runtime log categories (QtDebugMsg / QtInfoMsg / etc) to our
// internal Logger levels so every existing qDebug() / qWarning() in the
// codebase ends up in the same file the user can export.
static void qtMessageHandler(QtMsgType type, const QMessageLogContext &ctx,
                             const QString &msg)
{
    Logger::Level lvl = Logger::Debug;
    switch (type) {
    case QtDebugMsg:    lvl = Logger::Debug;    break;
    case QtInfoMsg:     lvl = Logger::Info;     break;
    case QtWarningMsg:  lvl = Logger::Warning;  break;
    case QtCriticalMsg: lvl = Logger::Error;    break;
    case QtFatalMsg:    lvl = Logger::Critical; break;
    }
    QString prefix;
    if (ctx.category && qstrcmp(ctx.category, "default") != 0)
        prefix = QStringLiteral("[%1] ").arg(QString::fromUtf8(ctx.category));
    Logger::instance().log(lvl, prefix + msg);
    // Keep stderr live for `--debug` console use.
    fprintf(stderr, "%s\n", qPrintable(prefix + msg));
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    // Set the org name so default-constructed QSettings() resolves to the same
    // store as the explicit QSettings("BATorrent","BATorrent") used elsewhere —
    // otherwise the QML UI's settings (language, theme, …) fork into a separate
    // store the legacy UI can't see.
    app.setOrganizationName("BATorrent");
    app.setApplicationName("BATorrent");
    app.setApplicationVersion(APP_VERSION);
    app.setWindowIcon(QIcon(":/images/logo1.png"));
    app.setQuitOnLastWindowClosed(false); // keep running in tray when window is closed

    // CLI flag: --debug / -d forces verbose logging for this session (without
    // mutating the persisted setting). Comes before init() so the lowered
    // level takes effect immediately.
    const bool debugFlag = app.arguments().contains("--debug")
                        || app.arguments().contains("-d");
    Logger::instance().init();
    if (debugFlag) {
        Logger::instance().setLevel(Logger::Trace);
        // Make Qt dump the scene-graph/RHI init (which GPU backend it picked and
        // why it fell back, if it did) — captured into our log via the handler.
        qputenv("QSG_INFO", "1");
    }
    qInstallMessageHandler(qtMessageHandler);

    // Speed unit display preference (0 = bytes/sec, 1 = bits/sec). Read once
    // here so formatSpeed() doesn't hit QSettings on every call (UI refresh
    // rate would otherwise translate to ~10 QSettings opens/sec).
    setSpeedUnit(QSettings("BATorrent", "BATorrent").value("speedUnit", 0).toInt());

    // One-time migration of plaintext secrets from QSettings into the OS
    // keyring (no-op on subsequent runs and on builds without QtKeychain).
    SecretStore::instance().migrateFromSettings({
        "proxyPass", "plexToken", "jellyfinApiKey"
    });

    // webUiPasswordHash used to live in the keychain, but on unsigned macOS
    // builds reading it at every cold start pops a login-keychain prompt. It's
    // only a SHA-256 hash (not a usable credential), so it now lives in
    // QSettings; pull any existing one back out of the keychain (one-time — the
    // plaintext webUiPassword stays in the keychain). Reading a missing item
    // doesn't prompt, so users who never enabled the WebUI see nothing.
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

    // Single-instance check: if another instance is running, forward args and quit
    QString argsPayload = collectArgs(app.arguments());
    if (sendToRunningInstance(argsPayload))
        return 0;

    // Load Inter font family
    QFontDatabase::addApplicationFont(":/fonts/Inter-Regular.ttf");
    QFontDatabase::addApplicationFont(":/fonts/Inter-Medium.ttf");
    QFontDatabase::addApplicationFont(":/fonts/Inter-SemiBold.ttf");
    QFontDatabase::addApplicationFont(":/fonts/Inter-Bold.ttf");
    QFontDatabase::addApplicationFont(":/fonts/NewRocker-Regular.ttf");   // brand wordmark

    QFont defaultFont("Inter", 10);
    defaultFont.setStyleStrategy(QFont::PreferAntialias);
    app.setFont(defaultFont);

    // Use the Qt Quick default (distance-field) text rendering on every
    // platform so Windows matches macOS. Native Windows rendering was crisper
    // but rendered the Inter weights noticeably thinner than the Mac reference.

    QQuickStyle::setStyle("Basic");
    {

        SessionManager session;

        auto *resolver = new MetadataResolver(&app);
        auto *posterModel = new QmlPosterModel(&session, resolver, &app);
        auto *themeBridge = new QmlThemeBridge(&app);
        auto *sessionBridge = new QmlSessionBridge(&session, resolver, &app);
        RssManager::instance().setSession(&session, sessionBridge->defaultSavePath());
        auto *rssBridge = new QmlRssBridge(&app);
        auto *settingsBridge = new QmlSettingsBridge(&session, &app);
        auto *addonBridge = new QmlAddonBridge(&app);
        auto *searchBridge = new QmlSearchBridge(&session, &app);
        auto *discoveryService = new DiscoveryService(&app);

#ifndef BAT_STORE_BUILD
        // Seed a default community game catalog once so "Jogos" works out of the
        // box (open → search → download). Store builds stay clean. The user can
        // remove it; the seed flag keeps it from coming back.
        {
            QSettings gs;
            if (!gs.value(QStringLiteral("gameSourcesSeeded"), false).toBool()) {
                gs.setValue(QStringLiteral("gameSourcesSeeded"), true);
                auto &gsm = GameSourceManager::instance();
                if (gsm.sources().isEmpty())
                    gsm.addSource(QStringLiteral("RuTracker (Combined)"),
                                  QStringLiteral("https://raw.githubusercontent.com/Jdjsjjqq/rutracker-hydra/main/combined_torrents.json"));
            }
        }
#endif
        auto *logBridge = new QmlLogBridge(&app);
        auto *pairingBridge = new QmlPairingBridge(&app);
        auto *notificationBridge = new QmlNotificationBridge(&app);
        QObject::connect(&session, &SessionManager::torrentFinished,
                         notificationBridge, &QmlNotificationBridge::onTorrentFinished);
        QObject::connect(&session, &SessionManager::torrentError,
                         notificationBridge, &QmlNotificationBridge::onTorrentError);
        QObject::connect(&session, &SessionManager::killSwitchTriggered,
                         notificationBridge, &QmlNotificationBridge::onKillSwitchTriggered);
        QObject::connect(&RssManager::instance(), &RssManager::itemAutoDownloaded,
                         notificationBridge, &QmlNotificationBridge::onRssAutoDownloaded);
        // Telegram webhook notifications (same event surfaces as the toasts above).
        auto *telegram = new TelegramNotifier(&app);
        QObject::connect(&session, &SessionManager::torrentFinished,
                         telegram, &TelegramNotifier::onTorrentFinished);
        QObject::connect(&session, &SessionManager::killSwitchTriggered,
                         telegram, &TelegramNotifier::onKillSwitchTriggered);
        QObject::connect(&session, &SessionManager::torrentError,
                         telegram, &TelegramNotifier::onTorrentError);
        QObject::connect(&RssManager::instance(), &RssManager::itemAutoDownloaded,
                         telegram, &TelegramNotifier::onRssAutoDownloaded);
        settingsBridge->setTelegramNotifier(telegram);

        // Media-server library refresh: ping Plex/Jellyfin when a download finishes.
        auto *mediaNam = new QNetworkAccessManager(&app);
        QObject::connect(&session, &SessionManager::torrentFinished, &app, [mediaNam](const QString &, const QString &) {
            QSettings st;
            if (st.value("plexEnabled", false).toBool()) {
                const QString url = st.value("plexUrl").toString();
                const QString token = SecretStore::instance().get("plexToken");
                if (!url.isEmpty() && !token.isEmpty()) {
                    QNetworkRequest req(QUrl(url + "/library/sections/all/refresh?X-Plex-Token=" + token));
                    req.setHeader(QNetworkRequest::UserAgentHeader, "BATorrent");
                    auto *r = mediaNam->get(req);
                    QObject::connect(r, &QNetworkReply::finished, r, &QNetworkReply::deleteLater);
                }
            }
            if (st.value("jellyfinEnabled", false).toBool()) {
                const QString url = st.value("jellyfinUrl").toString();
                const QString key = SecretStore::instance().get("jellyfinApiKey");
                if (!url.isEmpty() && !key.isEmpty()) {
                    QNetworkRequest req(QUrl(url + "/Library/Refresh?api_key=" + key));
                    req.setHeader(QNetworkRequest::UserAgentHeader, "BATorrent");
                    auto *r = mediaNam->post(req, QByteArray());
                    QObject::connect(r, &QNetworkReply::finished, r, &QNetworkReply::deleteLater);
                }
            }
        });

        auto *discordBridge = new DiscordRpcBridge(&session, &app);
        QObject::connect(&session, &SessionManager::torrentsUpdated,
                         discordBridge, &DiscordRpcBridge::refresh);
#ifndef BAT_STORE_BUILD
        auto *updaterBridge = new QmlUpdaterBridge(&app);
#endif
        QObject::connect(&session, &SessionManager::torrentsUpdated,
                         sessionBridge, &QmlSessionBridge::emitStats);
        QObject::connect(resolver, &MetadataResolver::metadataReady,
                         sessionBridge, &QmlSessionBridge::emitStats);

        QObject::connect(&session, &SessionManager::torrentsUpdated,
                         posterModel, &QmlPosterModel::refresh);
        // Index-aware removal — beginRemoveRows for the exact row instead of a
        // full model reset, so the grid doesn't flash and jump to the top.
        QObject::connect(&session, &SessionManager::torrentRemoved,
                         posterModel, &QmlPosterModel::removeRow);
        // Keep the bridge's stored selection indices valid across a removal.
        QObject::connect(&session, &SessionManager::torrentRemoved,
                         sessionBridge, &QmlSessionBridge::onTorrentRemoved);
        // A resolved poster only touches one row's poster/title roles.
        QObject::connect(resolver, &MetadataResolver::metadataReady,
                         posterModel, &QmlPosterModel::posterResolved);
        // Explicit edits (rename/category/restore/import) need every role.
        QObject::connect(sessionBridge, &QmlSessionBridge::queueRefreshNeeded,
                         posterModel, &QmlPosterModel::refreshFull);
        QObject::connect(sessionBridge, &QmlSessionBridge::queueMoved,
                         posterModel, &QmlPosterModel::moveRow);
        QObject::connect(&session, &SessionManager::torrentAdded,
                         &app, [&session, resolver, notificationBridge](int index) {
            const auto info = session.torrentAt(index);
            QString hash = session.torrentHashAt(index);
            if (!hash.isEmpty()) {
                // A catalog add carries a clean title + known type (game catalog →
                // Game, Stremio → Movie/Series). Query the API directly with it
                // instead of guessing from the messy torrent/metadata name, which
                // mismatched (e.g. GoW Ragnarök → wrong game).
                const auto hint = session.takeCoverHint(hash);
                if (!hint.title.isEmpty() && hint.type >= 0)
                    resolver->resolveManual(hash, hint.title, static_cast<ContentType>(hint.type));
                else
                    resolver->resolve(hash, info.name, session.torrentFileNames(index));
            }
            // Toast on user-initiated adds. Resume-loaded torrents don't reach
            // here: loadResumeData() runs in the SessionManager ctor, before
            // this connection exists.
            notificationBridge->onTorrentAdded(
                info.totalSize > 0 ? info.name + " · " + formatSize(info.totalSize) : info.name);
            // auto-add the public tracker list to each new torrent (if enabled)
            if (AddonManager::instance().autoTrackersEnabled())
                for (const QString &tr : AddonManager::instance().trackerList())
                    session.addTracker(index, tr);
        });
        AddonManager::instance().fetchTrackerList();   // refresh the list on startup

        {
            QStringList hashes, names;
            for (int i = 0; i < session.torrentCount(); ++i) {
                QString h = session.torrentHashAt(i);
                if (!h.isEmpty() && !resolver->hasCached(h)) {
                    hashes << h;
                    names << session.torrentAt(i).name;
                }
            }
            if (!hashes.isEmpty())
                resolver->batchResolve(hashes, names);
        }

        auto *filterProxy = new QmlTorrentFilterProxy(&app);
        filterProxy->setSourceModel(posterModel);

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
                else                           lang = 0;
            }
            Translator::instance().setLanguage(static_cast<Translator::Language>(lang));
        }
        auto *i18nBridge = new QmlI18nBridge(&app);

        // OS-scheme-aware app/window icon (so the white logo isn't invisible on
        // a light Windows taskbar). The bridge keeps it live on scheme changes.
        app.setWindowIcon(themeBridge->trayIcon());

        QQmlApplicationEngine engine;
        engine.addImageProvider(QStringLiteral("applogo"), new AppLogoImageProvider());
        engine.rootContext()->setContextProperty("torrentModel", filterProxy);
        engine.rootContext()->setContextProperty("torrentFilter", filterProxy);
        engine.rootContext()->setContextProperty("themeBridge", themeBridge);
        engine.rootContext()->setContextProperty("session", sessionBridge);
        engine.rootContext()->setContextProperty("rss", rssBridge);
        engine.rootContext()->setContextProperty("settings", settingsBridge);
        engine.rootContext()->setContextProperty("addons", addonBridge);
        engine.rootContext()->setContextProperty("search", searchBridge);
        engine.rootContext()->setContextProperty("discovery", discoveryService);
        engine.rootContext()->setContextProperty("logs", logBridge);
        engine.rootContext()->setContextProperty("pairing", pairingBridge);
        engine.rootContext()->setContextProperty("notifications", notificationBridge);
        engine.rootContext()->setContextProperty("i18n", i18nBridge);
#ifndef BAT_STORE_BUILD
        engine.rootContext()->setContextProperty("updater", updaterBridge);
#else
        engine.rootContext()->setContextProperty("updater", nullptr);
#endif
        engine.load(QUrl("qrc:/src/qml/Main.qml"));
        if (engine.rootObjects().isEmpty())
            return -1;

        // Record which scene-graph backend is actually in use. If a machine
        // falls back to the Software renderer (no GPU), the whole UI stutters
        // like a game compiling shaders even on a fast card — this line in the
        // log tells us that immediately instead of guessing.
        if (auto *qw = qobject_cast<QQuickWindow *>(engine.rootObjects().first())) {
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
            Logger::instance().log(Logger::Info,
                QStringLiteral("[render] scene graph backend: %1").arg(QLatin1String(backend)));
        }
#ifndef BAT_STORE_BUILD
        updaterBridge->check(true);   // silent check on startup
#endif

        // Single-instance server (the legacy path had this; the QML path didn't,
        // so launching again opened a second copy). Listen for forwarded args
        // from a second launch, raise our window, and add any .torrent/magnet.
        QObject *rootObj = engine.rootObjects().first();
        QLocalServer::removeServer(kServerName);
        auto *instanceServer = new QLocalServer(&app);
        instanceServer->listen(kServerName);
        QObject::connect(instanceServer, &QLocalServer::newConnection, &app,
                         [instanceServer, rootObj, sessionBridge]() {
            QLocalSocket *client = instanceServer->nextPendingConnection();
            if (auto *w = qobject_cast<QWindow *>(rootObj)) {
                w->show(); w->raise(); w->requestActivate();
            }
            QObject::connect(client, &QLocalSocket::readyRead, client, [client, sessionBridge]() {
                const QStringList lines = QString::fromUtf8(client->readAll()).split('\n', Qt::SkipEmptyParts);
                for (const QString &line : lines) {
                    if (line.endsWith(".torrent")) sessionBridge->requestAddTorrentFile(line);
                    else if (line.startsWith("magnet:")) sessionBridge->addMagnetUri(line);
                }
                client->deleteLater();
            });
        });

        // First-instance CLI args (.torrent / magnet passed on launch)
        for (int i = 1; i < app.arguments().size(); ++i) {
            const QString &arg = app.arguments().at(i);
            if (arg.endsWith(".torrent")) sessionBridge->requestAddTorrentFile(arg);
            else if (arg.startsWith("magnet:")) sessionBridge->addMagnetUri(arg);
        }

        return app.exec();
    }
}
