// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "services/security/defender.h"
#include <QDir>

#if defined(Q_OS_WIN) && !defined(BAT_STORE_BUILD)
#include <QProcess>
#endif

namespace Defender {

bool addExclusion(const QString &path)
{
#if defined(Q_OS_WIN) && !defined(BAT_STORE_BUILD)
    if (path.isEmpty() || !QDir(path).exists()) return false;
    // Escape ' for the PowerShell single-quoted literal, then base64(UTF-16LE)
    // the whole command and run it via -EncodedCommand. This sidesteps the
    // nested-quoting/command-injection trap of interpolating a path into a
    // single-quoted string that is itself inside another single-quoted arg.
    QString escaped = path; escaped.replace(QLatin1Char('\''), QStringLiteral("''"));
    const QString inner = QStringLiteral("Add-MpPreference -ExclusionPath '%1'").arg(escaped);
    QByteArray utf16le;
    for (QChar c : inner) { ushort u = c.unicode(); utf16le.append(char(u & 0xFF)); utf16le.append(char((u >> 8) & 0xFF)); }
    const QString b64 = QString::fromLatin1(utf16le.toBase64());  // [A-Za-z0-9+/=] — quote-safe
    int ret = QProcess::execute(QStringLiteral("powershell.exe"),
        {QStringLiteral("-Command"),
         QStringLiteral("Start-Process powershell -ArgumentList '-EncodedCommand','%1' -Verb RunAs -Wait").arg(b64)});
    return ret == 0;
#else
    Q_UNUSED(path);
    return false;   // Windows-only
#endif
}

}
