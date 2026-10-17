// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "app/vpnwiring.h"

#include "services/vpn/vpnmanager.h"
#include "torrent/iengine.h"

#include <QSettings>
#include <QTimer>
#include <QVariantMap>

namespace VpnWiring {

// The in-app VPN is a SOCKS5 proxy on 127.0.0.1 (batwg). Up: the session
// sends everything through it. Down on purpose: the session goes back to the
// way it was. Dropped with the kill switch on: the session is pointed at a
// proxy that is not there, so the torrents stop and nothing else on the
// machine is affected; Disconnect lifts it. "vpnHold" remembers the VPN was
// on, so the next launch holds traffic and reconnects.
static void wireSocks(QObject *context, VpnManager *vpn, IEngine *eng)
{
    auto setProxy = [eng](int port, const QString &user = QString(), const QString &pass = QString()) {
        eng->applySetting(QStringLiteral("vpnProxy"),
                          QVariantMap{ {QStringLiteral("port"), port},
                                       {QStringLiteral("user"), user},
                                       {QStringLiteral("pass"), pass} });
    };
    QObject::connect(vpn, &VpnManager::interfaceUp, context, [vpn, eng, setProxy](const QString &) {
        const QVariantMap s = vpn->socks();
        if (s.value(QStringLiteral("port")).toInt() <= 0) return;
        setProxy(s.value(QStringLiteral("port")).toInt(), s.value(QStringLiteral("user")).toString(),
                 s.value(QStringLiteral("pass")).toString());
        if (vpn->forwardedPort() > 0)
            eng->applySetting(QStringLiteral("vpnAnnouncePort"), vpn->forwardedPort());
        QSettings(QStringLiteral("BATorrent"), QStringLiteral("BATorrent")).setValue(QStringLiteral("vpnHold"), true);
    });
    // a port forwarded by the VPN is what peers are told to reach us on
    QObject::connect(vpn, &VpnManager::forwardedPortChanged, context, [vpn, eng]() {
        if (vpn->state() == VpnManager::State::Connected)
            eng->applySetting(QStringLiteral("vpnAnnouncePort"), vpn->forwardedPort());
    });
    QObject::connect(vpn, &VpnManager::interfaceDown, context, [setProxy](bool deliberate) {
        QSettings s(QStringLiteral("BATorrent"), QStringLiteral("BATorrent"));
        // unset reads as on, the way the VPN page shows it
        if (!deliberate && s.value(QStringLiteral("killSwitchEnabled"), true).toBool()) {
            setProxy(1);
            return;
        }
        setProxy(0);
        s.remove(QStringLiteral("vpnHold"));
    });
    QSettings s(QStringLiteral("BATorrent"), QStringLiteral("BATorrent"));
    const QString last = s.value(QStringLiteral("vpnLastProfileId")).toString();
    bool known = false;
    for (const QVariant &p : vpn->profiles())
        known = known || p.toMap().value(QStringLiteral("id")).toString() == last;
    const bool hold = s.value(QStringLiteral("vpnHold"), false).toBool();
    // Held for a VPN that can no longer come back (its profile is gone, or the
    // VPN component is): let the torrents go rather than hold them forever.
    if (hold && (!known || !vpn->tunnelIsReal())) {
        setProxy(0);
        s.remove(QStringLiteral("vpnHold"));
        return;
    }
    if (known && vpn->tunnelIsReal() && (hold || s.value(QStringLiteral("vpnAutoConnect"), false).toBool()))
        QTimer::singleShot(500, vpn, &VpnManager::connectLastUsed);
}

void wire(QObject *context, VpnManager *vpn, IEngine *eng)
{
    if (vpn->viaSocks()) {
        wireSocks(context, vpn, eng);
        return;
    }
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
