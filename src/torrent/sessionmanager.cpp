// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "torrent/sessionmanager.h"
#include "torrent/memguard.h"
#include "services/platform/logger.h"
#include "services/platform/translator.h"
#include "services/security/suspiciousscan.h"
#include "services/security/defender.h"
#include "services/security/archivescan.h"
#include "services/security/archiveextractor.h"
#include <QProcess>
#include <QCoreApplication>
#if defined(Q_OS_MACOS)
#include <mach/mach.h>
#elif defined(Q_OS_WIN)
#include <windows.h>
#include <psapi.h>
#elif defined(Q_OS_LINUX)
#include <unistd.h>
#include <cstdio>
#endif
#include <QStorageInfo>
#include <libtorrent/torrent_info.hpp>
#include <libtorrent/version.hpp>
#include <libtorrent/magnet_uri.hpp>
#include <libtorrent/create_torrent.hpp>
#include <libtorrent/bencode.hpp>
#include <libtorrent/write_resume_data.hpp>
#include <libtorrent/read_resume_data.hpp>
#include <libtorrent/session_params.hpp>
#include <libtorrent/alert_types.hpp>
#include <sstream>
#include <libtorrent/peer_info.hpp>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QFile>
#include <QSaveFile>
#include <QUrl>
#include <QSettings>
#include <QNetworkInterface>
#include <QDateTime>
#include <QTextStream>
#include <QRegularExpression>
#include <libtorrent/ip_filter.hpp>
#include <boost/asio/ip/address.hpp>
#ifdef BAT_LIBTORRENT_FORK
#include <libtorrent/aux_/ip_helpers.hpp>   // fork-only geo-locality hook
#endif

// DHT routing table persisted across runs — a warm table means peer discovery
// (and the download ramp) starts fast instead of re-bootstrapping the DHT from
// scratch every launch (~30-60s cold). Path sits next to the resume data.
static QString dhtStatePath()
{
    return QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation))
           .filePath(QStringLiteral("resume/session_state.dat"));
}

static lt::session_params loadStartupParams()
{
    QFile f(dhtStatePath());
    if (f.open(QIODevice::ReadOnly)) {
        const QByteArray data = f.readAll();
        if (!data.isEmpty()) {
            try {
                return lt::read_session_params(
                    lt::span<const char>(data.constData(), data.size()),
                    lt::session_handle::save_dht_state);
            } catch (...) { /* corrupt/incompatible state — start fresh */ }
        }
    }
    return lt::session_params();
}

SessionManager::SessionManager(QObject *parent)
    : IEngine(parent), m_session(loadStartupParams())
{
    m_extractor = new ArchiveExtractor(this);
    connect(m_extractor, &ArchiveExtractor::extractionStarted,
            this, &SessionManager::extractionStarted);
    connect(m_extractor, &ArchiveExtractor::extractionCompleted,
            this, &SessionManager::extractionCompleted);
    connect(m_extractor, &ArchiveExtractor::extractError,
            this, &SessionManager::torrentError);

    lt::settings_pack pack;
    pack.set_int(lt::settings_pack::alert_mask,
                 lt::alert_category::status | lt::alert_category::error
                 | lt::alert_category::storage
                 // port_mapping delivers portmap/portmap_error alerts used by
                 // the listen-port reachability indicator.
                 | lt::alert_category::port_mapping
                 // piece_progress delivers piece_finished_alert; we use it
                 // to opportunistically save resume data (rate-limited to
                 // once per minute per torrent) so a crash mid-download
                 // doesn't force a full re-hash on next launch.
                 | lt::alert_category::piece_progress
                 // file_progress is required for file_completed_alert —
                 // without it the alert is filtered and the .!bt suffix
                 // never gets stripped when a file finishes.
                 | lt::alert_category::file_progress);

    // Enable DHT for trackerless torrents / magnet links
    pack.set_bool(lt::settings_pack::enable_dht, true);

    // PEX (Peer Exchange) is enabled by default in libtorrent 2.x

    // Enable LSD (Local Service Discovery) so peers on the same LAN find each
    // other without going through trackers/DHT. Default in qBittorrent and
    // Transmission; substantially helps home / corporate networks.
    pack.set_bool(lt::settings_pack::enable_lsd, true);

    // Identify ourselves to peers and trackers — private trackers
    // sometimes reject "LT" defaults; sending our own user-agent + a "BT"
    // fingerprint avoids that whole class of refusal.
    pack.set_str(lt::settings_pack::peer_fingerprint,
                 lt::generate_fingerprint("BT", 2, 5, 3));
    pack.set_str(lt::settings_pack::user_agent, "BATorrent/" APP_VERSION);

    // (.!bt suffix for incomplete files is applied per-file in addTorrent
    //  and stripped on file_completed_alert below — libtorrent doesn't have
    //  a session-wide setting for this.)

    // Enable UPnP and NAT-PMP for automatic port forwarding
    pack.set_bool(lt::settings_pack::enable_upnp, true);
    pack.set_bool(lt::settings_pack::enable_natpmp, true);

    // Stability settings learned from qBittorrent's codebase:
    // - Only 1 torrent rechecks at a time (prevents OOM on 96GB+ torrents)
    // - Generous alert queue so disk-full storms don't silently drop alerts
    // - Explicit checking memory budget (16KB * value, 512 = 8MB)
    // - 10 async I/O threads for disk operations
    pack.set_int(lt::settings_pack::active_checking, 1);
    pack.set_int(lt::settings_pack::alert_queue_size, 1000000);
    pack.set_int(lt::settings_pack::checking_mem_usage, 512);
    pack.set_int(lt::settings_pack::aio_threads, 10);
    pack.set_int(lt::settings_pack::hashing_threads, 2);
    // On libtorrent 2.0's mmap disk backend a large file pool throttles throughput
    // (upstream #6561 — qBT's 5000 cut NVMe speed to a third); 40 matches our UI default.
    pack.set_int(lt::settings_pack::file_pool_size, 40);
    // The default 1 MiB disk write queue is the most common real-world download
    // stall: when it fills, libtorrent stops requesting blocks. 6 MiB keeps the
    // pipe full without a meaningful memory cost.
    pack.set_int(lt::settings_pack::max_queued_disk_bytes, 6 * 1024 * 1024);
    // Deeper outstanding-request pipeline helps high-BDP links (seedboxes, distant CDN/web-seeds).
    pack.set_int(lt::settings_pack::max_out_request_queue, 1500);
#ifdef BAT_LIBTORRENT_FORK
    // Fork patch: fill that pipeline geometrically during slow-start instead of
    // one block per received piece. Measured +9-27% on pipeline-bound transfers
    // (fat link or high-RTT head) and neutral on bandwidth-capped swarms.
    pack.set_bool(lt::settings_pack::piece_request_fast_ramp, true);
#endif
    // Connection tuning: qBT defaults, with a much faster connect ramp so a fresh
    // torrent reaches a healthy peer set in seconds instead of slowly climbing.
    pack.set_int(lt::settings_pack::connections_limit, 500);
    pack.set_int(lt::settings_pack::connection_speed, 150);
    // Burst of outgoing connections fired the instant a torrent is added — this is
    // the biggest lever on the slow first-30s ramp (default 30 trickles in). Capped
    // by peers actually available, so it's harmless on small swarms.
    pack.set_int(lt::settings_pack::torrent_connect_boost, 100);
    pack.set_int(lt::settings_pack::unchoke_slots_limit, 20);
    // Opt-in (default off): stop uTP from throttling our own TCP peers. Faster on
    // a dedicated fat link; can add bufferbloat on a shared line, so it's gated.
    pack.set_int(lt::settings_pack::mixed_mode_algorithm,
                 QSettings("BATorrent", "BATorrent").value("preferTcp", false).toBool()
                     ? lt::settings_pack::prefer_tcp
                     : lt::settings_pack::peer_proportional);
    // Prefer RC4 encryption (like qBittorrent) — some private trackers
    // penalize clients that accept plaintext.
    pack.set_int(lt::settings_pack::allowed_enc_level,
                 lt::settings_pack::pe_rc4);
    pack.set_bool(lt::settings_pack::prefer_rc4, true);
    // Don't fall back to a random port if the configured one is busy —
    // the user explicitly set a port for their port-forward rule.
    pack.set_bool(lt::settings_pack::listen_system_port_fallback, false);

    // Enable protocol encryption (prefer encrypted, allow unencrypted fallback)
    pack.set_int(lt::settings_pack::out_enc_policy, lt::settings_pack::pe_enabled);
    pack.set_int(lt::settings_pack::in_enc_policy, lt::settings_pack::pe_enabled);

    m_session.apply_settings(pack);

    // Load global stats from previous sessions
    QSettings settings("BATorrent", "BATorrent");
    m_globalDownBase = settings.value("globalDownloaded", 0).toLongLong();
    m_globalUpBase = settings.value("globalUploaded", 0).toLongLong();

    // Record session start time
    settings.setValue("sessionStartTime", QDateTime::currentSecsSinceEpoch());

    // Load stop-seeding rules
    m_stopAfterDownload = settings.value("stopAfterDownload", false).toBool();
    m_maxSeedSeconds = settings.value("maxSeedSeconds", 0).toLongLong();
    m_autoCompleteSeconds = settings.value("autoCompleteSeconds", 0).toLongLong();
    m_anonymousMode = settings.value("anonymousMode", false).toBool();
    m_forceIpv4 = settings.value("forceIpv4", false).toBool();
    m_ptMode = settings.value("ptMode", false).toBool();
    for (const auto &h : settings.value("forceStartHashes").toStringList())
        m_forceStartHashes.insert(h);
    m_blockLeechers = settings.value("blockLeechers", false).toBool();
    // Apply persisted advanced settings on startup
    setAdvancedSettings(advancedSettings());

    // Torrent export + run on complete + watched folder + temp path
    m_torrentExportDir = settings.value("torrentExportDir").toString();
    m_tempPath = settings.value("tempPath").toString();
    m_autoExtract = settings.value("autoExtract", false).toBool();
    m_warnSuspiciousFiles = settings.value("warnSuspiciousFiles", true).toBool();
    m_autoDefenderExclude = settings.value("autoDefenderExclude", false).toBool();
    for (const auto &h : settings.value("securityWarned").toStringList())
        m_securityWarned.insert(h);
    for (const auto &r : settings.value("defenderExcludedRoots").toStringList())
        m_defenderExcludedRoots.insert(r);
    m_autoExtractDelete = settings.value("autoExtractDelete", false).toBool();
    m_extractPasswords = settings.value("extractPasswords").toStringList();
    m_contentLayout = settings.value("contentLayout", 0).toInt();
    m_excludedFilePatterns = settings.value("excludedFilePatterns").toStringList();
    m_runOnComplete = settings.value("runOnComplete").toString();
    QString watchPath = settings.value("watchedFolder").toString();
    if (!watchPath.isEmpty()) setWatchedFolder(watchPath);

    if (m_anonymousMode || m_forceIpv4 || m_ptMode) {
        // Apply immediately so the first session starts with the right pack;
        // setters above persist to QSettings, but the in-memory pack was
        // built before those values loaded.
        if (m_anonymousMode) setAnonymousMode(true);
        if (m_forceIpv4)     setForceIpv4(true);
        if (m_ptMode)        setPtMode(true);
    }
    for (const auto &h : settings.value("completedTorrents").toStringList())
        m_completedTorrents.insert(h);
    m_completedAtStartup = m_completedTorrents;   // freeze: don't re-notify these

    // Load categories
    settings.beginGroup("categories");
    for (const auto &key : settings.childKeys())
        m_categories[key] = settings.value(key).toString();
    settings.endGroup();

    settings.beginGroup("categorySavePaths");
    for (const auto &key : settings.childKeys())
        m_categorySavePaths[key] = settings.value(key).toString();
    settings.endGroup();

    settings.beginGroup("torrentTags");
    for (const auto &key : settings.childKeys()) {
        const QString joined = settings.value(key).toString();
        if (!joined.isEmpty())
            m_torrentTags[key] = joined.split(',', Qt::SkipEmptyParts);
    }
    settings.endGroup();

    // Load per-torrent stop-after overrides
    settings.beginGroup("torrentStopAfter");
    for (const auto &key : settings.childKeys())
        m_perTorrentStopAfter[key] = settings.value(key).toInt();
    settings.endGroup();

    settings.beginGroup("torrentMaxSeed");
    for (const auto &key : settings.childKeys())
        m_perTorrentMaxSeed[key] = settings.value(key).toLongLong();
    settings.endGroup();

    settings.beginGroup("torrentDownLimit");
    for (const auto &key : settings.childKeys())
        m_perTorrentDownLimit[key] = settings.value(key).toInt();
    settings.endGroup();

    settings.beginGroup("torrentUpLimit");
    for (const auto &key : settings.childKeys())
        m_perTorrentUpLimit[key] = settings.value(key).toInt();
    settings.endGroup();

    // Restore persisted speed / queue / network / VPN / proxy preferences. The
    // matching setters write these on change; without this replay they reset to
    // defaults every launch (the "settings don't save" bug). outgoingInterface
    // must run before listenPort so the port binds onto the chosen interface.
    if (settings.contains("downloadLimit")) setDownloadLimit(settings.value("downloadLimit").toInt());
    if (settings.contains("uploadLimit"))   setUploadLimit(settings.value("uploadLimit").toInt());
    setMaxActiveDownloads(settings.value("maxActiveDownloads", 0).toInt());
    setSeedRatioLimit(settings.value("seedRatioLimit", 0.0).toFloat());
    setAltSpeedLimits(settings.value("altDownLimit", 0).toInt(),
                      settings.value("altUpLimit", 0).toInt());
    setScheduleFromHour(settings.value("scheduleFromHour", 0).toInt());
    setScheduleToHour(settings.value("scheduleToHour", 0).toInt());
    setScheduleDays(settings.value("scheduleDays", 0).toInt());
    setSchedulerEnabled(settings.value("schedulerEnabled", false).toBool());
    if (settings.contains("maxConnections")) setMaxConnections(settings.value("maxConnections").toInt());
    if (settings.contains("dhtEnabled"))     setDhtEnabled(settings.value("dhtEnabled").toBool());
    if (settings.contains("utpEnabled"))     setUtpEnabled(settings.value("utpEnabled").toBool());
    if (settings.contains("encryptionMode")) setEncryptionMode(settings.value("encryptionMode").toInt());
    const QString persistedIface = settings.value("outgoingInterface").toString();
    if (!persistedIface.isEmpty()) setOutgoingInterface(persistedIface);
    if (settings.value("listenPort", 0).toInt() > 0) setListenPort(settings.value("listenPort").toInt());
    setKillSwitchEnabled(settings.value("killSwitchEnabled", false).toBool());
    setAutoResumeOnReconnect(settings.value("autoResumeOnReconnect", false).toBool());
    m_proxyLeakProof = settings.value("proxyLeakProof", true).toBool();
    if (settings.value("proxyType", 0).toInt() != 0)
        setProxySettings(settings.value("proxyType").toInt(), settings.value("proxyHost").toString(),
                         settings.value("proxyPort").toInt(), settings.value("proxyUser").toString(),
                         settings.value("proxyPass").toString());
    setAutoMove(settings.value("autoMoveEnabled", false).toBool(),
                settings.value("autoMovePath").toString());
    m_preallocate = settings.value("preallocate", false).toBool();
    m_autoRecheck = settings.value("autoRecheck", false).toBool();

    // Crash-loop guard. The only place a bad/corrupt resume file can hard-crash
    // the process is the synchronous parse inside loadResumeData(). So we raise
    // the flag right before it and lower it right after: if the parse crashes,
    // the flag survives to the next launch and we skip resume data once to
    // recover. Crucially the window is just loadResumeData()'s duration (ms) —
    // an earlier version only cleared the flag after 15s of uptime, so quitting
    // before then looked like a crash and made every torrent vanish for a launch.
    migrateLegacyResumeData();   // pull torrents from the pre-3.0 data dir if needed

    const bool prevCrash = settings.value("startupInProgress", false).toBool();
    if (prevCrash) {
        qWarning("Crash loop detected — skipping resume data to allow recovery. "
                 "Previously active torrents will reappear after a clean restart.");
        settings.setValue("startupInProgress", false);
        settings.sync();
    } else {
        settings.setValue("startupInProgress", true);
        settings.sync();
        loadResumeData();
        // Survived the parse — clear immediately; not crash-gated on uptime.
        settings.setValue("startupInProgress", false);
        settings.sync();
    }

    m_statsHistory = std::make_unique<StatsHistory>(
        QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation))
            .filePath(QStringLiteral("stats-history.json")));

#ifdef BAT_LIBTORRENT_FORK
    // Same-country peer biasing (fork engine hook). The DB loads async; our own
    // country comes from external_ip_alert. Whichever lands last installs it.
    connect(&m_geoIp, &GeoIpProvider::loaded, this, &SessionManager::tryInstallGeoClassifier);
    m_geoIp.start(QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation))
                      .filePath(QStringLiteral("geoip")));
#endif

    connect(&m_updateTimer, &QTimer::timeout, this, &SessionManager::updateStats);
    m_updateTimer.start(1000);
}

SessionManager::~SessionManager()
{
#ifdef BAT_LIBTORRENT_FORK
    // Drop the geo classifier before the session tears down so no ranking call
    // reaches into half-destroyed state. The lambda owns its own DB ref anyway.
    lt::aux::set_geo_local_fn(nullptr);
#endif
    // On shutdown we want a synchronous flush so resume files are durable
    // before Qt tears the app down. The periodic 5-min timer and the
    // piece_finished_alert path both call saveResumeData() (non-blocking).
    flushResumeDataBlocking(5000);
    // Persist the DHT routing table for a warm start next launch (faster ramp).
    try {
        const lt::session_params p = m_session.session_state(lt::session_handle::save_dht_state);
        const std::vector<char> buf = lt::write_session_params_buf(p, lt::session_handle::save_dht_state);
        QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)).mkpath(QStringLiteral("resume"));
        QFile f(dhtStatePath());
        if (f.open(QIODevice::WriteOnly))
            f.write(buf.data(), static_cast<qint64>(buf.size()));
    } catch (...) {}
    Logger::instance().log(Logger::Info, QStringLiteral("--- log closed ---"));
}

bool SessionManager::isDuplicate(const lt::info_hash_t &ih) const
{
    for (const auto &h : m_torrents) {
        if (h.is_valid() && h.info_hashes() == ih)
            return true;
    }
    return false;
}

void SessionManager::addTorrent(const QString &filePath, const QString &savePath)
{
    try {
        qDebug() << "[session] addTorrent:" << filePath << "save:" << savePath;
        // Auto-copy .torrent to export directory (backup archive)
        if (!m_torrentExportDir.isEmpty()) {
            QDir exportDir(m_torrentExportDir);
            if (exportDir.exists()) {
                QFileInfo fi(filePath);
                QFile::copy(filePath, exportDir.filePath(fi.fileName()));
            }
        }
        lt::add_torrent_params atp;
        atp.ti = std::make_shared<lt::torrent_info>(filePath.toStdString());
        // Reject duplicates by checking our own list (not m_session.find_torrent,
        // whose handle lingers after the async remove_torrent — that would wrongly
        // block a legit re-add right after removal). m_torrents is erased
        // synchronously on remove, so it reflects the visible state.
        if (atp.ti && isDuplicate(atp.ti->info_hashes())) {
            qDebug() << "[session] addTorrent: duplicate ignored";
            return;
        }
        atp.save_path = savePath.toStdString();
        atp.flags &= ~(lt::torrent_flags::auto_managed
                       | lt::torrent_flags::paused);
        if (atp.ti && atp.ti->priv()) {
            atp.flags |= lt::torrent_flags::disable_dht
                       | lt::torrent_flags::disable_lsd
                       | lt::torrent_flags::disable_pex;
        }
        applyContentLayout(atp);
        applyExcludedPatterns(atp);
        applyIncompleteSuffix(atp);
        applyStorageMode(atp);

        // Temp path: download to temp dir, move to real path on finish
        if (!m_tempPath.isEmpty() && QDir(m_tempPath).exists()) {
            std::string hash = (std::ostringstream() << atp.ti->info_hashes().get_best()).str();
            m_torrentIntendedPath[QString::fromStdString(hash)] = savePath;
            atp.save_path = m_tempPath.toStdString();
        }

        lt::torrent_handle h = m_session.add_torrent(atp);
        m_torrents.push_back(h);
        incrementTorrentCount();
        if (m_autoRecheck && h.is_valid()) h.force_recheck();   // verify pre-existing data on disk
        stageResumeSave(h);   // persist now — an idle 0%/no-peer torrent never

        emit torrentAdded(static_cast<int>(m_torrents.size()) - 1);
        scanTorrentForThreats(h, QString::fromStdString(h.status().name));   // .torrent: metadata is ready now
        maybeAutoExcludeDefender(QString::fromStdString(atp.save_path));
    } catch (const std::exception &e) {
        emit torrentError(QString::fromStdString(e.what()));
    }
}

void SessionManager::addTorrentWithPriorities(const QString &filePath,
                                                const QString &savePath,
                                                const std::vector<int> &filePriorities)
{
    try {
        lt::add_torrent_params atp;
        atp.ti = std::make_shared<lt::torrent_info>(filePath.toStdString());
        if (atp.ti && isDuplicate(atp.ti->info_hashes())) {
            qDebug() << "[session] addTorrentWithPriorities: duplicate ignored";
            return;
        }
        atp.save_path = savePath.toStdString();
        atp.flags &= ~(lt::torrent_flags::auto_managed
                       | lt::torrent_flags::paused);
        if (atp.ti && atp.ti->priv()) {
            atp.flags |= lt::torrent_flags::disable_dht
                       | lt::torrent_flags::disable_lsd
                       | lt::torrent_flags::disable_pex;
        }
        atp.file_priorities.reserve(filePriorities.size());
        for (int p : filePriorities) {
            atp.file_priorities.push_back(
                static_cast<lt::download_priority_t>(static_cast<std::uint8_t>(p)));
        }
        applyContentLayout(atp);
        applyExcludedPatterns(atp);
        applyIncompleteSuffix(atp);
        applyStorageMode(atp);

        if (!m_tempPath.isEmpty() && QDir(m_tempPath).exists()) {
            std::string hash = (std::ostringstream() << atp.ti->info_hashes().get_best()).str();
            m_torrentIntendedPath[QString::fromStdString(hash)] = savePath;
            atp.save_path = m_tempPath.toStdString();
        }

        lt::torrent_handle h = m_session.add_torrent(atp);
        m_torrents.push_back(h);
        incrementTorrentCount();
        if (m_autoRecheck && h.is_valid()) h.force_recheck();   // verify pre-existing data on disk
        stageResumeSave(h);   // persist immediately (see addTorrent)
        emit torrentAdded(static_cast<int>(m_torrents.size()) - 1);
        scanTorrentForThreats(h, QString::fromStdString(h.status().name));   // .torrent: metadata is ready now
        maybeAutoExcludeDefender(QString::fromStdString(atp.save_path));
    } catch (const std::exception &e) {
        emit torrentError(QString::fromStdString(e.what()));
    }
}

void SessionManager::applyIncompleteSuffix(lt::add_torrent_params &atp)
{
    if (!atp.ti) return; // magnet without metadata yet; handled after fetch
    const auto &files = atp.ti->files();
    for (lt::file_index_t i(0); i < files.end_file(); ++i) {
        std::string original = files.file_path(i);
        if (original.size() >= 4
            && original.compare(original.size() - 4, 4, ".!bt") == 0)
            continue; // already suffixed (resume data round-trip)
        atp.renamed_files[i] = original + ".!bt";
    }
}

void SessionManager::addMagnet(const QString &uri, const QString &savePath,
                               const QString &coverHint, int coverType)
{
    try {
        qDebug() << "[session] addMagnet:" << uri.left(80) << "save:" << savePath;
        lt::add_torrent_params atp = lt::parse_magnet_uri(uri.toStdString());
        atp.save_path = savePath.toStdString();
        atp.flags &= ~(lt::torrent_flags::auto_managed
                       | lt::torrent_flags::paused);

        // Real hash from the URI (known even before metadata, unlike a magnet's
        // torrent_status which reports all-zeros until then).
        const QString realHash = QString::fromStdString(
            (std::ostringstream() << atp.info_hashes.get_best()).str());

        if (!coverHint.isEmpty()) {
            atp.name = coverHint.toStdString();        // instant display name
            m_coverHints[realHash] = CoverHint{coverHint, coverType};
        }

        if (!m_tempPath.isEmpty() && QDir(m_tempPath).exists()) {
            m_torrentIntendedPath[realHash] = savePath;
            atp.save_path = m_tempPath.toStdString();
        }
        applyStorageMode(atp);

        lt::torrent_handle h = m_session.add_torrent(atp);
        m_torrents.push_back(h);
        m_magnetAddedAt[h] = QDateTime::currentSecsSinceEpoch();
        m_magnetHashes[h] = realHash;
        incrementTorrentCount();

        emit torrentAdded(static_cast<int>(m_torrents.size()) - 1);
        maybeAutoExcludeDefender(QString::fromStdString(atp.save_path));   // scan happens on metadata_received
    } catch (const std::exception &e) {
        emit torrentError(QString::fromStdString(e.what()));
    }
}

SessionManager::CoverHint SessionManager::takeCoverHint(const QString &hash)
{
    return m_coverHints.take(hash);
}

QStringList SessionManager::torrentFileNames(int index) const
{
    if (index < 0 || index >= static_cast<int>(m_torrents.size())) return {};
    const auto &h = m_torrents[index];
    if (!h.is_valid()) return {};
    auto ti = h.torrent_file();
    if (!ti) return {};
    QStringList out;
    const auto &fs = ti->files();
    for (lt::file_index_t i(0); i < fs.end_file(); ++i)
        out << QString::fromStdString(fs.file_path(i));
    return out;
}

void SessionManager::checkMagnetTimeouts()
{
    if (m_magnetTimeoutSeconds <= 0 || m_magnetAddedAt.empty())
        return;
    const qint64 now = QDateTime::currentSecsSinceEpoch();
    for (auto it = m_magnetAddedAt.begin(); it != m_magnetAddedAt.end(); ) {
        const lt::torrent_handle &h = it->first;
        if (!h.is_valid()) { it = m_magnetAddedAt.erase(it); continue; }
        const lt::torrent_status st = cachedStatus(h);
        if (st.has_metadata) {
            // Done — metadata arrived, stop watching this magnet.
            it = m_magnetAddedAt.erase(it);
            continue;
        }
        if (now - it->second >= m_magnetTimeoutSeconds) {
            QString name = QString::fromStdString(st.name);
            if (name.isEmpty()) name = "magnet";
            emit torrentError(QString("Metadata timeout: %1").arg(name));
            // Remove the dead magnet so it doesn't sit forever consuming a
            // queue slot.
            int idx = -1;
            for (int i = 0; i < static_cast<int>(m_torrents.size()); ++i) {
                if (m_torrents[i] == h) { idx = i; break; }
            }
            it = m_magnetAddedAt.erase(it);
            if (idx >= 0) removeTorrent(idx, false);
            continue;
        }
        ++it;
    }
}

void SessionManager::removeTorrent(int index, bool deleteFiles, bool permanent)
{
    qDebug() << "[session] removeTorrent index:" << index << "deleteFiles:" << deleteFiles;
    if (index < 0 || index >= static_cast<int>(m_torrents.size()))
        return;

    lt::torrent_handle h = m_torrents[index];
    if (!h.is_valid()) {
        m_torrents.erase(m_torrents.begin() + index);
        emit torrentRemoved(index);
        return;
    }

    try {
        lt::torrent_status st = h.status();
        QString hash = QString::fromStdString(
            (std::ostringstream() << st.info_hashes.get_best()).str());
        QDir dir(resumeDataDir());
        QDir removedDir(QFileInfo(dir, "../removed").absoluteFilePath());
        if (!removedDir.exists()) removedDir.mkpath(".");
        QFile::remove(removedDir.filePath(hash + ".resume"));
        QFile::rename(dir.filePath(hash + ".resume"),
                      removedDir.filePath(hash + ".resume"));
        QSettings meta(removedDir.filePath("history.ini"), QSettings::IniFormat);
        meta.beginGroup(hash);
        meta.setValue("name", QString::fromStdString(st.name));
        meta.setValue("size", static_cast<qint64>(st.total_wanted));
        meta.setValue("removedAt", QDateTime::currentSecsSinceEpoch());
        meta.endGroup();
        meta.beginGroup("");
        QStringList groups = meta.childGroups();
        if (groups.size() > 50) {
            QList<QPair<qint64, QString>> sorted;
            for (const QString &g : groups) {
                meta.beginGroup(g);
                sorted.append({meta.value("removedAt").toLongLong(), g});
                meta.endGroup();
            }
            std::sort(sorted.begin(), sorted.end());
            for (int i = 0; i < sorted.size() - 50; ++i) {
                meta.remove(sorted[i].second);
                QFile::remove(removedDir.filePath(sorted[i].second + ".resume"));
            }
        }
        meta.endGroup();
        m_perTorrentStopAfter.remove(hash);
        m_perTorrentMaxSeed.remove(hash);
        if (m_perTorrentDownLimit.remove(hash) || m_perTorrentUpLimit.remove(hash)) {
            QSettings s("BATorrent", "BATorrent");
            s.remove("torrentDownLimit/" + hash);
            s.remove("torrentUpLimit/" + hash);
        }
        if (m_completedTorrents.remove(hash))
            saveCompletedSet();
        if (m_forceStartHashes.remove(hash)) {
            QSettings("BATorrent", "BATorrent").setValue(
                "forceStartHashes", QStringList(m_forceStartHashes.values()));
        }
        if (m_torrentTags.remove(hash))
            QSettings("BATorrent", "BATorrent").remove("torrentTags/" + hash);
        m_removedHashes.insert(hash);
        if (m_removedHashes.size() > 500) m_removedHashes.clear();

        m_globalDownBase += st.total_payload_download;
        m_globalUpBase += st.total_payload_upload;

        m_queuePaused.erase(h);
        m_killSwitchPaused.erase(h);
        m_statusCache.erase(h);
        m_lastResumeSaveAt.erase(h);
        m_lastFastAt.erase(h);
        m_pendingResumeStripCheck.erase(h);
        m_magnetAddedAt.erase(h);

        // "delete files" sends the data to the OS trash instead of erasing it —
        // recoverable removal is a safety net users expect from a desktop app.
        // The move runs shortly after remove_torrent so libtorrent has released
        // its file handles (Windows can't rename open files). Anything
        // moveToTrash can't handle is left on disk rather than force-deleted.
        QStringList trashTargets;
        if (deleteFiles) {
            const QString savePath = QString::fromStdString(st.save_path);
            QSet<QString> tops;
            if (auto ti = h.torrent_file()) {
                const auto &fs = ti->files();
                for (const auto i : fs.file_range()) {
                    QString p = QString::fromStdString(std::string(fs.file_path(i)));
                    // libtorrent reports backslash paths on Windows; normalize so
                    // multi-file torrents resolve to their top-level folder instead
                    // of being trashed file-by-file (folder left behind).
                    p.replace(QLatin1Char('\\'), QLatin1Char('/'));
                    const int slash = p.indexOf(QLatin1Char('/'));
                    tops.insert(slash > 0 ? p.left(slash) : p);
                }
            } else {
                tops.insert(QString::fromStdString(st.name));
            }
            for (const QString &t : tops) {
                if (t.isEmpty()) continue;
                // the in-memory file_path may already carry the incomplete-file
                // ".!bt" suffix (rename_file persists on the handle) — normalize
                // to the base name and target both on-disk variants
                QString base = t;
                if (base.endsWith(QLatin1String(".!bt"))) base.chop(4);
                trashTargets << QDir(savePath).filePath(base)
                             << QDir(savePath).filePath(base + QStringLiteral(".!bt"));
            }
            // also the hidden partial-pieces sidecar (.{hash}.parts) — otherwise it
            // lingers orphaned in the save folder after a remove-with-files
            trashTargets << QDir(savePath).filePath(QStringLiteral(".") + hash + QStringLiteral(".parts"));
        }

        // An actively-downloading torrent still has its files open when we ask
        // libtorrent to drop it; on Windows moveToTrash then hits a sharing
        // violation and the data is silently left on disk. Pausing first starts
        // the handle release immediately, and we retry on a backoff (~27s total)
        // until the handles are gone instead of giving up after one attempt.
        if (deleteFiles) h.pause();
        m_session.remove_torrent(h, {});
        if (!trashTargets.isEmpty()) {
            if (permanent) scheduleDelete(trashTargets, 0);
            else scheduleTrash(trashTargets, 0);
        }
    } catch (const std::exception &e) {
        qWarning() << "[session] removeTorrent exception:" << e.what();
    }
    m_torrents.erase(m_torrents.begin() + index);
    emit torrentRemoved(index);
}

void SessionManager::pauseTorrent(int index)
{
    qDebug() << "[session] pauseTorrent index:" << index;
    if (index < 0 || index >= static_cast<int>(m_torrents.size()))
        return;
    m_torrents[index].pause();
    // persist the pause now — periodic/shutdown saves can run before this and
    // otherwise the torrent reloads un-paused (resumes downloading on its own)
    if (m_torrents[index].is_valid())
        m_torrents[index].save_resume_data(lt::torrent_handle::save_info_dict);
}

void SessionManager::resumeTorrent(int index)
{
    qDebug() << "[session] resumeTorrent index:" << index;
    if (index < 0 || index >= static_cast<int>(m_torrents.size()))
        return;
    // Resume on a completed torrent un-marks it — the user is explicitly
    // asking it to participate again, so the "frozen" flag has to clear.
    unmarkCompleted(index);
    m_torrents[index].resume();
    if (m_torrents[index].is_valid())
        m_torrents[index].save_resume_data(lt::torrent_handle::save_info_dict);
}

void SessionManager::pauseAll()
{
    qDebug() << "[session] pauseAll";
    for (auto &h : m_torrents) {
        h.pause();
        if (h.is_valid()) h.save_resume_data(lt::torrent_handle::save_info_dict);
    }
}

void SessionManager::resumeAll()
{
    qDebug() << "[session] resumeAll";
    for (auto &h : m_torrents) {
        h.resume();
        if (h.is_valid()) h.save_resume_data(lt::torrent_handle::save_info_dict);
    }
}

int SessionManager::torrentCount() const
{
    return static_cast<int>(m_torrents.size());
}

TorrentInfo SessionManager::torrentAt(int index) const
{
    TorrentInfo info{};
    if (index < 0 || index >= static_cast<int>(m_torrents.size()))
        return info;
    if (!m_torrents[index].is_valid())
        return info;

    lt::torrent_status st = cachedStatus(m_torrents[index]);
    info.handle = m_torrents[index];
    info.name = QString::fromStdString(st.name);
    info.savePath = QString::fromStdString(st.save_path);
    info.totalSize = st.total_wanted;
    info.totalDone = st.total_wanted_done;
    info.progress = st.progress;
    info.numPeers = st.num_peers;
    info.numSeeds = st.num_seeds;
    info.stateString = stateToString(st.state);
    info.paused = (st.flags & lt::torrent_flags::paused) != lt::torrent_flags_t{};
    QString hash;
    if (st.has_metadata)
        hash = QString::fromStdString(
            (std::ostringstream() << st.info_hashes.get_best()).str());

    if (!hash.isEmpty())
        info.completed = m_completedTorrents.contains(hash);

    if (info.completed) {
        info.stateString = tr_("state_completed");
        info.downloadRate = 0;
        info.uploadRate = 0;
    } else if (info.paused) {
        info.stateString = tr_("state_paused");
        info.downloadRate = 0;
        info.uploadRate = 0;
    } else {
        info.downloadRate = st.download_rate;
        info.uploadRate = st.upload_rate;
    }

    // qBittorrent's most-repeated complaint is a silent "stalled" — name the
    // actual blocker so the state cell can explain itself on hover
    if (!info.completed && !info.paused && info.progress < 1.0f
            && st.state == lt::torrent_status::downloading
            && info.downloadRate < 1024) {
        if (st.errc)
            info.stateDetail = QString::fromStdString(st.errc.message());
        else if (info.numPeers == 0)
            info.stateDetail = m_dhtEnabled ? tr_("state_no_peers_dht") : tr_("state_no_peers");
        else if (info.numSeeds == 0)
            info.stateDetail = tr_("state_no_seeds");
        else
            info.stateDetail = tr_("state_choked");
    }

    qint64 uploaded = st.total_payload_upload;
    qint64 downloaded = st.total_payload_download;
    info.ratio = downloaded > 0 ? static_cast<float>(uploaded) / static_cast<float>(downloaded) : 0.0f;
    info.totalUploaded = st.all_time_upload;
    info.availability = st.distributed_copies < 0 ? 0.0f : st.distributed_copies;
    info.addedTime = static_cast<qint64>(st.added_time);

    if (!hash.isEmpty()) {
        info.category = m_categories.value(hash);
        info.tags = m_torrentTags.value(hash);
    }

    return info;
}


// Global session configuration (rate limits, listen port, connection cap,
// DHT/uTP/anonymous/PT toggles, encryption, seed-ratio) + leecher-client
// blocking live in sessionmanager_config.cpp.

QString SessionManager::torrentHash(int index) const
{
    if (index < 0 || index >= static_cast<int>(m_torrents.size()))
        return {};
    if (!m_torrents[index].is_valid()) return {};
    lt::torrent_status st = cachedStatus(m_torrents[index]);
    // Magnet links report an all-zeros hash from get_best() until metadata
    // is downloaded. Returning that string would cause every still-resolving
    // magnet to share the same key — categories and per-torrent seeding
    // overrides set on one would silently apply to all others. Use the real
    // per-magnet hash captured from the URI at add time instead, so the row has
    // a stable unique key (cover/name resolve without waiting for metadata).
    if (!st.has_metadata) {
        auto it = m_magnetHashes.find(m_torrents[index]);
        return it != m_magnetHashes.end() ? it->second : QString();
    }
    return QString::fromStdString(
        (std::ostringstream() << st.info_hashes.get_best()).str());
}

// Per-torrent overrides, force-start/super-seeding, the "completed" state +
// auto-complete, and the effective-rule resolvers live in
// sessionmanager_pertorrent.cpp.

void SessionManager::forceRecheck(int index)
{
    qDebug() << "[session] forceRecheck index:" << index;
    if (index < 0 || index >= static_cast<int>(m_torrents.size()))
        return;
    if (!m_torrents[index].is_valid()) return;
    // Clear the cached status BEFORE calling force_recheck. Without this,
    // the update timer reads stale state (e.g. "downloading") while
    // libtorrent is internally in checking_files. That mismatch caused
    // crashes on large torrents (96GB+) because queue management and
    // auto-pause logic acted on inconsistent state.
    m_statusCache.erase(m_torrents[index]);
    m_torrents[index].force_recheck();
}

void SessionManager::forceReannounce(int index)
{
    qDebug() << "[session] forceReannounce index:" << index;
    if (index < 0 || index >= static_cast<int>(m_torrents.size()))
        return;
    if (!m_torrents[index].is_valid()) return;
    m_torrents[index].force_reannounce();
    m_torrents[index].force_dht_announce();
}

QString SessionManager::torrentMagnetUri(int index) const
{
    if (index < 0 || index >= static_cast<int>(m_torrents.size())) return {};
    const auto &h = m_torrents[index];
    if (!h.is_valid()) return {};
    try {
        return QString::fromStdString(lt::make_magnet_uri(h));
    } catch (...) {
        return {};
    }
}

// Fast-resume persistence (save/load/flush/persist, resumeDataDir, legacy
// migration, capture/restoreFromResumeData) lives in
// sessionmanager_persistence.cpp.

QList<RemovedEntry> SessionManager::recentlyRemoved() const
{
    QList<RemovedEntry> out;
    QDir removedDir(QFileInfo(QDir(resumeDataDir()), "../removed").absoluteFilePath());
    if (!removedDir.exists()) return out;
    QSettings meta(removedDir.filePath("history.ini"), QSettings::IniFormat);
    for (const QString &hash : meta.childGroups()) {
        meta.beginGroup(hash);
        RemovedEntry e;
        e.hash = hash;
        e.name = meta.value("name").toString();
        e.totalSize = meta.value("size").toLongLong();
        e.removedAt = meta.value("removedAt").toLongLong();
        e.resumePath = removedDir.filePath(hash + ".resume");
        meta.endGroup();
        if (QFile::exists(e.resumePath)) out.append(e);
    }
    std::sort(out.begin(), out.end(), [](const RemovedEntry &a, const RemovedEntry &b) {
        return a.removedAt > b.removedAt; // newest first
    });
    return out;
}

bool SessionManager::restoreRemoved(const QString &hash)
{
    QDir removedDir(QFileInfo(QDir(resumeDataDir()), "../removed").absoluteFilePath());
    QString path = removedDir.filePath(hash + ".resume");
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return false;
    QByteArray bytes = f.readAll();
    f.close();
    if (!restoreFromResumeData(bytes)) return false;
    // Successful restore — remove the history entry so it doesn't stay
    // there forever after the user re-added.
    QFile::remove(path);
    QSettings meta(removedDir.filePath("history.ini"), QSettings::IniFormat);
    meta.remove(hash);
    return true;
}

void SessionManager::clearRemovedHistory()
{
    QDir removedDir(QFileInfo(QDir(resumeDataDir()), "../removed").absoluteFilePath());
    if (!removedDir.exists()) return;
    for (const QString &f : removedDir.entryList({"*.resume"}, QDir::Files))
        QFile::remove(removedDir.filePath(f));
    QFile::remove(removedDir.filePath("history.ini"));
}

QString SessionManager::torrentHashAt(int index) const
{
    return torrentHash(index);
}

QString SessionManager::torrentRootPath(int index) const
{
    if (index < 0 || index >= static_cast<int>(m_torrents.size()))
        return {};
    const auto &h = m_torrents[index];
    if (!h.is_valid()) return {};
    lt::torrent_status st = cachedStatus(h);
    QString save = QString::fromStdString(st.save_path);

    // Existence checker that also tries the ".!bt" suffix (in-progress
    // rename) AND native separators (Windows backslash → forward-slash
    // mismatch from libtorrent can cause QFileInfo::exists to fail on
    // some configs).
    auto existsOnDisk = [](const QString &path) -> QString {
        if (QFileInfo::exists(path)) return path;
        const QString native = QDir::toNativeSeparators(path);
        if (native != path && QFileInfo::exists(native)) return native;
        if (QFileInfo::exists(path + ".!bt")) return path + ".!bt";
        return {};
    };

    // Strategy 1: file_path(0) — the most reliable source since it comes
    // from the torrent metadata and matches what libtorrent wrote to disk.
    auto ti = h.torrent_file();
    if (ti && ti->num_files() > 0) {
        // libtorrent's file_path uses the native separator ('\' on Windows),
        // so normalize to '/' before stripping — otherwise indexOf('/') misses
        // the folder boundary on Windows and we resolve to file index 0 (an
        // arbitrary .dll / .rNN), selecting it instead of opening the folder.
        QString rel = QDir::fromNativeSeparators(QString::fromStdString(
            ti->files().file_path(lt::file_index_t(0))));
        if (ti->num_files() > 1) {
            const int slash = rel.indexOf(QLatin1Char('/'));
            // Strip to the top-level folder. A multi-file torrent with no
            // common folder (files written straight into save_path) has no
            // slash — leave rel empty so we fall through to save_path rather
            // than selecting an arbitrary first file.
            rel = slash > 0 ? rel.left(slash) : QString();
        }
        if (!rel.isEmpty()) {
            QString found = existsOnDisk(save + QLatin1Char('/') + rel);
            if (!found.isEmpty()) {
                qInfo().noquote() << "[reveal] root via file_path:" << found;
                return found;
            }
        }
    }

    // Strategy 2: torrent display name (st.name). May differ from
    // file_path(0) after renames, sanitization, or cross-platform transfer
    // (Windows strips characters that macOS/Linux kept). Covers the common
    // case where file_path was sanitized but name wasn't, or vice-versa.
    QString name = QString::fromStdString(st.name);
    if (!name.isEmpty()) {
        QString found = existsOnDisk(save + QLatin1Char('/') + name);
        if (!found.isEmpty()) {
            qInfo().noquote() << "[reveal] root via name:" << found;
            return found;
        }
    }

    // Strategy 3: scan save_path for a directory or file whose name
    // case-insensitively matches the torrent display name. Handles partial
    // renames and encoding mismatches (e.g. ñ vs n, full-width chars).
    if (!name.isEmpty()) {
        const QDir dir(save);
        const auto entries = dir.entryList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot);
        for (const QString &entry : entries) {
            if (entry.compare(name, Qt::CaseInsensitive) == 0) {
                qInfo().noquote() << "[reveal] root via dir scan:" << (save + QLatin1Char('/') + entry);
                return save + QLatin1Char('/') + entry;
            }
        }
    }

    // All strategies exhausted — fall back to the save directory itself. If
    // this fires, the torrent's folder/file wasn't found on disk under
    // save_path — the reveal lands in the (possibly huge) save folder.
    qWarning().noquote() << "[reveal] FELL BACK to save_path:" << save
                         << "| torrent name=" << name;
    return save;
}



void SessionManager::updateStats()
{
    // Ask libtorrent to deliver a state_update_alert with fresh statuses for
    // every torrent. The alert lands inside processAlerts() below; until it
    // arrives, m_statusCache may be one tick stale — acceptable trade-off
    // for getting rid of dozens of synchronous status() calls per second.
    m_session.post_torrent_updates();

    processAlerts();
    // daily usage history sample (cheap: cachedStatus sums), every 5s
    {
        static int statsTick = 0;
        if (++statsTick >= 5 && m_statsHistory) {
            statsTick = 0;
            m_statsHistory->recordTransfer(globalDownloaded(), globalUploaded());
        }
    }
    checkSeedRatios();
    checkSeedingLimits();
    checkAutoComplete();
    checkAndBlockLeechers();
    // Disk pressure: warn under 1 GB, and *act* under 512 MB by pausing active
    // downloads (with hysteresis) so we don't fill the disk or corrupt data.
    {
        static qint64 lastDiskWarn = 0;
        const qint64 now = QDateTime::currentSecsSinceEpoch();
        QSettings s("BATorrent", "BATorrent");
        const QString savePath = s.value("lastSavePath",
            QStandardPaths::writableLocation(QStandardPaths::DownloadLocation)).toString();
        QStorageInfo storage(savePath);
        const qint64 freeB = storage.isValid() ? storage.bytesAvailable() : -1;

        if (freeB >= 0 && freeB < 512LL * 1024 * 1024 && !m_diskAutoPaused) {
            int paused = 0;
            for (auto &h : m_torrents) {
                if (!h.is_valid()) continue;
                const lt::torrent_status st = cachedStatus(h);
                const bool active = st.state == lt::torrent_status::downloading
                                 || st.state == lt::torrent_status::downloading_metadata;
                if (active && !(st.flags & lt::torrent_flags::paused)) { h.pause(); ++paused; }
            }
            m_diskAutoPaused = true;   // runtime-only pause (not persisted) — recovers on disk free
            qWarning() << "[session] CRITICAL DISK:" << freeB / (1024*1024) << "MB — paused" << paused << "downloads";
            if (paused > 0) emit torrentError(tr_("warn_disk_autopause").arg(paused).arg(freeB / (1024*1024)));
            lastDiskWarn = now;
        } else if (freeB >= 2LL * 1024 * 1024 * 1024) {
            m_diskAutoPaused = false;   // recovered → re-arm (user decides whether to resume)
        } else if (freeB >= 0 && freeB < 1024LL * 1024 * 1024 && now - lastDiskWarn >= 300) {
            qWarning() << "[session] LOW DISK SPACE:" << freeB / (1024*1024) << "MB remaining on" << savePath;
            emit torrentError(tr_("warn_low_disk").arg(freeB / (1024*1024)));
            lastDiskWarn = now;
        }
    }
    checkInterfaceStatus();
    checkBandwidthSchedule();
    checkMagnetTimeouts();
    checkMemoryGuard();
    enforceDownloadQueue();
    if (!m_torrents.empty())
        emit torrentsUpdated();
}

lt::torrent_status SessionManager::cachedStatus(const lt::torrent_handle &h) const
{
    auto it = m_statusCache.find(h);
    if (it != m_statusCache.end())
        return it->second;
    // Cache miss: brand-new torrent before the first state_update_alert
    // landed. Fall back to a live call (and warm the cache) so the first
    // refresh after add doesn't show "-" everywhere.
    if (h.is_valid()) {
        lt::torrent_status st = h.status();
        m_statusCache[h] = st;
        return st;
    }
    return {};
}

// processAlerts() + every on*() alert handler live in sessionmanager_alerts.cpp.

void SessionManager::checkSeedRatios()
{
    if (m_seedRatioLimit <= 0.0f) return;

    for (auto &h : m_torrents) {
        lt::torrent_status st = cachedStatus(h);
        if (st.state != lt::torrent_status::seeding) continue;
        if (st.flags & lt::torrent_flags::paused) continue;

        // Use payload counters so the pause-at-ratio threshold lines up with
        // the ratio shown to the user and what trackers report.
        float ratio = st.total_payload_download > 0
            ? static_cast<float>(st.total_payload_upload)
              / static_cast<float>(st.total_payload_download)
            : 0.0f;

        if (ratio >= m_seedRatioLimit)
            h.pause();
    }
}

void SessionManager::checkSeedingLimits()
{
    for (auto &h : m_torrents) {
        if (!h.is_valid()) continue;
        lt::torrent_status st = cachedStatus(h);
        if (st.state != lt::torrent_status::seeding) continue;
        if (st.flags & lt::torrent_flags::paused) continue;

        QString hash = QString::fromStdString(
            (std::ostringstream() << st.info_hashes.get_best()).str());
        qint64 maxSec = effectiveMaxSeedSeconds(hash);
        if (maxSec <= 0) continue;

        // st.seeding_duration is a std::chrono::seconds duration tracking
        // total time the torrent has been in the seeding state.
        qint64 seeded = std::chrono::duration_cast<std::chrono::seconds>(
                            st.seeding_duration).count();
        if (seeded >= maxSec)
            h.pause();
    }
}


int SessionManager::importFromQBittorrent(const QString &defaultSavePath)
{
    // qBittorrent stores data in BT_backup:
    //   Linux:   ~/.local/share/qBittorrent/BT_backup/
    //   Windows: %APPDATA%\qBittorrent\BT_backup\
    //   macOS:   ~/Library/Application Support/qBittorrent/BT_backup/
#ifdef Q_OS_WIN
    // GenericDataLocation resolves to %APPDATA% on Windows, which is the
    // parent dir qBittorrent stores its own state under. AppDataLocation
    // appends the app's own name and would land in the wrong place.
    QString btBackup = QDir(QStandardPaths::writableLocation(
        QStandardPaths::GenericDataLocation)).filePath("qBittorrent/BT_backup");
#elif defined(Q_OS_MACOS)
    QString btBackup = QDir::homePath()
        + "/Library/Application Support/qBittorrent/BT_backup";
#else
    QString btBackup = QDir::homePath() + "/.local/share/qBittorrent/BT_backup";
#endif
    QDir dir(btBackup);
    if (!dir.exists())
        return 0;

    QStringList torrentFiles = dir.entryList({"*.torrent"}, QDir::Files);
    int imported = 0;

    for (const auto &fileName : torrentFiles) {
        QString torrentPath = dir.filePath(fileName);
        QString baseName = fileName.left(fileName.length() - 8); // remove .torrent
        QString resumePath = dir.filePath(baseName + ".fastresume");

        try {
            lt::add_torrent_params atp;

            // Try to load fastresume data first (contains save_path and state)
            if (QFile::exists(resumePath)) {
                QFile resumeFile(resumePath);
                if (resumeFile.open(QIODevice::ReadOnly)) {
                    QByteArray data = resumeFile.readAll();
                    lt::error_code ec;
                    atp = lt::read_resume_data(
                        lt::span<const char>(data.data(), data.size()), ec);
                    if (ec) {
                        // Fastresume failed, load torrent file instead
                        atp.ti = std::make_shared<lt::torrent_info>(torrentPath.toStdString());
                        atp.save_path = defaultSavePath.toStdString();
                    }
                }
            } else {
                atp.ti = std::make_shared<lt::torrent_info>(torrentPath.toStdString());
                atp.save_path = defaultSavePath.toStdString();
            }

            // If save_path is empty, use default
            if (atp.save_path.empty())
                atp.save_path = defaultSavePath.toStdString();

            // Check if we already have this torrent
            bool duplicate = false;
            for (const auto &h : m_torrents) {
                if (h.is_valid() && h.info_hashes() == atp.info_hashes) {
                    duplicate = true;
                    break;
                }
            }
            if (duplicate) continue;

            lt::torrent_handle h = m_session.add_torrent(atp);
            m_torrents.push_back(h);
            ++imported;
        } catch (const std::exception &e) {
            qWarning() << "importFromQBittorrent skipped" << fileName << ":" << e.what();
        } catch (...) {
            qWarning() << "importFromQBittorrent skipped" << fileName
                       << ": unknown exception";
        }
    }

    if (imported > 0)
        emit torrentsUpdated();

    return imported;
}

QString SessionManager::stateToString(lt::torrent_status::state_t state)
{
    switch (state) {
    case lt::torrent_status::checking_files: return tr_("state_checking");
    case lt::torrent_status::downloading_metadata: return tr_("state_metadata");
    case lt::torrent_status::downloading: return tr_("state_downloading");
    case lt::torrent_status::finished: return tr_("state_finished");
    case lt::torrent_status::seeding: return tr_("state_seeding");
    case lt::torrent_status::checking_resume_data: return tr_("state_checking");
    default: return tr_("state_unknown");
    }
}

qint64 SessionManager::globalDownloaded() const
{
    qint64 sessionDown = 0;
    for (const auto &h : m_torrents) {
        if (!h.is_valid()) continue;
        sessionDown += cachedStatus(h).total_payload_download;
    }
    return m_globalDownBase + sessionDown;
}

qint64 SessionManager::globalUploaded() const
{
    qint64 sessionUp = 0;
    for (const auto &h : m_torrents) {
        if (!h.is_valid()) continue;
        sessionUp += cachedStatus(h).total_payload_upload;
    }
    return m_globalUpBase + sessionUp;
}

float SessionManager::globalRatio() const
{
    qint64 down = m_globalDownBase, up = m_globalUpBase;
    for (const auto &h : m_torrents) {
        if (!h.is_valid()) continue;
        auto st = cachedStatus(h);
        down += st.total_payload_download;
        up += st.total_payload_upload;
    }
    return down > 0 ? static_cast<float>(up) / static_cast<float>(down) : 0.0f;
}

QVariantMap SessionManager::statsWrapped(int year) const
{
    QVariantMap out;
    out["year"] = year;
    if (!m_statsHistory) return out;
    const QJsonObject days = m_statsHistory->days();
    const QString prefix = QString::number(year) + QLatin1Char('-');
    qint64 down = 0, up = 0, added = 0, completed = 0, busiestDown = 0;
    int activeDays = 0;
    QString busiestDay;
    QMap<QString, int> cats;
    QVector<qint64> months(12, 0);
    for (auto it = days.begin(); it != days.end(); ++it) {
        if (!it.key().startsWith(prefix)) continue;
        const QJsonObject e = it.value().toObject();
        const qint64 d = qint64(e.value(QStringLiteral("down")).toDouble());
        const qint64 u = qint64(e.value(QStringLiteral("up")).toDouble());
        down += d; up += u;
        added += qint64(e.value(QStringLiteral("added")).toDouble());
        completed += qint64(e.value(QStringLiteral("completed")).toDouble());
        if (d > 0 || u > 0 || e.value(QStringLiteral("added")).toDouble() > 0) ++activeDays;
        if (d > busiestDown) { busiestDown = d; busiestDay = it.key(); }
        const int month = it.key().mid(5, 2).toInt();
        if (month >= 1 && month <= 12) months[month - 1] += d;
        const QJsonObject c = e.value(QStringLiteral("cats")).toObject();
        for (auto ci = c.begin(); ci != c.end(); ++ci) cats[ci.key()] += ci.value().toInt();
    }
    out["down"] = down; out["up"] = up;
    out["added"] = added; out["completed"] = completed;
    out["activeDays"] = activeDays;
    out["busiestDay"] = busiestDay; out["busiestDown"] = busiestDown;

    QList<QPair<QString, int>> sorted;
    for (auto ci = cats.begin(); ci != cats.end(); ++ci) sorted.append({ ci.key(), ci.value() });
    std::sort(sorted.begin(), sorted.end(), [](const QPair<QString,int> &a, const QPair<QString,int> &b) {
        return a.second > b.second;
    });
    QVariantList catList;
    for (const auto &p : sorted) { QVariantMap cm; cm["name"] = p.first; cm["count"] = p.second; catList << cm; }
    out["categories"] = catList;

    QVariantList monthList;
    for (int i = 0; i < 12; ++i) monthList << QVariant(qlonglong(months[i]));
    out["months"] = monthList;
    return out;
}

qint64 SessionManager::sessionDownloaded() const
{
    qint64 sessionDown = 0;
    for (const auto &h : m_torrents) {
        if (!h.is_valid()) continue;
        sessionDown += cachedStatus(h).total_payload_download;
    }
    return sessionDown;
}

qint64 SessionManager::sessionUploaded() const
{
    qint64 sessionUp = 0;
    for (const auto &h : m_torrents) {
        if (!h.is_valid()) continue;
        sessionUp += cachedStatus(h).total_payload_upload;
    }
    return sessionUp;
}

DetailedStats SessionManager::detailedStats() const
{
    DetailedStats ds;
    for (const auto &h : m_torrents) {
        if (!h.is_valid()) continue;
        auto st = cachedStatus(h);
        ds.peersCount += st.num_peers;
        ds.totalWasted += st.total_redundant_bytes + st.total_failed_bytes;
    }
    ds.hasIncomingConnections = m_session.is_listening();
    return ds;
}

// Resident set size of this process, in bytes; -1 if it can't be read.
// The app's PRIVATE memory footprint — deliberately NOT the working/resident set.
// libtorrent 2.x maps the download files (mmap storage), so the working set grows
// ~1:1 with what's being downloaded as file-cache pages go resident. Those pages
// are reclaimable by the OS on demand, not a leak — but the old WorkingSetSize /
// resident_size readings counted them, so the memory guard false-paused real
// downloads "every ~1 GB" (Windows user report). Private memory excludes the
// file-backed mmap cache, so the guard only sees genuine allocation growth.
static qint64 currentRssBytes()
{
#if defined(Q_OS_MACOS)
    // phys_footprint == Activity Monitor's "Memory": excludes clean file-backed
    // (mmap) pages, unlike resident_size.
    task_vm_info_data_t info;
    mach_msg_type_number_t count = TASK_VM_INFO_COUNT;
    if (task_info(mach_task_self(), TASK_VM_INFO,
                  reinterpret_cast<task_info_t>(&info), &count) == KERN_SUCCESS)
        return static_cast<qint64>(info.phys_footprint);
    return -1;
#elif defined(Q_OS_WIN)
    // PrivateUsage == private commit (pagefile-backed). File-mapped pages are
    // file-backed, so the libtorrent mmap cache is excluded.
    PROCESS_MEMORY_COUNTERS_EX pmc;
    pmc.cb = sizeof(pmc);
    if (GetProcessMemoryInfo(GetCurrentProcess(),
                             reinterpret_cast<PROCESS_MEMORY_COUNTERS *>(&pmc), sizeof(pmc)))
        return static_cast<qint64>(pmc.PrivateUsage);
    return -1;
#elif defined(Q_OS_LINUX)
    // statm "resident" still counts file-backed pages; acceptable for the guard's
    // coarse purpose, and the Linux mmap path is less prone to the Windows blow-up.
    qint64 rss = -1;
    if (FILE *f = std::fopen("/proc/self/statm", "r")) {
        long total = 0, resident = 0;
        if (std::fscanf(f, "%ld %ld", &total, &resident) == 2 && resident > 0)
            rss = static_cast<qint64>(resident) * sysconf(_SC_PAGESIZE);
        std::fclose(f);
    }
    return rss;
#else
    return -1;
#endif
}

// Safety valve against a runaway allocation bug eating the user's machine.
// Stage 1 (over the cap): pause everything — the likeliest growth source is
// piece/disk buffers — and warn, at most once per 5 min. Recoverable.
// Stage 2 (2× the cap, i.e. the pause didn't help → genuine runaway): save
// state and quit gracefully before the OS starts swapping/OOM-killing.
void SessionManager::checkMemoryGuard()
{
    const int capMB = QSettings("BATorrent", "BATorrent").value("memGuardMB", 8192).toInt();
    if (capMB <= 0) return;   // 0 = disabled

    const qint64 rss = currentRssBytes();
    if (rss < 0) return;
    const qint64 rssMB = rss / (1024 * 1024);

    switch (bat::memGuardEvaluate(rssMB, capMB)) {
    case bat::MemGuardAction::Panic:
        qCritical() << "[session] MEMORY GUARD PANIC: private" << rssMB
                    << "MB >= 2x cap" << capMB << "MB — saving state and quitting";
        saveResumeData();
        QCoreApplication::quit();
        return;
    case bat::MemGuardAction::Pause: {
        static qint64 lastWarn = 0;
        const qint64 now = QDateTime::currentSecsSinceEpoch();
        if (now - lastWarn >= 300) {
            lastWarn = now;
            qWarning() << "[session] MEMORY GUARD: private" << rssMB
                       << "MB >= cap" << capMB << "MB — pausing all torrents";
            pauseAll();
            emit torrentError(tr_("warn_mem_guard").arg(rssMB).arg(capMB));
        }
        return;
    }
    case bat::MemGuardAction::None:
        return;
    }
}

void SessionManager::saveSecurityWarned()
{
    QSettings("BATorrent", "BATorrent")
        .setValue("securityWarned", QStringList(m_securityWarned.values()));
}

// Fault isolation: a torrent that keeps erroring (bad disk sector, vanished
// file, corrupt resume) shouldn't be allowed to thrash forever and drag the
// whole app down. Count clustered errors; once a torrent crosses the threshold
// we pause it and tell the user, instead of retrying endlessly.
void SessionManager::noteTorrentFault(const lt::torrent_handle &h, const QString &name)
{
    if (!h.is_valid()) return;
    const qint64 now = QDateTime::currentSecsSinceEpoch();
    qint64 &last = m_faultLastAt[h];
    int &n = m_faultCount[h];
    if (now - last > 120) n = 0;     // errors must cluster (<2 min apart) to count as thrashing
    last = now;
    if (++n < 8) return;

    lt::torrent_status st = h.status();
    if (!(st.flags & lt::torrent_flags::paused)) {
        h.pause();
        const QString nm = name.isEmpty() ? QString::fromStdString(st.name) : name;
        qWarning() << "[session] fault-isolated torrent (repeated errors):" << nm;
        emit torrentError(tr_("warn_torrent_faulted").arg(nm));
    }
    n = 0;   // reset; if the user fixes it, resumes, and it re-faults, we isolate again
    if (m_faultCount.size() > 500) { m_faultCount.clear(); m_faultLastAt.clear(); }
}

// Opt-in (default off): add a new download root to Defender's exclusions once.
// Keyed by save root so torrents sharing a folder cost a single UAC prompt.
void SessionManager::maybeAutoExcludeDefender(const QString &savePath)
{
#if defined(Q_OS_WIN) && !defined(BAT_STORE_BUILD)
    if (!m_autoDefenderExclude || savePath.isEmpty()) return;
    if (m_defenderExcludedRoots.contains(savePath)) return;
    m_defenderExcludedRoots.insert(savePath);
    QSettings("BATorrent", "BATorrent")
        .setValue("defenderExcludedRoots", QStringList(m_defenderExcludedRoots.values()));
    Defender::addExclusion(savePath);
#else
    Q_UNUSED(savePath);
#endif
}

// Warn-only malware awareness. Runs the local heuristic over the torrent's file
// list and, if it flags anything, raises a single non-blocking warning per
// torrent (never blocks, deletes, or claims "safe"). Silent when clean.
void SessionManager::scanTorrentForThreats(const lt::torrent_handle &h, const QString &name)
{
    if (!m_warnSuspiciousFiles || !h.is_valid()) return;
    auto ti = h.torrent_file();
    if (!ti) return;

    const QString hash = QString::fromStdString(
        (std::ostringstream() << h.info_hashes().get_best()).str());
    if (m_securityWarned.contains(hash)) return;

    const auto &fs = ti->files();
    QList<SuspiciousScan::ScanFile> scanFiles;
    for (lt::file_index_t i(0); i < fs.end_file(); ++i) {
        QString p = QString::fromStdString(fs.file_path(i));
        if (p.endsWith(QLatin1String(".!bt"))) p.chop(4);   // ignore in-progress suffix
        scanFiles.append({ p, static_cast<qint64>(fs.file_size(i)) });
    }

    const auto findings = SuspiciousScan::scan(scanFiles);
    if (findings.isEmpty()) return;

    m_securityWarned.insert(hash);
    saveSecurityWarned();

    QStringList files;
    for (const auto &f : findings) {
        const QString shown = QFileInfo(f.file).fileName();
        if (!files.contains(shown)) files << shown;
    }
    qWarning() << "[session] suspicious files in" << name << ":" << files;
    emit suspiciousFilesDetected(name, files);
}

void SessionManager::scheduleTrash(const QStringList &targets, int attempt)
{
    // back off a little more each round; ~30s budget before we give up and leave
    // the files on disk (never force-delete).
    const int delay = qMin(2000 + attempt * 800, 4000);
    QTimer::singleShot(delay, this, [this, targets, attempt]() {
        QStringList remaining;
        for (const QString &p : targets) {
            if (!QFileInfo::exists(p)) continue;          // gone (trashed, or never there)
            if (!QFile::moveToTrash(p)) remaining << p;   // still locked — try again
        }
        if (remaining.isEmpty()) return;
        if (attempt < 10) scheduleTrash(remaining, attempt + 1);
        else qWarning() << "[session] moveToTrash gave up (files still locked):" << remaining;
    });
}

void SessionManager::scheduleDelete(const QStringList &targets, int attempt)
{
    const int delay = qMin(2000 + attempt * 800, 4000);
    QTimer::singleShot(delay, this, [this, targets, attempt]() {
        QStringList remaining;
        for (const QString &p : targets) {
            QFileInfo fi(p);
            if (!fi.exists()) continue;
            const bool ok = fi.isDir() ? QDir(p).removeRecursively() : QFile::remove(p);
            if (!ok) remaining << p;
        }
        if (remaining.isEmpty()) return;
        if (attempt < 10) scheduleDelete(remaining, attempt + 1);
        else qWarning() << "[session] permanent delete gave up (files still locked):" << remaining;
    });
}

void SessionManager::incrementTorrentCount()
{
    if (m_statsHistory) m_statsHistory->recordAdded();
    QSettings settings("BATorrent", "BATorrent");
    int count = settings.value("totalTorrentsAdded", 0).toInt();
    settings.setValue("totalTorrentsAdded", count + 1);
}

int SessionManager::totalTorrentsAdded() const
{
    QSettings settings("BATorrent", "BATorrent");
    return settings.value("totalTorrentsAdded", 0).toInt();
}

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
    else if (key == "ipFilterPath")        { QString p = v.toString(); if (p.isEmpty()) clearIpFilter(); else loadIpFilter(p); }
    else if (key == "tempPath")            setTempPath(v.toString());
    else if (key == "preallocate")         setPreallocate(v.toBool());
    else if (key == "autoRecheck")         setAutoRecheck(v.toBool());
    else if (key == "contentLayout")       setContentLayout(v.toInt());
    else if (key == "torrentExportDir")    setTorrentExportDir(v.toString());
    else if (key == "extractPasswords") {
        QStringList pw;
        const auto parts = v.toString().split(QRegularExpression(QStringLiteral("[;\\n]")), Qt::SkipEmptyParts);
        for (const QString &p : parts) { QString t = p.trimmed(); if (!t.isEmpty()) pw << t; }
        setExtractPasswords(pw);
    }
    else if (key == "autoExtract")         setAutoExtract(v.toBool());
    else if (key == "autoExtractDelete")   setAutoExtractDelete(v.toBool());
    else if (key == "runOnComplete")       setRunOnComplete(v.toString());
    else if (key == "watchedFolder")       setWatchedFolder(v.toString());
    else if (key == "autoMoveEnabled")     setAutoMove(v.toBool(), autoMovePath());
    else if (key == "autoMovePath")        setAutoMove(autoMoveEnabled(), v.toString());
    else if (key == "autoComplete") {
        const qint64 days[] = {0, 1, 3, 7, 14, 30};
        int i = v.toInt();
        setAutoCompleteSeconds((i >= 0 && i < 6 ? days[i] : 0) * 86400);
    }
    else return false;
    return true;
}

void SessionManager::executeOnComplete(const QString &name, const QString &savePath,
                                       const QString &hash, qint64 totalSize)
{
    if (m_runOnComplete.isEmpty()) return;

    auto shellQuote = [](const QString &s) -> QString {
        QString q = s;
        q.replace(QLatin1Char('\''), QStringLiteral("'\\''"));
        return QLatin1Char('\'') + q + QLatin1Char('\'');
    };

    QString cmd = m_runOnComplete;
    cmd.replace("%N", shellQuote(name));
    cmd.replace("%D", shellQuote(savePath));
    cmd.replace("%H", shellQuote(hash));
    cmd.replace("%Z", shellQuote(QString::number(totalSize)));
    cmd.replace("%F", shellQuote(savePath + "/" + name));
    qDebug() << "[session] executeOnComplete:" << cmd;
    QProcess::startDetached("/bin/sh", {"-c", cmd});
}

void SessionManager::setWatchedFolder(const QString &path)
{
    m_watchedFolder = path;
    QSettings("BATorrent", "BATorrent").setValue("watchedFolder", path);
    if (path.isEmpty()) {
        if (m_watchedFolderTimer) m_watchedFolderTimer->stop();
    } else {
        if (!m_watchedFolderTimer) {
            m_watchedFolderTimer = new QTimer(this);
            connect(m_watchedFolderTimer, &QTimer::timeout, this, &SessionManager::scanWatchedFolder);
        }
        m_watchedFolderTimer->start(10000); // scan every 10s
    }
}

QString SessionManager::watchedFolder() const { return m_watchedFolder; }

void SessionManager::scanWatchedFolder()
{
    if (m_watchedFolder.isEmpty()) return;
    QDir dir(m_watchedFolder);
    if (!dir.exists()) return;
    const auto files = dir.entryList({"*.torrent"}, QDir::Files);
    for (const QString &f : files) {
        const QString path = dir.filePath(f);
        qDebug() << "[session] watched folder: auto-adding" << f;
        // Use the global save path (last used)
        QSettings s("BATorrent", "BATorrent");
        QString savePath = s.value("lastSavePath",
            QStandardPaths::writableLocation(QStandardPaths::DownloadLocation)).toString();
        addTorrent(path, savePath);
        // Move the .torrent to a "processed" subfolder to avoid re-adding
        QDir processed(dir.filePath(".processed"));
        if (!processed.exists()) processed.mkpath(".");
        QFile::rename(path, processed.filePath(f));
    }
}

void SessionManager::setAltSpeedLimits(int downKbps, int upKbps)
{
    m_altDownLimit = downKbps;
    m_altUpLimit = upKbps;
    QSettings st("BATorrent", "BATorrent");
    st.setValue("altDownLimit", downKbps);
    st.setValue("altUpLimit", upKbps);
    // Re-apply live if alt mode is currently active so the new ceiling takes hold.
    if (m_altSpeedsActive) {
        lt::settings_pack pack;
        pack.set_int(lt::settings_pack::download_rate_limit, downKbps > 0 ? downKbps * 1024 : 0);
        pack.set_int(lt::settings_pack::upload_rate_limit,   upKbps   > 0 ? upKbps   * 1024 : 0);
        m_session.apply_settings(pack);
    }
}

int SessionManager::altDownloadLimit() const { return m_altDownLimit; }
int SessionManager::altUploadLimit() const { return m_altUpLimit; }

void SessionManager::setSchedulerEnabled(bool enabled)
{
    m_schedulerEnabled = enabled;
    QSettings("BATorrent", "BATorrent").setValue("schedulerEnabled", enabled);
    if (!enabled && m_altSpeedsActive) {
        // Restore normal speeds
        m_altSpeedsActive = false;
        setDownloadLimit(m_normalDownLimit);
        setUploadLimit(m_normalUpLimit);
    }
}

bool SessionManager::schedulerEnabled() const { return m_schedulerEnabled; }

void SessionManager::setScheduleFromHour(int hour) { m_scheduleFromHour = hour; QSettings("BATorrent", "BATorrent").setValue("scheduleFromHour", hour); }
void SessionManager::setScheduleToHour(int hour) { m_scheduleToHour = hour; QSettings("BATorrent", "BATorrent").setValue("scheduleToHour", hour); }
int SessionManager::scheduleFromHour() const { return m_scheduleFromHour; }
int SessionManager::scheduleToHour() const { return m_scheduleToHour; }

void SessionManager::setScheduleDays(int daysMask) { m_scheduleDays = daysMask; QSettings("BATorrent", "BATorrent").setValue("scheduleDays", daysMask); }
int SessionManager::scheduleDays() const { return m_scheduleDays; }

bool SessionManager::altSpeedsActive() const { return m_altSpeedsActive; }

void SessionManager::setAltSpeedsActive(bool active)
{
    if (active == m_altSpeedsActive) return;
    m_altSpeedsActive = active;
    const int d = active ? m_altDownLimit : m_normalDownLimit;
    const int u = active ? m_altUpLimit   : m_normalUpLimit;
    lt::settings_pack pack;
    pack.set_int(lt::settings_pack::download_rate_limit, d > 0 ? d * 1024 : 0);
    pack.set_int(lt::settings_pack::upload_rate_limit,   u > 0 ? u * 1024 : 0);
    m_session.apply_settings(pack);
    emit altSpeedsActiveChanged(active);
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

bool SessionManager::exportTorrent(int index, const QString &destPath)
{
    if (index < 0 || index >= static_cast<int>(m_torrents.size())) return false;
    const auto &h = m_torrents[index];
    if (!h.is_valid()) return false;
    auto ti = h.torrent_file();
    if (!ti) return false;   // magnet whose metadata hasn't arrived yet
    try {
        lt::create_torrent ct(*ti);
        std::vector<char> buf;
        lt::bencode(std::back_inserter(buf), ct.generate());
        const QString local = destPath.startsWith(QStringLiteral("file:"))
            ? QUrl(destPath).toLocalFile() : destPath;
        QFile f(local);
        if (!f.open(QIODevice::WriteOnly)) return false;
        f.write(buf.data(), static_cast<qint64>(buf.size()));
        return true;
    } catch (...) {
        return false;
    }
}

void SessionManager::applyStorageMode(lt::add_torrent_params &atp)
{
    atp.storage_mode = m_preallocate ? lt::storage_mode_allocate
                                     : lt::storage_mode_sparse;
}

int SessionManager::portStatus() const { return m_portStatus; }

void SessionManager::updatePortStatus()
{
    const int s = !m_listenOk ? 3        // closed — not even listening
                : m_portmapOk ? 1        // open — UPnP/NAT-PMP mapped the port
                              : 2;       // firewalled/unknown — listening, no map
    if (s == m_portStatus) return;
    m_portStatus = s;
    emit portStatusChanged(s);
}

void SessionManager::checkBandwidthSchedule()
{
    if (!m_schedulerEnabled || (m_altDownLimit == 0 && m_altUpLimit == 0))
        return;

    QDateTime now = QDateTime::currentDateTime();
    int currentHour = now.time().hour();
    int dayOfWeek = now.date().dayOfWeek() - 1; // Qt: Mon=1, we want Mon=0
    bool dayMatches = (m_scheduleDays & (1 << dayOfWeek)) != 0;

    bool inSchedule = false;
    if (dayMatches) {
        if (m_scheduleFromHour <= m_scheduleToHour) {
            inSchedule = (currentHour >= m_scheduleFromHour && currentHour < m_scheduleToHour);
        } else {
            // Wraps midnight: e.g. 22:00 to 06:00
            inSchedule = (currentHour >= m_scheduleFromHour || currentHour < m_scheduleToHour);
        }
    }

    // Push values straight to libtorrent — must not go through
    // setDownloadLimit/setUploadLimit because those are "the user wants X as
    // their normal limit" and would clobber m_normalDownLimit during alt mode.
    auto applyLimits = [this](int dKbps, int uKbps) {
        lt::settings_pack pack;
        pack.set_int(lt::settings_pack::download_rate_limit, dKbps > 0 ? dKbps * 1024 : 0);
        pack.set_int(lt::settings_pack::upload_rate_limit,   uKbps > 0 ? uKbps * 1024 : 0);
        m_session.apply_settings(pack);
    };

    if (inSchedule && !m_altSpeedsActive) {
        m_altSpeedsActive = true;
        applyLimits(m_altDownLimit, m_altUpLimit);
    } else if (!inSchedule && m_altSpeedsActive) {
        m_altSpeedsActive = false;
        applyLimits(m_normalDownLimit, m_normalUpLimit);
    }
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
    const int numFiles = static_cast<int>(files.num_files());
    if (numFiles == 0) return;

    if (m_contentLayout == 2) {
        // No subfolder: strip the common root directory from all file paths.
        std::string root = files.file_path(lt::file_index_t(0));
        auto slash = root.find('/');
        if (slash != std::string::npos && numFiles > 1) {
            std::string prefix = root.substr(0, slash + 1);
            bool allMatch = true;
            for (lt::file_index_t i(0); i < files.end_file(); ++i) {
                if (files.file_path(i).compare(0, prefix.size(), prefix) != 0) {
                    allMatch = false;
                    break;
                }
            }
            if (allMatch) {
                for (lt::file_index_t i(0); i < files.end_file(); ++i) {
                    std::string p = files.file_path(i);
                    atp.renamed_files[i] = p.substr(prefix.size());
                }
            }
        }
    } else if (m_contentLayout == 1 && numFiles == 1) {
        // Create subfolder for single-file torrents.
        std::string name = atp.ti->name();
        std::string filePath = files.file_path(lt::file_index_t(0));
        if (filePath.find('/') == std::string::npos) {
            atp.renamed_files[lt::file_index_t(0)] = name + "/" + filePath;
        }
    }
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

// --- Download Queue ---

void SessionManager::setMaxActiveDownloads(int max)
{
    m_maxActiveDownloads = max;
    QSettings("BATorrent", "BATorrent").setValue("maxActiveDownloads", max);
}

int SessionManager::maxActiveDownloads() const
{
    return m_maxActiveDownloads;
}

void SessionManager::setTorrentQueuePosition(int index, int position)
{
    if (index < 0 || index >= static_cast<int>(m_torrents.size()))
        return;
    if (position < 0 || position >= static_cast<int>(m_torrents.size()))
        return;
    if (index == position)
        return;

    lt::torrent_handle h = m_torrents[index];
    m_torrents.erase(m_torrents.begin() + index);
    m_torrents.insert(m_torrents.begin() + position, h);
}

void SessionManager::enforceDownloadQueue()
{
    if (m_maxActiveDownloads <= 0)
        return;

    // Constants for slow-torrent detection. A torrent counts as "fast" if
    // it's actually transferring above this rate; stalled torrents fall
    // off the active list after the timeout so a stuck download can't
    // permanently consume a queue slot.
    constexpr int kSlowTorrentThresholdBps = 10 * 1024; // 10 KB/s
    constexpr qint64 kSlowTorrentTimeoutSec = 60;
    const qint64 now = QDateTime::currentSecsSinceEpoch();

    // Count active (non-paused) downloading torrents
    std::vector<int> activeIndices;
    std::vector<int> queuedIndices;

    for (int i = 0; i < static_cast<int>(m_torrents.size()); ++i) {
        if (!m_torrents[i].is_valid()) continue;
        lt::torrent_status st = cachedStatus(m_torrents[i]);

        bool isPaused = (st.flags & lt::torrent_flags::paused) != lt::torrent_flags_t{};
        bool isDownloading = (st.state == lt::torrent_status::downloading
                              || st.state == lt::torrent_status::downloading_metadata);

        // Force-start torrents are exempt from the queue entirely — they
        // neither count against the cap nor get auto-paused. Resume them
        // here if they're paused for any reason.
        if (st.has_metadata) {
            QString hash = QString::fromStdString(
                (std::ostringstream() << st.info_hashes.get_best()).str());
            if (m_forceStartHashes.contains(hash)) {
                if (isPaused) m_torrents[i].resume();
                continue;
            }
        }

        if (isDownloading && !isPaused) {
            // Stamp the "last fast" timestamp on tick ticks where the
            // torrent is moving; fresh adds also get an initial stamp so
            // they're given a grace period before being demoted.
            auto it = m_lastFastAt.find(m_torrents[i]);
            if (it == m_lastFastAt.end()) {
                m_lastFastAt[m_torrents[i]] = now;
            } else if (st.download_rate >= kSlowTorrentThresholdBps) {
                it->second = now;
            }

            const qint64 lastFast = m_lastFastAt[m_torrents[i]];
            const bool isStalled = (now - lastFast) > kSlowTorrentTimeoutSec;
            // Stalled torrents stay running (we don't pause them — the
            // user can do that), they just don't count against the active
            // limit. New downloads can therefore start in their place.
            if (!isStalled)
                activeIndices.push_back(i);
        } else if (isDownloading && isPaused && m_queuePaused.count(m_torrents[i])) {
            // This torrent was paused by queue logic -- it's waiting
            queuedIndices.push_back(i);
        }
    }

    int activeCount = static_cast<int>(activeIndices.size());

    // If over limit, pause lowest priority (highest index) torrents
    if (activeCount > m_maxActiveDownloads) {
        for (int i = activeCount - 1; i >= m_maxActiveDownloads; --i) {
            int idx = activeIndices[i];
            m_torrents[idx].pause();
            m_queuePaused.insert(m_torrents[idx]);
        }
    }
    // If under limit, resume queued torrents
    else if (activeCount < m_maxActiveDownloads && !queuedIndices.empty()) {
        int toResume = m_maxActiveDownloads - activeCount;
        for (int i = 0; i < toResume && i < static_cast<int>(queuedIndices.size()); ++i) {
            int idx = queuedIndices[i];
            m_torrents[idx].resume();
            m_queuePaused.erase(m_torrents[idx]);
        }
    }
}
