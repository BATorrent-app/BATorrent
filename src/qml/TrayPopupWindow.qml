// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Rich tray popup shown on right-click of the system tray icon. Mirrors the
// legacy TrayPopup: header (logo + name + counts), DOWN/UP speed strip, and a
// list of menu actions. VPN / auto-shutdown rows from the legacy version are
// omitted — the QML session bridge doesn't expose those yet.
import QtQuick
import QtQuick.Effects
import QtQuick.Layouts
import "theme"
import "widgets"

Window {
    id: pop
    width: 280
    height: col.implicitHeight + 2
    flags: Qt.Popup | Qt.FramelessWindowHint | Qt.NoDropShadowWindowHint
    color: "transparent"
    visible: false

    // actions delegated to Main (keeps this file free of global ids)
    signal showApp()
    signal openTorrent()
    signal openMagnet()
    signal pauseAll()
    signal resumeAll()
    signal openSettings()
    signal quitApp()

    readonly property var sess: typeof session !== "undefined" ? session : null

    // Anchor near the tray icon using its reported geometry (passed in). macOS
    // menu bar is at the top → drop below the icon; Windows/Linux tray is at the
    // bottom → rise above it. Falls back to a screen corner if geometry is empty.
    function popUpAt(iconRect) {
        var scr = pop.screen || Qt.application.screens[0]
        var g = scr ? Qt.rect(scr.virtualX, scr.virtualY, scr.width, scr.height) : Qt.rect(0,0,1920,1080)
        var margin = 6
        // SystemTrayIcon.geometry is unreliable on Windows (often empty or a bogus
        // rect that lands the popup off-screen — issue #20). Anchor to the tray
        // corner of the screen there; only trust the icon rect on macOS/Linux.
        var useIcon = iconRect && iconRect.width > 0 && Qt.platform.os !== "windows"
        if (useIcon) {
            // center under/over the icon
            pop.x = Math.round(iconRect.x + iconRect.width / 2 - pop.width / 2)
            if (Qt.platform.os === "osx")
                pop.y = Math.round(iconRect.y + iconRect.height + margin)
            else
                pop.y = Math.round(iconRect.y - pop.height - margin)
        } else {
            // anchor at the click itself — centered above the cursor (the
            // Windows taskbar sits at the bottom); beats any corner guess
            var c = typeof themeBridge !== "undefined" ? themeBridge.cursorPos() : null
            if (c) {
                // clamp against the monitor under the cursor, not pop.screen
                var scrs = Qt.application.screens
                for (var i = 0; i < scrs.length; ++i) {
                    var sc = scrs[i]
                    if (c.x >= sc.virtualX && c.x < sc.virtualX + sc.width
                            && c.y >= sc.virtualY && c.y < sc.virtualY + sc.height) {
                        g = Qt.rect(sc.virtualX, sc.virtualY, sc.width, sc.height)
                        break
                    }
                }
                pop.x = Math.round(c.x - pop.width / 2)
                pop.y = Math.round(c.y - pop.height - margin)
            } else {
                pop.x = g.x + g.width - pop.width - 8
                pop.y = (Qt.platform.os === "osx") ? g.y + 30 : g.y + g.height - pop.height - 48
            }
        }
        // clamp to the screen
        if (pop.x + pop.width > g.x + g.width - 4) pop.x = g.x + g.width - pop.width - 4
        if (pop.x < g.x + 4) pop.x = g.x + 4
        if (pop.y + pop.height > g.y + g.height - 4) pop.y = g.y + g.height - pop.height - 4
        if (pop.y < g.y + 4) pop.y = g.y + 4
        pop.show()
        pop.raise()
        pop.requestActivate()
    }

    onActiveChanged: if (!active) pop.hide()   // click-away closes

    Rectangle {
        anchors.fill: parent
        radius: 10
        color: Theme.panel
        border.color: Theme.hair
        border.width: 1

        ColumnLayout {
            id: col
            anchors.fill: parent
            anchors.margins: 1
            spacing: 0

            // ── header ──
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 56
                color: Theme.bg
                topLeftRadius: 10; topRightRadius: 10
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 12; anchors.rightMargin: 12
                    spacing: 10
                    Image {
                        source: "qrc:/images/logo.svg"
                        sourceSize: Qt.size(56, 56)
                        Layout.preferredWidth: 28; Layout.preferredHeight: 28
                        fillMode: Image.PreserveAspectFit
                        layer.enabled: Theme.isLight
                        layer.effect: MultiEffect { colorization: 1.0; colorizationColor: Theme.t1 }
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 1
                        Text { text: "BATorrent"; color: Theme.t1; font.pixelSize: 13; font.weight: Font.Bold; font.family: Theme.fontSans }
                        Text {
                            text: (pop.sess ? pop.sess.torrentCount : 0) + " torrents · "
                                + (pop.sess ? pop.sess.activeCount : 0) + " " + (i18n.language, i18n.t("word_active"))
                            color: Theme.t3; font.pixelSize: 10; font.family: Theme.fontMono
                        }
                    }
                    Rectangle {
                        implicitWidth: 8; implicitHeight: 8; radius: 4
                        Layout.alignment: Qt.AlignTop
                        color: (pop.sess && pop.sess.activeCount > 0) ? Theme.amber : Theme.t4
                    }
                }
            }

            // ── speed strip ──
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 58
                color: Theme.field
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 12; anchors.rightMargin: 12
                    spacing: 16
                    Repeater {
                        model: [
                            { eb: "↓ DOWN", val: pop.sess ? pop.sess.totalDownSpeed : "0 B/s", c: Theme.accent },
                            { eb: "↑ UP",   val: pop.sess ? pop.sess.totalUpSpeed   : "0 B/s", c: Theme.amber }
                        ]
                        delegate: ColumnLayout {
                            required property var modelData
                            Layout.fillWidth: true
                            spacing: 2
                            Text { text: modelData.eb; color: modelData.c; font.pixelSize: 8; font.weight: Font.Bold; font.letterSpacing: 1.2; font.family: Theme.fontSans }
                            Text { text: modelData.val; color: Theme.t1; font.pixelSize: 14; font.weight: Font.Bold; font.family: Theme.fontMono }
                        }
                    }
                }
            }

            // ── menu rows ──
            component Row_: Rectangle {
                id: r
                property string label
                property string shortcut: ""
                property bool danger: false
                signal clicked()
                Layout.fillWidth: true
                Layout.preferredHeight: 32
                color: rma.containsMouse ? Theme.hover : "transparent"
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 14; anchors.rightMargin: 12
                    Text { text: r.label; color: r.danger ? Theme.accentText : Theme.t1; font.pixelSize: 11; font.family: Theme.fontSans; Layout.fillWidth: true }
                    Text { text: r.shortcut; visible: r.shortcut.length > 0; color: Theme.t4; font.pixelSize: 10; font.family: Theme.fontMono }
                }
                MouseArea { id: rma; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: { r.clicked(); pop.hide() } }
            }
            component Div_: Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: Theme.hairSoft }

            Item { Layout.preferredHeight: 4; Layout.fillWidth: true }
            Row_ { label: qsTr((i18n.language, i18n.t("tray_show")));          shortcut: "Ctrl+1"; onClicked: pop.showApp() }
            Row_ { label: qsTr((i18n.language, i18n.t("tray_open_torrent")));   shortcut: "Ctrl+O"; onClicked: pop.openTorrent() }
            Row_ { label: qsTr((i18n.language, i18n.t("tray_open_magnet"))); shortcut: "Ctrl+V"; onClicked: pop.openMagnet() }
            Div_ {}
            Row_ { label: qsTr((i18n.language, i18n.t("tray_pause_all")));     onClicked: pop.pauseAll() }
            Row_ { label: qsTr((i18n.language, i18n.t("tray_resume_all")));  onClicked: pop.resumeAll() }
            Div_ {}
            Row_ { label: qsTr((i18n.language, i18n.t("tray_preferences")));    shortcut: "Ctrl+,"; onClicked: pop.openSettings() }
            Row_ { label: qsTr((i18n.language, i18n.t("tray_quit"))); danger: true; shortcut: "Ctrl+Q"; onClicked: pop.quitApp() }
            Item { Layout.preferredHeight: 4; Layout.fillWidth: true }
        }
    }
}
