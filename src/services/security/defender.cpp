// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "services/security/defender.h"
#include <QDir>

#if defined(Q_OS_WIN) && !defined(BAT_STORE_BUILD)
#include <QProcess>
#include <QPointer>
#endif

namespace Defender {

bool addExclusion(const QString &path)
{
#if defined(Q_OS_WIN) && !defined(BAT_STORE_BUILD)
    // Fire-and-forget elevated PowerShell. The old QProcess::execute path blocked
    // the GUI thread on UAC (Windows ghosted the window — "tela cinza").
    addExclusionAsync(path, {});
    return true;   // queued; actual success is unknown until the process exits
#else
    Q_UNUSED(path);
    return false;
#endif
}

void addExclusionAsync(const QString &path, std::function<void(bool)> done)
{
#if defined(Q_OS_WIN) && !defined(BAT_STORE_BUILD)
    if (path.isEmpty() || !QDir(path).exists()) {
        if (done) done(false);
        return;
    }
    QString escaped = path; escaped.replace(QLatin1Char('\''), QStringLiteral("''"));
    const QString inner = QStringLiteral("Add-MpPreference -ExclusionPath '%1'").arg(escaped);
    QByteArray utf16le;
    for (QChar c : inner) {
        ushort u = c.unicode();
        utf16le.append(char(u & 0xFF));
        utf16le.append(char((u >> 8) & 0xFF));
    }
    const QString b64 = QString::fromLatin1(utf16le.toBase64());
    auto *p = new QProcess;
    QObject::connect(p, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                     p, [p, done](int code, QProcess::ExitStatus) {
        if (done) done(code == 0);
        p->deleteLater();
    });
    QObject::connect(p, &QProcess::errorOccurred, p, [p, done](QProcess::ProcessError) {
        if (done) done(false);
        p->deleteLater();
    });
    p->start(QStringLiteral("powershell.exe"),
             {QStringLiteral("-Command"),
              QStringLiteral("Start-Process powershell -ArgumentList '-EncodedCommand','%1' -Verb RunAs -Wait")
                  .arg(b64)});
#else
    Q_UNUSED(path);
    if (done) done(false);
#endif
}

}
