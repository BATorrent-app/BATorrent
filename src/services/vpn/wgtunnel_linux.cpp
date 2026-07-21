// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "services/vpn/wgtunnel_linux.h"

#ifdef Q_OS_LINUX

#include <QProcess>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QStandardPaths>
#include <QCoreApplication>

namespace {
// Linux caps an interface name at IFNAMSIZ-1 (15). wg-quick names the interface
// after the .conf basename, and our stored id is a UUID — too long — so we bring
// the tunnel up under a fixed short name.
constexpr auto kIface = "batorrent-wg";

QString firstExisting(const QStringList &paths)
{
    for (const QString &p : paths)
        if (QFile::exists(p)) return p;
    return QString();
}
}

QString WgTunnelLinux::wgQuickPath()
{
    const QString bundled = QCoreApplication::applicationDirPath()
                          + QStringLiteral("/wireguard/wg-quick");
    return firstExisting({bundled,
                          QStringLiteral("/usr/bin/wg-quick"),
                          QStringLiteral("/usr/local/bin/wg-quick"),
                          QStringLiteral("/bin/wg-quick")});
}

void WgTunnelLinux::runElevated(const QStringList &argv, std::function<void(bool, QString)> done)
{
    // pkexec pops the desktop PolicyKit password dialog and runs argv as root.
    auto *proc = new QProcess(this);
    proc->setProcessChannelMode(QProcess::MergedChannels);
    connect(proc, &QProcess::finished, this,
            [proc, done](int code, QProcess::ExitStatus st) {
        const QString out = QString::fromUtf8(proc->readAll());
        proc->deleteLater();
        done(st == QProcess::NormalExit && code == 0, out);
    });
    connect(proc, &QProcess::errorOccurred, this, [proc, done](QProcess::ProcessError) {
        proc->deleteLater();
        done(false, QStringLiteral("pkexec failed to start"));
    });
    proc->start(QStringLiteral("pkexec"), argv);
}

void WgTunnelLinux::up(const QString &confPath, const bat::WgConfig &)
{
    const QString wgQuick = wgQuickPath();
    if (wgQuick.isEmpty()) { emit failed(QStringLiteral("wg-quick not found")); return; }

    // wg-quick derives the iface from the basename, so stage a short-named copy in
    // a user-private dir (0600 — it holds the private key).
    QString dir = QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation);
    if (dir.isEmpty()) dir = QDir::tempPath();
    const QString shortConf = dir + QLatin1Char('/') + QLatin1String(kIface) + QStringLiteral(".conf");

    QFile::remove(shortConf);
    if (!QFile::copy(confPath, shortConf)) {
        emit failed(QStringLiteral("could not stage the tunnel config"));
        return;
    }
    QFile::setPermissions(shortConf, QFile::ReadOwner | QFile::WriteOwner);
    m_activeConf = shortConf;
    m_iface = QLatin1String(kIface);

    runElevated({wgQuick, QStringLiteral("up"), shortConf}, [this](bool ok, QString out) {
        if (ok) emit connected(m_iface);
        else {
            QFile::remove(m_activeConf);
            m_activeConf.clear();
            emit failed(out.trimmed().isEmpty()
                        ? QStringLiteral("could not start the WireGuard tunnel") : out.trimmed());
        }
    });
}

void WgTunnelLinux::down()
{
    const QString wgQuick = wgQuickPath();
    if (m_iface.isEmpty() || wgQuick.isEmpty()) { m_iface.clear(); emit disconnected(); return; }

    const QString target = m_activeConf.isEmpty() ? m_iface : m_activeConf;
    runElevated({wgQuick, QStringLiteral("down"), target}, [this](bool, QString) {
        if (!m_activeConf.isEmpty()) { QFile::remove(m_activeConf); m_activeConf.clear(); }
        m_iface.clear();
        emit disconnected();
    });
}

#endif // Q_OS_LINUX
