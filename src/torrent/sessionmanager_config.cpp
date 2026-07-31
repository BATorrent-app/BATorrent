// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details
//
// SessionManager — configuration slice. Global session knobs (rate limits,
// listen port, connection cap, DHT/uTP/anonymous/PT toggles, encryption,
// seed-ratio) plus leecher-client blocking. Split out of sessionmanager.cpp
// verbatim; no behaviour change. Per-torrent overrides live elsewhere.

#include "torrent/sessionmanager.h"
#include "torrent/sessionconfig.h"

#include <libtorrent/settings_pack.hpp>
#include <libtorrent/ip_filter.hpp>
#include <libtorrent/peer_info.hpp>
#include <QSettings>
#include <QNetworkInterface>
#include <QAbstractSocket>
#include <QByteArray>
#include <QList>
#include <QDebug>
#include <QProcess>
#include <QRegularExpression>
#include "services/security/archivescan.h"
#include "services/security/archiveextractor.h"
#include <libtorrent/torrent_info.hpp>
#include <vector>

void SessionManager::setDownloadLimit(int kbps)
{
    // Treat "user-set" as the normal rate. When alt-speed mode is active
    // libtorrent's currently-applied limit is the alt value; we don't want
    // the user's preferences UI to silently clobber the alt value, and we
    // don't want our own scheduler to clobber the user's new normal when it
    // restores. So always update m_normalDownLimit, and only push to
    // libtorrent immediately if alt mode isn't active.
    m_normalDownLimit = kbps;
    QSettings("BATorrent", "BATorrent").setValue("downloadLimit", kbps);
    if (m_altSpeedsActive)
        return;
    lt::settings_pack pack;
    pack.set_int(lt::settings_pack::download_rate_limit, kbps > 0 ? kbps * 1024 : 0);
    m_session.apply_settings(pack);
}

void SessionManager::setUploadLimit(int kbps)
{
    m_normalUpLimit = kbps;
    QSettings("BATorrent", "BATorrent").setValue("uploadLimit", kbps);
    if (m_altSpeedsActive)
        return;
    lt::settings_pack pack;
    pack.set_int(lt::settings_pack::upload_rate_limit, kbps > 0 ? kbps * 1024 : 0);
    m_session.apply_settings(pack);
}

int SessionManager::downloadLimit() const
{
    // Always return the user-set "normal" preference; the alt value lives in
    // m_altDownLimit and is read via altDownloadLimit().
    return m_normalDownLimit;
}

int SessionManager::uploadLimit() const
{
    return m_normalUpLimit;
}

void SessionManager::setListenPort(int port)
{
    lt::settings_pack pack;
    QString listenAddr = "0.0.0.0";
    if (!m_outgoingInterface.isEmpty()) {
        QNetworkInterface ni = QNetworkInterface::interfaceFromName(m_outgoingInterface);
        for (const auto &entry : ni.addressEntries()) {
            if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol) {
                listenAddr = entry.ip().toString();
                break;
            }
        }
    }
    // Dual-stack unless bound to a specific interface IP or force-v4 is on —
    // a v4-only listen silently halves reachability on v6-capable swarms.
    const QString iface = SessionConfig::listenInterfaces(listenAddr, port, m_forceIpv4);
    pack.set_str(lt::settings_pack::listen_interfaces, iface.toStdString());
    m_session.apply_settings(pack);
    QSettings("BATorrent", "BATorrent").setValue("listenPort", port);
}

int SessionManager::listenPort() const
{
    // Prefer the configured value. m_session.listen_port() reports the live
    // socket, which reads 0 or stale right after a re-bind (libtorrent applies
    // listen_interfaces asynchronously) — that made the settings field appear
    // to "reset" to the old port. Fall back to the live port only when unset.
    const int configured = QSettings("BATorrent", "BATorrent").value("listenPort", 0).toInt();
    return configured > 0 ? configured : m_session.listen_port();
}

void SessionManager::setMaxConnections(int max)
{
    lt::settings_pack pack;
    pack.set_int(lt::settings_pack::connections_limit, max);
    m_session.apply_settings(pack);
    QSettings("BATorrent", "BATorrent").setValue("maxConnections", max);
}

int SessionManager::maxConnections() const
{
    return m_session.get_settings().get_int(lt::settings_pack::connections_limit);
}

void SessionManager::setDhtEnabled(bool enabled)
{
    m_dhtEnabled = enabled;
    lt::settings_pack pack;
    pack.set_bool(lt::settings_pack::enable_dht, enabled);
    m_session.apply_settings(pack);
    QSettings("BATorrent", "BATorrent").setValue("dhtEnabled", enabled);
}

bool SessionManager::dhtEnabled() const
{
    return m_dhtEnabled;
}

void SessionManager::setUtpEnabled(bool enabled)
{
    m_utpEnabled = enabled;
    lt::settings_pack pack;
    pack.set_bool(lt::settings_pack::enable_outgoing_utp, enabled);
    pack.set_bool(lt::settings_pack::enable_incoming_utp, enabled);
    // Always keep TCP available so peer connectivity doesn't collapse when
    // uTP is off; the user just biases libtorrent's transport choice.
    pack.set_bool(lt::settings_pack::enable_outgoing_tcp, true);
    pack.set_bool(lt::settings_pack::enable_incoming_tcp, true);
    m_session.apply_settings(pack);
    QSettings("BATorrent", "BATorrent").setValue("utpEnabled", enabled);
}

bool SessionManager::utpEnabled() const
{
    return m_utpEnabled;
}

void SessionManager::setAnonymousMode(bool enabled)
{
    qDebug() << "[session] anonymousMode:" << enabled;
    m_anonymousMode = enabled;
    lt::settings_pack pack;
    pack.set_bool(lt::settings_pack::anonymous_mode, enabled);
    m_session.apply_settings(pack);
    QSettings("BATorrent", "BATorrent").setValue("anonymousMode", enabled);
}

bool SessionManager::anonymousMode() const { return m_anonymousMode; }

void SessionManager::setForceIpv4(bool enabled)
{
    qDebug() << "[session] forceIpv4:" << enabled;
    m_forceIpv4 = enabled;
    lt::settings_pack pack;
    int port = listenPort();
    if (port <= 0) port = 6881;
    // listen_interfaces format: "ip:port[,ip:port]..." — drop the v6 entry
    // (0.0.0.0 only) when force-v4 is on; otherwise bind both stacks.
    QString iface = enabled
        ? QString("0.0.0.0:%1").arg(port)
        : QString("0.0.0.0:%1,[::]:%1").arg(port);
    pack.set_str(lt::settings_pack::listen_interfaces, iface.toStdString());
    m_session.apply_settings(pack);
    QSettings("BATorrent", "BATorrent").setValue("forceIpv4", enabled);
}

bool SessionManager::forceIpv4() const { return m_forceIpv4; }

void SessionManager::setPtMode(bool enabled)
{
    qDebug() << "[session] ptMode:" << enabled;
    m_ptMode = enabled;
    lt::settings_pack pack;
    pack.set_bool(lt::settings_pack::enable_dht, !enabled && m_dhtEnabled);
    pack.set_bool(lt::settings_pack::enable_lsd, !enabled);
    // PEX has no global on/off in libtorrent 2.x; it's disabled per-torrent
    // via the disable_pex flag at add time. Existing torrents stay as-is.
    pack.set_bool(lt::settings_pack::announce_to_all_trackers, enabled);
    // tiers stays on either way — it's the session default (qBittorrent parity);
    // toggling PT mode off must not drop it below that baseline.
    pack.set_bool(lt::settings_pack::announce_to_all_tiers, true);
    pack.set_bool(lt::settings_pack::anonymous_mode, enabled || m_anonymousMode);
    m_session.apply_settings(pack);
    QSettings("BATorrent", "BATorrent").setValue("ptMode", enabled);
}

bool SessionManager::ptMode() const { return m_ptMode; }

void SessionManager::setBlockLeecherClients(bool enabled)
{
    qDebug() << "[session] blockLeechers:" << enabled;
    m_blockLeechers = enabled;
    QSettings("BATorrent", "BATorrent").setValue("blockLeechers", enabled);
}

bool SessionManager::blockLeecherClients() const { return m_blockLeechers; }
int SessionManager::blockedLeecherCount() const { return m_blockedLeecherCount; }

void SessionManager::checkAndBlockLeechers()
{
    if (!m_blockLeechers) return;
    static const QList<QByteArray> kLeecherPrefixes = {
        "-SD", "-XL", "XL", "-DL", "-QD", "-BN", "-SP",
    };
    lt::ip_filter filter = m_session.get_ip_filter();
    bool filterChanged = false;
    for (auto &h : m_torrents) {
        if (!h.is_valid()) continue;
        std::vector<lt::peer_info> peers;
        try { h.get_peer_info(peers); } catch (...) { continue; }
        for (const auto &p : peers) {
            QByteArray pid(p.pid.data(), 8);
            for (const auto &prefix : kLeecherPrefixes) {
                if (pid.startsWith(prefix)) {
                    h.connect_peer(p.ip, lt::peer_source_flags_t{});
                    try {
                        auto addr = p.ip.address();
                        filter.add_rule(addr, addr, lt::ip_filter::blocked);
                        filterChanged = true;
                    } catch (...) { /* malformed peer address — skip blocking it */ }
                    ++m_blockedLeecherCount;
                    qDebug() << "[session] blocked leecher peer:" << QString::fromStdString(p.ip.address().to_string()) << "client:" << pid.left(8);
                    break;
                }
            }
        }
    }
    if (filterChanged)
        m_session.set_ip_filter(filter);
}

void SessionManager::setEncryptionMode(int mode)
{
    m_encryptionMode = mode;
    lt::settings_pack pack;
    int policy;
    switch (mode) {
    case 1: policy = lt::settings_pack::pe_forced; break;
    case 2: policy = lt::settings_pack::pe_disabled; break;
    default: policy = lt::settings_pack::pe_enabled; break;
    }
    pack.set_int(lt::settings_pack::out_enc_policy, policy);
    pack.set_int(lt::settings_pack::in_enc_policy, policy);
    m_session.apply_settings(pack);
    QSettings("BATorrent", "BATorrent").setValue("encryptionMode", mode);
}

int SessionManager::encryptionMode() const
{
    return m_encryptionMode;
}

void SessionManager::setSeedRatioLimit(float ratio)
{
    m_seedRatioLimit = ratio;
    QSettings("BATorrent", "BATorrent").setValue("seedRatioLimit", ratio);
}

float SessionManager::seedRatioLimit() const
{
    return m_seedRatioLimit;
}

void SessionManager::setStopAfterDownload(bool enabled)
{
    m_stopAfterDownload = enabled;
}

bool SessionManager::stopAfterDownload() const
{
    return m_stopAfterDownload;
}

void SessionManager::setMaxSeedSeconds(qint64 seconds)
{
    m_maxSeedSeconds = seconds;
}

qint64 SessionManager::maxSeedSeconds() const
{
    return m_maxSeedSeconds;
}

// --- moved from sessionmanager.cpp (settings / storage prefs) ---

// --- Bandwidth Scheduler ---

AdvancedSettings SessionManager::advancedSettings() const
{
    return SessionConfig::loadAdvanced(QSettings(QStringLiteral("BATorrent"), QStringLiteral("BATorrent")));
}

void SessionManager::setAdvancedSettings(const AdvancedSettings &a)
{
    QSettings s(QStringLiteral("BATorrent"), QStringLiteral("BATorrent"));
    SessionConfig::persistAdvanced(s, a);
    lt::settings_pack pack;
    SessionConfig::fillAdvancedPack(pack, a);
    m_session.apply_settings(pack);
    qDebug() << "[session] advanced settings applied";
}

void SessionManager::setTorrentExportDir(const QString &path)
{
    m_torrentExportDir = path;
    QSettings("BATorrent", "BATorrent").setValue("torrentExportDir", path);
}

QString SessionManager::torrentExportDir() const { return m_torrentExportDir; }

void SessionManager::setRunOnComplete(const QString &command)
{
    m_runOnComplete = command;
    QSettings("BATorrent", "BATorrent").setValue("runOnComplete", command);
}

QString SessionManager::runOnComplete() const { return m_runOnComplete; }

// Centralised key→setter routing for every session-affecting setting, shared by
// the in-process path and the IPC engine (over the applySetting RPC) so settings
// apply live in split mode too. Returns false for keys that aren't ours (UI-only
// prefs the caller persists to QSettings). Mirrors QmlSettingsBridge::set().
bool SessionManager::applySetting(const QString &key, const QVariant &v)
{
    if (key.startsWith(QLatin1String("adv"))) {
        AdvancedSettings a = advancedSettings();
        if (!SessionConfig::patchAdvancedKey(a, key, v))
            return false;
        setAdvancedSettings(a);
        return true;
    }
    if (key == "downloadLimit")            setDownloadLimit(v.toInt());
    else if (key == "uploadLimit")         setUploadLimit(v.toInt());
    else if (key == "maxActiveDownloads")  setMaxActiveDownloads(v.toInt());
    else if (key == "seedRatioLimit")      setSeedRatioLimit(v.toFloat());
    else if (key == "stopAfterDownload")   setStopAfterDownload(v.toBool());
    else if (key == "maxSeedDays")         setMaxSeedSeconds(qint64(v.toInt()) * 86400);
    else if (key == "schedulerEnabled")    setSchedulerEnabled(v.toBool());
    else if (key == "altDownloadLimit")    setAltSpeedLimits(v.toInt(), altUploadLimit());
    else if (key == "altUploadLimit")      setAltSpeedLimits(altDownloadLimit(), v.toInt());
    else if (key == "scheduleFromHour")    setScheduleFromHour(v.toInt());
    else if (key == "scheduleToHour")      setScheduleToHour(v.toInt());
    else if (key == "scheduleDays")        setScheduleDays(v.toInt());
    else if (key == "listenPort")          setListenPort(v.toInt());
    else if (key == "maxConnections")      setMaxConnections(v.toInt());
    else if (key == "dhtEnabled")          setDhtEnabled(v.toBool());
    else if (key == "utpEnabled")          setUtpEnabled(v.toBool());
    else if (key == "encryptionMode")      setEncryptionMode(v.toInt());
    else if (key == "anonymousMode")       setAnonymousMode(v.toBool());
    else if (key == "forceIpv4")           setForceIpv4(v.toBool());
    else if (key == "ptMode")              setPtMode(v.toBool());
    else if (key == "blockLeechers")       setBlockLeecherClients(v.toBool());
    else if (key == "outgoingInterface")   setOutgoingInterface(v.toString());
    else if (key == "killSwitchEnabled")   setKillSwitchEnabled(v.toBool());
    else if (key == "autoResumeOnReconnect") setAutoResumeOnReconnect(v.toBool());
    else if (key == "proxyType")           setProxySettings(v.toInt(), proxyHost(), proxyPort(), proxyUser(), proxyPass());
    else if (key == "proxyHost")           setProxySettings(proxyType(), v.toString(), proxyPort(), proxyUser(), proxyPass());
    else if (key == "proxyPort")           setProxySettings(proxyType(), proxyHost(), v.toInt(), proxyUser(), proxyPass());
    else if (key == "proxyUser")           setProxySettings(proxyType(), proxyHost(), proxyPort(), v.toString(), proxyPass());
    else if (key == "proxyPass")           setProxySettings(proxyType(), proxyHost(), proxyPort(), proxyUser(), v.toString());
    else if (key == "proxyLeakProof")      setProxyLeakProof(v.toBool());
    else if (key == "ipFilterPath")        loadIpFilter(v.toString());
    else if (key == "autoBlocklistFile")   loadAutoBlocklist(v.toString());
    else if (key == "tempPath")            setTempPath(v.toString());
    else if (key == "preallocate")         setPreallocate(v.toBool());
    else if (key == "autoRecheck")         setAutoRecheck(v.toBool());
    else if (key == "contentLayout")       setContentLayout(v.toInt());
    else if (key == "torrentExportDir")    setTorrentExportDir(v.toString());
    else if (key == "extractPasswords") {
        setExtractPasswords(SessionConfig::parseExtractPasswords(v.toString()));
    }
    else if (key == "autoExtract")         setAutoExtract(v.toBool());
    else if (key == "autoExtractDelete")   setAutoExtractDelete(v.toBool());
    else if (key == "runOnComplete")       setRunOnComplete(v.toString());
    else if (key == "watchedFolder")       setWatchedFolder(v.toString());
    else if (key == "autoMoveEnabled")     setAutoMove(v.toBool(), autoMovePath());
    else if (key == "autoMovePath")        setAutoMove(autoMoveEnabled(), v.toString());
    else if (key == "autoComplete") {
        setAutoCompleteSeconds(SessionConfig::autoCompleteSecondsFromIndex(v.toInt()));
    }
    else return false;
    return true;
}

void SessionManager::executeOnComplete(const QString &name, const QString &savePath,
                                       const QString &hash, qint64 totalSize)
{
    if (m_runOnComplete.isEmpty()) return;
    const QString cmd = SessionConfig::expandOnCompleteCommand(
        m_runOnComplete, name, savePath, hash, totalSize);
    qDebug() << "[session] executeOnComplete:" << cmd;
    QProcess::startDetached("/bin/sh", {"-c", cmd});
}

void SessionManager::setPreallocate(bool on)
{
    m_preallocate = on;
    QSettings("BATorrent", "BATorrent").setValue("preallocate", on);
}
bool SessionManager::preallocate() const { return m_preallocate; }

void SessionManager::setAutoRecheck(bool on)
{
    m_autoRecheck = on;
    QSettings("BATorrent", "BATorrent").setValue("autoRecheck", on);
}
bool SessionManager::autoRecheck() const { return m_autoRecheck; }


int SessionManager::portStatus() const { return m_portStatus; }

void SessionManager::updatePortStatus()
{
    const int s = SessionConfig::portStatusCode(m_listenOk, m_portmapOk);
    if (s == m_portStatus) return;
    m_portStatus = s;
    emit portStatusChanged(s);
}


// --- Auto-move ---

void SessionManager::setAutoMove(bool enabled, const QString &path)
{
    m_autoMoveEnabled = enabled;
    m_autoMovePath = path;
    QSettings st("BATorrent", "BATorrent");
    st.setValue("autoMoveEnabled", enabled);
    st.setValue("autoMovePath", path);
}

bool SessionManager::autoMoveEnabled() const { return m_autoMoveEnabled; }
QString SessionManager::autoMovePath() const { return m_autoMovePath; }

// --- Auto-extract ---

void SessionManager::setAutoExtract(bool enabled)
{
    m_autoExtract = enabled;
    QSettings("BATorrent", "BATorrent").setValue("autoExtract", enabled);
}

bool SessionManager::autoExtract() const { return m_autoExtract; }

void SessionManager::setAutoExtractDelete(bool deleteAfter)
{
    m_autoExtractDelete = deleteAfter;
    QSettings("BATorrent", "BATorrent").setValue("autoExtractDelete", deleteAfter);
}

bool SessionManager::autoExtractDelete() const { return m_autoExtractDelete; }

void SessionManager::setExtractPasswords(const QStringList &passwords)
{
    m_extractPasswords = passwords;
    QSettings("BATorrent", "BATorrent").setValue("extractPasswords", passwords);
}

QStringList SessionManager::extractPasswords() const { return m_extractPasswords; }


// --- Temp path ---

void SessionManager::setTempPath(const QString &path)
{
    m_tempPath = path;
    QSettings s("BATorrent", "BATorrent");
    s.setValue("tempPath", path);
}

QString SessionManager::tempPath() const { return m_tempPath; }

// --- Content layout ---

void SessionManager::setContentLayout(int layout)
{
    m_contentLayout = layout;
    QSettings s("BATorrent", "BATorrent");
    s.setValue("contentLayout", layout);
}

int SessionManager::contentLayout() const { return m_contentLayout; }

void SessionManager::applyContentLayout(lt::add_torrent_params &atp)
{
    if (!atp.ti || m_contentLayout == 0) return;
    const auto &files = atp.ti->files();
    std::vector<std::string> paths;
    paths.reserve(static_cast<size_t>(files.num_files()));
    for (lt::file_index_t i(0); i < files.end_file(); ++i)
        paths.push_back(files.file_path(i));
    const auto plan = SessionConfig::planContentLayout(
        m_contentLayout, atp.ti->name(), paths);
    for (const auto &kv : plan)
        atp.renamed_files[lt::file_index_t(kv.first)] = kv.second;
}

// --- Excluded file patterns ---

void SessionManager::setExcludedFilePatterns(const QStringList &patterns)
{
    m_excludedFilePatterns = patterns;
    QSettings s("BATorrent", "BATorrent");
    s.setValue("excludedFilePatterns", patterns);
}

QStringList SessionManager::excludedFilePatterns() const { return m_excludedFilePatterns; }

void SessionManager::applyExcludedPatterns(lt::add_torrent_params &atp)
{
    if (!atp.ti || m_excludedFilePatterns.isEmpty()) return;
    const auto &files = atp.ti->files();
    const int numFiles = static_cast<int>(files.num_files());

    QList<QRegularExpression> regexes;
    for (const QString &p : m_excludedFilePatterns) {
        QString trimmed = p.trimmed();
        if (trimmed.isEmpty()) continue;
        QRegularExpression re(trimmed, QRegularExpression::CaseInsensitiveOption);
        if (re.isValid())
            regexes.append(re);
    }
    if (regexes.isEmpty()) return;

    if (atp.file_priorities.empty())
        atp.file_priorities.resize(numFiles, lt::default_priority);

    for (lt::file_index_t i(0); i < files.end_file(); ++i) {
        QString path = QString::fromStdString(files.file_path(i));
        for (const auto &re : regexes) {
            if (re.match(path).hasMatch()) {
                atp.file_priorities[static_cast<int>(i)] = lt::dont_download;
                break;
            }
        }
    }
}

