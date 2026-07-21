// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef SERVICES_VPN_WIREGUARDCONFIG_H
#define SERVICES_VPN_WIREGUARDCONFIG_H

// Parse a WireGuard .conf (the file a VPN provider — IVPN, Mullvad, Proton —
// hands the user) into a validated struct. Pure and unit-tested; the actual
// tunnel bring-up lives elsewhere. Format is the standard wg-quick INI:
//
//   [Interface]
//   PrivateKey = <44-char base64>
//   Address    = 10.64.0.2/32, fd7a:1::2/128
//   DNS        = 10.64.0.1
//   MTU        = 1420            (optional)
//   [Peer]
//   PublicKey    = <44-char base64>
//   PresharedKey = <44-char base64>   (optional)
//   AllowedIPs   = 0.0.0.0/0, ::/0
//   Endpoint     = 193.32.127.1:51820
//   PersistentKeepalive = 25          (optional)

#include <QString>
#include <QStringList>
#include <QVector>

namespace bat {

struct WgPeer {
    QString publicKey;
    QString presharedKey;
    QString endpoint;                 // host:port
    QStringList allowedIps;           // CIDRs
    int persistentKeepalive = 0;      // seconds; 0 = off
};

struct WgConfig {
    QString privateKey;
    QStringList addresses;            // interface CIDRs
    QStringList dns;
    int mtu = 0;                      // 0 = default
    QVector<WgPeer> peers;
    bool valid = false;
    QString error;                    // set when !valid
};

namespace detail {

// Comma-separated list, each item trimmed, empties dropped.
inline QStringList wgList(const QString &v)
{
    QStringList out;
    const auto parts = v.split(QLatin1Char(','), Qt::SkipEmptyParts);
    for (const QString &p : parts) {
        const QString t = p.trimmed();
        if (!t.isEmpty()) out << t;
    }
    return out;
}

// A WireGuard key is a 32-byte value in base64 → 44 chars ending in '='. Reject
// obvious garbage (a pasted half-config) without being a full base64 validator.
inline bool looksLikeWgKey(const QString &k)
{
    if (k.size() != 44 || !k.endsWith(QLatin1Char('=')))
        return false;
    for (const QChar c : k) {
        if (!(c.isLetterOrNumber() || c == QLatin1Char('+') || c == QLatin1Char('/')
              || c == QLatin1Char('=')))
            return false;
    }
    return true;
}

} // namespace detail

inline WgConfig parseWireguardConfig(const QString &text)
{
    WgConfig cfg;
    enum class Section { None, Interface, Peer } section = Section::None;
    bool sawInterface = false;

    const auto fail = [&cfg](const QString &why) { cfg.valid = false; cfg.error = why; return cfg; };

    for (QString raw : text.split(QLatin1Char('\n'))) {
        // strip inline comments and trim; wg-quick treats '#' as a comment start
        const int hash = raw.indexOf(QLatin1Char('#'));
        if (hash >= 0) raw = raw.left(hash);
        const QString line = raw.trimmed();
        if (line.isEmpty()) continue;

        if (line.startsWith(QLatin1Char('[')) && line.endsWith(QLatin1Char(']'))) {
            const QString name = line.mid(1, line.size() - 2).trimmed().toLower();
            if (name == QLatin1String("interface")) { section = Section::Interface; sawInterface = true; }
            else if (name == QLatin1String("peer"))  { section = Section::Peer; cfg.peers.push_back(WgPeer{}); }
            else section = Section::None;   // unknown section: ignore its keys
            continue;
        }

        const int eq = line.indexOf(QLatin1Char('='));
        if (eq < 0) continue;               // not a key=value line
        const QString key = line.left(eq).trimmed().toLower();
        const QString val = line.mid(eq + 1).trimmed();
        if (key.isEmpty()) continue;

        if (section == Section::Interface) {
            if (key == QLatin1String("privatekey"))      cfg.privateKey = val;
            else if (key == QLatin1String("address"))    cfg.addresses = detail::wgList(val);
            else if (key == QLatin1String("dns"))        cfg.dns = detail::wgList(val);
            else if (key == QLatin1String("mtu"))        cfg.mtu = val.toInt();
        } else if (section == Section::Peer && !cfg.peers.isEmpty()) {
            WgPeer &p = cfg.peers.back();
            if (key == QLatin1String("publickey"))            p.publicKey = val;
            else if (key == QLatin1String("presharedkey"))    p.presharedKey = val;
            else if (key == QLatin1String("endpoint"))        p.endpoint = val;
            else if (key == QLatin1String("allowedips"))      p.allowedIps = detail::wgList(val);
            else if (key == QLatin1String("persistentkeepalive")) p.persistentKeepalive = val.toInt();
        }
    }

    // --- validate ---
    if (!sawInterface)                       return fail(QStringLiteral("missing [Interface] section"));
    if (cfg.privateKey.isEmpty())            return fail(QStringLiteral("missing PrivateKey"));
    if (!detail::looksLikeWgKey(cfg.privateKey)) return fail(QStringLiteral("PrivateKey is not a valid WireGuard key"));
    if (cfg.addresses.isEmpty())             return fail(QStringLiteral("missing interface Address"));
    if (cfg.peers.isEmpty())                 return fail(QStringLiteral("missing [Peer] section"));

    for (const WgPeer &p : cfg.peers) {
        if (!detail::looksLikeWgKey(p.publicKey)) return fail(QStringLiteral("a Peer is missing a valid PublicKey"));
        if (p.endpoint.isEmpty() || !p.endpoint.contains(QLatin1Char(':')))
            return fail(QStringLiteral("a Peer is missing a host:port Endpoint"));
        if (p.allowedIps.isEmpty())          return fail(QStringLiteral("a Peer is missing AllowedIPs"));
        if (!p.presharedKey.isEmpty() && !detail::looksLikeWgKey(p.presharedKey))
            return fail(QStringLiteral("a Peer's PresharedKey is invalid"));
    }

    cfg.valid = true;
    return cfg;
}

// Split-tunnel variant of a config. `Table = off` stops wg-quick / the Windows
// client from installing the default route, so only sockets explicitly bound to
// the tunnel interface (the torrent session) go through the VPN. DNS lines are
// dropped too — the provider's resolver sits inside the tunnel and is
// unreachable without that route, and a half-set system DNS would break name
// resolution for everything else.
inline QString splitTunnelConf(const QString &text)
{
    QStringList out;
    bool inInterface = false;
    for (const QString &raw : text.split(QLatin1Char('\n'))) {
        QString line = raw;
        const int hash = line.indexOf(QLatin1Char('#'));
        if (hash >= 0) line = line.left(hash);
        line = line.trimmed();

        if (line.startsWith(QLatin1Char('[')) && line.endsWith(QLatin1Char(']'))) {
            inInterface = line.mid(1, line.size() - 2).trimmed().toLower()
                          == QLatin1String("interface");
            out << raw;
            if (inInterface) out << QStringLiteral("Table = off");
            continue;
        }
        if (inInterface) {
            const int eq = line.indexOf(QLatin1Char('='));
            const QString key = eq >= 0 ? line.left(eq).trimmed().toLower() : QString();
            if (key == QLatin1String("dns") || key == QLatin1String("table"))
                continue;
        }
        out << raw;
    }
    return out.join(QLatin1Char('\n'));
}

} // namespace bat

#endif
