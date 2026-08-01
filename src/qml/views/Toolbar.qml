// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Top action toolbar: open/magnet/pause/resume/stop/remove/search/rss/settings
// buttons + the global down/up speed readout. Carved out of Main.qml; reads window
// state via `win`, raises intent through signals (the parent owns the dialogs and
// nav), and exposes the Open button via `tbOpen` for the onboarding tour.
import QtQuick
import QtQuick.Layouts
import "../theme"
import "../widgets"

Rectangle {
    id: toolbar
    property var win
    property alias tbOpen: tbOpen
    signal openFile()
    signal addMagnet()
    signal addLink()
    signal removeSelected()
    signal openRss()
    signal makeRoomRequested()

    component TBtn: Rectangle {
        id: tb
        property string label
        property string icon
        property bool disabled: false
        property bool active: false       // toggled-on state (e.g. alt-speed turtle)
        property bool spinOnClick: false  // spin the icon on click — visible "it happened" feedback (Refresh)
        signal clicked()
        Layout.preferredWidth: 52
        Layout.minimumWidth: 52          // never let the RowLayout squeeze/clip the button
        Layout.preferredHeight: 54
        color: !disabled && tbMa.containsMouse ? Theme.hover : "transparent"
        radius: 8
        opacity: disabled ? 0.35 : 1.0

        function trigger() { if (tb.spinOnClick) tbSpin.restart(); tb.clicked() }
        // without these a screen reader reads nothing at all here — the label is
        // a plain Text with no semantic tie to the control
        Accessible.role: Accessible.Button
        Accessible.name: tb.label
        // no Accessible.disabled in QML — hide the control from the tree instead
        // so a reader doesn't offer an action that does nothing
        Accessible.ignored: tb.disabled
        Accessible.onPressAction: if (!tb.disabled) tb.trigger()
        activeFocusOnTab: !disabled
        Keys.onReturnPressed: if (!disabled) tb.trigger()
        Keys.onSpacePressed: if (!disabled) tb.trigger()
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
                id: tbIcon
                anchors.horizontalCenter: parent.horizontalCenter
                src: tb.icon
                // one step brighter than the label: a stroke carries far less ink
                // than a glyph, so matching colours makes the icon read fainter
                tint: tb.active ? Theme.accent : (!tb.disabled && tbMa.containsMouse ? Theme.t1 : Theme.t2)
                s: 18
                NumberAnimation { id: tbSpin; target: tbIcon; property: "rotation"; from: 0; to: 360; duration: 380; easing.type: Easing.OutCubic }
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
            onClicked: if (!tb.disabled) tb.trigger()
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
        // G1: Abrir, Magnet, Link
        TBtn { id: tbOpen; label: (i18n.language, i18n.t("tb_open"));   icon: "qrc:/icons/open.svg";  onClicked: toolbar.openFile() }
        TBtn { label: (i18n.language, i18n.t("tb_magnet"));  icon: "qrc:/icons/magnet.svg"; onClicked: toolbar.addMagnet() }
        TBtn { label: (i18n.language, i18n.t("tb_link"));    icon: "qrc:/icons/download.svg"; onClicked: toolbar.addLink() }
        TGrpDiv {}
        // G2: Pausar, Retomar, Parar, Atualizar (tester: keep transport controls together)
        TBtn { label: (i18n.language, i18n.t("tb_pause"));  icon: "qrc:/icons/pause.svg"; disabled: !win.hasSel; onClicked: session.pauseSelected() }
        TBtn { label: (i18n.language, i18n.t("tb_resume")); icon: "qrc:/icons/play.svg";  disabled: !win.hasSel; onClicked: session.resumeSelected() }
        TBtn { label: (i18n.language, i18n.t("tb_stop"));   icon: "qrc:/icons/stop.svg";  disabled: !win.hasSel; onClicked: session.pauseSelected() }
        TBtn { label: (i18n.language, i18n.t("tb_refresh")); icon: "qrc:/icons/refresh.svg"; spinOnClick: true; onClicked: { if (toolbar.win) toolbar.win.flashRefresh(); if (typeof session !== "undefined") session.refreshAll() } }
        TGrpDiv {}
        // G3: ações sobre a seleção (tester: copiar magnet e abrir pasta estavam
        // só no menu de contexto, apesar de anunciadas como estando aqui)
        TBtn { label: (i18n.language, i18n.t("tb_remove")); icon: "qrc:/icons/trash.svg"; disabled: !win.hasSel; onClicked: toolbar.removeSelected() }
        TBtn { label: (i18n.language, i18n.t("tb_copy"));   icon: "qrc:/icons/copy.svg";   disabled: !win.hasSel; onClicked: session.copyMagnetLink() }
        TBtn { label: (i18n.language, i18n.t("tb_folder")); icon: "qrc:/icons/folder.svg"; disabled: !win.hasSel; onClicked: session.openSaveFolder() }
        TGrpDiv {}
        // G4: RSS. The "Search" button used to live here and was removed: it
        // navigated to the Find page, but sat next to the downloads filter
        // field wearing the same magnifier — two meanings, one icon. Page
        // switching belongs to the nav rail; this toolbar acts on torrents.
        // Settings followed Search out of here for the same reason, one release
        // later: it wore the same gear as the nav bar's own Settings, two steps
        // away on screen, so the pair read as a bug rather than a shortcut.
        TBtn { label: (i18n.language, i18n.t("tb_rss"));     icon: "qrc:/icons/rss.svg";    onClicked: toolbar.openRss() }


        // .tb-spacer
        Item { Layout.fillWidth: true }

        // Free space sits with the transfer figures rather than in the nav:
        // all three are facts about this page's work, and the gauge followed
        // the user into Find and Settings for no reason.
        DiskGauge {
            onMakeRoomRequested: toolbar.makeRoomRequested()
        }
        TGrpDiv {}

        // global speed readout — boxless, neutral values; the tinted arrow
        // icons carry the down/up semantic without shouting
        Row {
            Layout.alignment: Qt.AlignVCenter
            Layout.rightMargin: Theme.sp2
            spacing: Theme.sp5

            Column {
                spacing: 3
                Text {
                    text: (i18n.language, i18n.t("graph_download").toLowerCase())
                    color: Theme.t4
                    font.pixelSize: 10
                    font.weight: Font.Bold
                    font.capitalization: Font.Capitalize
                    font.family: Theme.fontSans
                }
                Row {
                    spacing: 5
                    IconImg {
                        anchors.verticalCenter: parent.verticalCenter
                        src: "qrc:/icons/download.svg"
                        // same red/amber as the detail panel's arrows — the muted
                        // pair read as a different pair of colours one screen apart
                        tint: Theme.accent
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
                    text: (i18n.language, i18n.t("graph_upload").toLowerCase())
                    color: Theme.t4
                    font.pixelSize: 10
                    font.weight: Font.Bold
                    font.capitalization: Font.Capitalize
                    font.family: Theme.fontSans
                }
                Row {
                    spacing: 5
                    IconImg {
                        anchors.verticalCenter: parent.verticalCenter
                        src: "qrc:/icons/upload.svg"
                        tint: Theme.amber
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
