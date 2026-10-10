// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "ipc/mediasandbox.h"

#ifdef Q_OS_MACOS
#  include <sandbox.h>
#endif

namespace {

// An SBPL string literal, or empty if `s` can't be one safely.
QString literal(const QString &s)
{
    if (s.isEmpty()) return {};
    for (const QChar c : s)
        if (c.unicode() < 0x20 || c.unicode() == 0x7F) return {};
    QString out = s;
    out.replace(QLatin1Char('\\'), QLatin1String("\\\\"));
    out.replace(QLatin1Char('"'), QLatin1String("\\\""));
    return QLatin1Char('"') + out + QLatin1Char('"');
}

}

namespace MediaSandbox {

QString profile(const Grant &grant)
{
    QString p = QStringLiteral(
        "(version 1)\n"
        "(deny default)\n"
        "(import \"system.sb\")\n"
        "(allow file-read-metadata)\n"
        "(allow sysctl-read)\n"
        "(allow ipc-posix-shm-read* ipc-posix-shm-write-create ipc-posix-shm-write-data"
        " ipc-posix-shm-write-unlink (ipc-posix-name-regex #\"^/batm[0-9a-f]+$\"))\n"
        // VideoToolbox decodes in Apple's own XPC service; the child only maps
        // the IOSurfaces it hands back. The GPU driver (AGX*) stays closed.
        "(allow iokit-open (iokit-user-client-class \"IOSurfaceRootUserClient\"))\n");

    QStringList trees;
    for (const QString &t : grant.readTrees) {
        const QString lit = literal(t);
        if (lit.isEmpty()) return {};
        trees << QStringLiteral("(subpath %1)").arg(lit);
    }
    if (!trees.isEmpty())
        p += QStringLiteral("(allow file-read* %1)\n").arg(trees.join(QLatin1Char(' ')));

    if (!grant.readFile.isEmpty()) {
        const QString lit = literal(grant.readFile);
        if (lit.isEmpty()) return {};
        p += QStringLiteral("(allow file-read* (literal %1))\n").arg(lit);
    }
    if (grant.localPort > 0 && grant.localPort < 65536)
        p += QStringLiteral("(allow network-outbound (remote ip \"localhost:%1\"))\n").arg(grant.localPort);
    return p;
}

#ifdef Q_OS_MACOS

bool supported() { return true; }

bool enter(const Grant &grant, QString *error)
{
    const QString p = profile(grant);
    if (p.isEmpty()) { if (error) *error = QStringLiteral("unsafe path in grant"); return false; }
    char *err = nullptr;
    // flags 0 = compile `p` as an SBPL profile (what sandbox-exec -p does);
    // the header only documents named profiles.
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
    const int rc = sandbox_init(p.toUtf8().constData(), 0, &err);
    if (rc != 0) {
        if (error) *error = QString::fromUtf8(err ? err : "sandbox_init failed");
        sandbox_free_error(err);
    }
QT_WARNING_POP
    return rc == 0;
}

#else

bool supported() { return false; }
bool enter(const Grant &, QString *error)
{
    if (error) *error = QStringLiteral("no sandbox on this platform yet");
    return false;
}

#endif

}
