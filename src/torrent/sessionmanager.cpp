// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "sessionmanager.h"
#include "../app/translator.h"
#include <libtorrent/torrent_info.hpp>
#include <libtorrent/version.hpp>
#include <libtorrent/magnet_uri.hpp>
#include <libtorrent/write_resume_data.hpp>
#include <libtorrent/read_resume_data.hpp>
#include <libtorrent/alert_types.hpp>
#include <sstream>
#include <libtorrent/peer_info.hpp>
#include <QDebug>
#include <QDir>
#include <QStandardPaths>
#include <QFile>
#include <QSettings>
#include <QNetworkInterface>
#include <QDateTime>
#include <QTextStream>
#include <libtorrent/ip_filter.hpp>
#include <boost/asio/ip/address.hpp>

SessionManager::SessionManager(QObject *parent)
    : QObject(parent)
{
    lt::settings_pack pack;
    pack.set_int(lt::settings_pack::alert_mask,
                 lt::alert_category::status | lt::alert_category::error
                 | lt::alert_category::storage
                 // piece_progress delivers piece_finished_alert; we use it
                 // to opportunistically save resume data (rate-limited to
                 // once per minute per torrent) so a crash mid-download
                 // doesn't force a full re-hash on next launch.
                 | lt::alert_category::piece_progress);

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
                 lt::generate_fingerprint("BT", 2, 3, 2));
    pack.set_str(lt::settings_pack::user_agent, "BATorrent/2.3.4");

    // (.!bt suffix for incomplete files is applied per-file in addTorrent
    //  and stripped on file_completed_alert below — libtorrent doesn't have
    //  a session-wide setting for this.)

    // Enable UPnP and NAT-PMP for automatic port forwarding
    pack.set_bool(lt::settings_pack::enable_upnp, true);
    pack.set_bool(lt::settings_pack::enable_natpmp, true);

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

    // Load categories
    settings.beginGroup("categories");
    for (const auto &key : settings.childKeys())
        m_categories[key] = settings.value(key).toString();
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

    loadResumeData();

    connect(&m_updateTimer, &QTimer::timeout, this, &SessionManager::updateStats);
    m_updateTimer.start(1000);
}

SessionManager::~SessionManager()
{
    // On shutdown we want a synchronous flush so resume files are durable
    // before Qt tears the app down. The periodic 5-min timer and the
    // piece_finished_alert path both call saveResumeData() (non-blocking).
    flushResumeDataBlocking(5000);
}

void SessionManager::addTorrent(const QString &filePath, const QString &savePath)
{
    try {
        lt::add_torrent_params atp;
        atp.ti = std::make_shared<lt::torrent_info>(filePath.toStdString());
        atp.save_path = savePath.toStdString();
        // Disable libtorrent's auto-manager so user pause/resume always sticks.
        atp.flags &= ~lt::torrent_flags::auto_managed;
        // BEP-27: private torrents must not use DHT, LSD, or PEX. Trackers
        // banning peers that leak the info-hash through those channels is a
        // common reason users get kicked off private trackers.
        if (atp.ti && atp.ti->priv()) {
            atp.flags |= lt::torrent_flags::disable_dht
                       | lt::torrent_flags::disable_lsd
                       | lt::torrent_flags::disable_pex;
        }
        // Append ".!bt" to every file path so Plex/Jellyfin/Sonarr ignore
        // partial files during their library scans. The suffix is stripped
        // again as each file completes via file_completed_alert.
        applyIncompleteSuffix(atp);

        lt::torrent_handle h = m_session.add_torrent(atp);
        m_torrents.push_back(h);
        incrementTorrentCount();

        emit torrentAdded(static_cast<int>(m_torrents.size()) - 1);
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
        atp.save_path = savePath.toStdString();
        atp.flags &= ~lt::torrent_flags::auto_managed;
        if (atp.ti && atp.ti->priv()) {
            atp.flags |= lt::torrent_flags::disable_dht
                       | lt::torrent_flags::disable_lsd
                       | lt::torrent_flags::disable_pex;
        }
        // Apply the user's per-file selection before adding so libtorrent
        // never queues a single byte from an unchecked file.
        atp.file_priorities.reserve(filePriorities.size());
        for (int p : filePriorities) {
            atp.file_priorities.push_back(
                static_cast<lt::download_priority_t>(static_cast<std::uint8_t>(p)));
        }
        applyIncompleteSuffix(atp);

        lt::torrent_handle h = m_session.add_torrent(atp);
        m_torrents.push_back(h);
        incrementTorrentCount();
        emit torrentAdded(static_cast<int>(m_torrents.size()) - 1);
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

void SessionManager::addMagnet(const QString &uri, const QString &savePath)
{
    try {
        lt::add_torrent_params atp = lt::parse_magnet_uri(uri.toStdString());
        atp.save_path = savePath.toStdString();
        atp.flags &= ~lt::torrent_flags::auto_managed;

        lt::torrent_handle h = m_session.add_torrent(atp);
        m_torrents.push_back(h);
        m_magnetAddedAt[h] = QDateTime::currentSecsSinceEpoch();
        incrementTorrentCount();

        emit torrentAdded(static_cast<int>(m_torrents.size()) - 1);
    } catch (const std::exception &e) {
        emit torrentError(QString::fromStdString(e.what()));
    }
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

void SessionManager::removeTorrent(int index, bool deleteFiles)
{
    if (index < 0 || index >= static_cast<int>(m_torrents.size()))
        return;

    // Delete the .resume file so it doesn't come back on restart, and drop
    // any per-torrent seeding overrides so they don't accumulate forever.
    lt::torrent_handle h = m_torrents[index];
    if (h.is_valid()) {
        lt::torrent_status st = h.status();
        QString hash = QString::fromStdString(
            (std::ostringstream() << st.info_hashes.get_best()).str());
        QDir dir(resumeDataDir());
        dir.remove(hash + ".resume");
        m_perTorrentStopAfter.remove(hash);
        m_perTorrentMaxSeed.remove(hash);

        // Carry the removed torrent's lifetime stats into the all-time
        // counters so global totals don't decrease when a torrent is deleted.
        m_globalDownBase += st.total_payload_download;
        m_globalUpBase += st.total_payload_upload;
    }

    // Drop the handle from any internal "paused-by-us" sets so we never try
    // to resume a destroyed torrent on the next kill-switch / queue tick.
    m_queuePaused.erase(h);
    m_killSwitchPaused.erase(h);
    m_statusCache.erase(h);
    m_lastResumeSaveAt.erase(h);
    m_lastFastAt.erase(h);

    lt::remove_flags_t flags{};
    if (deleteFiles)
        flags = lt::session::delete_files;

    m_session.remove_torrent(h, flags);
    m_torrents.erase(m_torrents.begin() + index);
    emit torrentRemoved(index);
}

void SessionManager::pauseTorrent(int index)
{
    if (index < 0 || index >= static_cast<int>(m_torrents.size()))
        return;
    m_torrents[index].pause();
}

void SessionManager::resumeTorrent(int index)
{
    if (index < 0 || index >= static_cast<int>(m_torrents.size()))
        return;
    m_torrents[index].resume();
}

void SessionManager::pauseAll()
{
    for (auto &h : m_torrents)
        h.pause();
}

void SessionManager::resumeAll()
{
    for (auto &h : m_torrents)
        h.resume();
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
    if (info.paused) {
        info.stateString = tr_("state_paused");
        // libtorrent's download_rate / upload_rate are moving averages that
        // take a few seconds to settle after pause. Show zero immediately so
        // the UI matches the actual state.
        info.downloadRate = 0;
        info.uploadRate = 0;
    } else {
        info.downloadRate = st.download_rate;
        info.uploadRate = st.upload_rate;
    }

    // Calculate ratio from payload bytes so it matches what private trackers
    // report (BEP-3 ratio uses payload, not total wire bytes including
    // protocol overhead).
    qint64 uploaded = st.total_payload_upload;
    qint64 downloaded = st.total_payload_download;
    info.ratio = downloaded > 0 ? static_cast<float>(uploaded) / static_cast<float>(downloaded) : 0.0f;

    // Category — only meaningful once metadata is in. Without that guard
    // every still-resolving magnet would share the all-zeros hash and
    // inherit whatever category was last assigned to one of them.
    if (st.has_metadata) {
        QString hash = QString::fromStdString(
            (std::ostringstream() << st.info_hashes.get_best()).str());
        info.category = m_categories.value(hash);
    }

    return info;
}

std::vector<PeerInfo> SessionManager::peersAt(int index) const
{
    std::vector<PeerInfo> result;
    if (index < 0 || index >= static_cast<int>(m_torrents.size()))
        return result;
    if (!m_torrents[index].is_valid())
        return result;

    try {
        std::vector<lt::peer_info> peers;
        m_torrents[index].get_peer_info(peers);

        for (const auto &p : peers) {
            PeerInfo pi;
            try {
                pi.ip = QString::fromStdString(p.ip.address().to_string());
            } catch (...) { continue; }
            pi.port = p.ip.port();
            pi.downloadRate = p.down_speed;
            pi.uploadRate = p.up_speed;
            pi.progress = p.progress;
            // Guard against corrupt/oversized client strings
            if (p.client.size() < 256)
                pi.client = QString::fromStdString(p.client);
            result.push_back(pi);
        }
    } catch (const std::exception &e) {
        qWarning() << "peersAt:" << e.what();
    } catch (...) {
        qWarning() << "peersAt: unknown exception";
    }
    return result;
}

std::vector<FileInfo> SessionManager::filesAt(int index) const
{
    std::vector<FileInfo> result;
    if (index < 0 || index >= static_cast<int>(m_torrents.size()))
        return result;
    if (!m_torrents[index].is_valid())
        return result;

    auto ti = m_torrents[index].torrent_file();
    if (!ti) return result;

    std::vector<std::int64_t> fileProgress;
    m_torrents[index].file_progress(fileProgress);

    const auto &fs = ti->files();
    for (lt::file_index_t i(0); i < fs.end_file(); ++i) {
        int idx = static_cast<int>(i);
        FileInfo fi;
        fi.path = QString::fromStdString(fs.file_path(i));
        fi.size = fs.file_size(i);
        fi.progress = (fi.size > 0 && idx < static_cast<int>(fileProgress.size()))
            ? static_cast<float>(fileProgress[idx]) / fi.size : 0.0f;
        fi.priority = static_cast<std::uint8_t>(m_torrents[index].file_priority(i));
        result.push_back(fi);
    }
    return result;
}

std::vector<TrackerInfo> SessionManager::trackersAt(int index) const
{
    std::vector<TrackerInfo> result;
    if (index < 0 || index >= static_cast<int>(m_torrents.size()))
        return result;
    if (!m_torrents[index].is_valid())
        return result;

    try {
        auto trackers = m_torrents[index].trackers();
        for (const auto &t : trackers) {
            TrackerInfo ti;
            ti.url = QString::fromStdString(t.url);
            ti.tier = t.tier;
            ti.status = "OK";
            if (!t.endpoints.empty()) {
                const auto &msg = t.endpoints[0].info_hashes[lt::protocol_version::V1].message;
                if (!msg.empty())
                    ti.status = QString::fromStdString(msg);
            }
            result.push_back(ti);
        }
    } catch (const std::exception &e) {
        // Torrent may not be valid yet (metadata still downloading) — that's
        // expected, but log everything else so unexpected libtorrent
        // exceptions don't disappear.
        qWarning() << "trackersAt:" << e.what();
    } catch (...) {
        qWarning() << "trackersAt: unknown exception";
    }
    return result;
}

void SessionManager::addTracker(int index, const QString &url)
{
    if (index < 0 || index >= static_cast<int>(m_torrents.size()))
        return;
    lt::announce_entry ae(url.toStdString());
    m_torrents[index].add_tracker(ae);
}

void SessionManager::setFilePriority(int torrentIndex, int fileIndex, int priority)
{
    if (torrentIndex < 0 || torrentIndex >= static_cast<int>(m_torrents.size()))
        return;
    m_torrents[torrentIndex].file_priority(
        lt::file_index_t(fileIndex),
        static_cast<lt::download_priority_t>(static_cast<std::uint8_t>(priority)));
}

void SessionManager::setSequentialDownload(int index, bool sequential)
{
    if (index < 0 || index >= static_cast<int>(m_torrents.size()))
        return;
    if (sequential)
        m_torrents[index].set_flags(lt::torrent_flags::sequential_download);
    else
        m_torrents[index].unset_flags(lt::torrent_flags::sequential_download);
}

void SessionManager::renameFile(int torrentIndex, int fileIndex,
                                 const QString &newRelativePath)
{
    if (torrentIndex < 0 || torrentIndex >= static_cast<int>(m_torrents.size()))
        return;
    if (!m_torrents[torrentIndex].is_valid()) return;
    m_torrents[torrentIndex].rename_file(
        lt::file_index_t(fileIndex), newRelativePath.toStdString());
}

void SessionManager::moveStorage(int torrentIndex, const QString &newSavePath)
{
    if (torrentIndex < 0 || torrentIndex >= static_cast<int>(m_torrents.size()))
        return;
    if (!m_torrents[torrentIndex].is_valid()) return;
    m_torrents[torrentIndex].move_storage(newSavePath.toStdString());
    // A move can leave the filesystem and libtorrent's piece map out of
    // sync if any underlying file failed to move; re-checking is the safe
    // default so the torrent doesn't keep re-downloading good pieces.
}

void SessionManager::replaceTrackers(int torrentIndex, const QStringList &urls)
{
    if (torrentIndex < 0 || torrentIndex >= static_cast<int>(m_torrents.size()))
        return;
    if (!m_torrents[torrentIndex].is_valid()) return;
    std::vector<lt::announce_entry> entries;
    entries.reserve(urls.size());
    for (const QString &u : urls) {
        if (u.trimmed().isEmpty()) continue;
        entries.emplace_back(u.toStdString());
    }
    m_torrents[torrentIndex].replace_trackers(entries);
}

void SessionManager::setTorrentCategory(int index, const QString &category)
{
    if (index < 0 || index >= static_cast<int>(m_torrents.size()))
        return;
    QString hash = torrentHash(index);
    // No real hash yet (e.g. still resolving magnet metadata) — silently
    // skip; categorizing a magnet before its hash is known would collide
    // with every other still-resolving magnet's zero hash.
    if (hash.isEmpty())
        return;

    if (category.isEmpty())
        m_categories.remove(hash);
    else
        m_categories[hash] = category;
}

QStringList SessionManager::categories() const
{
    QStringList list = {"Movies", "Games", "Software", "Music", "Other"};
    // Add any custom categories that aren't in the built-in list
    for (auto it = m_categories.cbegin(); it != m_categories.cend(); ++it) {
        if (!list.contains(it.value()) && !it.value().isEmpty())
            list.append(it.value());
    }
    return list;
}

std::vector<bool> SessionManager::piecesAt(int index) const
{
    std::vector<bool> result;
    if (index < 0 || index >= static_cast<int>(m_torrents.size()))
        return result;
    if (!m_torrents[index].is_valid())
        return result;

    lt::torrent_status st = m_torrents[index].status(lt::torrent_handle::query_pieces);
    int numPieces = st.pieces.size();
    result.resize(numPieces, false);
    for (int i = 0; i < numPieces; ++i)
        result[i] = st.pieces[lt::piece_index_t(i)];

    return result;
}

void SessionManager::setDownloadLimit(int kbps)
{
    // Treat "user-set" as the normal rate. When alt-speed mode is active
    // libtorrent's currently-applied limit is the alt value; we don't want
    // the user's preferences UI to silently clobber the alt value, and we
    // don't want our own scheduler to clobber the user's new normal when it
    // restores. So always update m_normalDownLimit, and only push to
    // libtorrent immediately if alt mode isn't active.
    m_normalDownLimit = kbps;
    if (m_altSpeedsActive)
        return;
    lt::settings_pack pack;
    pack.set_int(lt::settings_pack::download_rate_limit, kbps > 0 ? kbps * 1024 : 0);
    m_session.apply_settings(pack);
}

void SessionManager::setUploadLimit(int kbps)
{
    m_normalUpLimit = kbps;
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
    QString iface = QString("%1:%2").arg(listenAddr).arg(port);
    pack.set_str(lt::settings_pack::listen_interfaces, iface.toStdString());
    m_session.apply_settings(pack);
}

int SessionManager::listenPort() const
{
    return m_session.listen_port();
}

void SessionManager::setMaxConnections(int max)
{
    lt::settings_pack pack;
    pack.set_int(lt::settings_pack::connections_limit, max);
    m_session.apply_settings(pack);
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
}

bool SessionManager::utpEnabled() const
{
    return m_utpEnabled;
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
}

int SessionManager::encryptionMode() const
{
    return m_encryptionMode;
}

void SessionManager::setSeedRatioLimit(float ratio)
{
    m_seedRatioLimit = ratio;
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

QString SessionManager::torrentHash(int index) const
{
    if (index < 0 || index >= static_cast<int>(m_torrents.size()))
        return {};
    if (!m_torrents[index].is_valid()) return {};
    lt::torrent_status st = cachedStatus(m_torrents[index]);
    // Magnet links report an all-zeros hash from get_best() until metadata
    // is downloaded. Returning that string would cause every still-resolving
    // magnet to share the same key — categories and per-torrent seeding
    // overrides set on one would silently apply to all others. Refuse to
    // expose the hash until libtorrent has the real one.
    if (!st.has_metadata)
        return {};
    return QString::fromStdString(
        (std::ostringstream() << st.info_hashes.get_best()).str());
}

void SessionManager::setTorrentStopAfterDownload(int index, int value)
{
    QString hash = torrentHash(index);
    if (hash.isEmpty()) return;
    if (value < 0)
        m_perTorrentStopAfter.remove(hash);
    else
        m_perTorrentStopAfter[hash] = value ? 1 : 0;
}

int SessionManager::torrentStopAfterDownload(int index) const
{
    QString hash = torrentHash(index);
    if (hash.isEmpty()) return -1;
    return m_perTorrentStopAfter.value(hash, -1);
}

void SessionManager::setTorrentMaxSeedSeconds(int index, qint64 seconds)
{
    QString hash = torrentHash(index);
    if (hash.isEmpty()) return;
    if (seconds < 0)
        m_perTorrentMaxSeed.remove(hash);
    else
        m_perTorrentMaxSeed[hash] = seconds;
}

qint64 SessionManager::torrentMaxSeedSeconds(int index) const
{
    QString hash = torrentHash(index);
    if (hash.isEmpty()) return -1;
    return m_perTorrentMaxSeed.value(hash, -1);
}

void SessionManager::stopSeedingTorrent(int index)
{
    if (index < 0 || index >= static_cast<int>(m_torrents.size()))
        return;
    m_torrents[index].pause();
}

void SessionManager::forceRecheck(int index)
{
    if (index < 0 || index >= static_cast<int>(m_torrents.size()))
        return;
    if (!m_torrents[index].is_valid()) return;
    m_torrents[index].force_recheck();
}

void SessionManager::forceReannounce(int index)
{
    if (index < 0 || index >= static_cast<int>(m_torrents.size()))
        return;
    if (!m_torrents[index].is_valid()) return;
    m_torrents[index].force_reannounce();
    m_torrents[index].force_dht_announce();
}

QString SessionManager::torrentHashAt(int index) const
{
    return torrentHash(index);
}

bool SessionManager::effectiveStopAfterDownload(const QString &hash) const
{
    int override_ = m_perTorrentStopAfter.value(hash, -1);
    if (override_ >= 0)
        return override_ == 1;
    return m_stopAfterDownload;
}

qint64 SessionManager::effectiveMaxSeedSeconds(const QString &hash) const
{
    qint64 override_ = m_perTorrentMaxSeed.value(hash, -1);
    if (override_ >= 0)
        return override_;
    return m_maxSeedSeconds;
}

void SessionManager::saveResumeData()
{
    // QSettings writes are cheap (memory-backed; flushed on app exit) so we
    // keep them inline here.
    QSettings settings("BATorrent", "BATorrent");
    settings.setValue("globalDownloaded", globalDownloaded());
    settings.setValue("globalUploaded", globalUploaded());

    settings.beginGroup("categories");
    settings.remove("");
    for (auto it = m_categories.cbegin(); it != m_categories.cend(); ++it)
        settings.setValue(it.key(), it.value());
    settings.endGroup();

    settings.setValue("stopAfterDownload", m_stopAfterDownload);
    settings.setValue("maxSeedSeconds", m_maxSeedSeconds);

    settings.beginGroup("torrentStopAfter");
    settings.remove("");
    for (auto it = m_perTorrentStopAfter.cbegin(); it != m_perTorrentStopAfter.cend(); ++it)
        settings.setValue(it.key(), it.value());
    settings.endGroup();

    settings.beginGroup("torrentMaxSeed");
    settings.remove("");
    for (auto it = m_perTorrentMaxSeed.cbegin(); it != m_perTorrentMaxSeed.cend(); ++it)
        settings.setValue(it.key(), it.value());
    settings.endGroup();

    QDir dir(resumeDataDir());
    if (!dir.exists())
        dir.mkpath(".");

    // Just kick off the async request for every torrent with metadata.
    // libtorrent will deliver save_resume_data_alert / _failed_alert for
    // each one; processAlerts persists them to disk and decrements
    // m_resumeOutstanding. The GUI thread doesn't block here.
    //
    // Reset the counter at the start so stale alerts from a previous
    // unfinished batch don't artificially inflate it. The earlier batch's
    // alerts will still be persisted (processAlerts always writes), they
    // just won't double-count against the shutdown timeout.
    m_resumeOutstanding = 0;
    const qint64 now = QDateTime::currentSecsSinceEpoch();
    for (auto &h : m_torrents) {
        if (!h.is_valid()) continue;
        lt::torrent_status st = cachedStatus(h);
        if (!st.has_metadata) continue;
        h.save_resume_data(lt::torrent_handle::save_info_dict);
        m_lastResumeSaveAt[h] = now;
        ++m_resumeOutstanding;
    }
}

void SessionManager::flushResumeDataBlocking(int timeoutMs)
{
    // Called from the destructor. Kick off the async requests, then turn
    // the crank on alerts synchronously with a bounded total wait time so
    // app shutdown is durable but never hangs indefinitely.
    saveResumeData();

    const auto deadline = std::chrono::steady_clock::now()
                          + std::chrono::milliseconds(timeoutMs);

    while (m_resumeOutstanding > 0
           && std::chrono::steady_clock::now() < deadline) {
        // Wait for the next alert up to whatever time is left in the
        // deadline window. wait_for_alert returns immediately if alerts
        // are already queued.
        auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
            deadline - std::chrono::steady_clock::now());
        if (remaining.count() <= 0) break;
        lt::alert const *a = m_session.wait_for_alert(remaining);
        if (!a) break;
        processAlerts();
    }
}

bool SessionManager::persistResumeAlert(const lt::save_resume_data_alert *rd)
{
    QDir dir(resumeDataDir());
    if (!dir.exists())
        dir.mkpath(".");
    auto buf = lt::write_resume_data_buf(rd->params);
    lt::torrent_status st = rd->handle.status();
    QString hash = QString::fromStdString(
        (std::ostringstream() << st.info_hashes.get_best()).str());
    QString filePath = dir.filePath(hash + ".resume");
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly))
        return false;
    file.write(buf.data(), static_cast<qint64>(buf.size()));
    return true;
}

void SessionManager::loadResumeData()
{
    QDir dir(resumeDataDir());
    if (!dir.exists()) return;

    QStringList files = dir.entryList({"*.resume"}, QDir::Files);
    for (const auto &fileName : files) {
        QFile file(dir.filePath(fileName));
        if (!file.open(QIODevice::ReadOnly)) continue;

        QByteArray data = file.readAll();
        lt::error_code ec;
        lt::add_torrent_params atp = lt::read_resume_data(
            lt::span<const char>(data.data(), data.size()), ec);
        if (ec) continue;

        // Manual queue management — never let libtorrent's auto-manager
        // override user pause state on resumed torrents.
        atp.flags &= ~lt::torrent_flags::auto_managed;
        // BEP-27: enforce private flag even on legacy resume data that
        // predates the addTorrent fix.
        if (atp.ti && atp.ti->priv()) {
            atp.flags |= lt::torrent_flags::disable_dht
                       | lt::torrent_flags::disable_lsd
                       | lt::torrent_flags::disable_pex;
        }

        lt::torrent_handle h = m_session.add_torrent(atp);
        m_torrents.push_back(h);
    }

    if (!m_torrents.empty())
        emit torrentsUpdated();
}

void SessionManager::updateStats()
{
    // Ask libtorrent to deliver a state_update_alert with fresh statuses for
    // every torrent. The alert lands inside processAlerts() below; until it
    // arrives, m_statusCache may be one tick stale — acceptable trade-off
    // for getting rid of dozens of synchronous status() calls per second.
    m_session.post_torrent_updates();

    processAlerts();
    checkSeedRatios();
    checkSeedingLimits();
    checkInterfaceStatus();
    checkBandwidthSchedule();
    checkMagnetTimeouts();
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

void SessionManager::processAlerts()
{
    std::vector<lt::alert *> alerts;
    m_session.pop_alerts(&alerts);

    for (auto *a : alerts) {
        // Status snapshots arrive in one batch per post_torrent_updates(),
        // refreshing every cache entry at once. This is what makes the UI
        // 1/Nth as expensive as polling each handle individually.
        if (auto *su = lt::alert_cast<lt::state_update_alert>(a)) {
            for (const auto &st : su->status)
                m_statusCache[st.handle] = st;
            continue;
        }

        if (auto *fa = lt::alert_cast<lt::torrent_finished_alert>(a)) {
            QString name = QString::fromStdString(fa->torrent_name());
            lt::torrent_status st = fa->handle.status();

            // Skip torrents that were already complete when the session
            // started — libtorrent fires one finish alert per torrent during
            // the resume check, even if no bytes were actually downloaded.
            bool downloadedThisSession = (st.total_payload_download > 0);

            if (downloadedThisSession) {
                // Auto-move completed download
                if (m_autoMoveEnabled && !m_autoMovePath.isEmpty()) {
                    fa->handle.move_storage(m_autoMovePath.toStdString());
                }

                // Apply stop-after-download (global or per-torrent).
                QString hash = QString::fromStdString(
                    (std::ostringstream() << st.info_hashes.get_best()).str());
                if (effectiveStopAfterDownload(hash))
                    fa->handle.pause();

                emit torrentFinished(name, hash);
            }

            // Remove from queue-paused set in either case (it's no longer
            // contributing to the active-download count).
            m_queuePaused.erase(fa->handle);
        }
        if (auto *ea = lt::alert_cast<lt::torrent_error_alert>(a)) {
            emit torrentError(QString::fromStdString(ea->message()));
        }
        // Surface previously-swallowed alert categories so the user actually
        // hears about disk-full, move-storage failures, port collisions, and
        // broken magnets instead of staring at silent empty state.
        if (auto *fe = lt::alert_cast<lt::file_error_alert>(a)) {
            emit torrentError(QString::fromStdString(fe->message()));
        }
        if (auto *sm = lt::alert_cast<lt::storage_moved_failed_alert>(a)) {
            emit torrentError(QString::fromStdString(sm->message()));
        }
        if (auto *lf = lt::alert_cast<lt::listen_failed_alert>(a)) {
            emit torrentError(QString::fromStdString(lf->message()));
        }
        if (auto *mf = lt::alert_cast<lt::metadata_failed_alert>(a)) {
            emit torrentError(QString::fromStdString(mf->message()));
        }
        // Async resume-data persistence: write the buffer the moment the
        // alert arrives, decrement the outstanding counter so
        // flushResumeDataBlocking() and other callers can tell when every
        // request has been serviced.
        if (auto *rd = lt::alert_cast<lt::save_resume_data_alert>(a)) {
            persistResumeAlert(rd);
            if (m_resumeOutstanding > 0) --m_resumeOutstanding;
        }
        if (lt::alert_cast<lt::save_resume_data_failed_alert>(a)) {
            if (m_resumeOutstanding > 0) --m_resumeOutstanding;
        }
        // Piece just verified — opportunistically save resume data for this
        // torrent so a crash before the next 5-min tick doesn't force a
        // full re-hash on the next launch. Rate-limited per handle (60 s)
        // so a fast torrent doesn't hammer the disk; with the limit, the
        // worst case is ~1 resume write per minute per active download.
        if (auto *pf = lt::alert_cast<lt::piece_finished_alert>(a)) {
            const qint64 now = QDateTime::currentSecsSinceEpoch();
            auto it = m_lastResumeSaveAt.find(pf->handle);
            if (it == m_lastResumeSaveAt.end() || now - it->second >= 60) {
                pf->handle.save_resume_data(lt::torrent_handle::save_info_dict);
                m_lastResumeSaveAt[pf->handle] = now;
                ++m_resumeOutstanding;
            }
        }
        // Magnet just got its metadata — file list is now known, so apply
        // the .!bt suffix to each file.
        if (auto *mr = lt::alert_cast<lt::metadata_received_alert>(a)) {
            auto ti = mr->handle.torrent_file();
            if (ti) {
                const auto &files = ti->files();
                for (lt::file_index_t i(0); i < files.end_file(); ++i) {
                    std::string original = files.file_path(i);
                    if (original.size() >= 4
                        && original.compare(original.size() - 4, 4, ".!bt") == 0)
                        continue;
                    mr->handle.rename_file(i, original + ".!bt");
                }
            }
        }
        // File done — drop the .!bt suffix so the file appears with its
        // final name in the file manager and media server scans.
        if (auto *fc = lt::alert_cast<lt::file_completed_alert>(a)) {
            auto ti = fc->handle.torrent_file();
            if (ti) {
                std::string original = ti->files().file_path(fc->index);
                fc->handle.rename_file(fc->index, original);
            }
        }
    }
}

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

QString SessionManager::resumeDataDir() const
{
    return QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)).filePath("resume");
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

// --- VPN / Interface binding ---

void SessionManager::setOutgoingInterface(const QString &interfaceName)
{
    m_outgoingInterface = interfaceName;
    m_killSwitchActive = false;
    m_killSwitchPaused.clear();

    lt::settings_pack pack;

    if (interfaceName.isEmpty()) {
        // Any interface
        pack.set_str(lt::settings_pack::outgoing_interfaces, "");
        int port = listenPort();
        QString listenIface = QString("0.0.0.0:%1").arg(port > 0 ? port : 6881);
        pack.set_str(lt::settings_pack::listen_interfaces, listenIface.toStdString());
    } else {
        pack.set_str(lt::settings_pack::outgoing_interfaces, interfaceName.toStdString());

        // Resolve the interface IP for listen_interfaces
        QNetworkInterface ni = QNetworkInterface::interfaceFromName(interfaceName);
        QString listenAddr = "0.0.0.0";
        for (const auto &entry : ni.addressEntries()) {
            if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol) {
                listenAddr = entry.ip().toString();
                break;
            }
        }
        int port = listenPort();
        QString listenIface = QString("%1:%2").arg(listenAddr).arg(port > 0 ? port : 6881);
        pack.set_str(lt::settings_pack::listen_interfaces, listenIface.toStdString());
    }

    m_session.apply_settings(pack);
}

QString SessionManager::outgoingInterface() const
{
    return m_outgoingInterface;
}

void SessionManager::setKillSwitchEnabled(bool enabled)
{
    m_killSwitchEnabled = enabled;
    if (!enabled) {
        m_killSwitchActive = false;
        m_killSwitchPaused.clear();
    }
}

bool SessionManager::killSwitchEnabled() const
{
    return m_killSwitchEnabled;
}

void SessionManager::setAutoResumeOnReconnect(bool enabled)
{
    m_autoResume = enabled;
}

bool SessionManager::autoResumeOnReconnect() const
{
    return m_autoResume;
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
    qint64 down = globalDownloaded();
    return down > 0 ? static_cast<float>(globalUploaded()) / static_cast<float>(down) : 0.0f;
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

void SessionManager::incrementTorrentCount()
{
    QSettings settings("BATorrent", "BATorrent");
    int count = settings.value("totalTorrentsAdded", 0).toInt();
    settings.setValue("totalTorrentsAdded", count + 1);
}

int SessionManager::totalTorrentsAdded() const
{
    QSettings settings("BATorrent", "BATorrent");
    return settings.value("totalTorrentsAdded", 0).toInt();
}

// --- Proxy ---

void SessionManager::setProxySettings(int type, const QString &host, int port,
                                       const QString &user, const QString &pass)
{
    m_proxyType = type;
    m_proxyHost = host;
    m_proxyPort = port;
    m_proxyUser = user;
    m_proxyPass = pass;

    lt::settings_pack pack;
    if (type == 0) {
        pack.set_int(lt::settings_pack::proxy_type, lt::settings_pack::none);
        pack.set_bool(lt::settings_pack::proxy_peer_connections, false);
        pack.set_bool(lt::settings_pack::proxy_hostnames, false);
    } else {
        int ltType = (type == 1) ? lt::settings_pack::socks5_pw : lt::settings_pack::http_pw;
        if (user.isEmpty())
            ltType = (type == 1) ? lt::settings_pack::socks5 : lt::settings_pack::http;
        pack.set_int(lt::settings_pack::proxy_type, ltType);
        pack.set_str(lt::settings_pack::proxy_hostname, host.toStdString());
        pack.set_int(lt::settings_pack::proxy_port, port);
        if (!user.isEmpty()) {
            pack.set_str(lt::settings_pack::proxy_username, user.toStdString());
            pack.set_str(lt::settings_pack::proxy_password, pass.toStdString());
        }
        // Route peer traffic through the proxy too — without this the
        // proxy only covers trackers and the user's real IP leaks to peers.
        // Resolve hostnames through the proxy as well so DNS doesn't leak.
        pack.set_bool(lt::settings_pack::proxy_peer_connections, true);
        pack.set_bool(lt::settings_pack::proxy_hostnames, true);
    }
    m_session.apply_settings(pack);
}

int SessionManager::proxyType() const { return m_proxyType; }
QString SessionManager::proxyHost() const { return m_proxyHost; }
int SessionManager::proxyPort() const { return m_proxyPort; }
QString SessionManager::proxyUser() const { return m_proxyUser; }
QString SessionManager::proxyPass() const { return m_proxyPass; }

// --- IP Filter ---

void SessionManager::loadIpFilter(const QString &filePath)
{
    m_ipFilterPath = filePath;
    m_ipFilterCount = 0;

    if (filePath.isEmpty()) {
        clearIpFilter();
        return;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;

    lt::ip_filter filter;
    QTextStream in(&file);
    int count = 0;

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith('#'))
            continue;

        // P2P format: "description:startIP-endIP"
        int colonIdx = line.lastIndexOf(':');
        QString range;
        if (colonIdx >= 0 && line.indexOf('.', colonIdx) > 0)
            range = line.mid(colonIdx + 1).trimmed();
        else
            range = line;

        int dashIdx = range.indexOf('-');
        if (dashIdx < 0) continue;

        QString startIp = range.left(dashIdx).trimmed();
        QString endIp = range.mid(dashIdx + 1).trimmed();

        try {
            boost::system::error_code ec1, ec2;
            auto addr1 = boost::asio::ip::make_address(startIp.toStdString(), ec1);
            auto addr2 = boost::asio::ip::make_address(endIp.toStdString(), ec2);
            if (ec1 || ec2) continue;
            filter.add_rule(addr1, addr2, lt::ip_filter::blocked);
            ++count;
        } catch (...) {}
    }

    m_session.set_ip_filter(filter);
    m_ipFilterCount = count;
}

void SessionManager::clearIpFilter()
{
    m_ipFilterPath.clear();
    m_ipFilterCount = 0;
    m_session.set_ip_filter(lt::ip_filter());
}

QString SessionManager::ipFilterPath() const { return m_ipFilterPath; }
int SessionManager::ipFilterCount() const { return m_ipFilterCount; }

// --- Bandwidth Scheduler ---

void SessionManager::setAltSpeedLimits(int downKbps, int upKbps)
{
    m_altDownLimit = downKbps;
    m_altUpLimit = upKbps;
}

int SessionManager::altDownloadLimit() const { return m_altDownLimit; }
int SessionManager::altUploadLimit() const { return m_altUpLimit; }

void SessionManager::setSchedulerEnabled(bool enabled)
{
    m_schedulerEnabled = enabled;
    if (!enabled && m_altSpeedsActive) {
        // Restore normal speeds
        m_altSpeedsActive = false;
        setDownloadLimit(m_normalDownLimit);
        setUploadLimit(m_normalUpLimit);
    }
}

bool SessionManager::schedulerEnabled() const { return m_schedulerEnabled; }

void SessionManager::setScheduleFromHour(int hour) { m_scheduleFromHour = hour; }
void SessionManager::setScheduleToHour(int hour) { m_scheduleToHour = hour; }
int SessionManager::scheduleFromHour() const { return m_scheduleFromHour; }
int SessionManager::scheduleToHour() const { return m_scheduleToHour; }

void SessionManager::setScheduleDays(int daysMask) { m_scheduleDays = daysMask; }
int SessionManager::scheduleDays() const { return m_scheduleDays; }

bool SessionManager::altSpeedsActive() const { return m_altSpeedsActive; }

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

void SessionManager::checkInterfaceStatus()
{
    if (m_outgoingInterface.isEmpty() || !m_killSwitchEnabled)
        return;

    QNetworkInterface ni = QNetworkInterface::interfaceFromName(m_outgoingInterface);
    bool isUp = ni.isValid()
                && (ni.flags() & QNetworkInterface::IsUp)
                && (ni.flags() & QNetworkInterface::IsRunning);

    // Check if interface has an IPv4 address
    bool hasIp = false;
    if (isUp) {
        for (const auto &entry : ni.addressEntries()) {
            if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol) {
                hasIp = true;
                break;
            }
        }
    }

    bool interfaceOk = isUp && hasIp;

    if (!interfaceOk && !m_killSwitchActive) {
        // Interface just went down — pause all running torrents
        m_killSwitchActive = true;
        m_killSwitchPaused.clear();
        for (auto &h : m_torrents) {
            if (!h.is_valid()) continue;
            lt::torrent_status st = h.status();
            if (!(st.flags & lt::torrent_flags::paused)) {
                h.pause();
                m_killSwitchPaused.insert(h);
            }
        }
        emit killSwitchTriggered();
    } else if (interfaceOk && m_killSwitchActive) {
        // Interface came back — re-apply binding and optionally resume
        m_killSwitchActive = false;
        setOutgoingInterface(m_outgoingInterface);

        if (m_autoResume) {
            for (auto &h : m_killSwitchPaused) {
                if (h.is_valid())
                    h.resume();
            }
        }
        m_killSwitchPaused.clear();
        emit interfaceRestored();
    }
}

// --- Auto-move ---

void SessionManager::setAutoMove(bool enabled, const QString &path)
{
    m_autoMoveEnabled = enabled;
    m_autoMovePath = path;
}

bool SessionManager::autoMoveEnabled() const { return m_autoMoveEnabled; }
QString SessionManager::autoMovePath() const { return m_autoMovePath; }

// --- Download Queue ---

void SessionManager::setMaxActiveDownloads(int max)
{
    m_maxActiveDownloads = max;
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
