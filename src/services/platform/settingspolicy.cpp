// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "services/platform/settingspolicy.h"

#include <QSet>

namespace SettingsPolicy {

int telegramEventBit(const QString &key)
{
    if (key == QLatin1String("telegramEvtFinished")) return 1 << 0;
    if (key == QLatin1String("telegramEvtKill"))     return 1 << 1;
    if (key == QLatin1String("telegramEvtRss"))      return 1 << 2;
    if (key == QLatin1String("telegramEvtError"))    return 1 << 3;
    return 0;
}

int applyTelegramEventMask(int mask, int bit, bool on)
{
    return on ? (mask | bit) : (mask & ~bit);
}

bool isUiBoolKey(const QString &key)
{
    // Force a real bool: Windows registry stores bool as DWORD and reads it
    // back as int, so a raw `=== true` check in QML fails.
    static const QSet<QString> keys = {
        QStringLiteral("closeToTray"), QStringLiteral("showSplash"), QStringLiteral("startTray"),
        QStringLiteral("notifSound"), QStringLiteral("randomPort"), QStringLiteral("autoShutdown"),
        QStringLiteral("autoTrackers"), QStringLiteral("addPublicTrackers"),
        QStringLiteral("assocTorrent"), QStringLiteral("assocMagnet"), QStringLiteral("assocBittorrent"),
        QStringLiteral("detailBottom"), QStringLiteral("showDownloadChip"),
        QStringLiteral("torrentSearchEnabled"),
        QStringLiteral("useDefaultPath"), QStringLiteral("verboseLogging"), QStringLiteral("useTor"),
        QStringLiteral("plexEnabled"), QStringLiteral("jellyfinEnabled"), QStringLiteral("tourSeen"),
        QStringLiteral("warnSuspiciousFiles"), QStringLiteral("autoDefenderExclude"),
        QStringLiteral("autoplayNext"), QStringLiteral("preferNativeLang"),
        QStringLiteral("gameAutoInstall"), QStringLiteral("blockBadPeers"),
        QStringLiteral("vpnSplitTunnel"), QStringLiteral("vpnAutoConnect")
    };
    return keys.contains(key);
}

namespace {
constexpr qint64 kAutoCompleteDays[] = {0, 1, 3, 7, 14, 30};
constexpr int kAutoCompleteCount = 6;
}

int autoCompleteIndex(qint64 seconds)
{
    const int d = int(seconds / 86400);
    for (int i = 0; i < kAutoCompleteCount; ++i)
        if (kAutoCompleteDays[i] == d) return i;
    return 0;
}

qint64 autoCompleteSeconds(int index)
{
    if (index < 0 || index >= kAutoCompleteCount) return 0;
    return kAutoCompleteDays[index] * 86400;
}

ProxyPreset proxyPreset(const QString &name)
{
    ProxyPreset p;
    if (name == QLatin1String("mullvad")) {
        p.known = true;
        p.host = QStringLiteral("10.64.0.1");
        p.port = 1080;
        return p;
    }
    if (name == QLatin1String("tor")) {
        p.known = true;
        p.host = QStringLiteral("127.0.0.1");
        p.port = 9050;
        return p;
    }
    // SOCKS5-capable tunnels without a known local endpoint — type only.
    p.known = true;
    p.keepHost = true;
    return p;
}

QString engineModeValue(bool split)
{
    return split ? QStringLiteral("ipc") : QStringLiteral("inprocess");
}

bool engineSplitFromMode(const QString &mode)
{
    return mode == QLatin1String("ipc");
}

bool webUiRemoteAllowed(bool remoteRequested, bool hasAuth)
{
    return remoteRequested && hasAuth;
}

QString pairingAlphabet()
{
    return QStringLiteral("ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnpqrstuvwxyz23456789");
}

QString generatePairingPassword(int length, const std::function<int(int)> &bounded)
{
    const QString alphabet = pairingAlphabet();
    QString pw;
    pw.reserve(length);
    for (int i = 0; i < length; ++i)
        pw.append(alphabet.at(bounded(alphabet.size())));
    return pw;
}

int advChokingUiIndex(int libtorrentAlgo)
{
    return libtorrentAlgo == 2 ? 1 : 0;
}

} // namespace SettingsPolicy
