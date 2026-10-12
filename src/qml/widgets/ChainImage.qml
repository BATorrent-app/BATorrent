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
    // Fade in only. A reused view (the game page opening another game) swaps the
    // source: fading the old art out slowly showed it under the new title.
    Behavior on opacity {
        enabled: img.status === Image.Ready
        NumberAnimation { duration: Theme.durSlower }
    }
}
