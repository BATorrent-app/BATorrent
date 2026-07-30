// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "torrent/sessionmanager.h"
#include "torrent/bandwidthschedule.h"
#include "torrent/magnettrackers.h"
#include "services/platform/logger.h"
#include "services/platform/translator.h"
#include "services/security/archivescan.h"
#include "services/security/archiveextractor.h"
#include <QProcess>
#include <QCoreApplication>
#include <QStorageInfo>
#include <QThread>
#include <libtorrent/torrent_info.hpp>
#include <libtorrent/version.hpp>
#include <libtorrent/magnet_uri.hpp>
#include <boost/system/error_code.hpp>   // errc for the files-missing check
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
#include <QRandomGenerator>
#include <algorithm>
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
    // Top Sentry crasher (NATIVE-QT-4) is an AV inside libtorrent during
    // session construction — a try/catch can't stop an access violation, so
    // after a crashed boot the retry starts with fresh params instead of
    // re-feeding whatever state killed the last run.
    if (QSettings().value(QStringLiteral("bootCrashes"), 0).toInt() >= 1)
        return lt::session_params();
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
    // libtorrent's default bootstrap is a single host (dht.libtorrent.org) —
    // if it's down or ISP-blocked, a fresh install never joins the DHT and
    // trackerless magnets can't fetch metadata. Same router list as qBittorrent.
    pack.set_str(lt::settings_pack::dht_bootstrap_nodes,
                 "dht.libtorrent.org:25401,router.bittorrent.com:6881,"
                 "router.utorrent.com:6881,dht.transmissionbt.com:6881,"
                 "dht.aelitis.com:6881");

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
    // Announce to every tracker tier at once (qBittorrent default). Magnet
    // trackers each land in their own tier, so without this only the first
    // responsive tracker is announced — a real peer-discovery deficit.
    pack.set_bool(lt::settings_pack::announce_to_all_tiers, true);
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
    {
        int port = settings.value("listenPort", 0).toInt();
        // First run: stable random high port instead of libtorrent's 6881
        // default — the 688x range is a classic ISP-throttle target and
        // collides with other clients on the same machine (with
        // listen_system_port_fallback off, a busy port = no listen socket
        // at all). Persisted so the user's port forward stays valid.
        // The "randomPort" toggle re-rolls it on every launch instead.
        if (port <= 0 || settings.value("randomPort", false).toBool()) {
            port = static_cast<int>(QRandomGenerator::global()->bounded(49152, 65536));
            settings.setValue("listenPort", port);
        }
        setListenPort(port);
    }
    setKillSwitchEnabled(settings.value("killSwitchEnabled", false).toBool());
    setAutoResumeOnReconnect(settings.value("autoResumeOnReconnect", false).toBool());
    m_proxy.loadFromSettings();
    if (m_proxy.type() != 0)
        m_session.apply_settings(m_proxy.settings(m_anonymousMode));
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

    // Remove-with-files whose retry window was cut short by an app quit: the
    // schedule*() timers die with the process, silently leaving data on disk.
    // The old session's file locks died with it too, so retry those now.
    {
        const QStringList t = settings.value("pendingTrashTargets").toStringList();
        const QStringList d = settings.value("pendingDeleteTargets").toStringList();
        settings.remove("pendingTrashTargets");
        settings.remove("pendingDeleteTargets");
        if (!t.isEmpty()) scheduleTrash(t, 0);
        if (!d.isEmpty()) scheduleDelete(d, 0);
    }

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

    try {
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
    if (info.paused && m_queuePaused.count(m_torrents[index])) {
        info.queued = true;
        info.queuePos = 1;
        for (int i = 0; i < index; ++i)
            if (m_queuePaused.count(m_torrents[i])) ++info.queuePos;
    }
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
        // "Stop seeding after download" pauses the handle directly (see
        // onTorrentFinished) without going through markCompleted(), so a
        // finished torrent otherwise reads as bare "Paused" — ambiguous
        // about whether the download itself is done (reported by a user).
        info.stateString = info.queued
            ? tr_("state_queued").arg(info.queuePos)
            : (info.progress >= 1.0f) ? tr_("state_paused_done") : tr_("state_paused");
        info.downloadRate = 0;
        info.uploadRate = 0;
    } else {
        info.downloadRate = st.download_rate;
        info.uploadRate = st.upload_rate;
    }

    // Data deleted/moved out from under a live torrent: libtorrent errors on the
    // missing path (ENOENT). Surface it as a distinct, actionable state instead of
    // a torrent that silently reads as complete/seeding while the files are gone
    // (tester: a movie sat inactive with no explanation after a manual delete). The
    // errc == enum compare is category-aware, so it matches on every platform.
    if (st.errc == boost::system::errc::no_such_file_or_directory) {
        info.filesMissing = true;
        info.stateString = tr_("state_files_missing");
        info.stateDetail = tr_("state_files_missing");
        info.downloadRate = 0;
        info.uploadRate = 0;
    }

    // qBittorrent's most-repeated complaint is a silent "stalled" — name the
    // actual blocker so the state cell can explain itself on hover
    if (!info.completed && !info.paused && info.progress < 1.0f
            && st.state == lt::torrent_status::downloading
            && info.downloadRate < 1024) {
        if (st.errc)
            info.stateDetail = QString::fromStdString(st.errc.message());
        else if (info.numPeers == 0)
            // candidates known but none connected yet = the "connecting" phase;
            // nothing found at all = still searching the swarm
            info.stateDetail = st.connect_candidates > 0
                ? tr_("state_connecting")
                : (m_dhtEnabled ? tr_("state_no_peers_dht") : tr_("state_no_peers"));
        else if (info.numSeeds == 0)
            info.stateDetail = tr_("state_no_seeds");
        else
            info.stateDetail = tr_("state_choked");
    } else if (!info.paused && st.state == lt::torrent_status::downloading_metadata) {
        // A rare-seeder magnet can take a long time to find a peer that'll
        // hand over metadata — we used to give up and silently delete it
        // after 5 minutes (issue reported by a user: "deleted without
        // warning"). Explain the wait instead; the user decides when to quit.
        auto it = m_magnetAddedAt.find(m_torrents[index]);
        if (it != m_magnetAddedAt.end()) {
            const qint64 mins = (QDateTime::currentSecsSinceEpoch() - it->second) / 60;
            info.stateDetail = tr_("state_fetching_metadata").arg(mins);
        }
    } else if (!info.completed && !info.paused && info.progress < 1.0f
               && st.state == lt::torrent_status::downloading
               && info.downloadRate >= 1024 && info.numPeers > 0
               && st.distributed_copies >= 0.0f && st.distributed_copies < 1.0f) {
        // distributed_copies < 1 means some piece of this torrent isn't held by
        // anyone currently in the swarm — the transfer can look healthy (decent
        // rate, progress moving) right up until it needs that missing piece and
        // stalls for good. Surface it early instead of only once it's stuck.
        info.stateDetail = tr_("state_missing_pieces");
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
    } catch (const std::exception &e) {
        qWarning() << "[session] torrentAt exception:" << e.what();
        return TorrentInfo{};
    } catch (...) {
        qWarning() << "[session] torrentAt: unknown exception";
        return TorrentInfo{};
    }
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


// Fast-resume persistence (save/load/flush/persist, resumeDataDir, legacy
// migration, capture/restoreFromResumeData) lives in
// sessionmanager_persistence.cpp.


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



