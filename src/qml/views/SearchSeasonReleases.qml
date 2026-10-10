// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Every release of a season in one dense list, for when picking episode by
// episode is the slow way: packs first, then each episode in order.
import QtQuick
import QtQuick.Layouts
import "../theme"
import "../widgets"

ListView {
    id: list
    required property var sv
    property var index
    property int season
    property var titles: []

    clip: true
    bottomMargin: 20
    boundsBehavior: Flickable.StopAtBounds
    WheelScroller { flick: list }

    function titleOf(ep) {
        for (var i = 0; i < titles.length; i++) if (titles[i].episode === ep) return titles[i].name || ""
        return ""
    }
    function tags(r) {
        var t = (r.langs || []).slice(0, 3)
        if (r.quality) t.push(r.quality)
        if (r.source && r.source !== "CAM") t.push(r.source)
        if (r.codec) t.push(r.codec)
        if (r.hdr) t.push("HDR")
        var tail = [r.releaseGroup || "", r.sub || r.provider || ""].filter(function (x) { return x.length > 0 })
        return t.join("  ") + (tail.length > 0 ? "  ·  " + tail.join("  ·  ") : "")
    }

    model: {
        if (!index) return []
        var out = index.packs.slice()
        var eps = Object.keys(index.byEp).map(Number).sort(function (a, b) { return a - b })
        for (var i = 0; i < eps.length; i++) out = out.concat(index.byEp[eps[i]])
        return out
    }

    delegate: Rectangle {
        id: row
        required property var modelData
        readonly property bool isPack: !((modelData.episode || 0) > 0)
        width: list.width
        height: body.implicitHeight + 22
        radius: 6
        color: ma.containsMouse ? Qt.rgba(1, 1, 1, 0.04) : "transparent"

        MouseArea { id: ma; anchors.fill: parent; hoverEnabled: true }

        RowLayout {
            id: body
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            anchors.leftMargin: 8
            anchors.rightMargin: 8
            spacing: 14
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 4
                Row {
                    Layout.fillWidth: true
                    spacing: 8
                    Text {
                        text: (i18n.language, i18n.t("search_season_abbr")).arg(list.season)
                              + (row.isPack ? "" : " " + i18n.t("search_episode_abbr").arg(row.modelData.episode))
                        color: Theme.stageT1
                        font.pixelSize: 13; font.weight: Font.Medium; font.family: Theme.fontSans
                    }
                    Text {
                        width: Math.max(0, body.width - 200)
                        text: row.isPack ? (i18n.language, i18n.t("find_season_pack"))
                                         : list.titleOf(row.modelData.episode)
                        color: Theme.stageMuted
                        font.pixelSize: 13; font.family: Theme.fontSans
                        elide: Text.ElideRight
                    }
                }
                Text {
                    Layout.fillWidth: true
                    text: list.tags(row.modelData)
                    color: Theme.stageDim
                    font.pixelSize: 11; font.family: Theme.fontSans
                    elide: Text.ElideRight
                }
            }
            Text {
                Layout.preferredWidth: 60
                horizontalAlignment: Text.AlignRight
                text: row.modelData.sizeStr || ""
                color: Theme.stageT2
                font.pixelSize: 12; font.family: Theme.fontSans; font.features: Theme.tnum
            }
            Text {
                Layout.preferredWidth: 40
                horizontalAlignment: Text.AlignRight
                text: row.modelData.seedsN || 0
                color: list.sv.seedColor(row.modelData.seedsN || 0)
                font.pixelSize: 12; font.family: Theme.fontSans; font.features: Theme.tnum
            }
            StageRoundButton {
                size: 28
                icon: "qrc:/icons/download.svg"
                onClicked: if (list.sv.api) list.sv.api.activateResult(row.modelData._idx)
            }
        }
    }
}
