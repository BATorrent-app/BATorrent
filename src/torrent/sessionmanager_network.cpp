// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details
//
// SessionManager — network/privacy slice. VPN interface binding + kill switch,
// proxy (SOCKS5/HTTP, leak-proof mode) and the peer-blocklist IP filter. Split
// out of sessionmanager.cpp verbatim; no behaviour change.

#include "torrent/sessionmanager.h"

#include <libtorrent/settings_pack.hpp>
#include <libtorrent/ip_filter.hpp>
#include <libtorrent/torrent_flags.hpp>
#include <boost/asio/ip/address.hpp>
#include <QSettings>
#include <QNetworkInterface>
#include <QAbstractSocket>
#include <QFile>
#include <QTextStream>
#include <QString>
#include <QDebug>

// --- VPN / Interface binding ---

void SessionManager::setOutgoingInterface(const QString &interfaceName)
{
    qDebug() << "[session] setOutgoingInterface:" << interfaceName;
    m_outgoingInterface = interfaceName;
    QSettings("BATorrent", "BATorrent").setValue("outgoingInterface", interfaceName);
    m_killSwitchActive = false;
    // Resume torrents that the killswitch paused — otherwise switching VPNs
    // or going back to "any interface" leaves them paused forever.
    for (auto &h : m_killSwitchPaused) {
        if (h.is_valid()) h.resume();
    }
    m_killSwitchPaused.clear();

    lt::settings_pack pack;

    if (interfaceName.isEmpty()) {
        // Any interface
        pack.set_str(lt::settings_pack::outgoing_interfaces, "");
        int port = listenPort();
        QString listenIface = QString("0.0.0.0:%1").arg(port > 0 ? port : 6881);
        pack.set_str(lt::settings_pack::listen_interfaces, listenIface.toStdString());
    } else {
        pack.set_str(lt::settings_pack::outgoing_interfaces, interfaceName.toStdString());

        // Resolve the interface IP for listen_interfaces
        QNetworkInterface ni = QNetworkInterface::interfaceFromName(interfaceName);
        QString listenAddr = "0.0.0.0";
        for (const auto &entry : ni.addressEntries()) {
            if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol) {
                listenAddr = entry.ip().toString();
                break;
            }
        }
        int port = listenPort();
        QString listenIface = QString("%1:%2").arg(listenAddr).arg(port > 0 ? port : 6881);
        pack.set_str(lt::settings_pack::listen_interfaces, listenIface.toStdString());
    }

    m_session.apply_settings(pack);
}

QString SessionManager::outgoingInterface() const
{
    return m_outgoingInterface;
}

void SessionManager::setKillSwitchEnabled(bool enabled)
{
    m_killSwitchEnabled = enabled;
    QSettings("BATorrent", "BATorrent").setValue("killSwitchEnabled", enabled);
    if (!enabled) {
        m_killSwitchActive = false;
        for (auto &h : m_killSwitchPaused) {
            if (h.is_valid()) h.resume();
        }
        m_killSwitchPaused.clear();
    }
}

bool SessionManager::killSwitchEnabled() const
{
    return m_killSwitchEnabled;
}

void SessionManager::setAutoResumeOnReconnect(bool enabled)
{
    m_autoResume = enabled;
    QSettings("BATorrent", "BATorrent").setValue("autoResumeOnReconnect", enabled);
}

bool SessionManager::autoResumeOnReconnect() const
{
    return m_autoResume;
}

void SessionManager::checkInterfaceStatus()
{
    if (m_outgoingInterface.isEmpty() || !m_killSwitchEnabled)
        return;

    QNetworkInterface ni = QNetworkInterface::interfaceFromName(m_outgoingInterface);
    bool isUp = ni.isValid()
                && (ni.flags() & QNetworkInterface::IsUp)
                && (ni.flags() & QNetworkInterface::IsRunning);

    // Check if interface has an IPv4 address
    bool hasIp = false;
    if (isUp) {
        for (const auto &entry : ni.addressEntries()) {
            if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol) {
                hasIp = true;
                break;
            }
        }
    }

    bool interfaceOk = isUp && hasIp;

    if (!interfaceOk && !m_killSwitchActive) {
        // Interface just went down — pause all running torrents
        qDebug() << "[session] KILL SWITCH TRIGGERED — interface" << m_outgoingInterface << "is down";
        m_killSwitchActive = true;
        m_killSwitchPaused.clear();
        for (auto &h : m_torrents) {
            if (!h.is_valid()) continue;
            lt::torrent_status st = h.status();
            if (!(st.flags & lt::torrent_flags::paused)) {
                h.pause();
                m_killSwitchPaused.insert(h);
            }
        }
        emit killSwitchTriggered();
    } else if (interfaceOk && m_killSwitchActive) {
        // Interface came back — re-apply binding and optionally resume
        m_killSwitchActive = false;
        setOutgoingInterface(m_outgoingInterface);

        if (m_autoResume) {
            for (auto &h : m_killSwitchPaused) {
                if (h.is_valid())
                    h.resume();
            }
        }
        m_killSwitchPaused.clear();
        qDebug() << "[session] VPN interface restored:" << m_outgoingInterface;
        emit interfaceRestored();
    }
}

// --- Proxy ---

void SessionManager::setProxySettings(int type, const QString &host, int port,
                                       const QString &user, const QString &pass)
{
    m_proxyType = type;
    m_proxyHost = host;
    m_proxyPort = port;
    m_proxyUser = user;
    m_proxyPass = pass;

    QSettings st("BATorrent", "BATorrent");
    st.setValue("proxyType", type);
    st.setValue("proxyHost", host);
    st.setValue("proxyPort", port);
    st.setValue("proxyUser", user);
    st.setValue("proxyPass", pass);

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
        pack.set_bool(lt::settings_pack::anonymous_mode, m_anonymousMode);
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
        if (m_proxyLeakProof) {
            // UPnP/NAT-PMP punch a port map advertising the real WAN IP; LSD
            // broadcasts it on the LAN — both bypass the proxy. Kill them, and
            // scrub the client fingerprint while tunneled.
            pack.set_bool(lt::settings_pack::enable_upnp, false);
            pack.set_bool(lt::settings_pack::enable_natpmp, false);
            pack.set_bool(lt::settings_pack::enable_lsd, false);
            pack.set_bool(lt::settings_pack::anonymous_mode, true);
        }
    }
    m_session.apply_settings(pack);
}

void SessionManager::setProxyLeakProof(bool enabled)
{
    m_proxyLeakProof = enabled;
    QSettings("BATorrent", "BATorrent").setValue("proxyLeakProof", enabled);
    // Re-apply the active proxy so the change takes effect immediately.
    setProxySettings(m_proxyType, m_proxyHost, m_proxyPort, m_proxyUser, m_proxyPass);
}

bool SessionManager::proxyLeakProof() const { return m_proxyLeakProof; }

int SessionManager::proxyType() const { return m_proxyType; }
QString SessionManager::proxyHost() const { return m_proxyHost; }
int SessionManager::proxyPort() const { return m_proxyPort; }
QString SessionManager::proxyUser() const { return m_proxyUser; }
QString SessionManager::proxyPass() const { return m_proxyPass; }

// --- IP Filter ---

void SessionManager::loadIpFilter(const QString &filePath)
{
    m_ipFilterPath = filePath;
    m_ipFilterCount = 0;

    if (filePath.isEmpty()) {
        clearIpFilter();
        return;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;

    lt::ip_filter filter;
    QTextStream in(&file);
    int count = 0;

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith('#'))
            continue;

        // P2P format: "description:startIP-endIP"
        int colonIdx = line.lastIndexOf(':');
        QString range;
        if (colonIdx >= 0 && line.indexOf('.', colonIdx) > 0)
            range = line.mid(colonIdx + 1).trimmed();
        else
            range = line;

        int dashIdx = range.indexOf('-');
        if (dashIdx < 0) continue;

        QString startIp = range.left(dashIdx).trimmed();
        QString endIp = range.mid(dashIdx + 1).trimmed();

        try {
            boost::system::error_code ec1, ec2;
            auto addr1 = boost::asio::ip::make_address(startIp.toStdString(), ec1);
            auto addr2 = boost::asio::ip::make_address(endIp.toStdString(), ec2);
            if (ec1 || ec2) continue;
            filter.add_rule(addr1, addr2, lt::ip_filter::blocked);
            ++count;
        } catch (...) { /* skip an unparseable blocklist range */ }
    }

    m_session.set_ip_filter(filter);
    m_ipFilterCount = count;
}

void SessionManager::clearIpFilter()
{
    m_ipFilterPath.clear();
    m_ipFilterCount = 0;
    m_session.set_ip_filter(lt::ip_filter());
}

QString SessionManager::ipFilterPath() const { return m_ipFilterPath; }
int SessionManager::ipFilterCount() const { return m_ipFilterCount; }
