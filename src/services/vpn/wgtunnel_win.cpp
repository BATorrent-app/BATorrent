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

#include <windows.h>
#include <shellapi.h>

#include <string>

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

    const std::wstring wexe = QDir::toNativeSeparators(exe).toStdWString();
    const std::wstring wparams = args.join(QLatin1Char(' ')).toStdWString();

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

void WinWgTunnel::up(const QString &confPath, const bat::WgConfig &)
{
    if (wireguardExe().isEmpty()) {
        emit failed(QStringLiteral("WireGuard not found"));
        return;
    }
    m_confPath = confPath;
    m_iface = QFileInfo(confPath).completeBaseName();   // WireGuard names the tunnel after the file
    const QString nativeConf = QDir::toNativeSeparators(confPath);
    runElevated({QStringLiteral("/installtunnelservice"),
                 QLatin1Char('"') + nativeConf + QLatin1Char('"')},
                [this](bool ok) {
        if (ok) emit connected(m_iface);
        else    emit failed(QStringLiteral("could not start the WireGuard tunnel"));
    });
}

void WinWgTunnel::down()
{
    if (m_iface.isEmpty() || wireguardExe().isEmpty()) { m_iface.clear(); emit disconnected(); return; }
    runElevated({QStringLiteral("/uninstalltunnelservice"), m_iface}, [this](bool) {
        m_iface.clear();
        emit disconnected();
    });
}

#endif // Q_OS_WIN
