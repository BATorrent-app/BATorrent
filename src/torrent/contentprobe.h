// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details
//
// Does a torrent's data still exist on disk? libtorrent only reports
// no_such_file_or_directory once it actually touches the file, so a completed
// torrent nobody is requesting never errors and keeps reading as "seeding"
// while its files are long gone. This is the active probe that answers instead.
//
// Pure by design: the existence check is injected, so the candidate logic is
// testable without a filesystem.

#pragma once

#include <QString>
#include <QStringList>
#include <functional>

namespace bat {

// On-disk locations where a torrent's content root could be, best first.
// Empty when metadata gives us nothing to look for — callers must treat that
// as "cannot tell", never as missing.
QStringList contentRootCandidates(const QString &savePath,
                                  const QString &firstFileRelPath,
                                  const QString &displayName,
                                  bool multiFile);

// Spellings of one path worth stat-ing: as given, with native separators, and
// with the in-progress suffix.
QStringList pathVariants(const QString &path);

using ExistsFn = std::function<bool(const QString &)>;

// True when any candidate resolves. An empty candidate list is true: we would
// rather miss a deletion than flag a healthy torrent as gone.
bool contentPresent(const QStringList &candidates, const ExistsFn &exists);

bool defaultExists(const QString &path);

} // namespace bat
