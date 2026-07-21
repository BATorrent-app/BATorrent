// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef SERVICES_VPN_VPNMANAGER_H
#define SERVICES_VPN_VPNMANAGER_H

// Owns imported WireGuard profiles and the connection state machine. Validates
// a pasted/loaded .conf with the parser, stores it (owner-only perms — it holds
// a private key), and drives a WgTunnel to connect/disconnect. Split-tunnel is
// left to the app: on connect it emits the interface name so main() can bind the
// torrent session to it (reusing the existing outgoing-interface + kill-switch).

#include "services/vpn/wgtunnel.h"

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVector>

class VpnManager : public QObject
{
    Q_OBJECT
public:
    enum class State { Disconnected, Connecting, Connected, Failed };
    Q_ENUM(State)

    // Takes ownership of `tunnel`. If null, a StubWgTunnel is used — the flow
    // works but traffic is NOT protected (see tunnelIsReal()).
    explicit VpnManager(WgTunnel *tunnel = nullptr, QObject *parent = nullptr);

    // Validate + store a config. Returns the new profile id, or "" on a bad
    // config (reason in lastError()).
    QString importConfig(const QString &name, const QString &confText);
    void removeProfile(const QString &id);
    QVariantList profiles() const;             // [{ id, name }] for QML
    int profileCount() const { return int(m_profiles.size()); }

    void connectProfile(const QString &id);
    void disconnectVpn();

    State state() const { return m_state; }
    QString activeProfileId() const { return m_activeId; }
    QString connectedInterface() const { return m_iface; }
    QString lastError() const { return m_error; }
    bool tunnelIsReal() const;

signals:
    void stateChanged();
    void profilesChanged();
    void interfaceUp(const QString &iface);    // connected → bind torrent traffic here
    void interfaceDown();

private:
    struct Profile { QString id; QString name; };

    void setState(State s);
    void load();
    void saveIndex() const;
    QString vpnDir() const;
    QString confPath(const QString &id) const;
    int indexOf(const QString &id) const;

    QVector<Profile> m_profiles;
    WgTunnel *m_tunnel;
    State m_state = State::Disconnected;
    QString m_activeId;
    QString m_iface;
    QString m_error;
};

#endif
