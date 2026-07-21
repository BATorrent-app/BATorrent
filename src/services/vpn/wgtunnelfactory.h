// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef SERVICES_VPN_WGTUNNELFACTORY_H
#define SERVICES_VPN_WGTUNNELFACTORY_H

// Pick the WgTunnel backend for this platform: the real per-OS tunnel where one
// exists (mac/Windows/Linux), the stub anywhere else.

#include "services/vpn/wgtunnel.h"
#ifdef Q_OS_MACOS
#include "services/vpn/wgtunnel_mac.h"
#elif defined(Q_OS_WIN)
#include "services/vpn/wgtunnel_win.h"
#elif defined(Q_OS_LINUX)
#include "services/vpn/wgtunnel_linux.h"
#endif

inline WgTunnel *makeWgTunnel(QObject *parent)
{
#ifdef Q_OS_MACOS
    return new MacWgTunnel(parent);
#elif defined(Q_OS_WIN)
    return new WinWgTunnel(parent);
#elif defined(Q_OS_LINUX)
    return new WgTunnelLinux(parent);
#else
    return new StubWgTunnel(parent);
#endif
}

#endif
