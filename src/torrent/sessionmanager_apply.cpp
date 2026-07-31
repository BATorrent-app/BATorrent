// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details
//
// SessionManager — key→setter routing for live settings (in-process + IPC).

#include "torrent/sessionmanager.h"
#include "torrent/sessionconfig.h"

#include <QVariant>

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

