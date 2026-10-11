// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef BATORRENT_MEDIASANDBOX_H
#define BATORRENT_MEDIASANDBOX_H

#include <QString>
#include <QStringList>

// The media child's sandbox (internal/ISOLATED_DECODER_PLAN.md, phase 2).
// Entered once the child is connected to the UI, before any media byte
// arrives. Media comes over the socket (RemoteSource), so the grant has no
// file or network in it at all.
namespace MediaSandbox {

struct Grant {
    QStringList readTrees;   // code the decoder may still load: bundle, Qt, system
};

// SBPL for `grant`, or empty when any path can't be expressed safely: a path is
// spliced into the profile text, so it must never be able to close a string.
// Install paths are the user's to choose, so this still matters.
QString profile(const Grant &grant);

bool supported();
bool enter(const Grant &grant, QString *error);

}

#endif
