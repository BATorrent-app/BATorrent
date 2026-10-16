// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef SERVICES_VPN_WGTUNNEL_USERSPACE_H
#define SERVICES_VPN_WGTUNNEL_USERSPACE_H

// The tunnel as a child process: batwg runs WireGuard with its own in-memory
// network stack and offers it as a SOCKS5 server on 127.0.0.1. No adapter, no
// route, no DNS change, no administrator prompt: the machine's connection is
// never touched, and only what the torrent session sends to the proxy goes
// through the VPN. batwg exits when its stdin closes, so it cannot outlive the
// app, crash included.

#include "services/vpn/wgtunnel.h"

#include <QByteArray>

class QProcess;

class UserspaceWgTunnel : public WgTunnel
{
    Q_OBJECT
public:
    explicit UserspaceWgTunnel(QObject *parent = nullptr);
    ~UserspaceWgTunnel() override;

    void up(const QString &confPath, const bat::WgConfig &cfg) override;
    void down() override;
    QString interfaceName() const override;
    bool isReal() const override { return !helperPath().isEmpty(); }
    bool viaSocks() const override { return true; }
    QVariantMap socks() const override;

    // batwg(.exe) beside the app, or empty when it was not shipped.
    static QString helperPath();
    // What a batwg error code means, for the person reading it.
    static QString describeError(const QString &code, const QString &detail);

private:
    void readEvents();
    void finish(const QString &error);

    QProcess *m_proc = nullptr;
    QByteArray m_buf;
    bool m_ready = false;
    bool m_stopping = false;
    QString m_error;      // the last error batwg reported, before it exited
    int m_port = 0;
    QString m_user;
    QString m_pass;
    QString m_ip;
};

#endif
