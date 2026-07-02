// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details
//
// SessionManager — network/privacy slice. VPN interface binding + kill switch,
// proxy (SOCKS5/HTTP, leak-proof mode) and the peer-blocklist IP filter. Split
// out of sessionmanager.cpp verbatim; no behaviour change.

#include "torrent/sessionmanager.h"
#include "torrent/ipblocklist.h"

#include <libtorrent/settings_pack.hpp>
#include <libtorrent/ip_filter.hpp>
#include <libtorrent/torrent_flags.hpp>
#include <QSettings>
#include <QNetworkInterface>
#include <QAbstractSocket>
#include <QFile>
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
    m_proxy.set(type, host, port, user, pass);
    m_session.apply_settings(m_proxy.settings(m_anonymousMode));
}

void SessionManager::setProxyLeakProof(bool enabled)
{
    m_proxy.setLeakProof(enabled);
    // Re-apply the active proxy so the change takes effect immediately.
    m_session.apply_settings(m_proxy.settings(m_anonymousMode));
}

bool SessionManager::proxyLeakProof() const { return m_proxy.leakProof(); }

int SessionManager::proxyType() const { return m_proxy.type(); }
QString SessionManager::proxyHost() const { return m_proxy.host(); }
int SessionManager::proxyPort() const { return m_proxy.port(); }
QString SessionManager::proxyUser() const { return m_proxy.user(); }
QString SessionManager::proxyPass() const { return m_proxy.pass(); }

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

    int count = 0;
    const lt::ip_filter filter = bat::parseP2pBlocklist(QString::fromUtf8(file.readAll()), &count);
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
