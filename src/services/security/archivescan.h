// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef ARCHIVESCAN_H
#define ARCHIVESCAN_H

#include <QString>
#include <QStringList>

// Pure archive-discovery logic for the auto-extract feature. Given the file
// names in a folder it returns just the archives worth handing to an extractor:
// the FIRST volume of every multi-part set, with continuation parts dropped
// (the extractor pulls those in itself). Knows the common multi-part styles:
// .partN.rar, .rNN, split .NNN, split-zip .zNN: and the usual formats.
namespace ArchiveScan {

// Any recognized archive name, first volume OR continuation part.
bool isArchive(const QString &name);

// A continuation volume that must NOT be launched on its own (the first volume
// pulls it in): .part2.rar, .r00, .002, .z01, …
bool isContinuationPart(const QString &name);

// The first-volume archives to extract, from a folder's file list (any path
// form: only the file name matters). De-duplicated, original order preserved.
QStringList archivesToExtract(const QStringList &fileNames);

// The folder a lone archive unpacks into, beside it: its name without the
// archive and volume suffixes ("Game-OFME.part1.rar" → "Game-OFME"). A torrent
// that is just an archive has no folder of its own, and unpacking it straight
// into the save folder mixed the game with every other download.
QString unpackFolderName(const QString &archiveName);

// How far an extractor is, from the last "NN%" in a chunk of its output:
// UnRAR's figure covers the whole archive, as does 7-Zip's with -bsp1.
// -1 when the chunk has none.
int progressPercent(const QByteArray &output);

// Passwords a release group puts on every archive it ships (OnlineFix:
// "online-fix.me"), so a game installs without the user knowing them. The ones
// whose group tag is in this archive's name come first; the rest follow.
QStringList knownPasswords(const QString &archiveName);
// Only the passwords whose group tag is in the name: worth trying before an
// empty password, since a wrong guess on a big archive can take minutes.
QStringList taggedPasswords(const QString &archiveName);

}

#endif
