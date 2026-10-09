// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef TORRENT_ADDKIND_H
#define TORRENT_ADDKIND_H

#include <QString>

// What someone handed us, and where it has to go.
//
// Four places used to answer this on their own and none of them knew
// everything: the drop overlay understood that an http link ending in
// .torrent belongs to the engine rather than the file downloader, the
// command line did not; the clipboard understood thunder:// links and bare
// info-hashes, neither of the others did. Same question, four answers.
enum class AddKind {
    Unknown,
    Magnet,       // hand to addMagnetUri
    TorrentFile,  // a .torrent on disk
    TorrentUrl,   // a .torrent behind http(s): fetch, then add
    WebFile,      // an ordinary download
};

struct AddTarget {
    AddKind kind = AddKind::Unknown;
    // Normalised and ready to use: a thunder:// wrapper is unwrapped, a bare
    // info-hash becomes a magnet, a file:// URL becomes a plain path.
    QString value;
};

AddTarget classifyAdd(const QString &input);

#endif // TORRENT_ADDKIND_H
