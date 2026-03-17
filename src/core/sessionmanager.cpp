#include "sessionmanager.h"
#include "translator.h"
#include <libtorrent/load_torrent.hpp>
#include <libtorrent/torrent_info.hpp>
#include <libtorrent/magnet_uri.hpp>
#include <libtorrent/write_resume_data.hpp>
#include <libtorrent/read_resume_data.hpp>
#include <libtorrent/alert_types.hpp>
#include <libtorrent/peer_info.hpp>
#include <QDir>
#include <QStandardPaths>
#include <QFile>

SessionManager::SessionManager(QObject *parent)
    : QObject(parent)
{
    lt::settings_pack pack;
    pack.set_int(lt::settings_pack::alert_mask,
                 lt::alert_category::status | lt::alert_category::error
                 | lt::alert_category::storage);

    // Enable DHT for trackerless torrents / magnet links
    pack.set_bool(lt::settings_pack::enable_dht, true);

    // PEX (Peer Exchange) is enabled by default in libtorrent 2.x

    // Enable UPnP and NAT-PMP for automatic port forwarding
    pack.set_bool(lt::settings_pack::enable_upnp, true);
    pack.set_bool(lt::settings_pack::enable_natpmp, true);

    // Enable protocol encryption (prefer encrypted, allow unencrypted fallback)
    pack.set_int(lt::settings_pack::out_enc_policy, lt::settings_pack::pe_enabled);
    pack.set_int(lt::settings_pack::in_enc_policy, lt::settings_pack::pe_enabled);

    m_session.apply_settings(pack);

    loadResumeData();

    connect(&m_updateTimer, &QTimer::timeout, this, &SessionManager::updateStats);
    m_updateTimer.start(1000);
}

SessionManager::~SessionManager()
{
    saveResumeData();
}

void SessionManager::addTorrent(const QString &filePath, const QString &savePath)
{
    try {
        lt::add_torrent_params atp = lt::load_torrent_file(filePath.toStdString());
        atp.save_path = savePath.toStdString();

        lt::torrent_handle h = m_session.add_torrent(atp);
        m_torrents.push_back(h);

        emit torrentAdded(static_cast<int>(m_torrents.size()) - 1);
    } catch (const std::exception &e) {
        emit torrentError(QString::fromStdString(e.what()));
    }
}

void SessionManager::addMagnet(const QString &uri, const QString &savePath)
{
    try {
        lt::add_torrent_params atp = lt::parse_magnet_uri(uri.toStdString());
        atp.save_path = savePath.toStdString();

        lt::torrent_handle h = m_session.add_torrent(atp);
        m_torrents.push_back(h);

        emit torrentAdded(static_cast<int>(m_torrents.size()) - 1);
    } catch (const std::exception &e) {
        emit torrentError(QString::fromStdString(e.what()));
    }
}

void SessionManager::removeTorrent(int index, bool deleteFiles)
{
    if (index < 0 || index >= static_cast<int>(m_torrents.size()))
        return;

    lt::remove_flags_t flags{};
    if (deleteFiles)
        flags = lt::session::delete_files;

    m_session.remove_torrent(m_torrents[index], flags);
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

    lt::torrent_status st = m_torrents[index].status();
    info.handle = m_torrents[index];
    info.name = QString::fromStdString(st.name);
    info.savePath = QString::fromStdString(st.save_path);
    info.totalSize = st.total_wanted;
    info.totalDone = st.total_wanted_done;
    info.progress = st.progress;
    info.downloadRate = st.download_rate;
    info.uploadRate = st.upload_rate;
    info.numPeers = st.num_peers;
    info.numSeeds = st.num_seeds;
    info.stateString = stateToString(st.state);
    info.paused = (st.flags & lt::torrent_flags::paused) != lt::torrent_flags_t{};
    if (info.paused)
        info.stateString = tr_("state_paused");

    // Calculate ratio
    qint64 uploaded = st.total_upload;
    qint64 downloaded = st.total_download;
    info.ratio = downloaded > 0 ? static_cast<float>(uploaded) / static_cast<float>(downloaded) : 0.0f;

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
            pi.ip = QString::fromStdString(p.ip.address().to_string());
            pi.port = p.ip.port();
            pi.downloadRate = p.down_speed;
            pi.uploadRate = p.up_speed;
            pi.progress = p.progress;
            pi.client = QString::fromStdString(p.client);
            result.push_back(pi);
        }
    } catch (...) {}
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
        FileInfo fi;
        fi.path = QString::fromStdString(fs.file_path(i));
        fi.size = fs.file_size(i);
        fi.progress = fi.size > 0 ? static_cast<float>(fileProgress[static_cast<int>(i)]) / fi.size : 1.0f;
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
    } catch (...) {
        // Torrent may not be valid yet (e.g. metadata still downloading)
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

void SessionManager::setDownloadLimit(int kbps)
{
    lt::settings_pack pack;
    pack.set_int(lt::settings_pack::download_rate_limit, kbps > 0 ? kbps * 1024 : 0);
    m_session.apply_settings(pack);
}

void SessionManager::setUploadLimit(int kbps)
{
    lt::settings_pack pack;
    pack.set_int(lt::settings_pack::upload_rate_limit, kbps > 0 ? kbps * 1024 : 0);
    m_session.apply_settings(pack);
}

int SessionManager::downloadLimit() const
{
    return m_session.get_settings().get_int(lt::settings_pack::download_rate_limit) / 1024;
}

int SessionManager::uploadLimit() const
{
    return m_session.get_settings().get_int(lt::settings_pack::upload_rate_limit) / 1024;
}

void SessionManager::setListenPort(int port)
{
    lt::settings_pack pack;
    QString iface = QString("0.0.0.0:%1").arg(port);
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

void SessionManager::saveResumeData()
{
    QDir dir(resumeDataDir());
    if (!dir.exists())
        dir.mkpath(".");

    for (size_t i = 0; i < m_torrents.size(); ++i) {
        lt::torrent_status st = m_torrents[i].status();
        if (!st.has_metadata) continue;

        m_torrents[i].save_resume_data(lt::torrent_handle::save_info_dict);
    }

    // Process save_resume_data alerts
    m_session.wait_for_alert(std::chrono::seconds(3));
    std::vector<lt::alert *> alerts;
    m_session.pop_alerts(&alerts);

    for (auto *a : alerts) {
        if (auto *rd = lt::alert_cast<lt::save_resume_data_alert>(a)) {
            auto buf = lt::write_resume_data_buf(rd->params);
            lt::torrent_status st = rd->handle.status();
            QString name = QString::fromStdString(st.name);
            QString filePath = dir.filePath(name + ".resume");
            QFile file(filePath);
            if (file.open(QIODevice::WriteOnly)) {
                file.write(buf.data(), static_cast<qint64>(buf.size()));
            }
        }
    }
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

        lt::torrent_handle h = m_session.add_torrent(atp);
        m_torrents.push_back(h);
    }

    if (!m_torrents.empty())
        emit torrentsUpdated();
}

void SessionManager::updateStats()
{
    processAlerts();
    checkSeedRatios();
    if (!m_torrents.empty())
        emit torrentsUpdated();
}

void SessionManager::processAlerts()
{
    std::vector<lt::alert *> alerts;
    m_session.pop_alerts(&alerts);

    for (auto *a : alerts) {
        if (auto *fa = lt::alert_cast<lt::torrent_finished_alert>(a)) {
            QString name = QString::fromStdString(fa->torrent_name());
            emit torrentFinished(name);
        }
        if (auto *ea = lt::alert_cast<lt::torrent_error_alert>(a)) {
            emit torrentError(QString::fromStdString(ea->message()));
        }
    }
}

void SessionManager::checkSeedRatios()
{
    if (m_seedRatioLimit <= 0.0f) return;

    for (auto &h : m_torrents) {
        lt::torrent_status st = h.status();
        if (st.state != lt::torrent_status::seeding) continue;
        if (st.flags & lt::torrent_flags::paused) continue;

        float ratio = st.total_download > 0
            ? static_cast<float>(st.total_upload) / static_cast<float>(st.total_download)
            : 0.0f;

        if (ratio >= m_seedRatioLimit)
            h.pause();
    }
}

QString SessionManager::resumeDataDir() const
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/resume";
}

int SessionManager::importFromQBittorrent(const QString &defaultSavePath)
{
    // qBittorrent stores data in ~/.local/share/qBittorrent/BT_backup/
    // Each torrent has <hash>.torrent and <hash>.fastresume
    QString btBackup = QDir::homePath() + "/.local/share/qBittorrent/BT_backup";
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
                        atp = lt::load_torrent_file(torrentPath.toStdString());
                        atp.save_path = defaultSavePath.toStdString();
                    }
                }
            } else {
                atp = lt::load_torrent_file(torrentPath.toStdString());
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
        } catch (...) {
            // Skip problematic torrents
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
