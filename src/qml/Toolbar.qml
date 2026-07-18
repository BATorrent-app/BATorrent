// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Top action toolbar: open/magnet/pause/resume/stop/remove/search/rss/settings
// buttons + the global down/up speed readout. Carved out of Main.qml; reads window
// state via `win`, raises intent through signals (the parent owns the dialogs and
// nav), and exposes the Open button via `tbOpen` for the onboarding tour.
import QtQuick
import QtQuick.Layouts
import "theme"
import "widgets"

Rectangle {
    id: toolbar
    property var win
    property alias tbOpen: tbOpen
    signal openFile()
    signal addMagnet()
    signal removeSelected()
    signal openRss()
    signal navigate(int index)

    component TBtn: Rectangle {
        id: tb
        property string label
        property string icon
        property bool disabled: false
        property bool active: false       // toggled-on state (e.g. alt-speed turtle)
        signal clicked()
        Layout.preferredWidth: 52
        Layout.minimumWidth: 52          // never let the RowLayout squeeze/clip the button
        Layout.preferredHeight: 54
        color: !disabled && tbMa.containsMouse ? Theme.hover : "transparent"
        radius: 8
        opacity: disabled ? 0.35 : 1.0

        activeFocusOnTab: !disabled
        Keys.onReturnPressed: if (!disabled) tb.clicked()
        Keys.onSpacePressed: if (!disabled) tb.clicked()
        scale: tbMa.pressed && !tb.disabled ? Theme.pressScale : 1
        Behavior on scale { NumberAnimation { duration: Theme.durFast; easing.type: Easing.OutCubic } }
        Rectangle {
            visible: tb.activeFocus
            anchors.fill: parent
            anchors.margins: -2
            radius: 10
            color: "transparent"
            border.color: Theme.focusRing
            border.width: Theme.focusRingWidth
        }

        Column {
            anchors.centerIn: parent
            spacing: 3
            IconImg {
                anchors.horizontalCenter: parent.horizontalCenter
                src: tb.icon
                tint: tb.active ? Theme.accent : (!tb.disabled && tbMa.containsMouse ? Theme.t1 : Theme.t3)
                s: 18
            }
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: tb.label
                color: tb.active ? Theme.accent : (!tb.disabled && tbMa.containsMouse ? Theme.t1 : Theme.t3)
                font.pixelSize: 11
                font.family: Theme.fontSans
                font.weight: Font.Medium
            }
        }
        MouseArea {
            id: tbMa
            anchors.fill: parent
            hoverEnabled: !tb.disabled
            cursorShape: tb.disabled ? Qt.ArrowCursor : Qt.PointingHandCursor
            onClicked: if (!tb.disabled) tb.clicked()
        }
    }

    component TGrpDiv: Rectangle {
        Layout.preferredWidth: 1
        Layout.preferredHeight: 26
        Layout.leftMargin: 8
        Layout.rightMargin: 8
        Layout.alignment: Qt.AlignVCenter
        color: Theme.hair
    }

    Layout.fillWidth: true
    Layout.preferredHeight: 66
    color: Theme.panel
    Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hair }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Theme.sp4
        anchors.rightMargin: Theme.sp4
        spacing: Theme.sp2

        // brand moved to the nav rail; toolbar starts at the actions
        // G1: Abrir, Magnet
        TBtn { id: tbOpen; label: (i18n.language, i18n.t("tb_open"));   icon: "qrc:/icons/open.svg";  onClicked: toolbar.openFile() }
        TBtn { label: (i18n.language, i18n.t("tb_magnet"));  icon: "qrc:/icons/magnet.svg"; onClicked: toolbar.addMagnet() }
        TGrpDiv {}
        // G2: Pausar, Retomar, Parar
        TBtn { label: (i18n.language, i18n.t("tb_pause"));  icon: "qrc:/icons/pause.svg"; disabled: !win.hasSel; onClicked: session.pauseSelected() }
        TBtn { label: (i18n.language, i18n.t("tb_resume")); icon: "qrc:/icons/play.svg";  disabled: !win.hasSel; onClicked: session.resumeSelected() }
        TBtn { label: (i18n.language, i18n.t("tb_stop"));   icon: "qrc:/icons/stop.svg";  disabled: !win.hasSel; onClicked: session.pauseSelected() }
        TGrpDiv {}
        // G3: Remover
        TBtn { label: (i18n.language, i18n.t("tb_remove")); icon: "qrc:/icons/trash.svg"; disabled: !win.hasSel; onClicked: toolbar.removeSelected() }
        TGrpDiv {}
        // G4: Buscar, RSS
        TBtn { label: (i18n.language, i18n.t("tb_search"));  icon: "qrc:/icons/search.svg"; onClicked: toolbar.navigate(1) }
        TBtn { label: (i18n.language, i18n.t("tb_refresh")); icon: "qrc:/icons/refresh.svg"; onClicked: if (typeof session !== "undefined") session.refreshAll() }
        TBtn { label: (i18n.language, i18n.t("tb_rss"));     icon: "qrc:/icons/rss.svg";    onClicked: toolbar.openRss() }
        TGrpDiv {}
        // G5: Config.
        TBtn { label: (i18n.language, i18n.t("tb_settings")); icon: "qrc:/icons/settings.svg"; onClicked: toolbar.navigate(3) }


        // .tb-spacer
        Item { Layout.fillWidth: true }

        // global speed readout — boxless, neutral values; the tinted arrow
        // icons carry the down/up semantic without shouting
        Row {
            Layout.alignment: Qt.AlignVCenter
            Layout.rightMargin: Theme.sp2
            spacing: Theme.sp5

            Column {
                spacing: 3
                Text {
                    text: (i18n.language, i18n.t("graph_download"))
                    color: Theme.t4
                    font.pixelSize: 9
                    font.weight: Font.Bold
                    font.letterSpacing: 1
                    font.family: Theme.fontSans
                }
                Row {
                    spacing: 5
                    IconImg {
                        anchors.verticalCenter: parent.verticalCenter
                        src: "qrc:/icons/download.svg"
                        tint: Theme.accentText
                        s: 12
                    }
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: typeof session !== "undefined" ? session.totalDownSpeed : "0 KB/s"
                        color: Theme.t1
                        font.pixelSize: 13
                        font.weight: Font.DemiBold
                        font.family: Theme.fontSans
                        font.features: Theme.tnum
                    }
                }
            }
            Column {
                spacing: 3
                Text {
                    text: (i18n.language, i18n.t("graph_upload"))
                    color: Theme.t4
                    font.pixelSize: 9
                    font.weight: Font.Bold
                    font.letterSpacing: 1
                    font.family: Theme.fontSans
                }
                Row {
                    spacing: 5
                    IconImg {
                        anchors.verticalCenter: parent.verticalCenter
                        src: "qrc:/icons/upload.svg"
                        tint: Theme.up
                        s: 12
                    }
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: typeof session !== "undefined" ? session.totalUpSpeed : "0 KB/s"
                        color: Theme.t1
                        font.pixelSize: 13
                        font.weight: Font.DemiBold
                        font.family: Theme.fontSans
                        font.features: Theme.tnum
                    }
                }
            }
        }
    }
}
