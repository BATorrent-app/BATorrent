// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "bridges/qmlposterbridge.h"
#include "torrent/sessionmanager.h"
#include "services/metadata/metadataresolver.h"
#include "services/discovery/discoveryservice.h"
#include "services/security/defender.h"
#include "services/metadata/nameparser.h"
#include "services/integrations/rssmanager.h"
#include "services/discovery/addonmanager.h"
#include "services/platform/logger.h"
#include "services/platform/qrcodegen.h"
#include "services/platform/utils.h"
#include "services/platform/translator.h"
#include "services/integrations/geoip.h"
#include "services/integrations/discordrpc.h"
#include "services/integrations/updater.h"
#include "services/integrations/notifier.h"
#include "services/security/passwordhash.h"
#include "services/security/secretstore.h"
#include "webui/webserver.h"
#include <QNetworkAccessManager>
#include <QNetworkProxy>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QDateTime>

#include <QNetworkInterface>

#include <QClipboard>
#include <QDesktopServices>
#include <QDir>
#include <QDirIterator>
#include <QProcess>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QApplication>
#include <QWindow>
#include <QEvent>
#ifdef Q_OS_WIN
#  include <windows.h>
#  include <dwmapi.h>
#  ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#    define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#  endif
#endif
#include <QCoreApplication>
#include <QStyleHints>
#include <QPainter>
#include <QSvgRenderer>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <cstring>
#include <algorithm>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QSettings>
#include <QSet>
#include <QStandardPaths>
#include <QUrl>

#include <libtorrent/torrent_info.hpp>
#include <libtorrent/file_storage.hpp>
#include <libtorrent/create_torrent.hpp>
#include <libtorrent/bencode.hpp>
#include <libtorrent/version.hpp>
#include <openssl/opensslv.h>
#include <boost/version.hpp>
#include <memory>
#include <sstream>

QmlSettingsBridge::QmlSettingsBridge(SessionManager *session, IEngine *engine, QObject *parent)
    : QObject(parent), m_session(session), m_engine(engine) { applyWebUi(); }

void QmlSettingsBridge::applyWebUi()
{
    QSettings st;
    if (m_webServer) { m_webServer->stop(); m_webServer->deleteLater(); m_webServer = nullptr; }
    if (!st.value("webUiEnabled", false).toBool()) return;
    if (!m_engine) return;
    m_webServer = new WebServer(m_engine, this);
    connect(m_webServer, &WebServer::passwordHashUpgraded, this,
            [](const QString &h) { QSettings().setValue("webUiPasswordHash", h); });
    const QString user = st.value("webUiUser", "admin").toString();
    const QString passHash = st.value("webUiPasswordHash").toString();   // hash, not a secret → QSettings (no keychain prompt at boot)
    const bool hasAuth = !user.isEmpty() && !passHash.isEmpty();
    if (hasAuth)
        m_webServer->setCredentials(user, passHash);
    // Never expose an unauthenticated control surface to the LAN: without
    // credentials, force localhost-only even if remote access was requested.
    bool remote = st.value("webUiRemoteAccess", false).toBool() && hasAuth;
    m_webServer->start(quint16(st.value("webUiPort", 8080).toInt()), remote);
}

QString QmlSettingsBridge::enablePairing()
{
    // Readable strong password (no 0/O/1/I/l ambiguity — it gets typed on a phone).
    static const QString alphabet =
        QStringLiteral("ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnpqrstuvwxyz23456789");
    QString pw;
    for (int i = 0; i < 14; ++i)
        pw.append(alphabet.at(QRandomGenerator::global()->bounded(alphabet.size())));

    QSettings st;
    st.setValue("webUiEnabled", true);
    st.setValue("webUiRemoteAccess", true);
    if (st.value("webUiUser").toString().isEmpty())
        st.setValue("webUiUser", QStringLiteral("admin"));
    // Plaintext → keychain (the pairing screen re-displays it). Hash → QSettings
    // (the server only ever needs the hash; keeping it out of the keychain avoids
    // a login-keychain prompt on every cold start of the unsigned macOS build).
    SecretStore::instance().set("webUiPassword", pw);
    st.setValue("webUiPasswordHash", PasswordHash::hash(pw));
    applyWebUi();
    emit changed();
    return pw;
}

void QmlSettingsBridge::disablePairing()
{
    QSettings().setValue("webUiRemoteAccess", false);   // back to localhost-only; WebUI stays on locally
    applyWebUi();
    emit changed();
}

bool QmlSettingsBridge::pairingActive() const
{
    QSettings st;
    return st.value("webUiEnabled", false).toBool()
        && st.value("webUiRemoteAccess", false).toBool()
        && !st.value("webUiPasswordHash").toString().isEmpty();
}

QString QmlSettingsBridge::webUiUser() const
{
    return QSettings().value("webUiUser", QStringLiteral("admin")).toString();
}

QString QmlSettingsBridge::webUiPassword() const
{
    return SecretStore::instance().get("webUiPassword");
}

QVariant QmlSettingsBridge::get(const QString &key) const
{
    // Engine mode is a meta-setting (which engine to run), not a session value;
    // exposed to the UI as a bool toggle. Applies on next app start.
    if (key == QLatin1String("engineSplit"))
        return QSettings().value(QStringLiteral("engineMode")).toString() == QLatin1String("ipc");
    // IPC engine mode: no in-process session — read the persisted value from the
    // shared QSettings store (which is what the engine child applies from).
    if (!m_session) return QSettings().value(key);
    SessionManager *s = m_session;
    // speed
    if (key == "downloadLimit")       return s->downloadLimit();
    if (key == "uploadLimit")         return s->uploadLimit();
    if (key == "maxActiveDownloads")  return s->maxActiveDownloads();
    if (key == "seedRatioLimit")      return s->seedRatioLimit();
    if (key == "stopAfterDownload")   return s->stopAfterDownload();
    if (key == "maxSeedDays")         return int(s->maxSeedSeconds() / 86400);
    if (key == "schedulerEnabled")    return s->schedulerEnabled();
    if (key == "altDownloadLimit")    return s->altDownloadLimit();
    if (key == "altUploadLimit")      return s->altUploadLimit();
    if (key == "scheduleFromHour")    return s->scheduleFromHour();
    if (key == "scheduleToHour")      return s->scheduleToHour();
    if (key == "scheduleDays")        return s->scheduleDays();
    // network
    if (key == "listenPort")          return s->listenPort();
    if (key == "maxConnections")      return s->maxConnections();
    if (key == "dhtEnabled")          return s->dhtEnabled();
    if (key == "utpEnabled")          return s->utpEnabled();
    if (key == "encryptionMode")      return s->encryptionMode();
    if (key == "anonymousMode")       return s->anonymousMode();
    if (key == "forceIpv4")           return s->forceIpv4();
    if (key == "ptMode")              return s->ptMode();
    if (key == "blockLeechers")       return s->blockLeecherClients();
    // vpn
    if (key == "outgoingInterface")   return s->outgoingInterface();
    if (key == "killSwitchEnabled")   return s->killSwitchEnabled();
    if (key == "autoResumeOnReconnect") return s->autoResumeOnReconnect();
    // proxy / ip filter
    if (key == "proxyType")           return s->proxyType();
    if (key == "proxyHost")           return s->proxyHost();
    if (key == "proxyPort")           return s->proxyPort();
    if (key == "proxyUser")           return s->proxyUser();
    if (key == "proxyPass")           return s->proxyPass();
    if (key == "proxyLeakProof")      return s->proxyLeakProof();
    if (key == "ipFilterPath")        return s->ipFilterPath();
    // files / media
    if (key == "tempPath")            return s->tempPath();
    if (key == "preallocate")         return s->preallocate();
    if (key == "autoRecheck")         return s->autoRecheck();
    if (key == "contentLayout")       return s->contentLayout();
    if (key == "torrentExportDir")    return s->torrentExportDir();
    if (key == "extractPasswords")    return s->extractPasswords().join(QStringLiteral("; "));
    if (key == "autoExtract")         return s->autoExtract();
    if (key == "autoExtractDelete")   return s->autoExtractDelete();
    if (key == "runOnComplete")       return s->runOnComplete();
    if (key == "watchedFolder")       return s->watchedFolder();
    if (key == "autoMoveEnabled")     return s->autoMoveEnabled();
    if (key == "autoMovePath")        return s->autoMovePath();
    if (key == "autoComplete") {
        const qint64 days[] = {0, 1, 3, 7, 14, 30};
        const int d = int(s->autoCompleteSeconds() / 86400);
        for (int i = 0; i < 6; ++i) if (days[i] == d) return i;
        return 0;
    }
    // advanced libtorrent tuning
    if (key.startsWith(QStringLiteral("adv"))) {
        auto a = s->advancedSettings();
        if (key == "advAioThreads")     return a.aioThreads;
        if (key == "advHashingThreads") return a.hashingThreads;
        if (key == "advFilePool")       return a.filePoolSize;
        if (key == "advCheckingMem")    return a.checkingMemUsage;
        if (key == "advSendBuffer")     return a.sendBufferWatermark;
        if (key == "advConnLimit")      return a.connectionsLimit;
        if (key == "advConnSpeed")      return a.connectionSpeed;
        if (key == "advUnchokeSlots")   return a.unchokeSlotsLimit;
        if (key == "advMaxUploadsTor")  return a.maxUploadsPerTorrent;
        if (key == "advMaxConnsTor")    return a.maxConnectionsPerTorrent;
        if (key == "advChokingAlgo")    return a.chokingAlgorithm == 2 ? 1 : 0;  // lt rate_based=2 → UI idx 1
        if (key == "advSeedChoking")    return a.seedChokingAlgorithm;
        if (key == "advRateOverhead")   return a.rateLimitIpOverhead;
        if (key == "advIgnoreLan")      return a.ignoreLimitsOnLAN;
    }
    // telegram (token lives in the keychain; events are a bitmask)
    if (key == "telegramToken")   return SecretStore::instance().get("telegramBotToken");
    {
        int bit = telegramEventBit(key);
        if (bit) {
            QSettings st;
            int mask = st.value("telegramEvents", 0x0F).toInt();   // default: all on
            return bool(mask & bit);
        }
    }
    if (key == "discordEnabled") { QSettings st; return st.value("discordEnabled", true).toBool(); }
    // webui
    if (key == "webUiEnabled")       { QSettings st; return st.value("webUiEnabled", false).toBool(); }
    if (key == "webUiPort")          { QSettings st; return st.value("webUiPort", 8080).toInt(); }
    if (key == "webUiRemoteAccess")  { QSettings st; return st.value("webUiRemoteAccess", false).toBool(); }
    if (key == "webUiUser")          { QSettings st; return st.value("webUiUser", QStringLiteral("admin")).toString(); }
    if (key == "webUiPassword")      return QString();   // never expose the stored hash
    // media-server secrets live in the keychain; never echoed back to the UI
    if (key == "plexToken" || key == "jellyfinApiKey") return QString();
    // Force a real bool: the Windows registry stores bool as DWORD and reads it
    // back as int, so a raw `=== true` check in QML fails and the dialog re-shows.
    if (key == "welcomeShown") { QSettings st; return st.value(QStringLiteral("welcomeShown"), false).toBool(); }
    // Same DWORD-vs-int trap for every generic UI bool toggle (close-to-tray,
    // splash, etc. "forgetting" they were turned off). Coerce to a real bool when
    // the key is set; leave it invalid when unset so QML's own `on:`/strict-compare
    // defaults still apply.
    static const QSet<QString> uiBoolKeys = {
        QStringLiteral("closeToTray"), QStringLiteral("showSplash"), QStringLiteral("startTray"),
        QStringLiteral("notifSound"), QStringLiteral("randomPort"), QStringLiteral("autoShutdown"),
        QStringLiteral("autoTrackers"), QStringLiteral("torrentSearchEnabled"),
        QStringLiteral("useDefaultPath"), QStringLiteral("verboseLogging"), QStringLiteral("useTor"),
        QStringLiteral("plexEnabled"), QStringLiteral("jellyfinEnabled"), QStringLiteral("tourSeen"),
        QStringLiteral("warnSuspiciousFiles"), QStringLiteral("autoDefenderExclude"),
        QStringLiteral("autoplayNext"), QStringLiteral("preferNativeLang"),
        QStringLiteral("gameAutoInstall")
    };
    if (uiBoolKeys.contains(key)) {
        QSettings st;
        return st.contains(key) ? QVariant(st.value(key).toBool()) : QVariant();
    }
    // UI-only prefs + media API keys
    QSettings st;
    return st.value(key);
}

// Maps a per-event toggle key to its TelegramNotifier::Events bit (0 if not one).
int QmlSettingsBridge::telegramEventBit(const QString &key)
{
    if (key == "telegramEvtFinished") return 1 << 0;
    if (key == "telegramEvtKill")     return 1 << 1;
    if (key == "telegramEvtRss")      return 1 << 2;
    if (key == "telegramEvtError")    return 1 << 3;
    return 0;
}

void QmlSettingsBridge::set(const QString &key, const QVariant &v)
{
    if (key == QLatin1String("engineSplit")) {
        QSettings().setValue(QStringLiteral("engineMode"),
                             v.toBool() ? QStringLiteral("ipc") : QStringLiteral("inprocess"));
        emit changed(); return;   // takes effect on next app start
    }
    // telegram: token → keychain, events → bitmask, chatId → settings; reload after.
    if (key == "telegramToken") {
        SecretStore::instance().set("telegramBotToken", v.toString());
        if (m_telegram) m_telegram->reload();
        emit changed(); return;
    }
    if (int bit = telegramEventBit(key)) {
        QSettings st;
        int mask = st.value("telegramEvents", 0x0F).toInt();
        if (v.toBool()) mask |= bit; else mask &= ~bit;
        st.setValue("telegramEvents", mask);
        if (m_telegram) m_telegram->reload();
        emit changed(); return;
    }
    if (key == "telegramChatId") {
        QSettings st; st.setValue("telegramChatId", v);
        if (m_telegram) m_telegram->reload();
        emit changed(); return;
    }
    if (key.startsWith(QStringLiteral("webUi"))) {
        if (key == "webUiPassword") {
            const QString p = v.toString();
            // hash lives in QSettings (no keychain prompt at boot); empty clears it
            if (p.isEmpty()) QSettings().remove("webUiPasswordHash");
            else QSettings().setValue("webUiPasswordHash", PasswordHash::hash(p));
        } else if (key == "webUiEnabled")      { QSettings().setValue("webUiEnabled", v.toBool()); }
        else if (key == "webUiPort")           { QSettings().setValue("webUiPort", v.toInt()); }
        else if (key == "webUiRemoteAccess")   { QSettings().setValue("webUiRemoteAccess", v.toBool()); }
        else if (key == "webUiUser")           { QSettings().setValue("webUiUser", v.toString()); }
        applyWebUi();
        emit changed(); return;
    }
    if (key == "plexToken" || key == "jellyfinApiKey") {   // → keychain (empty clears)
        SecretStore::instance().set(key, v.toString());
        emit changed(); return;
    }
    if (key == "useTor") {   // one-toggle Tor preset: route through 127.0.0.1:9050 SOCKS5
        QSettings st; st.setValue("useTor", v.toBool());
        if (v.toBool()) {
            if (m_engine) m_engine->setProxySettings(1, QStringLiteral("127.0.0.1"), 9050, QString(), QString());
            st.setValue("proxyType", 1); st.setValue("proxyHost", "127.0.0.1"); st.setValue("proxyPort", 9050);
        }
        emit changed(); return;
    }

    // Session-affecting settings live-apply through the engine — in-process AND
    // in split mode, where the engine child applies + persists via the applySetting
    // RPC. Unknown keys are UI-only prefs → the shared QSettings store.
    if (m_engine && m_engine->applySetting(key, v)) { emit changed(); return; }
    QSettings().setValue(key, v);
    emit changed();
}

void QmlSettingsBridge::testTelegram()
{
    const QString token = SecretStore::instance().get("telegramBotToken");
    QSettings st;
    const QString chatId = st.value("telegramChatId").toString();
    if (token.isEmpty() || chatId.isEmpty()) {
        emit telegramTestResult(false, tr_("settings_telegram_test_missing"));
        return;
    }
    auto *nam = new QNetworkAccessManager(this);
    QUrl url(QStringLiteral("https://api.telegram.org/bot%1/sendMessage").arg(token));
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QJsonObject body;
    body.insert("chat_id", chatId);
    body.insert("text", QStringLiteral("🦇 BATorrent test — webhook works."));
    auto *reply = nam->post(req, QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, reply, nam]() {
        if (reply->error() == QNetworkReply::NoError)
            emit telegramTestResult(true, tr_("settings_telegram_test_ok"));
        else
            emit telegramTestResult(false, QStringLiteral("✗ %1").arg(reply->errorString()));
        reply->deleteLater();
        nam->deleteLater();
    });
}

bool QmlSettingsBridge::excludeFromDefender()
{
    QSettings s;
    QString path = s.value(QStringLiteral("lastSavePath")).toString();
    if (path.isEmpty() || !QDir(path).exists())
        path = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    return Defender::addExclusion(path);
}

static QString localPath(const QString &p)
{
    return p.startsWith(QStringLiteral("file:")) ? QUrl(p).toLocalFile() : p;
}

QString QmlSettingsBridge::exportSettings(const QString &path)
{
    static const QStringList secrets = {"proxyPass","plexToken","jellyfinApiKey","webUiPasswordHash"};
    QSettings st;
    QJsonObject obj;
    for (const auto &k : st.allKeys())
        if (!secrets.contains(k)) obj[k] = QJsonValue::fromVariant(st.value(k));
    QFile f(localPath(path));
    if (!f.open(QIODevice::WriteOnly)) return tr_("full_restore_failed");
    f.write(QJsonDocument(obj).toJson());
    return tr_("export_success");
}

QString QmlSettingsBridge::importSettings(const QString &path)
{
    QFile f(localPath(path));
    if (!f.open(QIODevice::ReadOnly)) return tr_("full_restore_failed");
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    if (!doc.isObject()) return tr_("full_restore_bad_format");
    QSettings st;
    const QJsonObject obj = doc.object();
    for (auto it = obj.begin(); it != obj.end(); ++it)
        st.setValue(it.key(), it.value().toVariant());
    return tr_("import_success") + "\n" + tr_("import_restart");
}

QString QmlSettingsBridge::fullBackup(const QString &path)
{
    QFile f(localPath(path));
    if (!f.open(QIODevice::WriteOnly)) return tr_("full_restore_failed");
    QByteArray archive("BATBACKUP1\n");
    QList<QPair<QString, QByteArray>> entries;
    QSettings st;
    QJsonObject obj;
    for (const auto &k : st.allKeys()) obj[k] = QJsonValue::fromVariant(st.value(k));
    entries.append({"settings.json", QJsonDocument(obj).toJson()});
    QDir resumeDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/resume");
    if (resumeDir.exists())
        for (const auto &name : resumeDir.entryList({"*.resume"}, QDir::Files)) {
            QFile rf(resumeDir.filePath(name));
            if (rf.open(QIODevice::ReadOnly)) entries.append({"resume/" + name, rf.readAll()});
        }
    quint32 count = entries.size();
    archive.append(reinterpret_cast<const char *>(&count), 4);
    for (const auto &e : entries) {
        QByteArray nb = e.first.toUtf8();
        quint32 nl = nb.size(); quint64 dl = e.second.size();
        archive.append(reinterpret_cast<const char *>(&nl), 4);
        archive.append(nb);
        archive.append(reinterpret_cast<const char *>(&dl), 8);
        archive.append(e.second);
    }
    f.write(archive);
    return tr_("full_backup_done");
}

QString QmlSettingsBridge::fullRestore(const QString &path)
{
    QFile f(localPath(path));
    if (!f.open(QIODevice::ReadOnly)) return tr_("full_restore_failed");
    const QByteArray data = f.readAll();
    if (!data.startsWith("BATBACKUP1\n")) return tr_("full_restore_bad_format");
    const char *p = data.constData() + 11, *end = data.constData() + data.size();
    if (p + 4 > end) return tr_("full_restore_bad_format");
    quint32 count; memcpy(&count, p, 4); p += 4;
    const QString base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(base + "/resume");
    int restored = 0;
    for (quint32 i = 0; i < count; ++i) {
        if (end - p < 4) break;
        quint32 nl; memcpy(&nl, p, 4); p += 4;
        if (nl > 4096 || static_cast<ptrdiff_t>(nl) > end - p) break;
        const QString name = QString::fromUtf8(p, nl); p += nl;
        if (end - p < 8) break;
        quint64 dl; memcpy(&dl, p, 8); p += 8;
        if (dl > 1073741824ULL || static_cast<ptrdiff_t>(dl) > end - p) break;
        const QByteArray payload(p, dl); p += dl;
        if (name == "settings.json") {
            const auto obj = QJsonDocument::fromJson(payload).object();
            QSettings s;
            for (auto it = obj.begin(); it != obj.end(); ++it) s.setValue(it.key(), it.value().toVariant());
            ++restored;
        } else if (name.startsWith("resume/")) {
            QFile rf(base + "/" + name);
            if (rf.open(QIODevice::WriteOnly)) { rf.write(payload); ++restored; }
        }
    }
    return tr_("full_restore_done").arg(restored) + "\n" + tr_("import_restart");
}

QStringList QmlSettingsBridge::networkInterfaces() const
{
    QStringList out;
    out << tr_("settings_iface_any");
    for (const QNetworkInterface &iface : QNetworkInterface::allInterfaces()) {
        if (!(iface.flags() & QNetworkInterface::IsUp)) continue;
        if (iface.flags() & QNetworkInterface::IsLoopBack) continue;
        QString ip;
        for (const QNetworkAddressEntry &e : iface.addressEntries())
            if (e.ip().protocol() == QAbstractSocket::IPv4Protocol) { ip = e.ip().toString(); break; }
        out << (ip.isEmpty() ? iface.name() : QStringLiteral("%1 — %2").arg(iface.name(), ip));
    }
    return out;
}

bool QmlSettingsBridge::setAsDefaultApp()
{
    const QString exe = QCoreApplication::applicationFilePath();
    bool ok = false;
#ifdef Q_OS_WIN
    const QString nativeExe = QDir::toNativeSeparators(exe);
    QSettings reg("HKEY_CURRENT_USER\\Software\\Classes", QSettings::NativeFormat);
    reg.setValue(".torrent/.", "BATorrent.torrent");
    reg.setValue("BATorrent.torrent/.", "BATorrent Torrent File");
    reg.setValue("BATorrent.torrent/shell/open/command/.", "\"" + nativeExe + "\" \"%1\"");
    reg.setValue("BATorrent.torrent/DefaultIcon/.", nativeExe + ",0");
    reg.setValue("magnet/.", "URL:Magnet Protocol");
    reg.setValue("magnet/URL Protocol", "");
    reg.setValue("magnet/shell/open/command/.", "\"" + nativeExe + "\" \"%1\"");
    reg.setValue("magnet/DefaultIcon/.", nativeExe + ",0");
    reg.sync();
    ok = (reg.status() == QSettings::NoError);
    if (ok)
        QProcess::startDetached("cmd", {"/c", "assoc", ".torrent=BATorrent.torrent"});
#elif defined(Q_OS_LINUX) || defined(Q_OS_MACOS)
    // A missing helper (xdg-mime / duti) leaves exitCode() at its default 0,
    // which would look like success — gate on the process actually finishing.
    auto run = [](const QString &exe, const QStringList &args) {
        QProcess p;
        p.start(exe, args);
        return p.waitForFinished(3000)
            && p.exitStatus() == QProcess::NormalExit
            && p.exitCode() == 0;
    };
  #if defined(Q_OS_LINUX)
    ok = run("xdg-mime", {"default", "batorrent.desktop", "application/x-bittorrent"})
      && run("xdg-mime", {"default", "batorrent.desktop", "x-scheme-handler/magnet"});
  #else
    ok = run("duti", {"-s", "com.batorrent.app", ".torrent", "all"});
  #endif
#endif
    return ok;
}

void QmlSettingsBridge::applyProxyPreset(const QString &name)
{
    // Mullvad exposes a local SOCKS5 (only while connected to Mullvad); Tor's is
    // the local daemon. Other providers (AirVPN, etc.) need their own host/creds,
    // so a preset there just selects SOCKS5 and leaves the fields to the user.
    QSettings st;
    if (name == QStringLiteral("mullvad")) {
        m_session->setProxySettings(1, QStringLiteral("10.64.0.1"), 1080, QString(), QString());
        st.setValue("proxyType", 1); st.setValue("proxyHost", "10.64.0.1"); st.setValue("proxyPort", 1080);
    } else if (name == QStringLiteral("tor")) {
        m_session->setProxySettings(1, QStringLiteral("127.0.0.1"), 9050, QString(), QString());
        st.setValue("proxyType", 1); st.setValue("proxyHost", "127.0.0.1"); st.setValue("proxyPort", 9050);
    } else {
        m_session->setProxySettings(1, m_session->proxyHost(), m_session->proxyPort(),
                                    m_session->proxyUser(), m_session->proxyPass());
        st.setValue("proxyType", 1);
    }
    emit changed();
}

void QmlSettingsBridge::proxyLeakTest()
{
    const int type = m_session->proxyType();
    if (type == 0 || m_session->proxyHost().isEmpty()) {
        emit proxyLeakTestResult(false, QString());
        return;
    }
    auto *nam = new QNetworkAccessManager(this);
    nam->setProxy(QNetworkProxy(
        type == 1 ? QNetworkProxy::Socks5Proxy : QNetworkProxy::HttpProxy,
        m_session->proxyHost(), quint16(m_session->proxyPort()),
        m_session->proxyUser(), m_session->proxyPass()));
    QNetworkRequest req(QUrl(QStringLiteral("https://api.ipify.org")));
    req.setTransferTimeout(8000);
    QNetworkReply *reply = nam->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, nam]() {
        const bool ok = reply->error() == QNetworkReply::NoError;
        const QString ip = QString::fromUtf8(reply->readAll()).trimmed();
        emit proxyLeakTestResult(ok && !ip.isEmpty(), ok ? ip : reply->errorString());
        reply->deleteLater();
        nam->deleteLater();
    });
}

// --- QmlNotificationBridge ---

