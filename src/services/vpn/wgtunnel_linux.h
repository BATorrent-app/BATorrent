// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef SERVICES_VPN_WGTUNNEL_LINUX_H
#define SERVICES_VPN_WGTUNNEL_LINUX_H

// Linux WireGuard tunnel: drives the system `wg-quick` up/down, elevated through
// PolicyKit (`pkexec`, the desktop password prompt). Kernel WireGuard where
// available, wireguard-go otherwise — wg-quick decides. Full-tunnel for now.
// Not compilable/testable off Linux; CI build-linux compiles it, the tester runs
// it. A Linux interface name is capped at 15 chars, so the tunnel is brought up
// under a fixed short name rather than the (UUID) config id.

#include "services/vpn/wgtunnel.h"

#include <functional>

#ifdef Q_OS_LINUX

class WgTunnelLinux : public WgTunnel
{
    Q_OBJECT
public:
    explicit WgTunnelLinux(QObject *parent = nullptr) : WgTunnel(parent) {}

    void up(const QString &confPath, const bat::WgConfig &cfg) override;
    void down() override;
    QString interfaceName() const override { return m_iface; }
    bool isReal() const override { return !wgQuickPath().isEmpty(); }

private:
    static QString wgQuickPath();
    // Run `argv` elevated via pkexec; async → done(ok, output).
    void runElevated(const QStringList &argv, std::function<void(bool, QString)> done);

    QString m_iface;          // fixed short name (≤15 chars) — see header note
    QString m_activeConf;     // the short-named .conf we brought up
};

#endif // Q_OS_LINUX

#endif
