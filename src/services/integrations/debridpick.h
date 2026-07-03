// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#pragma once

#include <QJsonArray>
#include <QPair>
#include <QString>

// Pure file-selection logic shared by the debrid providers, split out so it
// can be unit-tested without a network. Both reduce to "largest video, else
// largest file" but each API's JSON shape differs.
namespace DebridPick {

bool looksLikeVideo(const QString &path);

// Real-Debrid: `links` has one entry per *selected* file, in file order.
// Returns an index into that selected/links order, or -1.
int bestRealDebridLinkIndex(const QJsonArray &files);

// TorBox: files carry their own `id`. Returns { id, name } of the best file,
// or { -1, {} }.
QPair<int, QString> bestTorBoxFile(const QJsonArray &files);

}
