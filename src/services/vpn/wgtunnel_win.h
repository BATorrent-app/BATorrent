// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef SERVICES_VPN_WGTUNNEL_WIN_H
#define SERVICES_VPN_WGTUNNEL_WIN_H

// Windows WireGuard tunnel: installs the config as a WireGuard tunnel *service*
// via the official `wireguard.exe` (/installtunnelservice), elevated through UAC
// (ShellExecuteEx "runas"). wireguard.exe uses the WireGuardNT driver + wintun.
// Full-tunnel for now. Not compilable or testable off Windows — the CI
// build-windows job compiles it and the tester exercises it.

#include "services/vpn/wgtunnel.h"

#include <functional>

#ifdef Q_OS_WIN

class QTimer;

class WinWgTunnel : public WgTunnel
{
    Q_OBJECT
public:
    explicit WinWgTunnel(QObject *parent = nullptr) : WgTunnel(parent) {}

    void up(const QString &confPath, const bat::WgConfig &cfg) override;
    void down() override;
    QString interfaceName() const override { return m_iface; }
    // Real only when wireguard.exe is actually present (bundled or installed).
    bool isReal() const override { return !wireguardExe().isEmpty(); }
    bool adopt(const QString &confPath, const QString &iface) override;

private:
    // wireguard.exe: bundled beside the app, else a system WireGuard install.
    static QString wireguardExe();
    // Run wireguard.exe with args elevated (UAC); poll for exit → done(ok).
    void runElevated(const QStringList &args, std::function<void(bool)> done);

    QString m_iface;       // tunnel service name = config basename
    QString m_confPath;
};

#endif // Q_OS_WIN

#endif
