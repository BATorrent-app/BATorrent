// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "torrent/filetree.h"

#include <QHash>

namespace {

// Indices into one flat vector, not pointers into nested lists: a pointer held
// across an append is a dangling read waiting for the container to grow.
struct Node {
    QString name;
    bool isDir = true;
    int fileIndex = -1;
    QList<int> children;
};

void emitRows(const QList<Node> &nodes, int self, int depth, QList<FileTreeRow> &out)
{
    for (int c : nodes.at(self).children) {
        const Node &n = nodes.at(c);
        out.append({ n.name, depth, n.isDir, n.fileIndex });
        if (n.isDir) emitRows(nodes, c, depth + 1, out);
    }
}

} // namespace

QList<FileTreeRow> buildFileTree(const QStringList &paths)
{
    QList<Node> nodes;
    nodes.append(Node{});                       // root
    QHash<QString, int> dirIndex;               // full dir path -> node

    for (int i = 0; i < paths.size(); ++i) {
        // Trailing and doubled separators would otherwise mint a folder with an
        // empty name, and a leading one a nameless root above everything.
        const QStringList segs = paths.at(i).split(QLatin1Char('/'), Qt::SkipEmptyParts);
        if (segs.isEmpty()) continue;

        int cur = 0;
        QString prefix;
        for (int s = 0; s < segs.size() - 1; ++s) {
            prefix += QLatin1Char('/') + segs.at(s);
            auto it = dirIndex.constFind(prefix);
            if (it != dirIndex.cend()) {
                cur = it.value();
                continue;
            }
            nodes.append(Node{ segs.at(s), true, -1, {} });
            const int added = nodes.size() - 1;
            nodes[cur].children.append(added);
            dirIndex.insert(prefix, added);
            cur = added;
        }
        nodes.append(Node{ segs.last(), false, i, {} });
        nodes[cur].children.append(nodes.size() - 1);
    }

    QList<FileTreeRow> out;
    out.reserve(paths.size());
    emitRows(nodes, 0, 0, out);
    return out;
}
