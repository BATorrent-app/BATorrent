// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "services/vpn/legacytunnels.h"

#include <QSettings>
#include <QTimer>

#ifdef Q_OS_WIN
#include <windows.h>
#include <shellapi.h>
#endif

namespace LegacyTunnels {

bool isOurs(const QString &serviceName, const QString &imagePath)
{
    if (!serviceName.startsWith(QLatin1String("WireGuardTunnel$"), Qt::CaseInsensitive))
        return false;
    QString path = imagePath;
    path.replace(QLatin1Char('/'), QLatin1Char('\\'));
    // BATorrent's install folder or its profile folder, by name: both have a
    // "\BATorrent\" component (…\Programs\BATorrent\wireguard\wireguard.exe,
    // …\Roaming\BATorrent\BATorrent\vpn\<id>.conf)
    return path.contains(QLatin1String("\\BATorrent\\"), Qt::CaseInsensitive)
        && path.contains(QLatin1String("wireguard.exe"), Qt::CaseInsensitive);
}

QStringList find()
{
    QStringList out;
#ifdef Q_OS_WIN
    QSettings services(QStringLiteral("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services"),
                       QSettings::NativeFormat);
    for (const QString &name : services.childGroups()) {
        if (!name.startsWith(QLatin1String("WireGuardTunnel$"), Qt::CaseInsensitive)) continue;
        const QString image = services.value(name + QStringLiteral("/ImagePath")).toString();
        if (isOurs(name, image)) out << name;
    }
#endif
    return out;
}

void remove(const QStringList &names, QObject *receiver, std::function<void(bool)> done)
{
#ifdef Q_OS_WIN
    if (names.isEmpty()) { QTimer::singleShot(0, receiver, [done]() { done(true); }); return; }
    // one elevated cmd for all of them: one prompt
    QStringList steps;
    for (const QString &n : names)
        steps << QStringLiteral("sc stop \"%1\"").arg(n) << QStringLiteral("sc delete \"%1\"").arg(n);
    const std::wstring params = (QStringLiteral("/c ") + steps.join(QStringLiteral(" & "))).toStdWString();

    SHELLEXECUTEINFOW sei;
    ZeroMemory(&sei, sizeof(sei));
    sei.cbSize = sizeof(sei);
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;
    sei.lpVerb = L"runas";
    sei.lpFile = L"cmd.exe";
    sei.lpParameters = params.c_str();
    sei.nShow = SW_HIDE;
    if (!ShellExecuteExW(&sei) || !sei.hProcess) {   // the prompt was declined
        QTimer::singleShot(0, receiver, [done]() { done(false); });
        return;
    }
    HANDLE proc = sei.hProcess;
    auto *poll = new QTimer(receiver);
    poll->setInterval(250);
    auto waited = std::make_shared<int>(0);
    QObject::connect(poll, &QTimer::timeout, receiver, [poll, proc, done, waited, names]() {
        const bool finished = WaitForSingleObject(proc, 0) == WAIT_OBJECT_0;
        if (!finished && ++*waited < 120) return;   // 30 s at most
        poll->stop();
        poll->deleteLater();
        CloseHandle(proc);
        // a service marked for deletion while running leaves the list once
        // it stops; what is still listed now is what failed
        const QStringList left = find();
        bool clean = true;
        for (const QString &n : names) clean = clean && !left.contains(n, Qt::CaseInsensitive);
        done(clean);
    });
    poll->start();
#else
    Q_UNUSED(names);
    QTimer::singleShot(0, receiver, [done]() { done(true); });
#endif
}

}
