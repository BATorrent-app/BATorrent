// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "services/security/archivescan.h"
#include "services/metadata/releasegroup.h"
#include <QFileInfo>
#include <QRegularExpression>

namespace {

QString baseName(const QString &path) { return QFileInfo(path).fileName().toLower(); }

// Plain single-file archive extensions (longest first so .tar.gz wins over .gz).
const QStringList &plainExts()
{
    static const QStringList s = {
        ".tar.gz", ".tar.bz2", ".tar.xz", ".tgz", ".tbz2", ".tbz", ".txz",
        ".rar", ".zip", ".7z", ".tar", ".gz", ".bz2", ".xz"
    };
    return s;
}

// .partN.rar: returns N, or -1 if it isn't that shape.
int rarPartNumber(const QString &n)
{
    static const QRegularExpression re(QStringLiteral("\\.part(\\d+)\\.rar$"));
    auto m = re.match(n);
    return m.hasMatch() ? m.captured(1).toInt() : -1;
}

// Generic 3-digit split (.001/.002 …, also .7z.001): returns the number, or -1.
int splitNumber(const QString &n)
{
    static const QRegularExpression re(QStringLiteral("\\.(\\d{3})$"));
    auto m = re.match(n);
    return m.hasMatch() ? m.captured(1).toInt() : -1;
}

// The groups that put one fixed password on every archive, by the name
// ReleaseGroup gives them: it already knows every way a release spells them.
const QList<QPair<QString, QString>> &groupPasswords()
{
    static const QList<QPair<QString, QString>> s = {
        { QStringLiteral("Online-Fix"), QStringLiteral("online-fix.me") },
    };
    return s;
}

}

namespace ArchiveScan {

QStringList taggedPasswords(const QString &archiveName)
{
    const QString group = ReleaseGroup::detect(QFileInfo(archiveName).fileName());
    for (const auto &g : groupPasswords())
        if (g.first == group) return { g.second };
    return {};
}

QStringList knownPasswords(const QString &archiveName)
{
    QStringList out = taggedPasswords(archiveName);
    for (const auto &g : groupPasswords())
        if (!out.contains(g.second)) out << g.second;
    return out;
}

bool isContinuationPart(const QString &name)
{
    const QString n = baseName(name);
    if (rarPartNumber(n) > 1) return true;                  // .part2.rar, .part02.rar …
    static const QRegularExpression rNN(QStringLiteral("\\.r\\d{2}$"));
    if (rNN.match(n).hasMatch()) return true;               // .r00 .r01 … (old rar split)
    static const QRegularExpression zNN(QStringLiteral("\\.z\\d{2}$"));
    if (zNN.match(n).hasMatch()) return true;               // .z01 .z02 … (split zip volumes)
    const int s = splitNumber(n);
    if (s > 1) return true;                                 // .002 .003 … (generic/7z split)
    return false;
}

bool isArchive(const QString &name)
{
    const QString n = baseName(name);
    if (isContinuationPart(n)) return true;
    if (rarPartNumber(n) >= 1) return true;
    if (splitNumber(n) >= 1) return true;
    for (const auto &ext : plainExts())
        if (n.endsWith(ext)) return true;
    return false;
}

QString unpackFolderName(const QString &archiveName)
{
    QString n = QFileInfo(archiveName).fileName();
    static const QRegularExpression volume(
        QStringLiteral("\\.(?:part\\d+\\.rar|r\\d{2}|z\\d{2}|\\d{3})$"), QRegularExpression::CaseInsensitiveOption);
    n.remove(volume);
    const QString lower = n.toLower();
    for (const QString &ext : plainExts())
        if (lower.endsWith(ext)) { n.chop(ext.size()); break; }
    return n.isEmpty() ? QFileInfo(archiveName).completeBaseName() : n;
}

int progressPercent(const QByteArray &output)
{
    static const QRegularExpression re(QStringLiteral("(\\d{1,3})%"));
    int pct = -1;
    auto it = re.globalMatch(QString::fromLatin1(output));
    while (it.hasNext()) pct = it.next().captured(1).toInt();
    return pct > 100 ? -1 : pct;
}

QStringList archivesToExtract(const QStringList &fileNames)
{
    QStringList out;
    QStringList seen;
    for (const QString &f : fileNames) {
        const QString n = baseName(f);
        if (isContinuationPart(n)) continue;

        bool first = false;
        if (rarPartNumber(n) == 1)        first = true;     // .part1.rar / .part01.rar
        else if (splitNumber(n) == 1)     first = true;     // .001 first split volume
        else if (rarPartNumber(n) < 0 && splitNumber(n) < 0) {
            for (const auto &ext : plainExts())
                if (n.endsWith(ext)) { first = true; break; }
        }
        if (first && !seen.contains(n)) { out << f; seen << n; }
    }
    return out;
}

}
