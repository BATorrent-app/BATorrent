// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details
//
// Groups a torrent's files into the parts people choose between when space
// is short: a repack's game data, language packs and optional extras, a
// season pack's seasons, a film's separate subtitles and audio. Only known
// patterns are grouped; anything else returns no groups and the Add window
// shows the plain file tree.
#ifndef BATORRENT_FILEGROUPS_H
#define BATORRENT_FILEGROUPS_H

#include <QList>
#include <QString>

namespace FileGroups {

struct File { int index = -1; QString path; qint64 size = 0; };

// one pickable piece of a group: a language, an episode, an extra
struct Chip {
    QString label;             // "Português (BR)", "E03", "Soundtrack"
    QString lang;              // ISO-ish code for languages ("pt-BR"), else empty
    QList<int> files;          // file indices; empty for "English · in game data"
    qint64 size = 0;
    bool inBase = false;       // carried by the game data, nothing to download
};

struct Group {
    QString kind;              // "game", "lang", "extras", "setup", "season", "subs"
    QString label;             // untranslated key for the UI ("grp_game", ...) or a season name
    int season = -1;
    QList<int> files;          // every file of the group
    qint64 size = 0;
    QList<Chip> chips;         // empty for a group taken as a whole
};

// The groups for these files, or an empty list when no pattern is known.
// torrentName helps tell a FitGirl or DODI repack from other archives.
QList<Group> group(const QList<File> &files, const QString &torrentName);

// A language name as it appears in repack file names ("brazilian",
// "spanish_latam", "russian") to {code, label in its own language}; empty
// code when it is not a language.
struct Lang { QString code; QString label; };
Lang languageFromToken(const QString &token);

}

#endif
