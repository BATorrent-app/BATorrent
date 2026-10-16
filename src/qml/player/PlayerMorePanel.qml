// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// The player's "more" popover: picture, window and the odd actions, laid out
// in sections instead of one long menu. Same card and click-away as the
// options panel beside it.
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import "../theme"
import "../widgets"

Item {
    id: more
    property var pw
    property bool open: false
    visible: open
    function popup() { open = !open }
    function closePanel() { open = false }

    MouseArea { anchors.fill: parent; enabled: more.open; onClicked: more.closePanel() }

    component SectionTitle: Text {
        color: Theme.t4; font.pixelSize: 10; font.weight: Font.Bold
        font.letterSpacing: 0.8; font.capitalization: Font.AllUppercase
        font.family: Theme.fontSans
    }
    // a row that does something once: icon, label, the key that does the same
    component ActionRow: Rectangle {
        id: ar
        property string icon
        property string label
        property string keyHint: ""
        signal activated()
        Layout.fillWidth: true
        implicitHeight: 34
        radius: 7
        color: arMa.containsMouse ? "#14ffffff" : "transparent"
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 8; anchors.rightMargin: 8
            spacing: 10
            IconImg { src: ar.icon; s: 16; tint: arMa.containsMouse ? Theme.t1 : Theme.t3 }
            Text { Layout.fillWidth: true; text: ar.label; color: Theme.t1; font.pixelSize: 13; font.family: Theme.fontSans; elide: Text.ElideRight }
            Text { visible: ar.keyHint.length > 0; text: ar.keyHint; color: Theme.t4; font.pixelSize: 11; font.family: Theme.fontSans }
        }
        MouseArea { id: arMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: ar.activated() }
    }
    // a row that stays on or off
    component SwitchRow: Rectangle {
        id: sr
        property string icon
        property string label
        property string keyHint: ""
        property bool on: false
        signal toggled()
        Layout.fillWidth: true
        implicitHeight: 34
        radius: 7
        color: srMa.containsMouse ? "#14ffffff" : "transparent"
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 8; anchors.rightMargin: 8
            spacing: 10
            IconImg { src: sr.icon; s: 16; tint: sr.on ? Theme.t1 : Theme.t3 }
            Text { Layout.fillWidth: true; text: sr.label; color: Theme.t1; font.pixelSize: 13; font.family: Theme.fontSans; elide: Text.ElideRight }
            Text { visible: sr.keyHint.length > 0; text: sr.keyHint; color: Theme.t4; font.pixelSize: 11; font.family: Theme.fontSans }
            TToggle { on: sr.on; enabled: false }
        }
        MouseArea { id: srMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: sr.toggled() }
    }

    Rectangle {
        id: card
        anchors.right: parent.right; anchors.bottom: parent.bottom
        anchors.rightMargin: 18; anchors.bottomMargin: 104
        width: Math.min(360, parent.width - 36)
        height: Math.min(body.implicitHeight + 28, parent.height - 140)
        radius: 12
        color: "#f50a0a0c"
        border.color: Theme.hair; border.width: 1
        clip: true
        transform: Translate {
            y: more.open ? 0 : 6
            Behavior on y { NumberAnimation { duration: Theme.durFast; easing.type: Theme.easeOut } }
        }
        MouseArea { anchors.fill: parent }   // clicks inside the card stay inside

        ColumnLayout {
            id: body
            anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
            anchors.margins: 14
            spacing: 6

            // ---- picture ----
            RowLayout {
                Layout.fillWidth: true
                SectionTitle { Layout.fillWidth: true; text: (i18n.language, i18n.t("player_picture")) }
                // the turn applied so far, and the button that adds a quarter
                Rectangle {
                    implicitWidth: rotRow.implicitWidth + 16; implicitHeight: 26
                    radius: 13
                    color: rotMa.containsMouse ? "#14ffffff" : "transparent"
                    border.color: Theme.hair; border.width: 1
                    RowLayout {
                        id: rotRow
                        anchors.centerIn: parent
                        spacing: 6
                        IconImg { src: "qrc:/icons/rotate-clockwise.svg"; s: 13; tint: Theme.t2 }
                        Text { text: more.pw ? more.pw.videoRotation + "°" : "0°"; color: Theme.t2; font.pixelSize: 11; font.family: Theme.fontSans; font.features: Theme.tnum }
                    }
                    MouseArea { id: rotMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: if (more.pw) more.pw.rotate() }
                    ToolTip.visible: rotMa.containsMouse
                    ToolTip.text: (i18n.language, i18n.t("player_rotate")) + "  (R)"
                    ToolTip.delay: 400
                }
            }
            Flow {
                Layout.fillWidth: true
                Layout.bottomMargin: 6
                spacing: 6
                Repeater {
                    model: more.pw ? more.pw.aspectModes : []
                    delegate: Rectangle {
                        required property string modelData
                        readonly property bool cur: more.pw && more.pw.aspectMode === modelData
                        implicitWidth: asT.implicitWidth + 20; implicitHeight: 28
                        radius: 14
                        color: asMa.containsMouse && !cur ? "#14ffffff" : (cur ? Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.14) : "transparent")
                        border.color: cur ? Theme.accent : Theme.hair; border.width: 1
                        Text {
                            id: asT; anchors.centerIn: parent
                            text: more.pw ? more.pw.aspectLabel(modelData) : modelData
                            color: parent.cur ? Theme.t1 : Theme.t2
                            font.pixelSize: 12; font.weight: Font.Medium; font.family: Theme.fontSans
                        }
                        MouseArea { id: asMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: more.pw.aspectMode = parent.modelData }
                    }
                }
            }

            Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: Theme.hairSoft }

            // ---- window ----
            SectionTitle { Layout.topMargin: 6; text: (i18n.language, i18n.t("player_window")) }
            SwitchRow {
                icon: "qrc:/icons/pinned.svg"
                label: (i18n.language, i18n.t("player_on_top"))
                keyHint: "T"
                on: !!more.pw && more.pw.stayOnTop
                onToggled: more.pw.toggleOnTop()
            }

            Rectangle { Layout.fillWidth: true; Layout.topMargin: 4; Layout.preferredHeight: 1; color: Theme.hairSoft }

            // ---- actions ----
            ActionRow {
                Layout.topMargin: 4
                icon: "qrc:/icons/camera.svg"
                label: (i18n.language, i18n.t("player_screenshot"))
                keyHint: "Ctrl+S"
                onActivated: { more.closePanel(); more.pw.screenshot() }
            }
            ActionRow {
                icon: "qrc:/icons/keyboard.svg"
                label: (i18n.language, i18n.t("help_title"))
                keyHint: "?"
                onActivated: { more.closePanel(); more.pw.helpOpen = true }
            }
            ActionRow {
                icon: "qrc:/icons/external.svg"
                label: (i18n.language, i18n.t("player_external"))
                onActivated: { more.closePanel(); more.pw.openExternalSaved() }
            }
        }
    }
}
