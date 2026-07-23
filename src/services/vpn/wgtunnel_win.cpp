// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "services/vpn/wgtunnel_win.h"

#ifdef Q_OS_WIN

#include <QTimer>
#include <QFileInfo>
#include <QDir>
#include <QFile>
#include <QCoreApplication>

// winsock2 must precede windows.h, and its socket-address types must be in scope
// before iphlpapi.h — otherwise netioapi.h's GetIfTable2 / MIB_IF_ROW2 (which
// reference SOCKADDR_INET) compile out with "undeclared identifier".
#include <winsock2.h>
#include <ws2ipdef.h>
#include <windows.h>
#include <shellapi.h>
#include <iphlpapi.h>

#include <memory>
#include <string>
#include <utility>

QString WinWgTunnel::wireguardExe()
{
    const QString bundled = QCoreApplication::applicationDirPath()
                          + QStringLiteral("/wireguard/wireguard.exe");
    if (QFile::exists(bundled)) return bundled;
    const QString sys = QStringLiteral("C:/Program Files/WireGuard/wireguard.exe");
    if (QFile::exists(sys)) return sys;
    return QString();
}

void WinWgTunnel::runElevated(const QStringList &args, std::function<void(bool)> done)
{
    const QString exe = wireguardExe();
    if (exe.isEmpty()) { done(false); return; }
    runElevatedRaw(exe, args.join(QLatin1Char(' ')), std::move(done));
}

void WinWgTunnel::runElevatedRaw(const QString &program, const QString &params,
                                 std::function<void(bool)> done)
{
    if (program.isEmpty()) { done(false); return; }

    const std::wstring wexe = QDir::toNativeSeparators(program).toStdWString();
    const std::wstring wparams = params.toStdWString();

    SHELLEXECUTEINFOW sei;
    ZeroMemory(&sei, sizeof(sei));
    sei.cbSize = sizeof(sei);
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;
    sei.lpVerb = L"runas";                 // UAC elevation prompt
    sei.lpFile = wexe.c_str();
    sei.lpParameters = wparams.c_str();
    sei.nShow = SW_HIDE;

    if (!ShellExecuteExW(&sei) || !sei.hProcess) { done(false); return; }
    HANDLE proc = sei.hProcess;

    // Poll the elevated process for completion (it runs async after the prompt).
    auto *timer = new QTimer(this);
    timer->setInterval(200);
    connect(timer, &QTimer::timeout, this, [timer, proc, done]() {
        DWORD code = STILL_ACTIVE;
        const BOOL got = GetExitCodeProcess(proc, &code);
        if (!got) { timer->stop(); timer->deleteLater(); CloseHandle(proc); done(false); return; }
        if (code != STILL_ACTIVE) {
            timer->stop(); timer->deleteLater(); CloseHandle(proc);
            done(code == 0);
        }
    });
    timer->start();
}

namespace {
// Inbound byte count for the tunnel adapter, matched by its friendly name (the
// alias WireGuard sets to the tunnel/config name). -1 if the adapter isn't found
// yet. A completed handshake is the only thing that makes this climb off zero.
qint64 rxBytesForAlias(const QString &alias)
{
    PMIB_IF_TABLE2 table = nullptr;
    if (GetIfTable2(&table) != NO_ERROR || !table) return -1;
    const std::wstring walias = alias.toStdWString();
    qint64 rx = -1;
    for (ULONG i = 0; i < table->NumEntries; ++i) {
        const MIB_IF_ROW2 &row = table->Table[i];
        if (walias == row.Alias) { rx = static_cast<qint64>(row.InOctets); break; }
    }
    FreeMibTable(table);
    return rx;
}
}

void WinWgTunnel::verifyHandshakeThenConnect()
{
    // WireGuard's own kill-switch (for a 0.0.0.0/0 config) blocks all untunneled
    // traffic the instant the service installs — so if the handshake never
    // completes, the machine is offline. Wait for real inbound bytes before
    // declaring success; if none arrive, tear the tunnel back down.
    auto *timer = new QTimer(this);
    timer->setInterval(500);
    auto elapsed = std::make_shared<int>(0);
    connect(timer, &QTimer::timeout, this, [this, timer, elapsed]() {
        *elapsed += 500;
        const qint64 rx = rxBytesForAlias(m_iface);
        // A fresh tunnel adapter starts at 0 inbound bytes; a dead peer never
        // replies (handshake initiations are outbound), so InOctets stays ~0.
        // >1 KiB inbound means the peer answered and traffic is flowing.
        if (rx > 1024) {
            timer->stop(); timer->deleteLater();
            emit connected(m_iface);
            return;
        }
        if (*elapsed >= 12000) {   // no traffic in 12s → dead tunnel
            timer->stop(); timer->deleteLater();
            // Remove the black-holing route so the user's connection comes back.
            runElevated({QStringLiteral("/uninstalltunnelservice"), m_iface}, [this](bool) {
                const QString gone = m_iface;
                m_iface.clear();
                Q_UNUSED(gone);
                emit failed(QStringLiteral("VPN connected but no traffic came through — "
                                          "the tunnel was removed to restore your connection. "
                                          "Check the server/endpoint in your WireGuard config."));
            });
        }
    });
    timer->start();
}

void WinWgTunnel::up(const QString &confPath, const bat::WgConfig &)
{
    if (wireguardExe().isEmpty()) {
        emit failed(QStringLiteral("WireGuard not found"));
        return;
    }
    m_confPath = confPath;
    m_iface = QFileInfo(confPath).completeBaseName();   // WireGuard names the tunnel after the file
    const QString nativeWg = QDir::toNativeSeparators(wireguardExe());
    const QString nativeConf = QDir::toNativeSeparators(confPath);
    // Uninstall any stale service of the same name FIRST (the '&' runs install
    // regardless of whether one existed), then install — both in a single UAC
    // elevation. Without this a leftover/orphaned tunnel from a crash or a failed
    // teardown blocks the connect with "The object already exists" and can wedge
    // Windows' network + a provider's own VPN app (the tester hit exactly that).
    const QString command =
        QStringLiteral("\"%1\" /uninstalltunnelservice %2 & \"%1\" /installtunnelservice \"%3\"")
            .arg(nativeWg, m_iface, nativeConf);
    runElevatedRaw(QStringLiteral("cmd.exe"),
                   QStringLiteral("/s /c \"%1\"").arg(command),
                   [this](bool ok) {
        // Service installed != tunnel working. Only report connected once the
        // adapter actually passes traffic (verifyHandshakeThenConnect), so we
        // never claim "protected" while a failed full-tunnel blackouts the user.
        if (ok) verifyHandshakeThenConnect();
        else    emit failed(QStringLiteral("could not start the WireGuard tunnel"));
    });
}

bool WinWgTunnel::adopt(const QString &confPath, const QString &iface)
{
    Q_UNUSED(iface);   // the service is named after the conf file, not the adapter
    if (wireguardExe().isEmpty() || !QFile::exists(confPath)) return false;
    m_confPath = confPath;
    m_iface = QFileInfo(confPath).completeBaseName();
    return true;
}

void WinWgTunnel::down()
{
    if (m_iface.isEmpty() || wireguardExe().isEmpty()) { m_iface.clear(); emit disconnected(); return; }
    runElevated({QStringLiteral("/uninstalltunnelservice"), m_iface}, [this](bool ok) {
        // Only consider the tunnel gone if the service actually uninstalled. A
        // lingering service keeps its full-tunnel default route installed, which
        // would black-hole the next connect — report failure so the UI doesn't
        // claim "disconnected" while traffic is still being rerouted.
        if (!ok) { emit failed(QStringLiteral("could not stop the WireGuard tunnel")); return; }
        m_iface.clear();
        emit disconnected();
    });
}

#endif // Q_OS_WIN
