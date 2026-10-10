// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// An image with fallbacks: the first source that loads wins, and one that
// fails (a 404 from a CDN, say) hands over to the next instead of leaving a hole.
import QtQuick
import "../theme"

Image {
    id: img
    property var sources: []
    property int at: 0
    onSourcesChanged: at = 0

    source: sources[at] || ""
    fillMode: Image.PreserveAspectCrop
    asynchronous: true
    cache: true
    onStatusChanged: if (status === Image.Error && at < sources.length - 1) at++
    opacity: status === Image.Ready ? 1 : 0
    Behavior on opacity { NumberAnimation { duration: Theme.durSlower } }
}
