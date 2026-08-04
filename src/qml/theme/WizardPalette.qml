// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

pragma Singleton
import QtQuick

QtObject {
    readonly property bool isDark: true
    readonly property color bg: "#0e0e10"
    readonly property color panel: "#141416"
    readonly property color elev: "#18181b"
    readonly property color field: "#0b0c0d"
    readonly property color hair: Qt.rgba(1, 1, 1, 0.08)
    readonly property color hairSoft: Qt.rgba(1, 1, 1, 0.05)
    readonly property color hover: Qt.rgba(1, 1, 1, 0.035)
    readonly property color track: Qt.rgba(1, 1, 1, 0.09)
    readonly property color t1: "#f3f3f4"
    readonly property color t2: "#b4b5ba"
    readonly property color t3: "#999a9f"
    readonly property color t4: "#7d7e87"
    readonly property color accent: "#e5332b"
    readonly property color accentDark: "#c01f18"
    readonly property color accentText: "#ec6a64"
    readonly property color focusRing: Qt.rgba(229/255, 51/255, 43/255, 0.9)
    readonly property int focusRingWidth: 2
    readonly property string fontSans: "IBM Plex Sans"
    readonly property real pressScale: 0.97
    readonly property int durFast: 120
}
