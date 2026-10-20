// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef SERVICES_VPN_VPNMANAGER_H
#define SERVICES_VPN_VPNMANAGER_H

// Owns imported WireGuard profiles and the connection state machine. Validates
// a pasted/loaded .conf with the parser, stores it (owner-only perms: it holds
// a private key), and drives a WgTunnel to connect/disconnect. What goes through
// the tunnel is left to the app (VpnWiring): on connect the torrent session is
// pointed at it, and only the torrent session.

#include "services/vpn/wgtunnel.h"

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVector>

class QTimer;

class GeoIpResolver;

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
    // The port the VPN forwards to us (0: none); peers can reach in on it.
    Q_PROPERTY(int forwardedPort READ forwardedPort NOTIFY forwardedPortChanged)
    // Seconds until the next automatic try after a drop; 0 when none is due.
    Q_PROPERTY(int retryIn READ retryIn NOTIFY retryChanged)
public:
    enum class State { Disconnected, Connecting, Connected, Failed };
    Q_ENUM(State)

    // Takes ownership of `tunnel`. If null, a StubWgTunnel is used: the flow
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
    // AFTER wiring the signals: it emits through them.
    void adoptRunningTunnel();

    State state() const { return m_state; }
    int stateInt() const { return int(m_state); }
    QString activeProfileId() const { return m_activeId; }
    QString connectedInterface() const { return m_iface; }
    QString lastError() const { return m_error; }
    bool tunnelIsReal() const;
    int forwardedPort() const { return m_fwdPort; }
    // Tunnel services an old build's in-app VPN left installed (Windows), and
    // their removal with one administrator prompt: answers legacyTunnelsRemoved.
    Q_INVOKABLE QStringList legacyTunnels() const;
    Q_INVOKABLE void removeLegacyTunnels();
    int retryIn() const;
    // The tunnel is a local SOCKS5 proxy (batwg) rather than an interface;
    // socks() is {port, user, pass, ip} while connected.
    bool viaSocks() const { return m_tunnel && m_tunnel->viaSocks(); }
    QVariantMap socks() const { return m_tunnel ? m_tunnel->socks() : QVariantMap(); }

signals:
    void stateChanged();
    void profilesChanged();
    void forwardedPortChanged();
    void legacyTunnelsRemoved(bool ok);
    void retryChanged();
    void interfaceUp(const QString &iface);    // connected → bind torrent traffic here
    // deliberate = the user disconnected; false = the tunnel failed/dropped.
    // main() uses this to fail closed on drops when the kill switch is on.
    void interfaceDown(bool deliberate);

private:
    struct Profile { QString id; QString name; QString endpoint; QString cc; QString pendingIp; };

    // Which country a profile lands in: TeoVeo asked for flags beside the
    // profiles, and the endpoint is the only honest source (a config named
    // "germany" can point anywhere). Resolved lazily, cached in memory only.
    void resolveCountry(const QString &id);

    void setState(State s);
    void load();
    void saveIndex() const;
    void saveActiveTunnel(const QString &iface) const;
    void clearActiveTunnel() const;
    QString vpnDir() const;
    QString confPath(const QString &id) const;
    int indexOf(const QString &id) const;

    GeoIpResolver *m_geoIp = nullptr;

    QVector<Profile> m_profiles;
    WgTunnel *m_tunnel;
    State m_state = State::Disconnected;
    QString m_activeId;
    QString m_iface;
    QString m_error;
    QString m_lastTunnelConf;   // the conf handed to the tunnel
    bool m_userDown = false;

    // Automatic reconnect: once a tunnel has been up (or was brought up on
    // the app's own, at launch), a drop is followed by new tries, 5 s, 10 s,
    // 30 s, 1 min, then every 2 min, until it is back or the person
    // disconnects. Never for a failure that trying again cannot fix.
    void scheduleRetry();
    void stopRetry();
    QTimer *m_retryTimer = nullptr;
    QTimer *m_retryTick = nullptr;   // ticks retryIn down for the page
    qint64 m_retryAt = 0;            // ms since epoch
    int m_retries = 0;
    bool m_keepUp = false;
    QString m_retryId;
    int m_fwdPort = 0;
};

#endif
