// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "bridges/qmlsettingsbridge.h"
#include "torrent/sessionmanager.h"
#include "services/discovery/addonmanager.h"
#include "services/security/defender.h"
#include "services/platform/translator.h"
#include "services/platform/settingsbackup.h"
#include "services/platform/settingspolicy.h"
#include "services/platform/fileassociation.h"
#include "services/integrations/notifier.h"
#include "services/security/passwordhash.h"
#include "services/security/secretstore.h"
#include "webui/webserver.h"

#include <QNetworkAccessManager>
#include <QNetworkProxy>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QNetworkInterface>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRandomGenerator>
#include <QSettings>
#include <QStandardPaths>
#include <QUrl>

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
    const bool remote = SettingsPolicy::webUiRemoteAllowed(
        st.value("webUiRemoteAccess", false).toBool(), hasAuth);
    m_webServer->start(quint16(st.value("webUiPort", 8080).toInt()), remote);
}

QString QmlSettingsBridge::enablePairing()
{
    const QString pw = SettingsPolicy::generatePairingPassword(14, [](int n) {
        return QRandomGenerator::global()->bounded(n);
    });

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
        return SettingsPolicy::engineSplitFromMode(
            QSettings().value(QStringLiteral("engineMode")).toString());
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
    if (key == "autoComplete")
        return SettingsPolicy::autoCompleteIndex(s->autoCompleteSeconds());
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
        if (key == "advChokingAlgo")    return SettingsPolicy::advChokingUiIndex(a.chokingAlgorithm);
        if (key == "advSeedChoking")    return a.seedChokingAlgorithm;
        if (key == "advRateOverhead")   return a.rateLimitIpOverhead;
        if (key == "advIgnoreLan")      return a.ignoreLimitsOnLAN;
    }
    // telegram (token lives in the keychain; events are a bitmask)
    if (key == "telegramToken")   return SecretStore::instance().get("telegramBotToken");
    if (int bit = SettingsPolicy::telegramEventBit(key)) {
        QSettings st;
        int mask = st.value("telegramEvents", 0x0F).toInt();   // default: all on
        return bool(mask & bit);
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
    if (SettingsPolicy::isUiBoolKey(key)) {
        QSettings st;
        return st.contains(key) ? QVariant(st.value(key).toBool()) : QVariant();
    }
    // UI-only prefs + media API keys
    QSettings st;
    return st.value(key);
}

void QmlSettingsBridge::set(const QString &key, const QVariant &v)
{
    // per-type file/protocol association toggles (Windows registry; a no-op
    // persist elsewhere — the rows are hidden off-Windows anyway)
    if (key == QLatin1String("assocTorrent") || key == QLatin1String("assocMagnet")
        || key == QLatin1String("assocBittorrent")) {
        QSettings().setValue(key, v.toBool());
        const QString kind = key == QLatin1String("assocTorrent") ? QStringLiteral("torrent")
                           : key == QLatin1String("assocMagnet")  ? QStringLiteral("magnet")
                                                                  : QStringLiteral("bittorrent");
        FileAssociation::apply(kind, v.toBool());
        emit changed(); return;
    }
    if (key == QLatin1String("engineSplit")) {
        QSettings().setValue(QStringLiteral("engineMode"),
                             SettingsPolicy::engineModeValue(v.toBool()));
        emit changed(); return;   // takes effect on next app start
    }
    // telegram: token → keychain, events → bitmask, chatId → settings; reload after.
    if (key == "telegramToken") {
        SecretStore::instance().set("telegramBotToken", v.toString());
        if (m_telegram) m_telegram->reload();
        emit changed(); return;
    }
    if (int bit = SettingsPolicy::telegramEventBit(key)) {
        QSettings st;
        int mask = st.value("telegramEvents", 0x0F).toInt();
        st.setValue("telegramEvents", SettingsPolicy::applyTelegramEventMask(mask, bit, v.toBool()));
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
    // "Block known bad peers": download/refresh + apply the auto blocklist, or clear
    // it. The actual fetch is owned by main.cpp (needs QNAM + the engine).
    if (key == "blockBadPeers") {
        QSettings().setValue(key, v.toBool());
        emit blockBadPeersToggled(v.toBool());
        emit changed(); return;
    }
    if (key == "useTor") {   // one-toggle Tor preset: route through 127.0.0.1:9050 SOCKS5
        QSettings st; st.setValue("useTor", v.toBool());
        if (v.toBool()) {
            const auto preset = SettingsPolicy::proxyPreset(QStringLiteral("tor"));
            if (m_engine) m_engine->setProxySettings(preset.type, preset.host, preset.port, QString(), QString());
            st.setValue("proxyType", preset.type);
            st.setValue("proxyHost", preset.host);
            st.setValue("proxyPort", preset.port);
        }
        emit changed(); return;
    }

    // Session-affecting settings live-apply through the engine — in-process AND
    // in split mode, where the engine child applies + persists via the applySetting
    // RPC. Unknown keys are UI-only prefs → the shared QSettings store.
    if (m_engine && m_engine->applySetting(key, v)) { emit changed(); return; }
    QSettings().setValue(key, v);
    if (key == QLatin1String("contentLanguage"))
        AddonManager::instance().syncCuratedAddons();
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

QString QmlSettingsBridge::exportSettings(const QString &path)
{
    QSettings st;
    QVariantMap map;
    for (const auto &k : st.allKeys())
        map.insert(k, st.value(k));
    const QJsonObject obj = SettingsBackup::settingsObjectFromMap(map, /*stripSecrets=*/true);
    QFile f(SettingsBackup::localPath(path));
    if (!f.open(QIODevice::WriteOnly)) return tr_("full_restore_failed");
    f.write(QJsonDocument(obj).toJson());
    return tr_("export_success");
}

QString QmlSettingsBridge::importSettings(const QString &path)
{
    QFile f(SettingsBackup::localPath(path));
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
    QFile f(SettingsBackup::localPath(path));
    if (!f.open(QIODevice::WriteOnly)) return tr_("full_restore_failed");
    QList<QPair<QString, QByteArray>> entries;
    QSettings st;
    QVariantMap map;
    for (const auto &k : st.allKeys())
        map.insert(k, st.value(k));
    entries.append({QStringLiteral("settings.json"),
                    QJsonDocument(SettingsBackup::settingsObjectFromMap(map, false)).toJson()});
    QDir resumeDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/resume");
    if (resumeDir.exists())
        for (const auto &name : resumeDir.entryList({"*.resume"}, QDir::Files)) {
            QFile rf(resumeDir.filePath(name));
            if (rf.open(QIODevice::ReadOnly))
                entries.append({QStringLiteral("resume/") + name, rf.readAll()});
        }
    f.write(SettingsBackup::pack(entries));
    return tr_("full_backup_done");
}

QString QmlSettingsBridge::fullRestore(const QString &path)
{
    QFile f(SettingsBackup::localPath(path));
    if (!f.open(QIODevice::ReadOnly)) return tr_("full_restore_failed");
    const auto unpacked = SettingsBackup::unpack(f.readAll());
    if (!unpacked.ok) return tr_("full_restore_bad_format");
    const QString base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(base + "/resume");
    int restored = 0;
    for (const auto &e : unpacked.entries) {
        if (e.first == QLatin1String("settings.json")) {
            const auto obj = QJsonDocument::fromJson(e.second).object();
            QSettings s;
            for (auto it = obj.begin(); it != obj.end(); ++it)
                s.setValue(it.key(), it.value().toVariant());
            ++restored;
        } else if (e.first.startsWith(QLatin1String("resume/"))) {
            QFile rf(base + "/" + e.first);
            if (rf.open(QIODevice::WriteOnly)) { rf.write(e.second); ++restored; }
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
    return FileAssociation::setAsDefaultApp();
}

void QmlSettingsBridge::applyProxyPreset(const QString &name)
{
    const auto preset = SettingsPolicy::proxyPreset(name);
    QSettings st;
    const int type = preset.type;
    QString host;
    int port = 0;
    QString user;
    QString pass;
    if (preset.keepHost) {
        if (m_session) {
            host = m_session->proxyHost();
            port = m_session->proxyPort();
            user = m_session->proxyUser();
            pass = m_session->proxyPass();
        } else {
            host = st.value(QStringLiteral("proxyHost")).toString();
            port = st.value(QStringLiteral("proxyPort")).toInt();
            user = st.value(QStringLiteral("proxyUser")).toString();
            pass = st.value(QStringLiteral("proxyPass")).toString();
        }
    } else {
        host = preset.host;
        port = preset.port;
    }
    if (m_engine)
        m_engine->setProxySettings(type, host, port, user, pass);
    else if (m_session)
        m_session->setProxySettings(type, host, port, user, pass);
    st.setValue("proxyType", type);
    if (!preset.keepHost) {
        st.setValue("proxyHost", host);
        st.setValue("proxyPort", port);
    }
    emit changed();
}

void QmlSettingsBridge::proxyLeakTest()
{
    if (!m_session) {
        emit proxyLeakTestResult(false, QString());
        return;
    }
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
