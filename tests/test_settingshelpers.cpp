// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include <catch2/catch_test_macros.hpp>

#include "services/platform/settingsbackup.h"
#include "services/platform/settingspolicy.h"

#include <QJsonDocument>
#include <QJsonObject>

TEST_CASE("localPath strips file: URLs", "[settingsbackup]")
{
    REQUIRE(SettingsBackup::localPath(QStringLiteral("/tmp/a.json")) == QStringLiteral("/tmp/a.json"));
    REQUIRE(SettingsBackup::localPath(QStringLiteral("file:///tmp/a.json")) == QStringLiteral("/tmp/a.json"));
}

TEST_CASE("export strips secret keys only", "[settingsbackup]")
{
    QVariantMap map;
    map.insert(QStringLiteral("listenPort"), 6881);
    map.insert(QStringLiteral("proxyPass"), QStringLiteral("secret"));
    map.insert(QStringLiteral("webUiPasswordHash"), QStringLiteral("hash"));
    map.insert(QStringLiteral("theme"), QStringLiteral("dark"));

    const QJsonObject stripped = SettingsBackup::settingsObjectFromMap(map, true);
    REQUIRE(stripped.contains(QStringLiteral("listenPort")));
    REQUIRE(stripped.contains(QStringLiteral("theme")));
    REQUIRE_FALSE(stripped.contains(QStringLiteral("proxyPass")));
    REQUIRE_FALSE(stripped.contains(QStringLiteral("webUiPasswordHash")));

    const QJsonObject full = SettingsBackup::settingsObjectFromMap(map, false);
    REQUIRE(full.contains(QStringLiteral("proxyPass")));
    REQUIRE(SettingsBackup::isExportSecret(QStringLiteral("plexToken")));
    REQUIRE_FALSE(SettingsBackup::isExportSecret(QStringLiteral("listenPort")));
}

TEST_CASE("BATBACKUP1 round-trips entries", "[settingsbackup]")
{
    QList<QPair<QString, QByteArray>> entries = {
        {QStringLiteral("settings.json"), QByteArrayLiteral("{\"a\":1}")},
        {QStringLiteral("resume/abc.resume"), QByteArrayLiteral("payload")},
    };
    const QByteArray packed = SettingsBackup::pack(entries);
    REQUIRE(packed.startsWith("BATBACKUP1\n"));

    const auto unpacked = SettingsBackup::unpack(packed);
    REQUIRE(unpacked.ok);
    REQUIRE(unpacked.entries.size() == 2);
    REQUIRE(unpacked.entries[0].first == QStringLiteral("settings.json"));
    REQUIRE(unpacked.entries[0].second == QByteArrayLiteral("{\"a\":1}"));
    REQUIRE(unpacked.entries[1].first == QStringLiteral("resume/abc.resume"));
    REQUIRE(unpacked.entries[1].second == QByteArrayLiteral("payload"));
}

TEST_CASE("BATBACKUP1 rejects bad magic and truncated frames", "[settingsbackup]")
{
    REQUIRE_FALSE(SettingsBackup::unpack(QByteArrayLiteral("NOPE")).ok);
    REQUIRE_FALSE(SettingsBackup::unpack(QByteArrayLiteral("BATBACKUP1\n")).ok);

    // Count claims 1 entry but no name/payload follows.
    QByteArray truncated("BATBACKUP1\n");
    const quint32 count = 1;
    truncated.append(reinterpret_cast<const char *>(&count), 4);
    REQUIRE_FALSE(SettingsBackup::unpack(truncated).ok);

    // Oversized name length.
    QByteArray fatName("BATBACKUP1\n");
    fatName.append(reinterpret_cast<const char *>(&count), 4);
    const quint32 nl = 5000;
    fatName.append(reinterpret_cast<const char *>(&nl), 4);
    REQUIRE_FALSE(SettingsBackup::unpack(fatName).ok);
}

TEST_CASE("telegram event bits and mask updates", "[settingspolicy]")
{
    REQUIRE(SettingsPolicy::telegramEventBit(QStringLiteral("telegramEvtFinished")) == (1 << 0));
    REQUIRE(SettingsPolicy::telegramEventBit(QStringLiteral("telegramEvtError")) == (1 << 3));
    REQUIRE(SettingsPolicy::telegramEventBit(QStringLiteral("listenPort")) == 0);

    REQUIRE(SettingsPolicy::applyTelegramEventMask(0x0F, 1 << 1, false) == 0x0D);
    REQUIRE(SettingsPolicy::applyTelegramEventMask(0x0D, 1 << 1, true) == 0x0F);
}

TEST_CASE("ui bool key table covers registry traps", "[settingspolicy]")
{
    REQUIRE(SettingsPolicy::isUiBoolKey(QStringLiteral("closeToTray")));
    REQUIRE(SettingsPolicy::isUiBoolKey(QStringLiteral("blockBadPeers")));
    REQUIRE_FALSE(SettingsPolicy::isUiBoolKey(QStringLiteral("listenPort")));
}

TEST_CASE("autoComplete index maps known day buckets", "[settingspolicy]")
{
    REQUIRE(SettingsPolicy::autoCompleteIndex(0) == 0);
    REQUIRE(SettingsPolicy::autoCompleteIndex(3 * 86400) == 2);
    REQUIRE(SettingsPolicy::autoCompleteIndex(30 * 86400) == 5);
    REQUIRE(SettingsPolicy::autoCompleteIndex(2 * 86400) == 0); // unknown → 0
    REQUIRE(SettingsPolicy::autoCompleteSeconds(4) == 14 * 86400);
    REQUIRE(SettingsPolicy::autoCompleteSeconds(99) == 0);
}

TEST_CASE("proxy presets resolve known tunnels", "[settingspolicy]")
{
    const auto mullvad = SettingsPolicy::proxyPreset(QStringLiteral("mullvad"));
    REQUIRE(mullvad.known);
    REQUIRE_FALSE(mullvad.keepHost);
    REQUIRE(mullvad.host == QStringLiteral("10.64.0.1"));
    REQUIRE(mullvad.port == 1080);
    REQUIRE(mullvad.type == 1);

    const auto tor = SettingsPolicy::proxyPreset(QStringLiteral("tor"));
    REQUIRE(tor.host == QStringLiteral("127.0.0.1"));
    REQUIRE(tor.port == 9050);

    const auto other = SettingsPolicy::proxyPreset(QStringLiteral("airvpn"));
    REQUIRE(other.known);
    REQUIRE(other.keepHost);
}

TEST_CASE("engine mode and webui remote policy", "[settingspolicy]")
{
    REQUIRE(SettingsPolicy::engineModeValue(true) == QStringLiteral("ipc"));
    REQUIRE(SettingsPolicy::engineModeValue(false) == QStringLiteral("inprocess"));
    REQUIRE(SettingsPolicy::engineSplitFromMode(QStringLiteral("ipc")));
    REQUIRE_FALSE(SettingsPolicy::engineSplitFromMode(QStringLiteral("inprocess")));

    REQUIRE(SettingsPolicy::webUiRemoteAllowed(true, true));
    REQUIRE_FALSE(SettingsPolicy::webUiRemoteAllowed(true, false));
    REQUIRE_FALSE(SettingsPolicy::webUiRemoteAllowed(false, true));
}

TEST_CASE("pairing password uses alphabet and length", "[settingspolicy]")
{
    const QString alphabet = SettingsPolicy::pairingAlphabet();
    REQUIRE_FALSE(alphabet.contains(QLatin1Char('0')));
    REQUIRE_FALSE(alphabet.contains(QLatin1Char('O')));
    REQUIRE_FALSE(alphabet.contains(QLatin1Char('1')));
    REQUIRE_FALSE(alphabet.contains(QLatin1Char('I')));
    REQUIRE_FALSE(alphabet.contains(QLatin1Char('l')));

    int calls = 0;
    const QString pw = SettingsPolicy::generatePairingPassword(14, [&](int n) {
        ++calls;
        REQUIRE(n == alphabet.size());
        return 0;
    });
    REQUIRE(calls == 14);
    REQUIRE(pw.size() == 14);
    REQUIRE(pw == QString(14, alphabet.at(0)));
}

TEST_CASE("adv choking UI maps libtorrent rate_based", "[settingspolicy]")
{
    REQUIRE(SettingsPolicy::advChokingUiIndex(2) == 1);
    REQUIRE(SettingsPolicy::advChokingUiIndex(0) == 0);
    REQUIRE(SettingsPolicy::advChokingUiIndex(1) == 0);
}
