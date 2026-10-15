// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// The title bar of the app's other windows (RSS and the like), drawn the
// same as the main window's strip: frame colour, the window's one title,
// the three window buttons. With QWindowKit it replaces the system frame,
// which would otherwise pick up the Windows accent colour; it hands the
// agent its buttons and drag area the way the navigation bar does.
import QtQuick
import QtQuick.Window
import "../theme"

Rectangle {
    id: bar
    property Window appWindow: null
    property string title: appWindow ? appWindow.title : ""
    // false where the system frame is kept (no QWindowKit build)
    readonly property bool ownsTitleBar: typeof framelessWindow !== "undefined" && framelessWindow === true

    Loader {
        id: agentLoader
        active: bar.ownsTitleBar
        source: bar.ownsTitleBar ? "../AppFramelessAgent.qml" : ""
    }
    // before the window's first show (MainWindowLoaders.showWin), so the system frame never flashes
    function prepareFrame() {
        var a = agentLoader.item
        if (!a) return
        a.window = bar.appWindow
        a.navBar = bar
        a.attach()
    }

    readonly property real stripH: 32
    implicitHeight: ownsTitleBar ? stripH + 1 : 0
    visible: ownsTitleBar
    color: Theme.frame

    readonly property alias titleStrip: bar
    readonly property alias minButton: minBtn
    readonly property alias maxButton: maxBtn
    readonly property alias closeButton: closeBtn
    readonly property var hitItems: []

    Row {
        x: 14
        height: bar.stripH
        spacing: 9
        Image {
            anchors.verticalCenter: parent.verticalCenter
            source: Theme.logoSource
            width: 16; height: 16
            sourceSize: Qt.size(32, 32)
        }
        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: bar.title
            color: Theme.t3
            font.pixelSize: 13; font.family: Theme.fontSans
        }
    }
    Row {
        anchors.right: parent.right
        height: bar.stripH
        WinButton { id: minBtn; glyph: "min"; onClicked: if (bar.appWindow) bar.appWindow.showMinimized() }
        WinButton {
            id: maxBtn
            glyph: bar.appWindow && bar.appWindow.visibility === Window.Maximized ? "restore" : "max"
            onClicked: {
                if (!bar.appWindow) return
                if (bar.appWindow.visibility === Window.Maximized) bar.appWindow.showNormal()
                else bar.appWindow.showMaximized()
            }
        }
        WinButton { id: closeBtn; glyph: "close"; onClicked: if (bar.appWindow) bar.appWindow.close() }
    }
    Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hair }

    component WinButton: Rectangle {
        id: wb
        property string glyph: "min"
        signal clicked()
        width: 46; height: bar.stripH
        color: wbMa.containsMouse ? (glyph === "close" ? Theme.winClose : Theme.hover2) : "transparent"
        readonly property color ink: wbMa.containsMouse && glyph === "close" ? Theme.winCloseInk : Theme.tSoft
        Rectangle { visible: wb.glyph === "min"; anchors.centerIn: parent; width: 10; height: 1; color: wb.ink }
        Rectangle {
            visible: wb.glyph === "max"
            anchors.centerIn: parent
            width: 10; height: 10; radius: Theme.rad(1)
            color: "transparent"; border.width: 1; border.color: wb.ink
        }
        Item {
            visible: wb.glyph === "restore"
            anchors.centerIn: parent
            width: 10; height: 10
            Rectangle { x: 2; y: 0; width: 8; height: 8; radius: Theme.rad(1); color: "transparent"; border.width: 1; border.color: wb.ink }
            Rectangle { x: 0; y: 2; width: 8; height: 8; radius: Theme.rad(1); color: wb.color.a > 0 ? wb.color : bar.color; border.width: 1; border.color: wb.ink }
        }
        Item {
            visible: wb.glyph === "close"
            anchors.centerIn: parent
            width: 13; height: 13
            Rectangle { anchors.centerIn: parent; width: 14; height: 1; rotation: 45; color: wb.ink; antialiasing: true }
            Rectangle { anchors.centerIn: parent; width: 14; height: 1; rotation: -45; color: wb.ink; antialiasing: true }
        }
        MouseArea { id: wbMa; anchors.fill: parent; hoverEnabled: true; onClicked: wb.clicked() }
    }
}
