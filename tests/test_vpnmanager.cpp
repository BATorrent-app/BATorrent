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
#include <QFile>
#include <QNetworkInterface>
#include <QSettings>
#include <QTimer>

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
    QSettings s;
    for (const char *k : {"vpnSplitTunnel", "vpnLastProfileId",
                          "vpnActiveIface", "vpnActiveConf", "vpnActiveProfile"})
        s.remove(QLatin1String(k));
}

// Captures the conf path the manager hands to the tunnel — the seam for
// asserting on split-tunnel staging without a real bring-up.
class RecordingTunnel : public WgTunnel
{
public:
    using WgTunnel::WgTunnel;
    void up(const QString &confPath, const bat::WgConfig &) override
    {
        lastConf = confPath;
        m_iface = ifaceToReport;
        QTimer::singleShot(0, this, [this]() { emit connected(m_iface); });
    }
    void down() override
    {
        m_iface.clear();
        QTimer::singleShot(0, this, [this]() { emit disconnected(); });
    }
    QString interfaceName() const override { return m_iface; }
    bool isReal() const override { return false; }

    QString lastConf;
    QString ifaceToReport = QStringLiteral("wg-test");

private:
    QString m_iface;
};

// isReal → the manager persists the active-tunnel record, enabling adoption.
class AdoptableTunnel : public RecordingTunnel
{
public:
    using RecordingTunnel::RecordingTunnel;
    bool isReal() const override { return true; }
    bool adopt(const QString &confPath, const QString &) override
    {
        lastConf = confPath;
        adopted = true;
        return true;
    }
    bool adopted = false;
};

// A real, up interface on this machine (loopback usually) — adoption verifies
// the recorded iface still exists, so the fake tunnel must report a real one.
QString anUpInterface()
{
    for (const QNetworkInterface &ni : QNetworkInterface::allInterfaces())
        if (ni.flags() & QNetworkInterface::IsUp) return ni.name();
    return QString();
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
        CHECK(mgr.profiles().first().toMap().value("endpoint").toString() == "1.2.3.4:51820");
    }   // index + .conf on disk

    VpnManager restored;
    REQUIRE(restored.profileCount() == 1);
    CHECK(restored.profiles().first().toMap().value("id").toString() == id);
    CHECK(restored.profiles().first().toMap().value("name").toString() == "Mullvad");
    CHECK(restored.profiles().first().toMap().value("endpoint").toString() == "1.2.3.4:51820");
}

TEST_CASE("VpnManager: split tunnel stages a Table=off config for the tunnel", "[vpn]")
{
    freshStore();
    QSettings().setValue("vpnSplitTunnel", true);

    auto *tunnel = new RecordingTunnel;   // manager takes ownership
    VpnManager mgr(tunnel);
    const QString id = mgr.importConfig("IVPN", validConf());
    REQUIRE(!id.isEmpty());

    QSignalSpy upSpy(&mgr, &VpnManager::interfaceUp);
    mgr.connectProfile(id);
    REQUIRE((upSpy.count() > 0 || upSpy.wait(3000)));

    CHECK(tunnel->lastConf.endsWith("-split.conf"));
    QFile staged(tunnel->lastConf);
    REQUIRE(staged.open(QIODevice::ReadOnly));
    const QString text = QString::fromUtf8(staged.readAll());
    CHECK(text.contains("Table = off"));
    const auto cfg = bat::parseWireguardConfig(text);
    REQUIRE(cfg.valid);
    CHECK(cfg.dns.isEmpty());

    QSettings().remove("vpnSplitTunnel");
}

TEST_CASE("VpnManager: full tunnel hands the original config to the tunnel", "[vpn]")
{
    freshStore();
    QSettings().remove("vpnSplitTunnel");

    auto *tunnel = new RecordingTunnel;
    VpnManager mgr(tunnel);
    const QString id = mgr.importConfig("IVPN", validConf());
    REQUIRE(!id.isEmpty());

    QSignalSpy upSpy(&mgr, &VpnManager::interfaceUp);
    mgr.connectProfile(id);
    REQUIRE((upSpy.count() > 0 || upSpy.wait(3000)));
    CHECK(tunnel->lastConf.endsWith(id + ".conf"));
}

TEST_CASE("VpnManager: adopts a tunnel that outlived the previous run", "[vpn]")
{
    freshStore();
    const QString realIface = anUpInterface();
    REQUIRE(!realIface.isEmpty());

    QString id;
    {
        auto *tunnel = new AdoptableTunnel;
        tunnel->ifaceToReport = realIface;
        VpnManager mgr(tunnel);
        id = mgr.importConfig("IVPN", validConf());
        REQUIRE(!id.isEmpty());
        QSignalSpy upSpy(&mgr, &VpnManager::interfaceUp);
        mgr.connectProfile(id);
        REQUIRE((upSpy.count() > 0 || upSpy.wait(3000)));
        // real tunnel connected → the adoptable record is persisted
        CHECK(QSettings().value("vpnActiveIface").toString() == realIface);
    }   // "app quits" — tunnel record stays behind

    auto *tunnel = new AdoptableTunnel;
    VpnManager restored(tunnel);
    QSignalSpy upSpy(&restored, &VpnManager::interfaceUp);
    restored.adoptRunningTunnel();
    CHECK(restored.state() == VpnManager::State::Connected);
    CHECK(restored.connectedInterface() == realIface);
    CHECK(restored.activeProfileId() == id);
    CHECK(tunnel->adopted);
    CHECK(upSpy.count() == 1);

    // and a disconnect after adoption clears the record
    QSignalSpy downSpy(&restored, &VpnManager::interfaceDown);
    restored.disconnectVpn();
    REQUIRE((downSpy.count() > 0 || downSpy.wait(3000)));
    CHECK(QSettings().value("vpnActiveIface").toString().isEmpty());
}

TEST_CASE("VpnManager: a dead recorded tunnel is not adopted and the record is dropped", "[vpn]")
{
    freshStore();
    QSettings s;
    s.setValue("vpnActiveIface", "wg-gone0");
    s.setValue("vpnActiveConf", "/nonexistent/conf");
    s.setValue("vpnActiveProfile", "nope");

    auto *tunnel = new AdoptableTunnel;
    VpnManager mgr(tunnel);
    mgr.adoptRunningTunnel();
    CHECK(mgr.state() == VpnManager::State::Disconnected);
    CHECK_FALSE(tunnel->adopted);
    CHECK(QSettings().value("vpnActiveIface").toString().isEmpty());
}

TEST_CASE("VpnManager: interfaceDown reports whether the user asked for it", "[vpn]")
{
    freshStore();
    VpnManager mgr;
    const QString id = mgr.importConfig("IVPN", validConf());
    REQUIRE(!id.isEmpty());

    QSignalSpy upSpy(&mgr, &VpnManager::interfaceUp);
    mgr.connectProfile(id);
    REQUIRE((upSpy.count() > 0 || upSpy.wait(3000)));

    QSignalSpy downSpy(&mgr, &VpnManager::interfaceDown);
    mgr.disconnectVpn();
    REQUIRE((downSpy.count() > 0 || downSpy.wait(3000)));
    CHECK(downSpy.first().at(0).toBool() == true);   // deliberate
}
