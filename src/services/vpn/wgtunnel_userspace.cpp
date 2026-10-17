// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "services/vpn/wgtunnel_userspace.h"

#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPointer>
#include <QProcess>
#include <QTimer>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

UserspaceWgTunnel::UserspaceWgTunnel(QObject *parent) : WgTunnel(parent) {}

UserspaceWgTunnel::~UserspaceWgTunnel()
{
    if (!m_proc) return;
    // closing stdin is batwg's cue to stop; a kill is the fallback
    m_proc->disconnect(this);
    m_proc->closeWriteChannel();
    if (!m_proc->waitForFinished(1500)) m_proc->kill();
}

QString UserspaceWgTunnel::helperPath()
{
#ifdef Q_OS_WIN
    const QString name = QStringLiteral("batwg.exe");
#else
    const QString name = QStringLiteral("batwg");
#endif
    const QString dir = QCoreApplication::applicationDirPath();
    for (const QString &p : { dir + QLatin1Char('/') + name,
                              dir + QStringLiteral("/../Resources/") + name }) {   // macOS bundle
        if (QFile::exists(p)) return p;
    }
    return QString();
}

QString UserspaceWgTunnel::describeError(const QString &code, const QString &detail)
{
    if (code == QLatin1String("conf"))
        return QStringLiteral("The config file is not valid: ") + detail;
    if (code == QLatin1String("server"))
        return QStringLiteral("Could not reach the VPN server: ") + detail;
    if (code == QLatin1String("handshake"))
        return QStringLiteral("The VPN server did not answer. The config may be expired or the server down.");
    if (code == QLatin1String("traffic"))
        return QStringLiteral("The VPN server answered, but nothing passes through it.");
    if (!detail.isEmpty()) return detail;
    return QStringLiteral("The VPN stopped.");
}

QString UserspaceWgTunnel::interfaceName() const
{
    // what the page shows: no adapter exists, so the tunnel's own address
    return m_ready ? QStringLiteral("WireGuard · %1").arg(m_ip) : QString();
}

QVariantMap UserspaceWgTunnel::socks() const
{
    if (!m_ready) return {};
    return { {QStringLiteral("port"), m_port},
             {QStringLiteral("user"), m_user},
             {QStringLiteral("pass"), m_pass},
             {QStringLiteral("ip"), m_ip} };
}

void UserspaceWgTunnel::up(const QString &confPath, const bat::WgConfig &)
{
    if (m_proc) return;
    const QString exe = helperPath();
    m_final = false;
    if (exe.isEmpty()) {
        m_final = true;
        QTimer::singleShot(0, this, [this]() { emit failed(QStringLiteral("The VPN component is missing from this install.")); });
        return;
    }
    QFile f(confPath);
    if (!f.open(QIODevice::ReadOnly)) {
        m_final = true;
        QTimer::singleShot(0, this, [this]() { emit failed(QStringLiteral("config missing on disk")); });
        return;
    }
    const QByteArray conf = f.readAll();

    m_ready = false;
    m_stopping = false;
    m_error.clear();
    m_buf.clear();
    m_proc = new QProcess(this);
    m_proc->setProgram(exe);
    m_proc->setProcessChannelMode(QProcess::SeparateChannels);
#ifdef Q_OS_WIN
    m_proc->setCreateProcessArgumentsModifier([](QProcess::CreateProcessArguments *a) {
        a->flags |= CREATE_NO_WINDOW;
    });
#endif
    connect(m_proc, &QProcess::readyReadStandardOutput, this, &UserspaceWgTunnel::readEvents);
    connect(m_proc, &QProcess::readyReadStandardError, this, [this]() {
        for (const QByteArray &l : m_proc->readAllStandardError().split('\n'))
            if (!l.trimmed().isEmpty()) qInfo().noquote() << "[vpn]" << QString::fromUtf8(l.trimmed());
    });
    connect(m_proc, &QProcess::errorOccurred, this, [this](QProcess::ProcessError e) {
        if (e == QProcess::FailedToStart) finish(QStringLiteral("The VPN component could not start."));
    });
    connect(m_proc, &QProcess::finished, this, [this]() {
        readEvents();
        finish(m_error.isEmpty() ? (m_ready ? QStringLiteral("The VPN stopped.")
                                            : QStringLiteral("The VPN component quit before connecting."))
                                 : m_error);
    });
    m_proc->start();
    // The config goes over stdin, never the command line: it holds the
    // private key. stdin stays open; closing it tells batwg to stop.
    m_proc->write(conf);
    m_proc->write("\n.\n");

    // batwg gives up on its own after ~25 s (server lookup, handshake,
    // traffic probe); this is the backstop if it hangs.
    QPointer<QProcess> proc = m_proc;
    QTimer::singleShot(40000, this, [this, proc]() {
        if (proc && proc == m_proc && !m_ready && !m_stopping) {
            m_error = QStringLiteral("The VPN took too long to connect.");
            m_proc->kill();
        }
    });
}

void UserspaceWgTunnel::readEvents()
{
    if (!m_proc) return;
    m_buf += m_proc->readAllStandardOutput();
    int nl;
    while ((nl = m_buf.indexOf('\n')) >= 0) {
        const QByteArray line = m_buf.left(nl).trimmed();
        m_buf.remove(0, nl + 1);
        const QJsonObject ev = QJsonDocument::fromJson(line).object();
        const QString kind = ev.value(QStringLiteral("ev")).toString();
        if (kind == QLatin1String("ready")) {
            m_port = ev.value(QStringLiteral("port")).toInt();
            m_user = ev.value(QStringLiteral("user")).toString();
            m_pass = ev.value(QStringLiteral("pass")).toString();
            m_ip = ev.value(QStringLiteral("ip")).toString();
            m_ready = m_port > 0;
            qInfo().noquote() << "[vpn] tunnel up, SOCKS on 127.0.0.1:" << m_port << "address" << m_ip;
            if (m_ready && !m_stopping) emit connected(interfaceName());
        } else if (kind == QLatin1String("fwd")) {
            m_fwd = ev.value(QStringLiteral("port")).toInt();
            qInfo().noquote() << "[vpn] forwarded port" << m_fwd;
            if (m_ready) emit forwardedPort(m_fwd);
        } else if (kind == QLatin1String("error")) {
            const QString code = ev.value(QStringLiteral("code")).toString();
            m_final = code == QLatin1String("conf");
            m_error = describeError(code, ev.value(QStringLiteral("msg")).toString());
            qInfo().noquote() << "[vpn] batwg:" << ev.value(QStringLiteral("code")).toString()
                              << ev.value(QStringLiteral("msg")).toString();
        } else if (kind == QLatin1String("stale")) {
            // The server stopped answering for minutes: say so and stop, so
            // the app's drop handling (the kill switch) takes over.
            qInfo() << "[vpn] the server stopped answering";
            m_error = QStringLiteral("The VPN server stopped answering.");
            m_proc->closeWriteChannel();
        }
    }
}

// The process is gone (or never started): report it once and forget it.
void UserspaceWgTunnel::finish(const QString &error)
{
    if (!m_proc) return;
    QProcess *p = m_proc;
    m_proc = nullptr;
    p->disconnect(this);
    p->deleteLater();
    const bool wasStopping = m_stopping;
    m_ready = false;
    m_stopping = false;
    m_port = 0;
    m_user.clear();
    m_pass.clear();
    if (m_fwd) { m_fwd = 0; emit forwardedPort(0); }
    if (wasStopping) emit disconnected();
    else emit failed(error);
}

void UserspaceWgTunnel::down()
{
    if (!m_proc) {
        QTimer::singleShot(0, this, [this]() { emit disconnected(); });
        return;
    }
    m_stopping = true;
    m_proc->closeWriteChannel();
    QPointer<QProcess> proc = m_proc;
    QTimer::singleShot(3000, this, [proc]() { if (proc) proc->kill(); });
}
