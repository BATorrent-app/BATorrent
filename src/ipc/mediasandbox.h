// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef BATORRENT_MEDIASANDBOX_H
#define BATORRENT_MEDIASANDBOX_H

#include <QString>
#include <QStringList>

// The media child's sandbox (internal/ISOLATED_DECODER_PLAN.md, phase 2).
// Entered once, right before the first source opens, and never widened: a
// source that needs more than the grant gets a fresh child instead.
namespace MediaSandbox {

struct Grant {
    QStringList readTrees;   // code the decoder may still load: bundle, Qt, system
    QString readFile;        // the one media file, for file:// sources
    int localPort = 0;       // the stream server, for http://127.0.0.1 sources
};

// SBPL for `grant`, or empty when any path can't be expressed safely: a path is
// spliced into the profile text, so it must never be able to close a string.
QString profile(const Grant &grant);

bool supported();
bool enter(const Grant &grant, QString *error);

}

#endif
