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

} // namespace VpnWiring

#endif
