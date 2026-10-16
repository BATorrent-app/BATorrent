// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef SERVICES_VPN_WGTUNNELFACTORY_H
#define SERVICES_VPN_WGTUNNELFACTORY_H

// The tunnel backend: the same on every platform, WireGuard in a child process
// reached over a local SOCKS5 proxy (see wgtunnel_userspace.h). Without batwg
// beside the app it reports itself as not real and the UI says so.

#include "services/vpn/wgtunnel_userspace.h"

inline WgTunnel *makeWgTunnel(QObject *parent)
{
    return new UserspaceWgTunnel(parent);
}

#endif
