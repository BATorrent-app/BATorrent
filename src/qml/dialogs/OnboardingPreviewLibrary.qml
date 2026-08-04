// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

import QtQuick
import "../views"

Item {
    required property var host
    required property var controller
    required property var demoModel

    LibraryView {
        anchors.fill: parent
        win: host
        controller: parent.controller
        modelOverride: parent.demoModel
        enabled: false
    }
}
