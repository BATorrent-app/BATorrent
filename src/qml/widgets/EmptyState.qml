// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Downloads with nothing in it: three ways to start, all in sight. A dashed
// area that says a file can be dropped anywhere, the one red button (it
// opens Add download), the paste shortcut, and six titles from Find that
// open their page. Offline, the Find row is not there.
import QtQuick
import QtQuick.Layouts
import "../theme"

Item {
    id: empty
    signal addClicked()
    signal findRequested()
    signal titleRequested(var item)

    implicitWidth: 860
    implicitHeight: col.implicitHeight

    readonly property var disco: typeof discovery !== "undefined" ? discovery : null
    onVisibleChanged: if (visible && disco && (disco.rows || []).length === 0) disco.load()
    Component.onCompleted: if (visible && disco && (disco.rows || []).length === 0) disco.load()

    // two films, two shows, two games from the top of Find's shelves; a kind
    // with none leaves its places to the others
    readonly property var picks: {
        var rows = disco ? (disco.rows || []) : []
        var byType = { movie: [], series: [], game: [] }, rest = [], seen = ({})
        for (var r = 0; r < rows.length; r++) {
            var items = rows[r].items || []
            for (var k = 0; k < items.length; k++) {
                var it = items[k]
                var key = (it.title || "") + "|" + (it.type || "")
                if (!it.poster || seen[key]) continue
                seen[key] = true
                var bucket = byType[it.type]
                if (bucket && bucket.length < 2) bucket.push(it)
                else rest.push(it)
            }
        }
        var out = byType.movie.concat(byType.series, byType.game)
        return out.concat(rest.slice(0, Math.max(0, 6 - out.length))).slice(0, 6)
    }
    function typeLabel(t) {
        return t === "movie" ? i18n.t("search_type_movie")
             : t === "series" ? i18n.t("search_type_series")
             : t === "game" ? i18n.t("search_type_game") : ""
    }

    ColumnLayout {
        id: col
        width: parent.width
        spacing: 0

        // the drop area: dashed, because a file can land here (and anywhere)
        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 270
            Canvas {
                id: dash
                anchors.fill: parent
                onPaint: {
                    var ctx = getContext("2d")
                    ctx.reset()
                    ctx.strokeStyle = String(Theme.t4)
                    ctx.globalAlpha = 0.55
                    ctx.lineWidth = 1
                    ctx.setLineDash([4, 4])
                    var r = Theme.rad(12)
                    ctx.beginPath()
                    ctx.roundedRect(0.5, 0.5, width - 1, height - 1, r, r)
                    ctx.stroke()
                }
                Connections { target: Theme; function onT4Changed() { dash.requestPaint() } }
            }
            ColumnLayout {
                anchors.centerIn: parent
                width: Math.min(parent.width - 64, 560)
                spacing: 0
                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: (i18n.language, i18n.t("empty_first_title"))
                    color: Theme.t1
                    font.pixelSize: 26; font.weight: Font.DemiBold; font.family: Theme.fontSans
                }
                Text {
                    Layout.fillWidth: true
                    Layout.topMargin: 12
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.WordWrap
                    text: (i18n.language, i18n.t("empty_first_body"))
                    color: Theme.t2
                    font.pixelSize: 15; font.family: Theme.fontSans
                    lineHeight: 1.35
                }
                Row {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.topMargin: 24
                    spacing: 14
                    Rectangle {
                        width: addRow.implicitWidth + 40; height: 44
                        radius: Theme.rad(8)
                        color: addMa.containsMouse ? Theme.accentDark : Theme.accent
                        Row {
                            id: addRow
                            anchors.centerIn: parent
                            spacing: 9
                            Text { anchors.verticalCenter: parent.verticalCenter; text: "+"; color: Theme.inkOnAccent; font.pixelSize: 18; font.weight: Font.DemiBold; font.family: Theme.fontSans }
                            Text { anchors.verticalCenter: parent.verticalCenter; text: (i18n.language, i18n.t("empty_add_btn")); color: Theme.inkOnAccent; font.pixelSize: 15; font.weight: Font.DemiBold; font.family: Theme.fontSans }
                        }
                        MouseArea { id: addMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: empty.addClicked() }
                    }
                    Row {
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 6
                        Text { anchors.verticalCenter: parent.verticalCenter; text: (i18n.language, i18n.t("empty_or_paste")); color: Theme.t3; font.pixelSize: 13; font.family: Theme.fontSans }
                        Repeater {
                            model: [Qt.platform.os === "osx" ? "⌘" : "Ctrl", "V"]
                            Rectangle {
                                required property var modelData
                                anchors.verticalCenter: parent.verticalCenter
                                width: Math.max(26, kTxt.implicitWidth + 14); height: 26
                                radius: Theme.rad(6)
                                color: Theme.field
                                border.color: Theme.hair; border.width: 1
                                Text { id: kTxt; anchors.centerIn: parent; text: modelData; color: Theme.t2; font.pixelSize: 12; font.weight: Font.DemiBold; font.family: Theme.fontMono }
                            }
                        }
                    }
                }
            }
        }

        // the third way: something from Find (its page, never a download)
        RowLayout {
            visible: empty.picks.length > 0
            Layout.fillWidth: true
            Layout.topMargin: 40
            Text {
                Layout.fillWidth: true
                text: (i18n.language, i18n.t("empty_from_find"))
                color: Theme.t1
                font.pixelSize: 16; font.weight: Font.DemiBold; font.family: Theme.fontSans
            }
            Text {
                text: (i18n.language, i18n.t("empty_open_find")) + "  →"
                color: ofMa.containsMouse ? Theme.t1 : Theme.t2
                font.pixelSize: 14; font.family: Theme.fontSans
                MouseArea { id: ofMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: empty.findRequested() }
            }
        }
        Row {
            visible: empty.picks.length > 0
            Layout.fillWidth: true
            Layout.topMargin: 14
            spacing: 14
            readonly property real cardW: (col.width - 5 * spacing) / 6
            Repeater {
                model: empty.picks
                Column {
                    id: pick
                    required property var modelData
                    width: parent.cardW
                    spacing: 6
                    Rectangle {
                        width: parent.width; height: width * 1.5
                        radius: Theme.rad(8)
                        color: Theme.panel
                        clip: true
                        RoundedImage {
                            anchors.fill: parent
                            radius: Theme.rad(8)
                            sourceWidth: 260
                            sources: [pick.modelData.poster]
                        }
                        Rectangle {
                            anchors.fill: parent
                            radius: Theme.rad(8)
                            color: "transparent"
                            border.width: 2; border.color: Theme.t1
                            opacity: pkMa.containsMouse ? 1 : 0
                            Behavior on opacity { NumberAnimation { duration: Theme.durFast } }
                        }
                        MouseArea { id: pkMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: empty.titleRequested(pick.modelData) }
                    }
                    Text {
                        width: parent.width
                        text: pick.modelData.title || ""
                        textFormat: Text.PlainText
                        elide: Text.ElideRight
                        color: Theme.t1
                        font.pixelSize: 14; font.weight: Font.DemiBold; font.family: Theme.fontSans
                    }
                    Text {
                        width: parent.width
                        text: (i18n.language, [empty.typeLabel(pick.modelData.type || ""), pick.modelData.year || ""]
                                              .filter(function (s) { return String(s).length > 0 }).join(" · "))
                        elide: Text.ElideRight
                        color: Theme.t3
                        font.pixelSize: 12; font.family: Theme.fontSans; font.features: Theme.tnum
                    }
                }
            }
        }
    }
}
