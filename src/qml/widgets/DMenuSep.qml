// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

import QtQuick
import QtQuick.Controls.Basic
import "../theme"

MenuSeparator {
    padding: 0
    topPadding: 5
    bottomPadding: 5
    leftPadding: 4
    rightPadding: 4
    contentItem: Rectangle { implicitHeight: 1; color: Theme.hairSoft }
}
