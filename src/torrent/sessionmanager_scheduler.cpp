// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details
//
// SessionManager — bandwidth scheduler + watched-folder slice. Split out of
// sessionmanager.cpp verbatim; no behaviour change.

#include "torrent/sessionmanager.h"
#include "torrent/bandwidthschedule.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QSettings>
#include <QStandardPaths>
#include <QTimer>
#include <QDateTime>

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
        // Move the .torrent to a "processed" subfolder to avoid re-adding.
        // rename() fails silently if a same-named file was archived before —
        // the leftover then re-adds the torrent on every scan (reported as
        // removed torrents "coming back"). Clear the slot first.
        QDir processed(dir.filePath(".processed"));
        if (!processed.exists()) processed.mkpath(".");
        QFile::remove(processed.filePath(f));
        if (!QFile::rename(path, processed.filePath(f)))
            qWarning() << "[session] watched folder: couldn't archive" << f;
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


void SessionManager::checkBandwidthSchedule()
{
    if (!m_schedulerEnabled || (m_altDownLimit == 0 && m_altUpLimit == 0))
        return;

    const QDateTime now = QDateTime::currentDateTime();
    const int currentHour = now.time().hour();
    const int dayOfWeek = now.date().dayOfWeek() - 1; // Qt: Mon=1, we want Mon=0
    const bool inSchedule = bat::inBandwidthSchedule(
        dayOfWeek, currentHour, m_scheduleDays, m_scheduleFromHour, m_scheduleToHour);

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

