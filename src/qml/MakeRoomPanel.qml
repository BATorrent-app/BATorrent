// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// "Free up space" panel: every torrent, sortable by size or age, one-click
// into the existing remove flow (deleteRequested → Main.qml opens removeDlg,
// which already has the delete-files + permanent options). Global overlay —
// opened from the NavRail disk bar, or from Search's "won't fit" dialog with
// a target (how many bytes still need to be freed).
import QtQuick
import QtQuick.Layouts
import "theme"
import "widgets"

Rectangle {
    id: root
    property bool open: false
    // extra bytes still needed beyond current free space, 0 = just browsing
    property double targetBytes: 0
    signal deleteRequested(string infoHash)

    property var rows: []
    function reload() { rows = (typeof session !== "undefined") ? session.makeRoomList() : [] }
    onOpenChanged: if (open) reload()

    property string sortKey: "size"   // "size" | "age"
    readonly property var sortedRows: {
        var arr = rows.slice()
        if (sortKey === "age") arr.sort(function (a, b) { return (a.addedTime || 0) - (b.addedTime || 0) })
        else arr.sort(function (a, b) { return (b.sizeBytes || 0) - (a.sizeBytes || 0) })
        return arr
    }

    readonly property double freeBytes: (typeof session !== "undefined") ? session.freeSaveBytes() : -1
    function fmtSize(b) {
        if (!b || b <= 0) return "0 B"
        var u = ["B", "KB", "MB", "GB", "TB"]
        var i = Math.min(u.length - 1, Math.floor(Math.log(b) / Math.log(1024)))
        return (b / Math.pow(1024, i)).toFixed(i >= 2 ? 1 : 0) + " " + u[i]
    }
    function fmtAge(secs) {
        if (!secs || secs <= 0) return "—"
        var days = Math.floor((Date.now() / 1000 - secs) / 86400)
        if (days <= 0) return i18n.t("make_room_today")
        if (days === 1) return i18n.t("make_room_yesterday")
        return i18n.t("make_room_days_ago").arg(days)
    }

    anchors.fill: parent
    visible: root.open
    color: "#aa000000"
    MouseArea { anchors.fill: parent; onClicked: root.open = false }

    Rectangle {
        anchors.centerIn: parent
        width: Math.min(parent.width - 48, 600)
        height: Math.min(parent.height - 48, 560)
        radius: 12
        color: Theme.elev
        border.color: Theme.hair
        border.width: 1
        MouseArea { anchors.fill: parent }   // swallow clicks so the dim layer doesn't close it

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: Theme.sp3

            RowLayout {
                Layout.fillWidth: true
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2
                    Text { text: (i18n.language, i18n.t("make_room_title")); color: Theme.t1; font.pixelSize: 15; font.weight: Font.DemiBold; font.family: Theme.fontSans }
                    Text {
                        text: {
                            var free = root.freeBytes >= 0 ? root.fmtSize(root.freeBytes) : "—"
                            if (root.targetBytes > 0)
                                return (i18n.language, i18n.t("make_room_need_more")).arg(free).arg(root.fmtSize(root.targetBytes))
                            return (i18n.language, i18n.t("make_room_free_now")).arg(free)
                        }
                        color: root.targetBytes > 0 ? "#e0a533" : Theme.t4
                        font.pixelSize: 11; font.weight: Font.DemiBold; font.family: Theme.fontSans
                    }
                }
                IconImg {
                    src: "qrc:/icons/close.svg"; tint: closeMa.containsMouse ? Theme.t1 : Theme.t4; s: 16
                    MouseArea { id: closeMa; anchors.fill: parent; anchors.margins: -6; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: root.open = false }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.sp2
                Text {
                    Layout.fillWidth: true
                    text: (i18n.language, i18n.t("make_room_sort_by"))
                    color: Theme.t4; font.pixelSize: 11; font.family: Theme.fontSans
                }
                BtnFlat {
                    primary: root.sortKey === "size"
                    text: (i18n.language, i18n.t("make_room_sort_size"))
                    onClicked: root.sortKey = "size"
                }
                BtnFlat {
                    primary: root.sortKey === "age"
                    text: (i18n.language, i18n.t("make_room_sort_age"))
                    onClicked: root.sortKey = "age"
                }
            }

            ListView {
                id: roomList
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                model: root.sortedRows
                boundsBehavior: Flickable.StopAtBounds
                spacing: 2
                WheelScroller { flick: roomList }

                Text {
                    anchors.centerIn: parent
                    visible: parent.count === 0
                    text: (i18n.language, i18n.t("make_room_empty"))
                    color: Theme.t4; font.pixelSize: 12; font.family: Theme.fontSans
                }

                delegate: Rectangle {
                    required property var modelData
                    width: ListView.view.width
                    height: 48
                    radius: 8
                    color: rowMa.containsMouse ? Theme.hover : "transparent"
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 12; anchors.rightMargin: 8
                        spacing: Theme.sp3
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 1
                            Text {
                                text: modelData.name
                                color: Theme.t1; font.pixelSize: 12; font.family: Theme.fontSans
                                elide: Text.ElideRight; Layout.fillWidth: true
                            }
                            RowLayout {
                                spacing: 8
                                Text {
                                    text: {
                                        if (modelData.seeding) return (i18n.language, i18n.t("make_room_seeding"))
                                        if (modelData.paused) return (i18n.language, i18n.t("make_room_paused"))
                                        if (modelData.completed) return (i18n.language, i18n.t("make_room_completed"))
                                        return (i18n.language, i18n.t("make_room_downloading"))
                                    }
                                    color: modelData.seeding ? Theme.grn : Theme.t4
                                    font.pixelSize: 10; font.family: Theme.fontSans
                                }
                                Text { text: "·"; color: Theme.t4; font.pixelSize: 10 }
                                Text { text: root.fmtAge(modelData.addedTime); color: Theme.t4; font.pixelSize: 10; font.family: Theme.fontSans }
                            }
                        }
                        Text {
                            text: modelData.size
                            color: Theme.t2; font.pixelSize: 13; font.weight: Font.DemiBold; font.family: Theme.fontMono
                        }
                        IconImg {
                            src: "qrc:/icons/trash.svg"; s: 16
                            tint: delMa.containsMouse ? Theme.accent : Theme.t3
                            MouseArea {
                                id: delMa; anchors.fill: parent; anchors.margins: -8
                                hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                                onClicked: root.deleteRequested(modelData.infoHash)
                            }
                        }
                    }
                    MouseArea { id: rowMa; anchors.fill: parent; hoverEnabled: true; z: -1 }
                }
            }
        }
    }
}
