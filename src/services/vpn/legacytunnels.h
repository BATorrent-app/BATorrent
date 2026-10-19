// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef SERVICES_VPN_LEGACYTUNNELS_H
#define SERVICES_VPN_LEGACYTUNNELS_H

// The in-app VPN of the 4.8 betas installed WireGuard as a Windows service
// (wireguard.exe /installtunnelservice), named WireGuardTunnel$<profile>. Such
// a service outlives the app, starts with Windows and can route the whole
// machine through a tunnel nothing manages any more: one tester was left
// without internet that way. 5.0 no longer installs anything, so it looks for
// what an old build left and offers to remove it.

#include <QObject>
#include <QString>
#include <QStringList>

#include <functional>

namespace LegacyTunnels {

// A service the old in-app VPN installed: a WireGuard tunnel service whose
// command line runs the wireguard.exe that BATorrent shipped, or one of
// BATorrent's own profile files. A WireGuard tunnel the person set up with
// the WireGuard app is never matched.
bool isOurs(const QString &serviceName, const QString &imagePath);

// The names of such services on this machine (Windows only; empty elsewhere).
QStringList find();

// Stop and delete them, with one administrator prompt. Answers through done
// (true when none is left), on the receiver's thread.
void remove(const QStringList &names, QObject *receiver, std::function<void(bool)> done);

}

#endif
