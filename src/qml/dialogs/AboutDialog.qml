// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// About: the mark, the version, where to find the project, and in plain text
// what the app does with your network and your data. No per-library version
// table: attribution lives in THIRD_PARTY.md, versions in the startup log.
import QtQuick
import QtQuick.Layouts
import "../theme"
import "../widgets"

Item {
    id: dlg
    anchors.fill: parent
    z: 200
    visible: opened || anim > 0

    property bool opened: false
    property real anim: 0
    Behavior on anim { NumberAnimation { duration: 150; easing.type: Theme.easeOut } }
    onOpenedChanged: {
        anim = opened ? 1 : 0
        if (opened) DialogStack.push(dlg); else DialogStack.pop(dlg)
    }
    Component.onDestruction: DialogStack.pop(dlg)
    function open() { opened = true }
    function close() { opened = false }

    readonly property string appVer: typeof themeBridge !== "undefined" && themeBridge.appVersion ? themeBridge.appVersion : ""
    readonly property int track: typeof settings !== "undefined" && Number(settings.get("updateTrack")) === 1 ? 1 : 0

    Shortcut {
        sequences: [StandardKey.Cancel]
        enabled: dlg.opened && DialogStack.topItem === dlg
        onActivated: dlg.close()
    }

    Rectangle {
        anchors.fill: parent
        opacity: dlg.anim
        color: Theme.isDark ? Qt.rgba(0, 0, 0, 0.7) : Qt.rgba(20/255, 20/255, 28/255, 0.4)
        MouseArea { anchors.fill: parent; onClicked: dlg.close(); onWheel: function (w) { w.accepted = true } }
    }

    Rectangle {
        anchors.centerIn: parent
        width: 500
        height: Math.min(col.implicitHeight, dlg.height - 48)
        opacity: dlg.anim
        scale: Theme.reduceMotion ? 1 : (0.97 + 0.03 * dlg.anim)
        radius: Theme.rad(12)
        color: Theme.elev
        border.color: Theme.hair; border.width: 1
        clip: true
        MouseArea { anchors.fill: parent }

        Rectangle {
            anchors.right: parent.right; anchors.rightMargin: 12; y: 12; z: 2
            width: 30; height: 30; radius: Theme.rad(6)
            color: xMa.containsMouse ? Theme.hover : "transparent"
            IconImg { anchors.centerIn: parent; src: "qrc:/icons/close.svg"; s: 14; tint: xMa.containsMouse ? Theme.t1 : Theme.t3 }
            MouseArea { id: xMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: dlg.close() }
        }

        ColumnLayout {
            id: col
            width: parent.width
            spacing: 0

            // ---- the mark ----
            ColumnLayout {
                Layout.fillWidth: true
                Layout.preferredWidth: col.width
                Layout.topMargin: 30
                spacing: 6
                Image {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.preferredWidth: 64; Layout.preferredHeight: 64
                    source: Theme.logoSource
                    sourceSize: Qt.size(128, 128)
                    fillMode: Image.PreserveAspectFit
                }
                Row {
                    Layout.alignment: Qt.AlignHCenter
                    Text { text: "BAT"; color: Theme.down; font.family: "New Rocker"; font.pixelSize: 32 }
                    Text { text: "orrent"; color: Theme.t1; font.family: "New Rocker"; font.pixelSize: 32 }
                }
                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: (i18n.language, [dlg.appVer, i18n.t(dlg.track === 1 ? "set_track_beta" : "set_track_stable"), i18n.t("about_mit")].filter(function (s) { return s !== "" }).join(" · "))
                    color: Theme.t3; font.pixelSize: 13; font.family: Theme.fontSans; font.features: Theme.tnum
                }
                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: (i18n.language, i18n.t("about_description"))
                    color: Theme.t1; font.pixelSize: 14; font.family: Theme.fontSans
                }
                Row {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.topMargin: 8
                    spacing: 8
                    BtnFlat { text: "GitHub ★"; onClicked: Qt.openUrlExternally("https://github.com/BATorrent-app/BATorrent") }
                    BtnFlat { text: (i18n.language, i18n.t("about_link_releases")); onClicked: Qt.openUrlExternally("https://github.com/BATorrent-app/BATorrent/releases") }
                    BtnFlat { text: (i18n.language, i18n.t("about_link_privacy")); onClicked: Qt.openUrlExternally("https://github.com/BATorrent-app/BATorrent/blob/main/PRIVACY.md") }
                }
            }

            // ---- in plain words ----
            ColumnLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 22; Layout.rightMargin: 22
                Layout.topMargin: 22; Layout.bottomMargin: 18
                spacing: 0
                Repeater {
                    model: [["share_notice_title", "share_notice_body"], ["about_data_title", "about_no_telemetry_body"]]
                    delegate: ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 5
                        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; Layout.bottomMargin: 10; color: Theme.hairSoft }
                        Text { text: (i18n.language, i18n.t(modelData[0])); color: Theme.t1; font.pixelSize: 14; font.weight: Font.DemiBold; font.family: Theme.fontSans }
                        Text {
                            Layout.fillWidth: true
                            Layout.bottomMargin: 14
                            text: (i18n.language, i18n.t(modelData[1]))
                            color: Theme.t2; font.pixelSize: 13; font.family: Theme.fontSans
                            wrapMode: Text.WordWrap; lineHeight: 1.4
                        }
                    }
                }
                Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; Layout.bottomMargin: 10; color: Theme.hairSoft }
                Text { Layout.bottomMargin: 6; text: (i18n.language, i18n.t("about_translations_title")); color: Theme.t1; font.pixelSize: 14; font.weight: Font.DemiBold; font.family: Theme.fontSans }
                Repeater {
                    model: [["Українська", "dkindratyuk-web"], ["Türkçe", "azizaktas"]]
                    delegate: RowLayout {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 24
                        Text { Layout.fillWidth: true; text: modelData[0]; color: Theme.t2; font.pixelSize: 13; font.family: Theme.fontSans }
                        Text {
                            text: "@" + modelData[1]
                            color: crMa.containsMouse ? Theme.t1 : Theme.t3
                            font.pixelSize: 13; font.family: Theme.fontSans
                            MouseArea { id: crMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: Qt.openUrlExternally("https://github.com/" + modelData[1]) }
                        }
                    }
                }
            }

            // ---- footer ----
            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: 60
                Rectangle { width: parent.width; height: 1; color: Theme.hairSoft }
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 22; anchors.rightMargin: 14
                    Text { Layout.fillWidth: true; text: (i18n.language, i18n.t("about_made_by")); color: Theme.t3; font.pixelSize: 13; font.family: Theme.fontSans }
                    BtnFlat { primary: true; text: (i18n.language, i18n.t("about_kofi")); onClicked: Qt.openUrlExternally("https://ko-fi.com/batorrent") }
                }
            }
        }
    }
}
