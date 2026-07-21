// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include <catch2/catch_test_macros.hpp>

#include "services/vpn/wireguardconfig.h"

using bat::parseWireguardConfig;

namespace {
// 44-char base64 (43 chars + '='), the shape of a real 32-byte WireGuard key.
const QString KEY_A = QString(43, QLatin1Char('A')) + "=";
const QString KEY_B = QString(43, QLatin1Char('B')) + "=";
const QString KEY_C = QString(43, QLatin1Char('C')) + "=";
}

TEST_CASE("parseWireguardConfig: a typical provider config parses fully", "[wireguard]")
{
    const QString conf = QStringLiteral(
        "[Interface]\n"
        "PrivateKey = %1\n"
        "Address = 10.64.0.2/32, fd7a:1::2/128\n"
        "DNS = 10.64.0.1\n"
        "MTU = 1420\n"
        "\n"
        "[Peer]\n"
        "PublicKey = %2\n"
        "AllowedIPs = 0.0.0.0/0, ::/0\n"
        "Endpoint = 193.32.127.1:51820\n"
        "PersistentKeepalive = 25\n").arg(KEY_A, KEY_B);

    const auto cfg = parseWireguardConfig(conf);
    REQUIRE(cfg.valid);
    CHECK(cfg.privateKey == KEY_A);
    CHECK(cfg.addresses == QStringList{"10.64.0.2/32", "fd7a:1::2/128"});
    CHECK(cfg.dns == QStringList{"10.64.0.1"});
    CHECK(cfg.mtu == 1420);
    REQUIRE(cfg.peers.size() == 1);
    CHECK(cfg.peers[0].publicKey == KEY_B);
    CHECK(cfg.peers[0].allowedIps == QStringList{"0.0.0.0/0", "::/0"});
    CHECK(cfg.peers[0].endpoint == "193.32.127.1:51820");
    CHECK(cfg.peers[0].persistentKeepalive == 25);
    CHECK(cfg.peers[0].presharedKey.isEmpty());
}

TEST_CASE("parseWireguardConfig: comments, blank lines and odd whitespace are tolerated", "[wireguard]")
{
    const QString conf = QStringLiteral(
        "# my provider config\n"
        "[Interface]\n"
        "   PrivateKey=%1   # inline comment\n"
        "Address = 10.0.0.2/32\n"
        "\n\n"
        "[Peer]\n"
        "publickey =    %2\n"          // lower-case key, extra spaces
        "AllowedIPs = 0.0.0.0/0\n"
        "endpoint = vpn.example.com:51820\n").arg(KEY_A, KEY_B);

    const auto cfg = parseWireguardConfig(conf);
    REQUIRE(cfg.valid);
    CHECK(cfg.privateKey == KEY_A);
    CHECK(cfg.peers[0].endpoint == "vpn.example.com:51820");
}

TEST_CASE("parseWireguardConfig: PresharedKey and multiple peers", "[wireguard]")
{
    const QString conf = QStringLiteral(
        "[Interface]\nPrivateKey = %1\nAddress = 10.0.0.2/32\n"
        "[Peer]\nPublicKey = %2\nPresharedKey = %3\nAllowedIPs = 0.0.0.0/0\nEndpoint = a:51820\n"
        "[Peer]\nPublicKey = %2\nAllowedIPs = ::/0\nEndpoint = b:51820\n").arg(KEY_A, KEY_B, KEY_C);

    const auto cfg = parseWireguardConfig(conf);
    REQUIRE(cfg.valid);
    REQUIRE(cfg.peers.size() == 2);
    CHECK(cfg.peers[0].presharedKey == KEY_C);
    CHECK(cfg.peers[1].endpoint == "b:51820");
}

TEST_CASE("splitTunnelConf: adds Table=off and strips Interface DNS", "[wireguard]")
{
    const QString conf = QStringLiteral(
        "[Interface]\n"
        "PrivateKey = %1\n"
        "Address = 10.64.0.2/32\n"
        "DNS = 10.64.0.1, 10.64.0.2\n"
        "MTU = 1420\n"
        "[Peer]\n"
        "PublicKey = %2\n"
        "AllowedIPs = 0.0.0.0/0, ::/0\n"
        "Endpoint = 193.32.127.1:51820\n").arg(KEY_A, KEY_B);

    const QString split = bat::splitTunnelConf(conf);

    CHECK(split.count(QStringLiteral("Table = off")) == 1);
    CHECK(!split.contains(QStringLiteral("DNS"), Qt::CaseInsensitive));
    // the split config must still parse, with everything else intact
    const auto cfg = parseWireguardConfig(split);
    REQUIRE(cfg.valid);
    CHECK(cfg.privateKey == KEY_A);
    CHECK(cfg.mtu == 1420);
    CHECK(cfg.dns.isEmpty());
    REQUIRE(cfg.peers.size() == 1);
    CHECK(cfg.peers[0].allowedIps == QStringList{"0.0.0.0/0", "::/0"});
    CHECK(cfg.peers[0].endpoint == "193.32.127.1:51820");
}

TEST_CASE("splitTunnelConf: replaces an existing Table and is idempotent", "[wireguard]")
{
    const QString conf = QStringLiteral(
        "[Interface]\nPrivateKey = %1\nAddress = 10.0.0.2/32\ntable = 1234\ndns=1.1.1.1 # keep private\n"
        "[Peer]\nPublicKey = %2\nAllowedIPs = 0.0.0.0/0\nEndpoint = a:51820\n").arg(KEY_A, KEY_B);

    const QString once = bat::splitTunnelConf(conf);
    CHECK(once.count(QStringLiteral("Table = off")) == 1);
    CHECK(!once.contains(QStringLiteral("1234")));
    CHECK(!once.contains(QStringLiteral("1.1.1.1")));

    const QString twice = bat::splitTunnelConf(once);
    CHECK(twice.count(QStringLiteral("Table = off")) == 1);
    CHECK(parseWireguardConfig(twice).valid);
}

TEST_CASE("splitTunnelConf: leaves Peer lines alone even if named like DNS", "[wireguard]")
{
    // a hostname endpoint containing "dns" must survive — only Interface keys are filtered
    const QString conf = QStringLiteral(
        "[Interface]\nPrivateKey = %1\nAddress = 10.0.0.2/32\n"
        "[Peer]\nPublicKey = %2\nAllowedIPs = 0.0.0.0/0\nEndpoint = dns.example.com:51820\n").arg(KEY_A, KEY_B);

    const auto cfg = parseWireguardConfig(bat::splitTunnelConf(conf));
    REQUIRE(cfg.valid);
    CHECK(cfg.peers[0].endpoint == "dns.example.com:51820");
}

TEST_CASE("parseWireguardConfig: rejects incomplete or malformed configs", "[wireguard]")
{
    SECTION("no Interface section") {
        const auto cfg = parseWireguardConfig("[Peer]\nPublicKey = " + KEY_B + "\n");
        CHECK_FALSE(cfg.valid);
        CHECK_FALSE(cfg.error.isEmpty());
    }
    SECTION("missing PrivateKey") {
        const auto cfg = parseWireguardConfig("[Interface]\nAddress = 10.0.0.2/32\n[Peer]\nPublicKey = "
                                              + KEY_B + "\nAllowedIPs = 0.0.0.0/0\nEndpoint = a:1\n");
        CHECK_FALSE(cfg.valid);
    }
    SECTION("PrivateKey wrong length") {
        const auto cfg = parseWireguardConfig("[Interface]\nPrivateKey = tooShort\nAddress = 10.0.0.2/32\n"
                                              "[Peer]\nPublicKey = " + KEY_B + "\nAllowedIPs = 0.0.0.0/0\nEndpoint = a:1\n");
        CHECK_FALSE(cfg.valid);
    }
    SECTION("no Peer") {
        const auto cfg = parseWireguardConfig("[Interface]\nPrivateKey = " + KEY_A + "\nAddress = 10.0.0.2/32\n");
        CHECK_FALSE(cfg.valid);
    }
    SECTION("Endpoint without a port") {
        const auto cfg = parseWireguardConfig("[Interface]\nPrivateKey = " + KEY_A + "\nAddress = 10.0.0.2/32\n"
                                              "[Peer]\nPublicKey = " + KEY_B + "\nAllowedIPs = 0.0.0.0/0\nEndpoint = noport\n");
        CHECK_FALSE(cfg.valid);
    }
    SECTION("empty input") {
        CHECK_FALSE(parseWireguardConfig("").valid);
    }
}
