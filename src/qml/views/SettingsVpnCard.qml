// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// WireGuard settings card: status hero, profile list, import. Context `vpn`.
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import QtQuick.Dialogs
import "../theme"
import "../widgets"

ColumnLayout {
    id: vpnCol
    spacing: 10

    function activeProfileLabel() {
        if (typeof vpn === "undefined") return ""
        for (var i = 0; i < vpn.profiles.length; ++i)
            if (vpn.profiles[i].id === vpn.activeProfileId) {
                var p = vpn.profiles[i]
                return p.endpoint && p.endpoint.length ? p.name + "  ·  " + p.endpoint : p.name
            }
        return ""
    }

    Rectangle {
        id: hero
        Layout.fillWidth: true
        radius: 14
        readonly property int st: (typeof vpn !== "undefined") ? vpn.connState : 0
        readonly property bool on: st === 2
        color: on ? Qt.rgba(Theme.grn.r, Theme.grn.g, Theme.grn.b, 0.08) : Theme.field
        border.color: on ? Qt.rgba(Theme.grn.r, Theme.grn.g, Theme.grn.b, 0.30) : Theme.hair
        border.width: 1
        implicitHeight: heroCol.implicitHeight + 32
        Behavior on color { ColorAnimation { duration: 200 } }
        ColumnLayout {
            id: heroCol
            anchors.left: parent.left; anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            anchors.leftMargin: 16; anchors.rightMargin: 16
            spacing: 12
            RowLayout {
                Layout.fillWidth: true; spacing: 14
                Rectangle {
                    Layout.preferredWidth: 52; Layout.preferredHeight: 52; radius: 26
                    color: hero.on ? Qt.rgba(Theme.grn.r, Theme.grn.g, Theme.grn.b, 0.15)
                                    : Qt.rgba(Theme.t4.r, Theme.t4.g, Theme.t4.b, 0.12)
                    IconImg {
                        anchors.centerIn: parent; src: "qrc:/icons/set-vpn.svg"; s: 26
                        tint: hero.on ? Theme.grn : (hero.st === 1 ? Theme.amber : Theme.t3)
                    }
                    Rectangle {
                        anchors.centerIn: parent; width: 52; height: 52; radius: 26
                        color: "transparent"; border.color: Theme.amber; border.width: 2
                        visible: hero.st === 1
                        SequentialAnimation on opacity {
                            running: hero.st === 1; loops: Animation.Infinite
                            NumberAnimation { from: 0.7; to: 0.0; duration: 300; easing.type: Easing.OutCubic }
                        }
                    }
                }
                ColumnLayout {
                    Layout.fillWidth: true; spacing: 3
                    Text {
                        text: (i18n.language, hero.st === 2 ? i18n.t("vpn_hero_protected")
                              : hero.st === 1 ? i18n.t("vpn_state_connecting")
                              : hero.st === 3 ? i18n.t("vpn_state_failed") : i18n.t("vpn_hero_exposed"))
                        color: hero.on ? Theme.grn : (hero.st === 3 ? "#e0574b" : Theme.t1)
                        font.pixelSize: 17; font.weight: Font.Bold; font.family: Theme.fontSans
                    }
                    Text {
                        Layout.fillWidth: true
                        text: hero.on ? (vpn.activeProfileId, vpnCol.activeProfileLabel())
                                      : (i18n.language, i18n.t("vpn_hero_sub"))
                        color: Theme.t3; font.pixelSize: 12; font.family: Theme.fontSans
                        wrapMode: Text.WordWrap; elide: Text.ElideRight
                    }
                }
            }
            BtnFlat {
                Layout.alignment: Qt.AlignLeft
                Layout.preferredWidth: Math.max(160, implicitWidth)
                primary: !hero.on
                enabled: hero.st === 1 || hero.st === 2
                         || (typeof vpn !== "undefined" && vpn.profiles.length > 0)
                text: (i18n.language, hero.on ? i18n.t("vpn_disconnect")
                      : hero.st === 1 ? i18n.t("vpn_state_connecting") : i18n.t("vpn_connect"))
                onClicked: {
                    if (typeof vpn === "undefined") return
                    if (hero.on || hero.st === 1) { vpn.disconnectVpn(); return }
                    if (vpn.profiles.length === 0) return
                    vpn.connectProfile(vpn.activeProfileId !== "" ? vpn.activeProfileId
                                                                  : vpn.profiles[0].id)
                }
            }
        }
    }

    Rectangle {
        visible: typeof vpn !== "undefined" && !vpn.tunnelReal
        Layout.fillWidth: true; radius: 8
        color: Qt.rgba(Theme.amber.r, Theme.amber.g, Theme.amber.b, 0.10)
        border.color: Qt.rgba(Theme.amber.r, Theme.amber.g, Theme.amber.b, 0.35); border.width: 1
        implicitHeight: wgWarn.implicitHeight + 16
        Row {
            anchors.fill: parent; anchors.margins: 8; spacing: 8
            IconImg { src: "qrc:/icons/triangle-alert.svg"; tint: Theme.amber; s: 13 }
            Text { id: wgWarn; width: parent.width - 24; color: Theme.t3; font.pixelSize: 11
                font.family: Theme.fontSans; wrapMode: Text.WordWrap; lineHeight: 1.4
                text: (i18n.language, i18n.t("vpn_stub_warning")) }
        }
    }

    Repeater {
        model: (typeof vpn !== "undefined") ? vpn.profiles : []
        delegate: Rectangle {
            id: profRow
            required property var modelData
            property bool editing: false
            Layout.fillWidth: true
            implicitHeight: 42; radius: 8; color: Theme.field; border.color: Theme.hair; border.width: 1
            function commitRename() {
                if (typeof vpn !== "undefined") vpn.renameProfile(modelData.id, nameEdit.text)
                profRow.editing = false
            }
            RowLayout {
                anchors.fill: parent; anchors.leftMargin: 12; anchors.rightMargin: 10; spacing: 8
                ColumnLayout {
                    Layout.fillWidth: true; spacing: 1
                    Text { visible: !profRow.editing; text: modelData.name; color: Theme.t1; font.pixelSize: 12; font.family: Theme.fontSans
                        Layout.fillWidth: true; elide: Text.ElideRight }
                    TextField {
                        id: nameEdit
                        visible: profRow.editing
                        Layout.fillWidth: true
                        text: modelData.name
                        color: Theme.t1; font.pixelSize: 12; font.family: Theme.fontSans
                        padding: 2; selectByMouse: true
                        background: Rectangle { color: "transparent"; border.color: Theme.accent; border.width: 1; radius: 4 }
                        onAccepted: profRow.commitRename()
                        onActiveFocusChanged: if (!activeFocus && profRow.editing) profRow.commitRename()
                        Keys.onEscapePressed: { text = modelData.name; profRow.editing = false }
                    }
                    Text { visible: !!modelData.endpoint && !profRow.editing; text: modelData.endpoint || ""
                        color: Theme.t4; font.pixelSize: 10; font.family: Theme.fontSans
                        Layout.fillWidth: true; elide: Text.ElideRight }
                }
                IconImg {
                    src: "qrc:/icons/pencil.svg"; s: 14; tint: renMa.containsMouse ? Theme.t1 : Theme.t4
                    MouseArea { id: renMa; anchors.fill: parent; anchors.margins: -4; hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: { profRow.editing = true; nameEdit.forceActiveFocus(); nameEdit.selectAll() } }
                }
                BtnFlat {
                    sm: true
                    readonly property bool active: typeof vpn !== "undefined" && vpn.activeProfileId === modelData.id
                        && (vpn.connState === 1 || vpn.connState === 2)
                    primary: !active
                    text: (i18n.language, active ? i18n.t("vpn_disconnect") : i18n.t("vpn_connect"))
                    onClicked: active ? vpn.disconnectVpn() : vpn.connectProfile(modelData.id)
                }
                IconImg {
                    src: "qrc:/icons/trash.svg"; s: 15; tint: rmMa.containsMouse ? "#e0574b" : Theme.t4
                    MouseArea { id: rmMa; anchors.fill: parent; anchors.margins: -4; hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor; onClicked: if (typeof vpn !== "undefined") vpn.removeProfile(modelData.id) }
                }
            }
        }
    }

    Text {
        visible: typeof vpn !== "undefined" && vpn.profiles.length === 0
        Layout.fillWidth: true; color: Theme.t4; font.pixelSize: 11; font.family: Theme.fontSans
        wrapMode: Text.WordWrap; lineHeight: 1.4; text: (i18n.language, i18n.t("vpn_empty"))
    }
    RowLayout {
        Layout.fillWidth: true; spacing: 8
        BtnFlat { icon: "qrc:/icons/download.svg"; text: (i18n.language, i18n.t("vpn_import"))
            onClicked: vpnFileDlg.open() }
        Item { Layout.fillWidth: true }
    }
    Text {
        visible: typeof vpn !== "undefined" && vpn.connState === 3 && vpn.lastError.length > 0
        Layout.fillWidth: true; color: "#e0574b"; font.pixelSize: 11; font.family: Theme.fontSans
        wrapMode: Text.WordWrap; text: (typeof vpn !== "undefined") ? vpn.lastError : ""
    }

    FileDialog {
        id: vpnFileDlg
        nameFilters: ["WireGuard config (*.conf)", "All files (*)"]
        onAccepted: if (typeof vpn !== "undefined") vpn.importFromFile(selectedFile.toString(), "")
    }
}
