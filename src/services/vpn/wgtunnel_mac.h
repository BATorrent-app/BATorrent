// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef SERVICES_VPN_WGTUNNEL_MAC_H
#define SERVICES_VPN_WGTUNNEL_MAC_H

// macOS WireGuard tunnel: drives `wg-quick` (bundled in the app, else Homebrew)
// up/down under an administrator prompt (osascript). Split-tunnel is handled a
// level up: VpnManager hands us a Table=off config and the engine binds to the
// utun. If the tools are absent we fail with a clear message, not pretend.

#include "services/vpn/wgtunnel.h"

#include <functional>

#ifdef Q_OS_MACOS

class QProcess;

class MacWgTunnel : public WgTunnel
{
    Q_OBJECT
public:
    explicit MacWgTunnel(QObject *parent = nullptr) : WgTunnel(parent) {}

    void up(const QString &confPath, const bat::WgConfig &cfg) override;
    void down() override;
    QString interfaceName() const override { return m_iface; }
    // Only a real tunnel when the system wg-quick is actually present — otherwise
    // the UI should keep showing the "not protecting" banner (honest).
    bool isReal() const override { return !wgQuickPath().isEmpty(); }
    bool adopt(const QString &confPath, const QString &iface) override;

private:
    // wg-quick + wireguard-go locations (Homebrew arm64/x86 + system).
    static QString toolsBinDir();
    static QString wgQuickPath();
    // Run `cmd` as admin via osascript; async, calls done(ok, output/err).
    void runElevated(const QString &shellCmd, std::function<void(bool, QString)> done);

    QString m_iface;
    QString m_confPath;
};

#endif // Q_OS_MACOS

#endif
