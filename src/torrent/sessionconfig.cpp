// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "torrent/sessionconfig.h"

#include <QRegularExpression>

namespace SessionConfig {

QString listenInterfaces(const QString &listenAddr, int port, bool forceIpv4)
{
    if (listenAddr != QLatin1String("0.0.0.0") || forceIpv4)
        return QStringLiteral("%1:%2").arg(listenAddr).arg(port);
    return QStringLiteral("0.0.0.0:%1,[::]:%1").arg(port);
}

static QString shellQuote(const QString &s)
{
    QString q = s;
    q.replace(QLatin1Char('\''), QStringLiteral("'\\''"));
    return QLatin1Char('\'') + q + QLatin1Char('\'');
}

QString expandOnCompleteCommand(const QString &tmpl,
                                const QString &name,
                                const QString &savePath,
                                const QString &hash,
                                qint64 totalSize)
{
    QString cmd = tmpl;
    cmd.replace(QLatin1String("%N"), shellQuote(name));
    cmd.replace(QLatin1String("%D"), shellQuote(savePath));
    cmd.replace(QLatin1String("%H"), shellQuote(hash));
    cmd.replace(QLatin1String("%Z"), shellQuote(QString::number(totalSize)));
    cmd.replace(QLatin1String("%F"), shellQuote(savePath + QLatin1Char('/') + name));
    return cmd;
}

QStringList parseExtractPasswords(const QString &raw)
{
    QStringList pw;
    const auto parts = raw.split(QRegularExpression(QStringLiteral("[;\\n]")),
                                 Qt::SkipEmptyParts);
    for (const QString &p : parts) {
        const QString t = p.trimmed();
        if (!t.isEmpty())
            pw << t;
    }
    return pw;
}

qint64 autoCompleteSecondsFromIndex(int index)
{
    static const qint64 days[] = {0, 1, 3, 7, 14, 30};
    if (index < 0 || index >= 6)
        return 0;
    return days[index] * 86400;
}

int portStatusCode(bool listenOk, bool portmapOk)
{
    if (!listenOk)
        return 3;
    if (portmapOk)
        return 1;
    return 2;
}

std::map<int, std::string> planContentLayout(int mode,
                                             const std::string &torrentName,
                                             const std::vector<std::string> &filePaths)
{
    std::map<int, std::string> out;
    const int numFiles = static_cast<int>(filePaths.size());
    if (numFiles == 0 || mode == 0)
        return out;

    if (mode == 2) {
        const std::string &root = filePaths[0];
        const auto slash = root.find('/');
        if (slash == std::string::npos || numFiles <= 1)
            return out;
        const std::string prefix = root.substr(0, slash + 1);
        for (const auto &p : filePaths) {
            if (p.compare(0, prefix.size(), prefix) != 0)
                return {};
        }
        for (int i = 0; i < numFiles; ++i)
            out[i] = filePaths[static_cast<size_t>(i)].substr(prefix.size());
        return out;
    }

    if (mode == 1 && numFiles == 1) {
        const std::string &filePath = filePaths[0];
        if (filePath.find('/') == std::string::npos)
            out[0] = torrentName + "/" + filePath;
    }
    return out;
}

} // namespace SessionConfig
