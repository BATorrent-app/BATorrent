// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "bridges/qmlpairingbridge.h"

#include "services/platform/qrcodegen.h"

#include <QClipboard>
#include <QDesktopServices>
#include <QGuiApplication>
#include <QNetworkInterface>
#include <QSettings>
#include <QUrl>

QString QmlPairingBridge::detectLanIp()
{
    // Score IPv4 addresses by how "real LAN" they look. The old version keyed
    // off interface *names* ("en"/"wlan"), which don't exist on Windows — there
    // it fell through to the first non-primary interface, often a Radmin/Hamachi
    // VPN adapter (26.x / 25.x), so the QR pointed at the wrong network.
    QString best;
    int bestScore = -1;
    for (const auto &iface : QNetworkInterface::allInterfaces()) {
        const auto flags = iface.flags();
        if (!flags.testFlag(QNetworkInterface::IsUp)) continue;
        if (!flags.testFlag(QNetworkInterface::IsRunning)) continue;
        if (flags.testFlag(QNetworkInterface::IsLoopBack)) continue;
        if (flags.testFlag(QNetworkInterface::IsPointToPoint)) continue;
        for (const auto &entry : iface.addressEntries()) {
            if (entry.ip().protocol() != QAbstractSocket::IPv4Protocol) continue;
            const QString ip = entry.ip().toString();
            if (ip.startsWith("169.254.")) continue;
            if (ip.startsWith("25.") || ip.startsWith("26.")) continue;
            int score = 1;
            if      (ip.startsWith("192.168.")) score = 4;
            else if (ip.startsWith("10."))      score = 3;
            else if (ip.startsWith("172."))     score = 3;
            if (flags.testFlag(QNetworkInterface::CanBroadcast)) score += 1;
            if (score > bestScore) { bestScore = score; best = ip; }
        }
    }
    return best;
}

QmlPairingBridge::QmlPairingBridge(QObject *parent) : QObject(parent)
{
    refresh();
}

void QmlPairingBridge::refresh()
{
    const int port = QSettings().value(QStringLiteral("webUiPort"), 8080).toInt();
    const QString ip = detectLanIp();
    m_url = ip.isEmpty() ? QString() : QStringLiteral("http://%1:%2/").arg(ip).arg(port);
    emit changed();
}

void QmlPairingBridge::copyUrl()
{
    if (!m_url.isEmpty()) QGuiApplication::clipboard()->setText(m_url);
}

void QmlPairingBridge::copyText(const QString &t)
{
    if (!t.isEmpty()) QGuiApplication::clipboard()->setText(t);
}

void QmlPairingBridge::openBrowser()
{
    if (!m_url.isEmpty()) QDesktopServices::openUrl(QUrl(m_url));
}

QStringList QmlPairingBridge::qrRows() const { return qrRowsForUrl(m_url); }

QStringList QmlPairingBridge::qrRowsForUrl(const QString &url) const
{
    QStringList rows;
    if (url.isEmpty()) return rows;
    const qrgen::Matrix m = qrgen::encode(url);
    if (m.size == 0) return rows;
    for (int y = 0; y < m.size; ++y) {
        QString s;
        s.reserve(m.size);
        for (int x = 0; x < m.size; ++x)
            s += m.at(x, y) ? QLatin1Char('1') : QLatin1Char('0');
        rows << s;
    }
    return rows;
}
