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
