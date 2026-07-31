// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details
//
// SessionManager — applySetting router, advanced/storage prefs, content layout,
// excluded patterns, auto-move/extract, and on-complete command.

#include "torrent/sessionmanager.h"
#include "torrent/sessionconfig.h"

#include <libtorrent/settings_pack.hpp>
#include <libtorrent/torrent_info.hpp>
#include <QSettings>
#include <QDebug>
#include <QProcess>
#include <vector>

// --- Advanced / storage prefs ---

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
    QStringList paths;
    paths.reserve(numFiles);
    for (lt::file_index_t i(0); i < files.end_file(); ++i)
        paths << QString::fromStdString(files.file_path(i));

    const auto hit = SessionConfig::excludedFileIndexes(m_excludedFilePatterns, paths);
    if (hit.empty()) return;

    if (atp.file_priorities.empty())
        atp.file_priorities.resize(numFiles, lt::default_priority);
    for (int i : hit)
        atp.file_priorities[i] = lt::dont_download;
}

