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
    QSettings s("BATorrent", "BATorrent");
    AdvancedSettings a;
    a.aioThreads = s.value("adv/aioThreads", 10).toInt();
    a.hashingThreads = s.value("adv/hashingThreads", 2).toInt();
    a.filePoolSize = s.value("adv/filePoolSize", 100).toInt();
    a.checkingMemUsage = s.value("adv/checkingMemUsage", 512).toInt();
    a.diskIOReadMode = s.value("adv/diskIOReadMode", 0).toInt();
    a.diskIOWriteMode = s.value("adv/diskIOWriteMode", 0).toInt();
    a.connectionsLimit = s.value("adv/connectionsLimit", 500).toInt();
    a.connectionSpeed = s.value("adv/connectionSpeed", 30).toInt();
    a.maxUploadsPerTorrent = s.value("adv/maxUploadsPerTorrent", 4).toInt();
    a.maxConnectionsPerTorrent = s.value("adv/maxConnectionsPerTorrent", 100).toInt();
    a.unchokeSlotsLimit = s.value("adv/unchokeSlotsLimit", 20).toInt();
    a.chokingAlgorithm = s.value("adv/chokingAlgorithm", 0).toInt();
    a.seedChokingAlgorithm = s.value("adv/seedChokingAlgorithm", 0).toInt();
    a.sendBufferWatermark = s.value("adv/sendBufferWatermark", 500).toInt();
    a.outgoingPortMin = s.value("adv/outgoingPortMin", 0).toInt();
    a.outgoingPortMax = s.value("adv/outgoingPortMax", 0).toInt();
    a.rateLimitIpOverhead = s.value("adv/rateLimitIpOverhead", false).toBool();
    a.ignoreLimitsOnLAN = s.value("adv/ignoreLimitsOnLAN", true).toBool();
    return a;
}

void SessionManager::setAdvancedSettings(const AdvancedSettings &a)
{
    QSettings s("BATorrent", "BATorrent");
    s.setValue("adv/aioThreads", a.aioThreads);
    s.setValue("adv/hashingThreads", a.hashingThreads);
    s.setValue("adv/filePoolSize", a.filePoolSize);
    s.setValue("adv/checkingMemUsage", a.checkingMemUsage);
    s.setValue("adv/diskIOReadMode", a.diskIOReadMode);
    s.setValue("adv/diskIOWriteMode", a.diskIOWriteMode);
    s.setValue("adv/connectionsLimit", a.connectionsLimit);
    s.setValue("adv/connectionSpeed", a.connectionSpeed);
    s.setValue("adv/maxUploadsPerTorrent", a.maxUploadsPerTorrent);
    s.setValue("adv/maxConnectionsPerTorrent", a.maxConnectionsPerTorrent);
    s.setValue("adv/unchokeSlotsLimit", a.unchokeSlotsLimit);
    s.setValue("adv/chokingAlgorithm", a.chokingAlgorithm);
    s.setValue("adv/seedChokingAlgorithm", a.seedChokingAlgorithm);
    s.setValue("adv/sendBufferWatermark", a.sendBufferWatermark);
    s.setValue("adv/outgoingPortMin", a.outgoingPortMin);
    s.setValue("adv/outgoingPortMax", a.outgoingPortMax);
    s.setValue("adv/rateLimitIpOverhead", a.rateLimitIpOverhead);
    s.setValue("adv/ignoreLimitsOnLAN", a.ignoreLimitsOnLAN);

    lt::settings_pack pack;
    pack.set_int(lt::settings_pack::aio_threads, a.aioThreads);
    pack.set_int(lt::settings_pack::hashing_threads, a.hashingThreads);
    pack.set_int(lt::settings_pack::file_pool_size, a.filePoolSize);
    pack.set_int(lt::settings_pack::checking_mem_usage, a.checkingMemUsage);
    pack.set_int(lt::settings_pack::disk_io_read_mode, a.diskIOReadMode);
    pack.set_int(lt::settings_pack::disk_io_write_mode, a.diskIOWriteMode);
    pack.set_int(lt::settings_pack::connections_limit, a.connectionsLimit);
    pack.set_int(lt::settings_pack::connection_speed, a.connectionSpeed);
    pack.set_int(lt::settings_pack::unchoke_slots_limit, a.unchokeSlotsLimit);
    pack.set_int(lt::settings_pack::choking_algorithm, a.chokingAlgorithm);
    pack.set_int(lt::settings_pack::seed_choking_algorithm, a.seedChokingAlgorithm);
    pack.set_int(lt::settings_pack::send_buffer_watermark, a.sendBufferWatermark * 1024);
    pack.set_bool(lt::settings_pack::rate_limit_ip_overhead, a.rateLimitIpOverhead);
    if (a.outgoingPortMin > 0 && a.outgoingPortMax >= a.outgoingPortMin) {
        pack.set_int(lt::settings_pack::outgoing_port, a.outgoingPortMin);
        pack.set_int(lt::settings_pack::num_outgoing_ports, a.outgoingPortMax - a.outgoingPortMin + 1);
    }
    // LAN peer class exemption: peers on local networks (10.x, 172.16.x,
    // 192.168.x) bypass speed limits. Uses peer_classes instead of the
    // deprecated ignore_limits_on_local_network.
    (void)a.ignoreLimitsOnLAN; // applied via peer classes, not settings_pack

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
        bool hit = true;
        if (key == "advAioThreads")          a.aioThreads = v.toInt();
        else if (key == "advHashingThreads") a.hashingThreads = v.toInt();
        else if (key == "advFilePool")       a.filePoolSize = v.toInt();
        else if (key == "advCheckingMem")    a.checkingMemUsage = v.toInt();
        else if (key == "advSendBuffer")     a.sendBufferWatermark = v.toInt();
        else if (key == "advConnLimit")      a.connectionsLimit = v.toInt();
        else if (key == "advConnSpeed")      a.connectionSpeed = v.toInt();
        else if (key == "advUnchokeSlots")   a.unchokeSlotsLimit = v.toInt();
        else if (key == "advMaxUploadsTor")  a.maxUploadsPerTorrent = v.toInt();
        else if (key == "advMaxConnsTor")    a.maxConnectionsPerTorrent = v.toInt();
        else if (key == "advChokingAlgo")    a.chokingAlgorithm = v.toInt() == 1 ? 2 : 0;
        else if (key == "advSeedChoking")    a.seedChokingAlgorithm = v.toInt();
        else if (key == "advRateOverhead")   a.rateLimitIpOverhead = v.toBool();
        else if (key == "advIgnoreLan")      a.ignoreLimitsOnLAN = v.toBool();
        else hit = false;
        if (hit) setAdvancedSettings(a);
        return hit;
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

void SessionManager::extractArchives(const QString &savePath, const QString &torrentName,
                                     const QString &priorityPassword, const QString &infoHash)
{
    m_extractor->setPasswords(m_extractPasswords);
    m_extractor->setDeleteAfter(m_autoExtractDelete);
    m_extractor->extract(savePath, torrentName, priorityPassword, infoHash);
}

bool SessionManager::torrentHasArchives(int index) const
{
    if (index < 0 || index >= static_cast<int>(m_torrents.size())) return false;
    if (!m_torrents[index].is_valid()) return false;   // torrent_file() throws on an invalid handle
    auto ti = m_torrents[index].torrent_file();
    if (!ti) return false;
    const auto &fs = ti->files();
    QStringList names;
    for (lt::file_index_t i(0); i < fs.end_file(); ++i) {
        QString p = QString::fromStdString(fs.file_path(i));
        if (p.endsWith(QLatin1String(".!bt"))) p.chop(4);
        names << p;
    }
    return !ArchiveScan::archivesToExtract(names).isEmpty();
}

bool SessionManager::torrentHasVideo(int index) const
{
    if (index < 0 || index >= static_cast<int>(m_torrents.size())) return false;
    if (!m_torrents[index].is_valid()) return false;   // torrent_file() throws on an invalid handle
    auto ti = m_torrents[index].torrent_file();
    if (!ti) return false;
    static const QStringList videoExts = {".mp4",".mkv",".avi",".mov",".wmv",".flv",".webm",".m4v",".ts",".mpg",".mpeg",".m2ts"};
    const auto &fs = ti->files();
    for (lt::file_index_t i(0); i < fs.end_file(); ++i) {
        QString p = QString::fromStdString(fs.file_path(i)).toLower();
        if (p.endsWith(QLatin1String(".!bt"))) p.chop(4);
        for (const auto &ext : videoExts)
            if (p.endsWith(ext)) return true;
    }
    return false;
}

void SessionManager::extractTorrent(int index, const QString &password)
{
    if (index < 0 || index >= static_cast<int>(m_torrents.size())) return;
    const TorrentInfo info = torrentAt(index);
    extractArchives(info.savePath, info.name, password, torrentHashAt(index));
}

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

