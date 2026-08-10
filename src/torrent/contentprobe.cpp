// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "torrent/contentprobe.h"

#include <QDir>
#include <QFileInfo>

namespace bat {

QStringList contentRootCandidates(const QString &savePath,
                                  const QString &firstFileRelPath,
                                  const QString &displayName,
                                  bool multiFile)
{
    if (savePath.isEmpty())
        return {};

    QStringList out;
    const QString base = savePath.endsWith(QLatin1Char('/'))
                         ? savePath.chopped(1) : savePath;

    // libtorrent hands back native separators, so normalise before looking for
    // the folder boundary — indexOf('/') misses it on Windows otherwise.
    QString rel = QDir::fromNativeSeparators(firstFileRelPath);
    if (multiFile) {
        const int slash = rel.indexOf(QLatin1Char('/'));
        // A multi-file torrent written straight into save_path has no common
        // folder. Dropping to file 0 would probe an arbitrary .rNN part, so
        // give up on this candidate rather than answer from a coin flip.
        rel = slash > 0 ? rel.left(slash) : QString();
    }
    if (!rel.isEmpty())
        out << base + QLatin1Char('/') + rel;

    // The display name diverges from file_path after a rename, or when one
    // platform sanitised characters the other kept.
    if (!displayName.isEmpty()) {
        const QString byName = base + QLatin1Char('/') + displayName;
        if (!out.contains(byName))
            out << byName;
    }

    return out;
}

QStringList pathVariants(const QString &path)
{
    if (path.isEmpty())
        return {};

    QStringList out{path};
    const QString native = QDir::toNativeSeparators(path);
    if (native != path)
        out << native;
    // An in-progress single-file torrent is on disk as <name>.<ext>.!bt;
    // probing only the final name would call every active download missing.
    out << path + QStringLiteral(".!bt");
    return out;
}

bool contentPresent(const QStringList &candidates, const ExistsFn &exists)
{
    if (candidates.isEmpty() || !exists)
        return true;

    for (const QString &candidate : candidates)
        for (const QString &variant : pathVariants(candidate))
            if (exists(variant))
                return true;

    return false;
}

bool defaultExists(const QString &path)
{
    return QFileInfo::exists(path);
}

} // namespace bat
