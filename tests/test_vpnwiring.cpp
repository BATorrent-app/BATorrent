// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details
//
// VpnWiring::resync: what a new engine process is told about the VPN. A tester
// found magnets stalled while direct downloads went on: the engine had been
// restarted after a crash and read only the saved settings, so it held traffic
// for a tunnel that was up and never heard the proxy.

#include <catch2/catch_test_macros.hpp>

#include "app/vpnwiring.h"
#include "services/vpn/vpnmanager.h"
#include "torrent/sessionmanager.h"
#include "httptestserver.h"   // ensureApp() (event loop + test-mode AppData)
#include "nointernet.h"

#include <QDir>
#include <QSettings>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTimer>
#include <QVariantMap>

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

void freshStore()
{
    httptest::ensureApp();
    nointernet::install();
    QCoreApplication::setOrganizationName(QStringLiteral("BATorrentVpnWiringTest"));
    QCoreApplication::setApplicationName(QStringLiteral("BATorrentVpnWiringTest"));
    QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/vpn").removeRecursively();
    QSettings("BATorrent", "BATorrent").clear();
}

// A tunnel that comes up at once and says it is a local SOCKS5 proxy, as batwg does.
class SocksTunnel : public WgTunnel
{
public:
    void up(const QString &, const bat::WgConfig &) override
    {
        m_up = true;
        QTimer::singleShot(0, this, [this]() { emit connected(QStringLiteral("batwg")); });
    }
    void down() override
    {
        m_up = false;
        QTimer::singleShot(0, this, [this]() { emit disconnected(); });
    }
    QString interfaceName() const override { return m_up ? QStringLiteral("batwg") : QString(); }
    bool isReal() const override { return false; }
    bool viaSocks() const override { return true; }
    QVariantMap socks() const override
    {
        if (!m_up) return {};
        return { {QStringLiteral("port"), 40123}, {QStringLiteral("user"), QStringLiteral("u")},
                 {QStringLiteral("pass"), QStringLiteral("p")} };
    }

private:
    bool m_up = false;
};

// The engine side: only what it was told.
class RecordingEngine : public SessionManager
{
public:
    bool applySetting(const QString &key, const QVariant &v) override
    {
        told.insert(key, v);
        return true;
    }
    int proxyPort() const { return told.value(QStringLiteral("vpnProxy")).toMap().value(QStringLiteral("port")).toInt(); }
    QVariantMap told;
};

} // namespace

TEST_CASE("VpnWiring: a new engine is given the proxy of a VPN that is up", "[vpn][wiring]")
{
    freshStore();
    VpnManager vpn(new SocksTunnel);
    const QString id = vpn.importConfig(QStringLiteral("Proton"), validConf());
    REQUIRE(!id.isEmpty());
    QSignalSpy upSpy(&vpn, &VpnManager::interfaceUp);
    vpn.connectProfile(id);
    REQUIRE((upSpy.count() > 0 || upSpy.wait(3000)));
    REQUIRE(vpn.state() == VpnManager::State::Connected);

    RecordingEngine eng;
    VpnWiring::resync(&vpn, &eng);
    const QVariantMap p = eng.told.value(QStringLiteral("vpnProxy")).toMap();
    CHECK(p.value(QStringLiteral("port")).toInt() == 40123);
    CHECK(p.value(QStringLiteral("user")).toString() == QStringLiteral("u"));
    CHECK(p.value(QStringLiteral("pass")).toString() == QStringLiteral("p"));
}

TEST_CASE("VpnWiring: a new engine holds traffic for a dropped VPN only with the kill switch", "[vpn][wiring]")
{
    freshStore();
    VpnManager vpn(new SocksTunnel);
    QSettings st(QStringLiteral("BATorrent"), QStringLiteral("BATorrent"));

    // The engine is created first, as in the app: it must not write the
    // kill switch's default ("off") over a value that was never set.
    RecordingEngine held;
    REQUIRE_FALSE(st.contains(QStringLiteral("killSwitchEnabled")));
    st.setValue(QStringLiteral("vpnHold"), true);   // kill switch never set: on
    VpnWiring::resync(&vpn, &held);
    CHECK(held.proxyPort() == 1);

    st.setValue(QStringLiteral("killSwitchEnabled"), false);
    RecordingEngine open;
    VpnWiring::resync(&vpn, &open);
    CHECK(open.told.contains(QStringLiteral("vpnProxy")));
    CHECK(open.proxyPort() == 0);

    st.setValue(QStringLiteral("vpnHold"), false);
    st.setValue(QStringLiteral("killSwitchEnabled"), true);
    RecordingEngine off;
    VpnWiring::resync(&vpn, &off);
    CHECK(off.proxyPort() == 0);
}
