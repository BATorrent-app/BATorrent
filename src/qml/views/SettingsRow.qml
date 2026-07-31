// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// One settings row: label/note/badge on the left, a type-driven control on
// the right. `sw` is the owning SettingsView; dialogs it can't own are signals.
// Controls live in SettingsThemeControls / SettingsFieldControls / SettingsVpnCard.
import QtQuick
import QtQuick.Layouts
import "../theme"
import "../widgets"

ColumnLayout {
    id: rowRoot
    property var sw
    signal renameProfileRequested()
    signal colorPickRequested(string role, color cur)
    signal bgImageRequested()
    property var field
    property bool showDivider: true
    property bool isLast: false
    spacing: 0

    readonly property bool rowVisible: (!field.customOnly || Theme.name === "custom")
                                       && (!field.winOnly || Qt.platform.os === "windows")
                                       && field.hidden !== true
    visible: rowVisible
    Layout.preferredHeight: rowVisible ? -1 : 0

    SettingsThemeControls {
        id: themeCtrls
        field: rowRoot.field
        sw: rowRoot.sw
        host: rowRoot
    }
    SettingsFieldControls {
        id: fieldCtrls
        field: rowRoot.field
        sw: rowRoot.sw
    }

    RowLayout {
        id: srow
        visible: field.type !== "warning" && field.type !== "vpn"
        Layout.fillWidth: true
        Layout.topMargin: 13
        Layout.bottomMargin: 13
        spacing: Theme.sp4

        ColumnLayout {
            Layout.fillWidth: true
            Layout.minimumWidth: 120
            spacing: 4
            RowLayout {
                spacing: Theme.sp2
                Layout.fillWidth: true
                Text {
                    text: field.label
                    color: Theme.t1
                    font.pixelSize: 13
                    font.family: Theme.fontSans
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }
                Rectangle {
                    visible: field.badge !== undefined
                    implicitWidth: bdg.implicitWidth + 14
                    implicitHeight: 18
                    radius: 999
                    color: Theme.field
                    border.color: Theme.hair
                    border.width: 1
                    Text { id: bdg; anchors.centerIn: parent; text: field.badge || ""; color: Theme.t3; font.pixelSize: 10; font.family: Theme.fontMono }
                }
            }
            Text {
                visible: field.note !== undefined
                Layout.fillWidth: true
                text: field.note || ""
                color: Theme.t4
                font.pixelSize: 11
                font.family: Theme.fontSans
                wrapMode: Text.WordWrap
                lineHeight: 1.5
            }
        }

        Loader {
            id: ctrl
            Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
            visible: field.type !== "warning"
            sourceComponent: {
                switch (field.type) {
                case "toggle": return themeCtrls.toggle
                case "path": return fieldCtrls.path
                case "timerange": return fieldCtrls.timerange
                case "days": return fieldCtrls.days
                case "anime": return themeCtrls.anime
                case "number": return fieldCtrls.number
                case "text": return fieldCtrls.text
                case "password": return fieldCtrls.text
                case "select": return fieldCtrls.select
                case "segmented": return fieldCtrls.segmented
                case "theme": return themeCtrls.theme
                case "appicon": return themeCtrls.appicon
                case "debrid": return fieldCtrls.debrid
                case "debridtoken": return fieldCtrls.debridtoken
                case "button": return fieldCtrls.button
                case "iface": return fieldCtrls.iface
                case "profiles": return themeCtrls.profiles
                case "color": return themeCtrls.color
                case "bgimage": return themeCtrls.bgimage
                case "slider": return themeCtrls.slider
                }
                return null
            }
        }
    }

    RowLayout {
        visible: field.type === "warning"
        Layout.fillWidth: true
        Layout.topMargin: 12
        Layout.bottomMargin: 12
        spacing: Theme.sp3
        IconImg { src: "qrc:/icons/triangle-alert.svg"; tint: Theme.amber; s: 13; Layout.alignment: Qt.AlignTop }
        Text {
            Layout.fillWidth: true
            visible: field.type === "warning"
            text: field.text || ""
            color: Theme.t3
            font.pixelSize: 11
            font.family: Theme.fontSans
            wrapMode: Text.WordWrap
            lineHeight: 1.45
        }
    }

    Loader {
        visible: field.type === "vpn"
        active: field.type === "vpn"
        Layout.fillWidth: true
        Layout.topMargin: 12
        Layout.bottomMargin: 12
        sourceComponent: field.type === "vpn" ? vpnComp : null
    }
    Component {
        id: vpnComp
        SettingsVpnCard { width: rowRoot.width }
    }

    Rectangle { visible: showDivider; Layout.fillWidth: true; Layout.preferredHeight: 1; color: Theme.hairSoft }
}
