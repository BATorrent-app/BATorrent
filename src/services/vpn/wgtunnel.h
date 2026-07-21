// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef SERVICES_VPN_WGTUNNEL_H
#define SERVICES_VPN_WGTUNNEL_H

// The bring-up boundary for a WireGuard tunnel. VpnManager drives an abstract
// WgTunnel; the real per-OS implementation (wireguard-go userspace, creating a
// TUN interface — needs admin) plugs in behind this interface later. StubWgTunnel
// lets the whole flow (import → connect → connected → disconnect) run and be
// tested on any platform meanwhile. IT DOES NOT PROTECT TRAFFIC — the UI must
// label it a stub until the real tunnel ships.

#include "services/vpn/wireguardconfig.h"

#include <QObject>
#include <QString>
#include <QTimer>

class WgTunnel : public QObject
{
    Q_OBJECT
public:
    explicit WgTunnel(QObject *parent = nullptr) : QObject(parent) {}
    ~WgTunnel() override = default;

    // Bring the tunnel up from a saved .conf (path) / its parsed form. Async:
    // answer with connected() or failed().
    virtual void up(const QString &confPath, const bat::WgConfig &cfg) = 0;
    virtual void down() = 0;
    // The OS interface the tunnel created (e.g. "utun6"), for split-tunnel bind.
    virtual QString interfaceName() const = 0;
    // Whether this backend actually protects traffic (false for the stub).
    virtual bool isReal() const = 0;

signals:
    void connected(const QString &ifaceName);
    void disconnected();
    void failed(const QString &error);
};

class StubWgTunnel : public WgTunnel
{
    Q_OBJECT
public:
    using WgTunnel::WgTunnel;
    void up(const QString &, const bat::WgConfig &) override
    {
        m_iface = QStringLiteral("wg-stub");
        QTimer::singleShot(50, this, [this]() { emit connected(m_iface); });
    }
    void down() override
    {
        m_iface.clear();
        QTimer::singleShot(0, this, [this]() { emit disconnected(); });
    }
    QString interfaceName() const override { return m_iface; }
    bool isReal() const override { return false; }

private:
    QString m_iface;
};

#endif
