// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Downloads action row: Add as the one primary button, then what acts on the
// selection (one Pause/Resume, Remove, Copy magnet, Folder), RSS, and at the
// right the free space and the global speeds. Stop is gone (it was Pause
// under another name) and Refresh lives on F5 and the app menu. Reads window
// state via `win`, raises intent through signals; `tbOpen` is the Add button,
// for the onboarding tour.
import QtQuick
import QtQuick.Layouts
import QtQuick.Effects
import QtQuick.Controls
import "../theme"
import "../widgets"

Rectangle {
    id: toolbar
    // nothing in the list: Add, RSS and the disk; the rest comes back with the first item
    readonly property bool empty: typeof session !== "undefined" && session.torrentCount === 0
    property var win
    property var controller: null
    property string downSpeedOverride: ""
    property string upSpeedOverride: ""
    property alias tbOpen: tbOpen
    signal openFile()
    signal addMagnet()
    signal addLink()
    signal removeSelected()
    signal openRss()
    signal makeRoomRequested()

    readonly property int selCount: controller && controller.selectedRows ? controller.selectedRows.length
                                    : (win && win.hasSel ? 1 : 0)
    readonly property bool hasSel: win ? win.hasSel : false
    // One button for both. A mixed selection pauses: the first item being
    // paused says nothing about the rest.
    readonly property bool resumes: hasSel && selCount <= 1
                                    && typeof session !== "undefined" && session.selectedPaused

    component TBtn: Rectangle {
        id: tb
        property string label
        property string icon
        property string hint
        property bool disabled: false
        signal clicked()
        Layout.preferredHeight: 34
        Layout.preferredWidth: tbRow.implicitWidth + 20
        Layout.minimumWidth: Layout.preferredWidth
        radius: Theme.rad(8)
        color: !disabled && tbMa.containsMouse ? Theme.hover : "transparent"
        opacity: disabled ? 0.4 : 1.0
        Behavior on color { ColorAnimation { duration: Theme.durFast } }
        readonly property color ink: !disabled && tbMa.containsMouse ? Theme.t1 : Theme.t2

        Accessible.role: Accessible.Button
        Accessible.name: tb.hint.length > 0 ? tb.hint : tb.label
        Accessible.ignored: tb.disabled
        Accessible.onPressAction: if (!tb.disabled) tb.clicked()
        ToolTip.visible: tb.hint.length > 0 && tbMa.containsMouse && !tb.disabled
        ToolTip.text: tb.hint
        ToolTip.delay: 400
        activeFocusOnTab: !disabled
        Keys.onReturnPressed: if (!disabled) tb.clicked()
        Keys.onSpacePressed: if (!disabled) tb.clicked()
        scale: tbMa.pressed && !tb.disabled ? Theme.pressScale : 1
        Behavior on scale { NumberAnimation { duration: Theme.durFast; easing.type: Theme.easeOut } }
        Rectangle {
            visible: tb.activeFocus
            anchors.fill: parent
            anchors.margins: -2
            radius: Theme.rad(10)
            color: "transparent"
            border.color: Theme.focusRing
            border.width: Theme.focusRingWidth
        }
        Row {
            id: tbRow
            anchors.centerIn: parent
            spacing: 7
            IconImg {
                anchors.verticalCenter: parent.verticalCenter
                src: tb.icon
                tint: tb.ink
                s: 16
            }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: tb.label
                color: tb.ink
                font.pixelSize: 13
                font.family: Theme.fontSans
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
        Layout.preferredHeight: 24
        Layout.leftMargin: 6
        Layout.rightMargin: 6
        Layout.alignment: Qt.AlignVCenter
        color: Theme.hair
    }

    Layout.fillWidth: true
    Layout.preferredHeight: 52
    color: Theme.panel
    Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hair }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Theme.sp4
        anchors.rightMargin: Theme.sp4
        spacing: 4

        // The universal add: the field works out whether it got a magnet, a
        // link or a file, and the file picker is one click inside it.
        Rectangle {
            id: tbOpen
            Layout.preferredHeight: 34
            Layout.preferredWidth: addRow.implicitWidth + 25
            Layout.rightMargin: 8
            radius: Theme.rad(8)
            // hover lifts it enough to see (8% did not show on red, 22% turned it pink)
            color: addMa.pressed ? Theme.accentDark : (addMa.containsMouse ? Qt.lighter(Theme.accent, 1.13) : Theme.accent)
            Behavior on color { ColorAnimation { duration: Theme.durFast } }
            scale: addMa.pressed ? Theme.pressScale : 1
            Behavior on scale { NumberAnimation { duration: Theme.durFast; easing.type: Theme.easeOut } }
            // Dark Star: the primary button gives off a halo
            Rectangle {
                id: addGlowSrc
                anchors.fill: parent
                radius: parent.radius
                color: Theme.accent
                visible: false
                layer.enabled: Theme.glow
            }
            MultiEffect {
                visible: Theme.glow
                z: -1
                source: addGlowSrc
                anchors.fill: addGlowSrc
                autoPaddingEnabled: true
                shadowEnabled: true
                shadowColor: Theme.accent
                shadowOpacity: 0.6
                shadowBlur: 0.9
                blurMax: 36
            }
            Row {
                id: addRow
                anchors.verticalCenter: parent.verticalCenter
                x: 11
                spacing: 7
                IconImg { anchors.verticalCenter: parent.verticalCenter; src: "qrc:/icons/plus.svg"; tint: Theme.inkOnAccent; s: 15 }
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: (i18n.language, i18n.t("tb_add"))
                    color: Theme.inkOnAccent
                    font.pixelSize: 13; font.weight: Font.DemiBold; font.family: Theme.fontSans
                }
            }
            MouseArea {
                id: addMa
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: toolbar.addMagnet()
            }
            activeFocusOnTab: true
            Keys.onReturnPressed: toolbar.addMagnet()
            Keys.onSpacePressed: toolbar.addMagnet()
            Accessible.role: Accessible.Button
            Accessible.name: (i18n.language, i18n.t("add_any_title"))
            Accessible.onPressAction: toolbar.addMagnet()
            ToolTip.visible: addMa.containsMouse
            ToolTip.text: (i18n.language, i18n.t("add_any_title"))
            ToolTip.delay: 600
        }
        TGrpDiv { visible: !toolbar.empty }
        Text {
            visible: !toolbar.empty
            Layout.leftMargin: 4
            Layout.rightMargin: 8
            text: (i18n.language, toolbar.selCount === 0 ? i18n.t("tb_sel_none")
                  : toolbar.selCount === 1 ? i18n.t("tb_sel_one")
                  : i18n.t("tb_sel_many").arg(toolbar.selCount))
            color: Theme.t4
            font.pixelSize: 12; font.family: Theme.fontSans; font.features: Theme.tnum
        }
        TBtn {
            visible: !toolbar.empty
            label: (i18n.language, i18n.t(toolbar.resumes ? "tb_resume" : "tb_pause"))
            icon: toolbar.resumes ? "qrc:/icons/play.svg" : "qrc:/icons/pause.svg"
            disabled: !toolbar.hasSel
            onClicked: toolbar.resumes ? session.resumeSelected() : session.pauseSelected()
        }
        TBtn { visible: !toolbar.empty; label: (i18n.language, i18n.t("tb_remove")); icon: "qrc:/icons/trash.svg"; disabled: !toolbar.hasSel; onClicked: toolbar.removeSelected() }
        TBtn { visible: !toolbar.empty; label: (i18n.language, i18n.t("tb_copy_magnet")); icon: "qrc:/icons/copy.svg"; disabled: !toolbar.hasSel; onClicked: session.copyMagnetLink() }
        TBtn { visible: !toolbar.empty; label: (i18n.language, i18n.t("tb_folder")); icon: "qrc:/icons/folder.svg"; hint: (i18n.language, i18n.t("ctx_open_folder")); disabled: !toolbar.hasSel; onClicked: session.openSaveFolder() }
        TGrpDiv { visible: !toolbar.empty }
        TBtn { label: (i18n.language, i18n.t("tb_rss")); icon: "qrc:/icons/rss.svg"; onClicked: toolbar.openRss() }

        Item { Layout.fillWidth: true }

        DiskGauge {
            onMakeRoomRequested: toolbar.makeRoomRequested()
        }
        TGrpDiv { visible: !toolbar.empty }

        // alt speed (turtle): a speed limit, so it lives with the speeds
        Rectangle {
            id: altChip
            visible: !toolbar.empty
            readonly property bool on: typeof session !== "undefined" && session.altSpeedsActive
            Layout.alignment: Qt.AlignVCenter
            Layout.preferredHeight: 32
            Layout.preferredWidth: altRow.implicitWidth + 24
            radius: Theme.rad(9)
            color: on ? Theme.accentTint : (altMa.containsMouse ? Theme.hover : "transparent")
            Behavior on color { ColorAnimation { duration: Theme.durFast } }
            readonly property color ink: on ? Theme.accentText : Theme.t3
            function limitText() {
                var kb = typeof settings !== "undefined" ? Number(settings.get("altDownloadLimit") || 0) : 0
                if (kb <= 0) return ""
                return kb >= 1024 ? (Math.round(kb / 102.4) / 10).toLocaleString(Qt.locale(), 'f', kb % 1024 === 0 ? 0 : 1) + " MB/s"
                                  : kb + " KB/s"
            }
            Row {
                id: altRow
                anchors.centerIn: parent
                spacing: 8
                IconImg {
                    anchors.verticalCenter: parent.verticalCenter
                    src: "qrc:/icons/turtle.svg"
                    tint: altChip.ink
                    s: 17
                }
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: (i18n.language, altChip.on
                           ? (altChip.limitText().length > 0 ? i18n.t("nav_alt_on_limit").arg(altChip.limitText())
                                                             : i18n.t("nav_alt_on"))
                           : i18n.t("nav_alt_off"))
                    color: altChip.ink
                    font.pixelSize: 13; font.weight: Font.Medium; font.family: Theme.fontSans
                }
            }
            MouseArea {
                id: altMa
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: if (typeof session !== "undefined") session.setAltSpeedsActive(!session.altSpeedsActive)
            }
            ToolTip.text: (i18n.language, i18n.t("tb_alt_speed"))
            ToolTip.visible: altMa.containsMouse
            ToolTip.delay: 400
            Accessible.role: Accessible.CheckBox
            Accessible.name: (i18n.language, i18n.t("tb_alt_speed"))
            Accessible.checked: altChip.on
        }


        // global speeds: the tinted arrows carry down/up, the figures stay neutral
        Row {
            visible: !toolbar.empty
            Layout.alignment: Qt.AlignVCenter
            Layout.leftMargin: 8
            Layout.rightMargin: 4
            spacing: 18
            Row {
                spacing: 5
                IconImg { anchors.verticalCenter: parent.verticalCenter; src: "qrc:/icons/download.svg"; tint: Theme.down; s: 12 }
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: toolbar.downSpeedOverride.length > 0 ? toolbar.downSpeedOverride
                          : (typeof session !== "undefined" ? session.totalDownSpeed : "0 KB/s")
                    color: Theme.t1
                    font.pixelSize: 14; font.weight: Font.DemiBold; font.family: Theme.fontSans; font.features: Theme.tnum
                }
            }
            Row {
                spacing: 5
                IconImg { anchors.verticalCenter: parent.verticalCenter; src: "qrc:/icons/upload.svg"; tint: Theme.amber; s: 12 }
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: toolbar.upSpeedOverride.length > 0 ? toolbar.upSpeedOverride
                          : (typeof session !== "undefined" ? session.totalUpSpeed : "0 KB/s")
                    color: Theme.t1
                    font.pixelSize: 14; font.weight: Font.DemiBold; font.family: Theme.fontSans; font.features: Theme.tnum
                }
            }
        }
    }
}
