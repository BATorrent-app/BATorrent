// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#pragma once

#include <QString>
#include <QVariant>
#include <functional>

// Pure settings-surface policy: key maps, presets, and coercion rules that used
// to live inline in QmlSettingsBridge. Keeps the bridge as glue over Session /
// QSettings / SecretStore.
namespace SettingsPolicy {

int telegramEventBit(const QString &key); // 0 if not a telegramEvt* toggle
int applyTelegramEventMask(int mask, int bit, bool on);

bool isUiBoolKey(const QString &key);

// How a stored toggle must be read back. QSettings does not round-trip a bool:
// the Windows registry returns it as an int and the Linux INI store as a
// string, so comparing against true in QML read every saved toggle as off, and
// comparing against false read a saved off as on — which left default-on
// settings impossible to turn off on Windows.
bool boolFromStored(const QVariant &v, bool def);

// UI combo index ↔ auto-complete days (0/1/3/7/14/30). Unknown day counts → 0.
int autoCompleteIndex(qint64 seconds);
qint64 autoCompleteSeconds(int index);

struct ProxyPreset {
    bool known = false;
    bool keepHost = false; // true = SOCKS5 type only; leave host/port alone
    int type = 1;          // 1 = SOCKS5
    QString host;
    int port = 0;
};
ProxyPreset proxyPreset(const QString &name);

QString engineModeValue(bool split); // "ipc" / "inprocess"
bool engineSplitFromMode(const QString &mode);

// Never expose an unauthenticated WebUI to the LAN.
bool webUiRemoteAllowed(bool remoteRequested, bool hasAuth);

// Readable alphabet (no 0/O/1/I/l) for phone pairing passwords.
QString pairingAlphabet();
QString generatePairingPassword(int length, const std::function<int(int)> &bounded);

int advChokingUiIndex(int libtorrentAlgo); // lt rate_based=2 → UI idx 1

} // namespace SettingsPolicy
