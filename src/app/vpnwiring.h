// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef BATORRENT_VPNWIRING_H
#define BATORRENT_VPNWIRING_H

class QObject;
class VpnManager;
class IEngine;

// Bind VpnManager interface up/down to the engine's outgoingInterface + kill-switch.
namespace VpnWiring {

void wire(QObject *context, VpnManager *vpn, IEngine *eng);
// A new engine process (after a crash, the app starts another) reads only the
// saved settings: it would hold traffic for a VPN that is in fact up, and
// never hear the proxy. Tell it where things stand.
void resync(VpnManager *vpn, IEngine *eng);

} // namespace VpnWiring

#endif
