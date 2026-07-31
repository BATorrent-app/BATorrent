// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "app/vpnwiring.h"

#include "services/vpn/vpnmanager.h"
#include "torrent/iengine.h"

#include <QSettings>
#include <QTimer>

namespace VpnWiring {

void wire(QObject *context, VpnManager *vpn, IEngine *eng)
{
    QObject::connect(vpn, &VpnManager::interfaceUp, context, [vpn, eng](const QString &iface) {
        if (!vpn->tunnelIsReal()) return;
        QSettings s;
        if (!s.contains(QStringLiteral("preVpnInterface")))
            s.setValue(QStringLiteral("preVpnInterface"), s.value(QStringLiteral("outgoingInterface")).toString());
        eng->applySetting(QStringLiteral("outgoingInterface"), iface);
    });
    QObject::connect(vpn, &VpnManager::interfaceDown, context, [vpn, eng](bool deliberate) {
        if (!vpn->tunnelIsReal()) return;
        QSettings s;
        if (!s.contains(QStringLiteral("preVpnInterface"))) return;
        if (!deliberate && s.value(QStringLiteral("killSwitchEnabled"), false).toBool()) return;
        eng->applySetting(QStringLiteral("outgoingInterface"),
                          s.value(QStringLiteral("preVpnInterface")).toString());
        s.remove(QStringLiteral("preVpnInterface"));
    });
    vpn->adoptRunningTunnel();
    if (vpn->state() != VpnManager::State::Connected) {
        QSettings s;
        if (s.contains(QStringLiteral("preVpnInterface"))
            && !s.value(QStringLiteral("killSwitchEnabled"), false).toBool()) {
            eng->applySetting(QStringLiteral("outgoingInterface"),
                              s.value(QStringLiteral("preVpnInterface")).toString());
            s.remove(QStringLiteral("preVpnInterface"));
        }
    }
    if (vpn->tunnelIsReal()
        && QSettings().value(QStringLiteral("vpnAutoConnect"), false).toBool()) {
        QTimer::singleShot(1500, vpn, &VpnManager::connectLastUsed);
    }
}

} // namespace VpnWiring
