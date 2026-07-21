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

QString WgTunnelLinux::systemWgQuick()
{
    return firstExisting({QStringLiteral("/usr/bin/wg-quick"),
                          QStringLiteral("/usr/local/bin/wg-quick"),
                          QStringLiteral("/bin/wg-quick")});
}

// The AppImage payload sits on a user-only FUSE mount that root cannot read
// (FUSE denies other users even root, unlike normal DAC), so the bundled
// wg/wg-quick must be staged into a real directory before pkexec runs them.
QString WgTunnelLinux::stagedBundledTools()
{
    const QString src = QCoreApplication::applicationDirPath() + QStringLiteral("/wireguard");
    if (!QFile::exists(src + QStringLiteral("/wg-quick"))) return QString();

    QString base = QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation);
    if (base.isEmpty()) base = QDir::tempPath();
    const QString dst = base + QStringLiteral("/batorrent-wgtools");
    QDir().mkpath(dst);
    for (const char *tool : {"wg", "wg-quick"}) {
        const QString from = src + QLatin1Char('/') + QLatin1String(tool);
        const QString to = dst + QLatin1Char('/') + QLatin1String(tool);
        QFile::remove(to);
        if (!QFile::copy(from, to)) return QString();
        QFile::setPermissions(to, QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner
                                  | QFile::ReadGroup | QFile::ExeGroup
                                  | QFile::ReadOther | QFile::ExeOther);
    }
    return dst;
}

bool WgTunnelLinux::haveWgQuick()
{
    return QFile::exists(QCoreApplication::applicationDirPath() + QStringLiteral("/wireguard/wg-quick"))
           || !systemWgQuick().isEmpty();
}

// pkexec strips the environment, so the bundled wg-quick would not find its
// sibling `wg` — run it through `env` with a PATH that leads with the staged dir.
QStringList WgTunnelLinux::wgQuickArgv(const QString &verb, const QString &target)
{
    const QString staged = stagedBundledTools();
    if (!staged.isEmpty())
        return {QStringLiteral("env"),
                QStringLiteral("PATH=%1:/usr/sbin:/usr/bin:/sbin:/bin").arg(staged),
                staged + QStringLiteral("/wg-quick"), verb, target};
    const QString sys = systemWgQuick();
    if (sys.isEmpty()) return {};
    return {sys, verb, target};
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

    const QStringList argv = wgQuickArgv(QStringLiteral("up"), shortConf);
    if (argv.isEmpty()) { emit failed(QStringLiteral("wg-quick not found")); return; }
    m_activeConf = shortConf;
    m_iface = QLatin1String(kIface);

    runElevated(argv, [this](bool ok, QString out) {
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
    const QString target = m_activeConf.isEmpty() ? m_iface : m_activeConf;
    const QStringList argv = m_iface.isEmpty() ? QStringList()
                                               : wgQuickArgv(QStringLiteral("down"), target);
    if (argv.isEmpty()) { m_iface.clear(); emit disconnected(); return; }
    runElevated(argv, [this](bool, QString) {
        if (!m_activeConf.isEmpty()) { QFile::remove(m_activeConf); m_activeConf.clear(); }
        m_iface.clear();
        emit disconnected();
    });
}

#endif // Q_OS_LINUX
