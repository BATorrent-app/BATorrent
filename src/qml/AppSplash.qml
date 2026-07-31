// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

import QtQuick

Loader {
    required property var host
    active: host.showSplash
    anchors.fill: parent
    z: 10000
    sourceComponent: Splash {
        onFinished: { host.showSplash = false; host.maybeShowWelcome() }
    }
}
