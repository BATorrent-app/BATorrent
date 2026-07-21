// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details
//
// VpnManager over the StubWgTunnel: import/validation, the connect→connected→
// disconnect state machine, and the profile persistence round-trip. The real
// per-OS wireguard-go bring-up is out of scope here.

#include <catch2/catch_test_macros.hpp>

#include "services/vpn/vpnmanager.h"
#include "httptestserver.h"   // ensureApp() (event loop + test-mode AppData)

#include <QSignalSpy>
#include <QStandardPaths>
#include <QDir>

namespace {

const QString KEY_A = QString(43, QLatin1Char('A')) + "=";
const QString KEY_B = QString(43, QLatin1Char('B')) + "=";

QString validConf()
{
    return QStringLiteral(
        "[Interface]\nPrivateKey = %1\nAddress = 10.64.0.2/32\nDNS = 10.64.0.1\n"
        "[Peer]\nPublicKey = %2\nAllowedIPs = 0.0.0.0/0, ::/0\nEndpoint = 1.2.3.4:51820\n")
        .arg(KEY_A, KEY_B);
}

// VpnManager persists to (test-mode) AppData/vpn — clear it so each case is clean.
void freshStore()
{
    httptest::ensureApp();
    QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/vpn").removeRecursively();
}

} // namespace

TEST_CASE("VpnManager: imports a valid config and rejects a bad one", "[vpn]")
{
    freshStore();
    VpnManager mgr;   // default StubWgTunnel

    const QString id = mgr.importConfig("IVPN", validConf());
    REQUIRE(!id.isEmpty());
    CHECK(mgr.profileCount() == 1);
    CHECK(mgr.profiles().first().toMap().value("name").toString() == "IVPN");

    CHECK(mgr.importConfig("junk", "not a wireguard config").isEmpty());
    CHECK_FALSE(mgr.lastError().isEmpty());
    CHECK(mgr.profileCount() == 1);   // the bad import didn't add anything

    mgr.removeProfile(id);
    CHECK(mgr.profileCount() == 0);
}

TEST_CASE("VpnManager: connect → connected → disconnect over the stub tunnel", "[vpn]")
{
    freshStore();
    VpnManager mgr;
    const QString id = mgr.importConfig("IVPN", validConf());
    REQUIRE(!id.isEmpty());

    QSignalSpy stateSpy(&mgr, &VpnManager::stateChanged);
    QSignalSpy upSpy(&mgr, &VpnManager::interfaceUp);

    mgr.connectProfile(id);
    CHECK(mgr.state() == VpnManager::State::Connecting);   // synchronous first hop

    REQUIRE((upSpy.count() > 0 || upSpy.wait(3000)));
    CHECK(mgr.state() == VpnManager::State::Connected);
    CHECK_FALSE(mgr.connectedInterface().isEmpty());
    CHECK(mgr.activeProfileId() == id);

    QSignalSpy downSpy(&mgr, &VpnManager::interfaceDown);
    mgr.disconnectVpn();
    REQUIRE((downSpy.count() > 0 || downSpy.wait(3000)));
    CHECK(mgr.state() == VpnManager::State::Disconnected);
    CHECK(mgr.connectedInterface().isEmpty());
}

TEST_CASE("VpnManager: profiles survive a restart", "[vpn]")
{
    freshStore();
    QString id;
    {
        VpnManager mgr;
        id = mgr.importConfig("Mullvad", validConf());
        REQUIRE(!id.isEmpty());
    }   // index + .conf on disk

    VpnManager restored;
    REQUIRE(restored.profileCount() == 1);
    CHECK(restored.profiles().first().toMap().value("id").toString() == id);
    CHECK(restored.profiles().first().toMap().value("name").toString() == "Mullvad");
}
