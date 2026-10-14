// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Under a Downloads grid card: the title, then the one place its state lives,
// a dot and a line. A problem comes first ("No seeds", "Missing"), then how
// it is going ("Downloading · 12m" with "4.6 / 7.4 GB" at the right, "Paused
// · 7.2 of 39.9 GB", "Uploading 2.3 MB/s"); with nothing to report, the year
// and genres. One colour per meaning: red is downloading or wrong, green is
// done, grey is waiting.
import QtQuick
import QtQuick.Controls.Basic
import "../theme"

Column {
    id: root
    required property var tile
    required property var win

    spacing: 3

    // "4.6 GB" and "7.4 GB" → "4.6 / 7.4 GB": the unit once
    function pair(a, b, joiner) {
        var ua = a.replace(/^[\d.,\s]+/, ""), ub = b.replace(/^[\d.,\s]+/, "")
        if (ua.length > 0 && ua === ub) a = a.slice(0, a.length - ua.length).trim()
        return a + joiner + b
    }
    function shortEta(sec) {
        if (sec < 0) return ""
        return sec >= 86400 ? Math.floor(sec / 86400) + "d"
             : sec >= 3600  ? Math.floor(sec / 3600) + "h"
             : sec >= 60    ? Math.floor(sec / 60) + "m"
             : sec + "s"
    }

    readonly property string k: tile.stateKey
    readonly property bool trouble: k === "missing" || k === "error"
    // stuck while downloading: the engine's short reason ("No seeds")
    readonly property bool slow: tile.isDownloading && tile.stateDetailShort.length > 0
    readonly property bool done: k === "seeding" || k === "completed"
    readonly property bool waiting: k === "paused" || k === "queued" || k === "checking"

    readonly property color dot: tile.isGroup ? (tile.progress >= 0.999 ? Theme.grn : Theme.accent)
                               : done ? Theme.grn
                               : waiting ? Theme.t3
                               : Theme.accent
    readonly property color ink: trouble || slow ? Theme.accentText
                               : waiting ? Theme.t3 : Theme.t2

    readonly property string catalogue: {
        var parts = []
        if (tile.year > 0) parts.push(String(tile.year))
        if (tile.genres.length > 0) parts.push(tile.genres)
        return parts.join(" · ")
    }
    readonly property string line: (i18n.language,
          tile.isGroup ? tile.groupCaption
        : k === "missing" ? i18n.t("tile_missing")
        : k === "error" ? i18n.t("tile_error") + (tile.stateDetailShort.length > 0 ? " · " + tile.stateDetailShort : "")
        : k === "fetching" ? i18n.t("tile_fetching") + (tile.stateDetailShort.length > 0 ? " · " + tile.stateDetailShort : "")
        : slow ? tile.stateDetailShort
        : tile.isDownloading ? i18n.t("state_downloading") + (tile.etaSec >= 0 ? " · " + shortEta(tile.etaSec) : "")
        : k === "paused" && tile.progress < 0.999 ? i18n.t("state_paused") + " · " + pair(tile.downloaded, tile.size, " " + i18n.t("word_of") + " ")
        : k === "paused" ? i18n.t("state_paused_done")
        : k === "queued" ? i18n.t("tile_queued").arg(tile.queuePos)
        : k === "checking" ? i18n.t("state_checking")
        : k === "seeding" && tile.upRate > 0 ? i18n.t("tile_uploading").arg(tile.upSpeed)
        : catalogue.length > 0 ? catalogue
        : i18n.t("state_completed"))
    readonly property string rightText:
          tile.isGroup ? tile.size
        : slow ? shortEta(tile.etaSec)
        : tile.isDownloading ? pair(tile.downloaded, tile.size, " / ")
        : k === "paused" || k === "fetching" || k === "queued" ? ""
        : tile.size

    Text {
        width: root.width
        text: tile.metaTitle || tile.torrentName
        color: Theme.t1
        font.pixelSize: 14
        font.weight: Font.DemiBold
        font.family: Theme.fontSans
        elide: Text.ElideRight
    }
    Item {
        width: root.width
        height: 17
        Rectangle {
            id: dotMark
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            width: 7; height: 7; radius: 3.5
            color: root.dot
            Behavior on color { ColorAnimation { duration: Theme.durBase } }
        }
        Text {
            id: lineTxt
            anchors.left: dotMark.right
            anchors.leftMargin: 6
            anchors.right: rightTxt.left
            anchors.rightMargin: rightTxt.text.length > 0 ? 6 : 0
            anchors.verticalCenter: parent.verticalCenter
            text: root.line
            color: root.ink
            font.pixelSize: 13
            font.weight: Font.Medium
            font.family: Theme.fontSans
            elide: Text.ElideRight
            MouseArea {
                id: lineMa
                anchors.fill: parent
                acceptedButtons: Qt.NoButton
                hoverEnabled: tile.stateDetail.length > 0 || lineTxt.truncated
            }
            ToolTip.visible: lineMa.containsMouse
            ToolTip.delay: 400
            ToolTip.text: tile.stateDetail.length > 0 ? tile.stateDetail : root.line
        }
        Text {
            id: rightTxt
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            text: root.rightText
            color: Theme.t4
            font.pixelSize: 13
            font.family: Theme.fontSans
            font.features: Theme.tnum
        }
    }
}
