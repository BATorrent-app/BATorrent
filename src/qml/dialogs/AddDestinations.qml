// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Where the Add download window's items go: the folders with the room each
// one has left after, and, when it does not fit, the real ways out, next to
// the choice they fix.
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import "../theme"
import "../widgets"

ColumnLayout {
    id: destinations
    required property var host
    Layout.fillWidth: true
    spacing: 10
    RowLayout {
        Layout.fillWidth: true
        Text { text: (i18n.language, i18n.t("addw_save_to")); color: Theme.t3; font.pixelSize: 14; font.family: Theme.fontSans }
        Item { Layout.fillWidth: true }
        Text {
            visible: host.needBytes > 0
            color: host.destFits ? Theme.t2 : Theme.amber
            font.pixelSize: 13; font.family: Theme.fontSans; font.features: Theme.tnum
            text: {
                i18n.language
                if (!host.destFits) return i18n.t("addw_needs").arg(host.fmt(host.needBytes))
                var f = host.freeAt(host.destPath)
                return f >= 0 ? i18n.t("addw_of_free").arg(host.fmt(host.needBytes)).arg(host.fmt(f)) : host.fmt(host.needBytes)
            }
        }
    }
    Flow {
        Layout.fillWidth: true
        spacing: 8
        Repeater {
            model: host.dests
            delegate: Rectangle {
                id: dchip
                readonly property bool on: modelData.path === host.destPath
                readonly property double free: host.freeAt(modelData.path)
                readonly property bool ok: free < 0 || host.needBytes <= free
                width: Math.max(dName.implicitWidth, dSub.implicitWidth) + 24
                height: 44
                radius: Theme.rad(8)
                color: on ? Theme.hover : (dMa.containsMouse ? Theme.hover : "transparent")
                border.width: on ? 1.5 : 1
                border.color: on ? (ok ? Theme.t1 : Theme.amber) : Theme.hair
                Column {
                    anchors.verticalCenter: parent.verticalCenter
                    x: 12
                    spacing: 1
                    Text { id: dName; text: modelData.name; color: dchip.on ? Theme.t1 : Theme.t2; font.pixelSize: 14; font.weight: dchip.on ? Font.DemiBold : Font.Medium; font.family: Theme.fontSans }
                    Text {
                        id: dSub
                        visible: text !== ""
                        color: !dchip.ok ? Theme.amber : (host.needBytes > 0 ? Theme.grn : Theme.t4)
                        font.pixelSize: 13; font.family: Theme.fontSans; font.features: Theme.tnum
                        text: {
                            i18n.language
                            if (dchip.free < 0) return ""
                            if (host.needBytes <= 0) return i18n.t("addw_free").arg(host.fmt(dchip.free))
                            return dchip.ok ? i18n.t("addw_left_after").arg(host.fmt(dchip.free - host.needBytes))
                                            : i18n.t("addw_short").arg(host.fmt(host.needBytes - dchip.free))
                        }
                    }
                }
                MouseArea {
                    id: dMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                    onClicked: { host.destPath = modelData.path; host.destTouched = true }
                }
                ToolTip.visible: dMa.containsMouse
                ToolTip.delay: 600
                ToolTip.text: modelData.path
            }
        }
        Rectangle {
            width: oTxt.implicitWidth + 24; height: 44
            radius: Theme.rad(8)
            color: oMa.containsMouse ? Theme.hover : "transparent"
            border.color: Theme.hair; border.width: 1
            Text { id: oTxt; anchors.centerIn: parent; text: (i18n.language, i18n.t("addw_other")); color: Theme.t2; font.pixelSize: 14; font.weight: Font.Medium; font.family: Theme.fontSans }
            MouseArea { id: oMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: host.chooseOtherFolder() }
        }
    }
    // the real ways out when it does not fit
    Rectangle {
        Layout.fillWidth: true
        visible: !host.destFits && host.needBytes > 0
        implicitHeight: warnCol.implicitHeight + 24
        radius: Theme.rad(9)
        color: Qt.rgba(Theme.amber.r, Theme.amber.g, Theme.amber.b, 0.08)
        border.color: Qt.rgba(Theme.amber.r, Theme.amber.g, Theme.amber.b, 0.3); border.width: 1
        ColumnLayout {
            id: warnCol
            x: 12; y: 12
            width: parent.width - 24
            spacing: 8
            Text {
                Layout.fillWidth: true
                text: (i18n.language, i18n.t("addw_no_fit")).arg(host.destName(host.destPath))
                color: Theme.amber; font.pixelSize: 13; font.family: Theme.fontSans
                wrapMode: Text.WordWrap
            }
            Flow {
                Layout.fillWidth: true
                spacing: 8
                BtnFlat {
                    visible: host.fitsElsewhere !== null
                    text: host.fitsElsewhere ? (i18n.language, i18n.t("addw_use")).arg(host.fitsElsewhere.name) : ""
                    onClicked: { host.destPath = host.fitsElsewhere.path; host.destTouched = true }
                }
                BtnFlat {
                    visible: host.keepChip !== null
                    text: host.keepChip ? (i18n.language, i18n.t("addw_keep_only")).arg(host.keepChip.chip.label).arg(host.fmt(host.keepChip.saves)) : ""
                    onClicked: {
                        var k = host.keepChip
                        host.setFiles(k.group.files, false)
                        host.setFiles(k.chip.files, true)
                    }
                }
                // the last way out, when no other folder fits and nothing can be left out
                BtnFlat {
                    visible: host.fitsElsewhere === null && host.keepChip === null
                    text: (i18n.language, i18n.t("search_wontfit_freeup"))
                    onClicked: {
                        var need = Math.max(0, host.needBytes - Math.max(0, host.freeAt(host.destPath)))
                        var what = host.single ? host.firstItem().name : i18n.t("addw_sum_items").arg(host.addCount)
                        var folder = host.destPath
                        host.reject()
                        host.freeSpaceRequested(need, what, folder)
                    }
                }
            }
        }
    }
}
