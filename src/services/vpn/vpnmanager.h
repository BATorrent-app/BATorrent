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
    // --- QML surface (exposed as the `vpn` context property) ---
    Q_PROPERTY(QVariantList profiles READ profiles NOTIFY profilesChanged)
    Q_PROPERTY(int connState READ stateInt NOTIFY stateChanged)          // State as int
    Q_PROPERTY(QString activeProfileId READ activeProfileId NOTIFY stateChanged)
    Q_PROPERTY(QString connectedInterface READ connectedInterface NOTIFY stateChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY stateChanged)
    Q_PROPERTY(bool tunnelReal READ tunnelIsReal CONSTANT)
public:
    enum class State { Disconnected, Connecting, Connected, Failed };
    Q_ENUM(State)

    // Takes ownership of `tunnel`. If null, a StubWgTunnel is used — the flow
    // works but traffic is NOT protected (see tunnelIsReal()).
    explicit VpnManager(WgTunnel *tunnel = nullptr, QObject *parent = nullptr);

    // Validate + store a config. Returns the new profile id, or "" on a bad
    // config (reason in lastError()).
    Q_INVOKABLE QString importConfig(const QString &name, const QString &confText);
    // Read a .conf from a path or file: URL, then importConfig it.
    Q_INVOKABLE QString importFromFile(const QString &fileUrlOrPath, const QString &name);
    Q_INVOKABLE void removeProfile(const QString &id);
    // Rename an imported profile (display only; the .conf is untouched).
    Q_INVOKABLE void renameProfile(const QString &id, const QString &name);
    QVariantList profiles() const;             // [{ id, name }] for QML
    int profileCount() const { return int(m_profiles.size()); }

    Q_INVOKABLE void connectProfile(const QString &id);
    Q_INVOKABLE void disconnectVpn();
    // Reconnect the last-used profile (the "connect on launch" toggle).
    Q_INVOKABLE void connectLastUsed();
    // Tunnels outlive the process; if the one we brought up last run is still
    // alive, re-attach to it (state → Connected, interfaceUp re-emitted). Call
    // AFTER wiring the signals — it emits through them.
    void adoptRunningTunnel();

    State state() const { return m_state; }
    int stateInt() const { return int(m_state); }
    QString activeProfileId() const { return m_activeId; }
    QString connectedInterface() const { return m_iface; }
    QString lastError() const { return m_error; }
    bool tunnelIsReal() const;

signals:
    void stateChanged();
    void profilesChanged();
    void interfaceUp(const QString &iface);    // connected → bind torrent traffic here
    // deliberate = the user disconnected; false = the tunnel failed/dropped.
    // main() uses this to fail closed on drops when the kill switch is on.
    void interfaceDown(bool deliberate);

private:
    struct Profile { QString id; QString name; QString endpoint; };

    void setState(State s);
    void load();
    void saveIndex() const;
    void saveActiveTunnel(const QString &iface) const;
    void clearActiveTunnel() const;
    QString vpnDir() const;
    QString confPath(const QString &id) const;
    QString splitConfPath(const QString &id) const;
    int indexOf(const QString &id) const;

    QVector<Profile> m_profiles;
    WgTunnel *m_tunnel;
    State m_state = State::Disconnected;
    QString m_activeId;
    QString m_iface;
    QString m_error;
    QString m_lastTunnelConf;   // the conf handed to the tunnel (split copy or original)
    bool m_userDown = false;
};

#endif
