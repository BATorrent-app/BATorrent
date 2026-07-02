// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "torrent/proxycontroller.h"

#include <QSettings>

namespace bat {

namespace lt = libtorrent;

lt::settings_pack buildProxySettings(int type, const QString &host, int port,
                                     const QString &user, const QString &pass,
                                     bool leakProof, bool anonymousMode)
{
    lt::settings_pack pack;
    if (type == 0) {
        pack.set_int(lt::settings_pack::proxy_type, lt::settings_pack::none);
        pack.set_bool(lt::settings_pack::proxy_peer_connections, false);
        pack.set_bool(lt::settings_pack::proxy_tracker_connections, false);
        pack.set_bool(lt::settings_pack::proxy_hostnames, false);
        // Restore the leak vectors leak-proof mode had disabled.
        pack.set_bool(lt::settings_pack::enable_upnp, true);
        pack.set_bool(lt::settings_pack::enable_natpmp, true);
        pack.set_bool(lt::settings_pack::enable_lsd, true);
        pack.set_bool(lt::settings_pack::anonymous_mode, anonymousMode);
    } else {
        int ltType = (type == 1) ? lt::settings_pack::socks5_pw : lt::settings_pack::http_pw;
        if (user.isEmpty())
            ltType = (type == 1) ? lt::settings_pack::socks5 : lt::settings_pack::http;
        pack.set_int(lt::settings_pack::proxy_type, ltType);
        pack.set_str(lt::settings_pack::proxy_hostname, host.toStdString());
        pack.set_int(lt::settings_pack::proxy_port, port);
        if (!user.isEmpty()) {
            pack.set_str(lt::settings_pack::proxy_username, user.toStdString());
            pack.set_str(lt::settings_pack::proxy_password, pass.toStdString());
        }
        // Route EVERYTHING through the tunnel — peers, trackers, and DNS — so
        // neither the real IP nor lookups leak. For SOCKS5, libtorrent carries
        // uTP/DHT over UDP ASSOCIATE automatically (a TCP-only proxy will drop
        // those — the leak-proof toggle disables the rest of the leak vectors).
        pack.set_bool(lt::settings_pack::proxy_peer_connections, true);
        pack.set_bool(lt::settings_pack::proxy_tracker_connections, true);
        pack.set_bool(lt::settings_pack::proxy_hostnames, true);
        if (leakProof) {
            // UPnP/NAT-PMP punch a port map advertising the real WAN IP; LSD
            // broadcasts it on the LAN — both bypass the proxy. Kill them, and
            // scrub the client fingerprint while tunneled.
            pack.set_bool(lt::settings_pack::enable_upnp, false);
            pack.set_bool(lt::settings_pack::enable_natpmp, false);
            pack.set_bool(lt::settings_pack::enable_lsd, false);
            pack.set_bool(lt::settings_pack::anonymous_mode, true);
        }
    }
    return pack;
}

void ProxyController::loadFromSettings()
{
    QSettings s(QStringLiteral("BATorrent"), QStringLiteral("BATorrent"));
    m_type = s.value(QStringLiteral("proxyType"), 0).toInt();
    m_host = s.value(QStringLiteral("proxyHost")).toString();
    m_port = s.value(QStringLiteral("proxyPort"), 0).toInt();
    m_user = s.value(QStringLiteral("proxyUser")).toString();
    m_pass = s.value(QStringLiteral("proxyPass")).toString();
    m_leakProof = s.value(QStringLiteral("proxyLeakProof"), true).toBool();
}

void ProxyController::set(int type, const QString &host, int port,
                          const QString &user, const QString &pass)
{
    m_type = type;
    m_host = host;
    m_port = port;
    m_user = user;
    m_pass = pass;

    QSettings st(QStringLiteral("BATorrent"), QStringLiteral("BATorrent"));
    st.setValue(QStringLiteral("proxyType"), type);
    st.setValue(QStringLiteral("proxyHost"), host);
    st.setValue(QStringLiteral("proxyPort"), port);
    st.setValue(QStringLiteral("proxyUser"), user);
    st.setValue(QStringLiteral("proxyPass"), pass);
}

void ProxyController::setLeakProof(bool enabled)
{
    m_leakProof = enabled;
    QSettings(QStringLiteral("BATorrent"), QStringLiteral("BATorrent"))
        .setValue(QStringLiteral("proxyLeakProof"), enabled);
}

}
