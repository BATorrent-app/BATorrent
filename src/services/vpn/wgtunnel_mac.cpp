// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "services/vpn/wgtunnel_mac.h"

#ifdef Q_OS_MACOS

#include <QProcess>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QCoreApplication>

QString MacWgTunnel::toolsBinDir()
{
    // Prefer the wireguard-go/wg/wg-quick we ship inside the app bundle (so the
    // user installs nothing); fall back to a Homebrew/system wireguard-tools.
    const QString bundled = QDir(QCoreApplication::applicationDirPath()
                                 + QStringLiteral("/../Resources/wireguard")).absolutePath();
    if (QFile::exists(bundled + QStringLiteral("/wg-quick"))) return bundled;
    for (const QString &d : {QStringLiteral("/opt/homebrew/bin"),
                             QStringLiteral("/usr/local/bin"),
                             QStringLiteral("/usr/bin")}) {
        if (QFile::exists(d + QStringLiteral("/wg-quick"))) return d;
    }
    return QString();
}

QString MacWgTunnel::wgQuickPath()
{
    const QString d = toolsBinDir();
    return d.isEmpty() ? QString() : d + QStringLiteral("/wg-quick");
}

void MacWgTunnel::runElevated(const QString &shellCmd, std::function<void(bool, QString)> done)
{
    // Wrap the shell command in an AppleScript that asks for admin rights (the
    // one macOS-native way to get a privilege prompt without a helper daemon).
    QString esc = shellCmd;
    esc.replace(QLatin1Char('\\'), QStringLiteral("\\\\"));
    esc.replace(QLatin1Char('"'), QStringLiteral("\\\""));
    const QString script = QStringLiteral("do shell script \"%1\" with administrator privileges").arg(esc);

    auto *proc = new QProcess(this);
    connect(proc, &QProcess::finished, this, [proc, done](int code, QProcess::ExitStatus) {
        const QString out = QString::fromUtf8(proc->readAllStandardOutput())
                          + QString::fromUtf8(proc->readAllStandardError());
        proc->deleteLater();
        done(code == 0, out.trimmed());
    });
    connect(proc, &QProcess::errorOccurred, this, [proc, done](QProcess::ProcessError) {
        proc->deleteLater();
        done(false, QStringLiteral("could not launch osascript"));
    });
    proc->start(QStringLiteral("osascript"), {QStringLiteral("-e"), script});
}

void MacWgTunnel::up(const QString &confPath, const bat::WgConfig &)
{
    const QString wgQuick = wgQuickPath();
    if (wgQuick.isEmpty()) {
        emit failed(QStringLiteral("WireGuard tools not found — install with: brew install wireguard-tools"));
        return;
    }
    m_confPath = confPath;
    // wg-quick shells out to wg + wireguard-go, so give it their bin dir on PATH
    // (the elevated osascript shell doesn't inherit Homebrew's PATH). Quoted —
    // the bundle path can contain spaces.
    const QString cmd = QStringLiteral("export PATH=\"%1:$PATH\"; '%2' up '%3'")
                            .arg(toolsBinDir(), wgQuick, confPath);

    runElevated(cmd, [this, confPath](bool ok, const QString &out) {
        if (!ok) { emit failed(out.isEmpty() ? QStringLiteral("wg-quick up failed") : out); return; }
        // wg-quick records the created utun in /var/run/wireguard/<name>.name
        const QString nameFile = QStringLiteral("/var/run/wireguard/%1.name")
                                     .arg(QFileInfo(confPath).completeBaseName());
        QFile f(nameFile);
        m_iface = f.open(QIODevice::ReadOnly) ? QString::fromUtf8(f.readAll()).trimmed() : QString();
        emit connected(m_iface);
    });
}

bool MacWgTunnel::adopt(const QString &confPath, const QString &iface)
{
    if (iface.isEmpty() || wgQuickPath().isEmpty()) return false;
    // wg-quick keeps /var/run/wireguard/<name>.name while wireguard-go runs —
    // if it still maps this config's name to this utun, the tunnel is ours.
    QFile f(QStringLiteral("/var/run/wireguard/%1.name")
                .arg(QFileInfo(confPath).completeBaseName()));
    if (!f.open(QIODevice::ReadOnly)) return false;
    if (QString::fromUtf8(f.readAll()).trimmed() != iface) return false;
    m_confPath = confPath;
    m_iface = iface;
    return true;
}

void MacWgTunnel::down()
{
    if (m_confPath.isEmpty() || wgQuickPath().isEmpty()) { m_iface.clear(); emit disconnected(); return; }
    const QString cmd = QStringLiteral("export PATH=\"%1:$PATH\"; '%2' down '%3'")
                            .arg(toolsBinDir(), wgQuickPath(), m_confPath);
    runElevated(cmd, [this](bool, const QString &) {   // treat any outcome as down
        m_iface.clear();
        emit disconnected();
    });
}

#endif // Q_OS_MACOS
