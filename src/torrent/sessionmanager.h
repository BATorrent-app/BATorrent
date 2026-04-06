// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef SESSIONMANAGER_H
#define SESSIONMANAGER_H

#include "types.h"
#include <QObject>
#include <QTimer>
#include <QString>
#include <QStringList>
#include <libtorrent/session.hpp>
#include <libtorrent/torrent_handle.hpp>
#include <libtorrent/torrent_status.hpp>
#include <QMap>
#include <vector>
#include <set>

class SessionManager : public QObject
{
    Q_OBJECT
public:
    explicit SessionManager(QObject *parent = nullptr);
    ~SessionManager();

    void addTorrent(const QString &filePath, const QString &savePath);
    void addMagnet(const QString &uri, const QString &savePath);
    void removeTorrent(int index, bool deleteFiles = false);
    void pauseTorrent(int index);
    void resumeTorrent(int index);
    void pauseAll();
    void resumeAll();

    int torrentCount() const;
    TorrentInfo torrentAt(int index) const;
    std::vector<PeerInfo> peersAt(int index) const;
    std::vector<FileInfo> filesAt(int index) const;
    std::vector<TrackerInfo> trackersAt(int index) const;
    void addTracker(int index, const QString &url);

    void setFilePriority(int torrentIndex, int fileIndex, int priority);
    void setSequentialDownload(int index, bool sequential);

    // Categories
    void setTorrentCategory(int index, const QString &category);
    QStringList categories() const;

    // Piece map
    std::vector<bool> piecesAt(int index) const;

    void setDownloadLimit(int kbps);
    void setUploadLimit(int kbps);
    int downloadLimit() const;
    int uploadLimit() const;

    // Network settings
    void setListenPort(int port);
    int listenPort() const;
    void setMaxConnections(int max);
    int maxConnections() const;
    void setDhtEnabled(bool enabled);
    bool dhtEnabled() const;
    void setEncryptionMode(int mode); // 0=enabled, 1=forced, 2=disabled
    int encryptionMode() const;

    // VPN / Interface binding
    void setOutgoingInterface(const QString &interfaceName); // "" = any
    QString outgoingInterface() const;
    void setKillSwitchEnabled(bool enabled);
    bool killSwitchEnabled() const;
    void setAutoResumeOnReconnect(bool enabled);
    bool autoResumeOnReconnect() const;

    // Seed limits
    void setSeedRatioLimit(float ratio);
    float seedRatioLimit() const;

    // Auto-move completed downloads
    void setAutoMove(bool enabled, const QString &path);
    bool autoMoveEnabled() const;
    QString autoMovePath() const;

    // Download queue
    void setMaxActiveDownloads(int max);
    int maxActiveDownloads() const;
    void setTorrentQueuePosition(int index, int position);

    // Proxy
    void setProxySettings(int type, const QString &host, int port,
                          const QString &user, const QString &pass);
    int proxyType() const;
    QString proxyHost() const;
    int proxyPort() const;
    QString proxyUser() const;
    QString proxyPass() const;

    // IP filter
    void loadIpFilter(const QString &filePath);
    void clearIpFilter();
    QString ipFilterPath() const;
    int ipFilterCount() const;

    // Bandwidth scheduler
    void setAltSpeedLimits(int downKbps, int upKbps);
    int altDownloadLimit() const;
    int altUploadLimit() const;
    void setSchedulerEnabled(bool enabled);
    bool schedulerEnabled() const;
    void setScheduleFromHour(int hour);
    void setScheduleToHour(int hour);
    int scheduleFromHour() const;
    int scheduleToHour() const;
    void setScheduleDays(int daysMask); // bit 0=Mon..6=Sun
    int scheduleDays() const;
    bool altSpeedsActive() const;

    void saveResumeData();
    void loadResumeData();

    // Global statistics
    qint64 globalDownloaded() const;
    qint64 globalUploaded() const;
    float globalRatio() const;

    // Per-session statistics
    qint64 sessionDownloaded() const;
    qint64 sessionUploaded() const;

    // Torrent count tracking
    void incrementTorrentCount();
    int totalTorrentsAdded() const;

    int importFromQBittorrent(const QString &defaultSavePath);

signals:
    void torrentAdded(int index);
    void torrentRemoved(int index);
    void torrentsUpdated();
    void torrentFinished(const QString &name);
    void torrentError(const QString &message);
    void killSwitchTriggered();
    void interfaceRestored();

private slots:
    void updateStats();

private:
    static QString stateToString(lt::torrent_status::state_t state);
    void processAlerts();
    void checkSeedRatios();
    void checkInterfaceStatus();
    void checkBandwidthSchedule();
    QString resumeDataDir() const;

    lt::session m_session;
    std::vector<lt::torrent_handle> m_torrents;
    QTimer m_updateTimer;
    bool m_dhtEnabled = true;
    int m_encryptionMode = 0;
    float m_seedRatioLimit = 0.0f; // 0 = no limit

    // Auto-move
    bool m_autoMoveEnabled = false;
    QString m_autoMovePath;

    // Download queue
    int m_maxActiveDownloads = 0; // 0 = unlimited
    std::set<lt::torrent_handle> m_queuePaused; // torrents paused by queue logic
    void enforceDownloadQueue();

    // Global stats (accumulated from previous sessions)
    qint64 m_globalDownBase = 0;
    qint64 m_globalUpBase = 0;

    // Categories per torrent (hash -> category name)
    QMap<QString, QString> m_categories;

    // VPN / Interface binding
    QString m_outgoingInterface;
    bool m_killSwitchEnabled = false;
    bool m_autoResume = false;
    bool m_killSwitchActive = false;
    std::set<lt::torrent_handle> m_killSwitchPaused;

    // Proxy
    int m_proxyType = 0; // 0=none, 1=SOCKS5, 2=HTTP
    QString m_proxyHost;
    int m_proxyPort = 0;
    QString m_proxyUser;
    QString m_proxyPass;

    // IP filter
    QString m_ipFilterPath;
    int m_ipFilterCount = 0;

    // Bandwidth scheduler
    int m_altDownLimit = 0;
    int m_altUpLimit = 0;
    bool m_schedulerEnabled = false;
    int m_scheduleFromHour = 1;
    int m_scheduleToHour = 7;
    int m_scheduleDays = 0x7F; // all days
    bool m_altSpeedsActive = false;
    int m_normalDownLimit = 0;
    int m_normalUpLimit = 0;
};

#endif
