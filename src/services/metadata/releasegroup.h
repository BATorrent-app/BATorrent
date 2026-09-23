// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#pragma once

#include <QString>
#include <QStringList>

// Who actually released a torrent, the repacker or scene group, under a name
// the user recognises. That's the axis people trust ("online-fix works online",
// "FitGirl compresses hard"); the indexer that happened to list it is not.
//
// Single source of truth on purpose: NameParser and QmlSearchBridge both use
// this table, so they can't drift apart and each miss groups the other knows.
namespace ReleaseGroup {

// Display name for an already-isolated tag, resolving the abbreviations a user
// can't be expected to decode ("OFME" → "Online-Fix"). Returns "" when the tag
// is a format token (WEB-DL, x264, …) rather than a group.
QString canonical(const QString &tag);

// The group a release name carries: a known group anywhere in the name, else the
// scene tail ("…x264-STARCKFILMES", "… by Pioneer"). "" when there's none.
QString detect(const QString &releaseName);

// Groups whose presence is a strong "this is a game" signal. Deliberately
// narrower than the display table: "Masquerade 2021" is a film, and a repacker
// sharing the word must not drag it into the Games bucket.
const QStringList &gameSignals();

}
