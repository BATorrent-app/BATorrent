// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef TORRENT_FILETREE_H
#define TORRENT_FILETREE_H

#include <QList>
#include <QString>
#include <QStringList>

// One row of the add-torrent file list: folders and files in one flat run,
// which is what a ListView can draw and what `depth` is for.
struct FileTreeRow {
    QString name;        // the last path segment, not the whole path
    int depth = 0;
    bool isDir = false;
    int fileIndex = -1;  // index into the torrent's own file order; -1 on folders
};

// Groups a torrent's file paths under their folders.
//
// fileIndex is the point of the whole thing: priorities are handed to
// libtorrent positionally, so a row's place in this list says nothing about
// which file it is. Grouping without carrying the index applies one file's
// choice to another.
//
// Order is first appearance, not sorted: the torrent's own order is the
// author's, and sorting would reshuffle episode lists that are only correct
// unpadded. Each folder still appears once, however the paths interleave.
QList<FileTreeRow> buildFileTree(const QStringList &paths);

#endif // TORRENT_FILETREE_H
