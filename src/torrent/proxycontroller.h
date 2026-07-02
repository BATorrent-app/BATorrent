// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef PROXYCONTROLLER_H
#define PROXYCONTROLLER_H

#include <QString>
#include <libtorrent/settings_pack.hpp>

// Owns the proxy configuration (SOCKS5/HTTP, optional leak-proof mode) that used
// to be a cluster of fields + branchy logic inside SessionManager. The session
// composes one of these and applies proxySettings() whenever it changes.
namespace bat {

// Pure: the libtorrent settings for a proxy config. Kept free-standing so the
// (branchy, security-relevant) logic can be unit-tested without a live session.
//   type: 0 = none, 1 = SOCKS5, 2 = HTTP.
//   leakProof: when tunneled, also kill UPnP/NAT-PMP/LSD and force anonymous mode.
//   anonymousMode: the session-wide toggle to honor when NOT tunneled (type 0).
libtorrent::settings_pack buildProxySettings(int type, const QString &host, int port,
                                             const QString &user, const QString &pass,
                                             bool leakProof, bool anonymousMode);

class ProxyController {
public:
    // Load persisted config into memory (does not touch the session).
    void loadFromSettings();

    // Update + persist. Getters mirror what was set.
    void set(int type, const QString &host, int port, const QString &user, const QString &pass);
    void setLeakProof(bool enabled);

    int type() const { return m_type; }
    QString host() const { return m_host; }
    int port() const { return m_port; }
    QString user() const { return m_user; }
    QString pass() const { return m_pass; }
    bool leakProof() const { return m_leakProof; }

    // The libtorrent settings for the current config; apply to a session.
    libtorrent::settings_pack settings(bool anonymousMode) const
    { return buildProxySettings(m_type, m_host, m_port, m_user, m_pass, m_leakProof, anonymousMode); }

private:
    int m_type = 0;                 // 0=none, 1=SOCKS5, 2=HTTP
    QString m_host;
    int m_port = 0;
    QString m_user;
    QString m_pass;
    bool m_leakProof = true;        // kill leak vectors while a proxy is active
};

}

#endif // PROXYCONTROLLER_H
