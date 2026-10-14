// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// The inspector for a series group: the show in a compact header (cover,
// title, the season and a segment per episode), then its episodes as short
// rows: code, the episode's own name from TMDB, size and where you are in it
// (watched, stopped at 14:32, uploading), and play. The grid keeps one card
// per show; this is where the episodes are. Picking one makes it what the
// tabs below show and the toolbar acts on.
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import QtQuick.Effects
import "../theme"

ColumnLayout {
    id: grp
    property var win
    property var controller
    property int groupRow: -1
    spacing: 0
    Layout.fillHeight: false

    property var summary: ({})
    property var episodes: []
    property var names: ({})      // "tmdb:season:episode" → the episode's name
    property var asked: ({})      // "tmdb:season" already requested

    function reload() {
        if (groupRow < 0 || !controller || !controller.rowModel) return
        summary = controller.rowModel.groupSummary(groupRow)
        episodes = controller.rowModel.groupEpisodes(groupRow)
        askNames()
    }
    function askNames() {
        if (typeof discovery === "undefined") return
        for (var i = 0; i < episodes.length; i++) {
            var e = episodes[i]
            var key = e.showTmdb + ":" + e.season
            if (e.showTmdb > 0 && e.season > 0 && !asked[key]) {
                asked[key] = true
                discovery.fetchEpisodes(e.showTmdb, e.season)
            }
        }
    }
    function epTitle(e) {
        if (e.episode <= 0 && e.season > 0) return i18n.t("season_complete").arg(e.season)
        return names[e.showTmdb + ":" + e.season + ":" + e.episode] || ""
    }
    function clock(ms) {
        var t = Math.floor(ms / 1000), h = Math.floor(t / 3600), m = Math.floor((t % 3600) / 60), s = t % 60
        var mm = (h > 0 && m < 10 ? "0" : "") + m, ss = (s < 10 ? "0" : "") + s
        return (h > 0 ? h + ":" : "") + mm + ":" + ss
    }
    // size · state, and for a single video how far you watched it
    function subLine(e) {
        var parts = [e.size]
        var k = e.stateKey
        if (k === "downloading") parts.push(i18n.t("state_downloading") + " " + Math.round((e.progress || 0) * 100) + "%")
        else if (k === "paused") parts.push(i18n.t("state_paused"))
        else if (k === "missing" || k === "error") parts.push(e.stateString || "")
        var w = typeof session !== "undefined" ? session.watchState(e.infoHash) : ({})
        if (w.videos === 1 && w.watched) parts.push(i18n.t("ep_watched"))
        else if (w.videos === 1 && w.resumeMs > 0) parts.push(i18n.t("ep_stopped_at").arg(clock(w.resumeMs)))
        else if (k === "seeding" || k === "completed")
            parts.push(i18n.t("state_completed") + (k === "seeding" && e.upRate > 0 ? " · " + i18n.t("ep_uploading") : ""))
        return parts.filter(function (x) { return x && x.length > 0 }).join(" · ")
    }
    onGroupRowChanged: { names = ({}); asked = ({}); reload() }
    onVisibleChanged: if (visible) reload()
    // sizes and states move while it is open
    Timer { interval: 1500; repeat: true; running: grp.visible && grp.groupRow >= 0; onTriggered: grp.reload() }
    Connections {
        target: typeof discovery !== "undefined" ? discovery : null
        ignoreUnknownSignals: true
        function onEpisodesReady(id, season, eps) {
            var n = Object.assign({}, grp.names)
            for (var i = 0; i < eps.length; i++) n[id + ":" + season + ":" + eps[i].episode] = eps[i].name
            grp.names = n
        }
        function onLanguageReloaded() { grp.names = ({}); grp.asked = ({}); grp.askNames() }
    }

    // ---- the show ----
    Item {
        Layout.fillWidth: true
        Layout.preferredHeight: 104
        Item {
            id: cover
            x: 16; y: 12
            width: 56; height: 80
            Item {
                id: coverArt
                anchors.fill: parent
                visible: false
                layer.enabled: true
                Rectangle { anchors.fill: parent; color: Theme.tileBlank }
                Image {
                    anchors.fill: parent
                    source: grp.win && grp.summary.poster ? grp.win.fileUrl(grp.summary.poster) : ""
                    fillMode: Image.PreserveAspectCrop
                    asynchronous: true
                    sourceSize: Qt.size(112, 160)
                }
            }
            Rectangle { id: coverMask; anchors.fill: parent; radius: 6; visible: false; layer.enabled: true }
            MultiEffect { source: coverArt; anchors.fill: parent; maskEnabled: true; maskSource: coverMask }
            Rectangle { anchors.fill: parent; radius: 6; color: "transparent"; border.color: Theme.hair; border.width: 1 }
        }
        Item {
            anchors.left: cover.right
            anchors.leftMargin: 14
            anchors.right: parent.right
            anchors.rightMargin: 16
            y: 12
            height: 80
            Column {
                width: parent.width
                spacing: 3
                Text {
                    width: parent.width
                    text: grp.summary.title || ""
                    color: Theme.t1
                    font.pixelSize: 17
                    font.weight: Font.DemiBold
                    font.family: Theme.fontSans
                    elide: Text.ElideRight
                }
                Text {
                    width: parent.width
                    text: [grp.summary.year > 0 ? String(grp.summary.year) : "", grp.summary.genres || "",
                           grp.win ? grp.win.catLabel("Series") : ""]
                          .filter(function (x) { return x.length > 0 }).join(" · ")
                    color: Theme.t3
                    font.pixelSize: 12
                    font.family: Theme.fontSans
                    elide: Text.ElideRight
                }
            }
            Column {
                anchors.bottom: parent.bottom
                width: parent.width
                spacing: 6
                Item {
                    width: parent.width
                    height: 16
                    Text {
                        anchors.left: parent.left
                        text: [grp.summary.seasons || "", (i18n.language, i18n.t("group_items_n")).arg(grp.summary.count || 0)]
                              .filter(function (x) { return x.length > 0 }).join(" · ")
                        color: Theme.t1
                        font.pixelSize: 12
                        font.weight: Font.DemiBold
                        font.family: Theme.fontSans
                        font.features: Theme.tnum
                    }
                    Text {
                        anchors.right: parent.right
                        text: grp.summary.size || ""
                        color: Theme.t4
                        font.pixelSize: 12
                        font.family: Theme.fontSans
                        font.features: Theme.tnum
                    }
                }
                // one segment per episode, coloured by how far each got
                Row {
                    width: parent.width
                    height: 4
                    spacing: 3
                    Repeater {
                        model: grp.episodes
                        Rectangle {
                            required property var modelData
                            readonly property real p: Math.max(0, Math.min(1, modelData.progress || 0))
                            width: (parent.width - parent.spacing * (grp.episodes.length - 1)) / Math.max(1, grp.episodes.length)
                            height: 4
                            radius: 2
                            color: Theme.track
                            Rectangle {
                                width: parent.width * parent.p
                                height: 4
                                radius: 2
                                color: parent.p >= 0.999 ? Theme.doneFill : Theme.accent
                            }
                        }
                    }
                }
            }
        }
    }

    Text {
        Layout.leftMargin: 16
        Layout.bottomMargin: 6
        text: (i18n.language, i18n.t("detail_episodes"))
        color: Theme.t4
        font.pixelSize: 10
        font.weight: Font.DemiBold
        font.letterSpacing: 1.0
        font.capitalization: Font.AllUppercase
        font.family: Theme.fontSans
    }

    // ---- the episodes ----
    ListView {
        id: epList
        Layout.fillWidth: true
        Layout.leftMargin: 8
        Layout.rightMargin: 8
        Layout.bottomMargin: 10
        // a long season scrolls here instead of pushing the tabs off the panel
        Layout.preferredHeight: Math.min(contentHeight, 42 * 6)
        clip: true
        interactive: contentHeight > height
        boundsBehavior: Flickable.StopAtBounds
        model: grp.episodes
        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
        delegate: Rectangle {
            id: row
            required property var modelData
            readonly property bool focused: !!grp.controller && grp.controller.focusedEpisodeSource === modelData.sourceRow
            readonly property string title: grp.epTitle(modelData)
            width: epList.width
            height: 42
            radius: 8
            color: focused ? Theme.hover2 : (rowMa.containsMouse || playMa.containsMouse ? Theme.hover : "transparent")
            Behavior on color { ColorAnimation { duration: Theme.durFast } }

            Rectangle {
                x: 8
                anchors.verticalCenter: parent.verticalCenter
                width: 2; height: 22; radius: 1
                color: row.focused ? Theme.t1 : "transparent"
            }
            Column {
                anchors.left: parent.left
                anchors.leftMargin: 20
                anchors.right: playBtn.left
                anchors.rightMargin: 10
                anchors.verticalCenter: parent.verticalCenter
                spacing: 2
                Text {
                    width: parent.width
                    textFormat: Text.StyledText
                    text: (row.modelData.label || row.modelData.name)
                          + (row.title.length > 0 ? "<font color='" + Theme.t3 + "'> · " + row.title.replace(/&/g, "&amp;").replace(/</g, "&lt;") + "</font>" : "")
                    color: Theme.t1
                    font.pixelSize: 13
                    font.weight: Font.Medium
                    font.family: Theme.fontSans
                    font.features: Theme.tnum
                    elide: Text.ElideRight
                }
                Text {
                    width: parent.width
                    text: (i18n.language, grp.subLine(row.modelData))
                    color: Theme.t4
                    font.pixelSize: 12
                    font.family: Theme.fontSans
                    font.features: Theme.tnum
                    elide: Text.ElideRight
                }
            }
            Rectangle {
                id: playBtn
                visible: !!row.modelData.playable
                anchors.right: parent.right
                anchors.rightMargin: 8
                anchors.verticalCenter: parent.verticalCenter
                width: 28; height: 28; radius: 14
                color: playMa.containsMouse ? Theme.accent : Theme.field
                border.color: playMa.containsMouse ? Theme.accent : Theme.hair
                border.width: 1
                IconImg { anchors.centerIn: parent; anchors.horizontalCenterOffset: 1; src: "qrc:/icons/play.svg"; s: 10; tint: playMa.containsMouse ? "#ffffff" : Theme.t1 }
            }
            MouseArea {
                id: rowMa
                anchors.fill: parent
                hoverEnabled: true
                acceptedButtons: Qt.LeftButton | Qt.RightButton
                cursorShape: Qt.PointingHandCursor
                onClicked: function (mouse) {
                    grp.controller.focusEpisode(grp.groupRow, row.modelData.sourceRow)
                    // the library's own menu, acting on this episode alone
                    if (mouse.button === Qt.RightButton && grp.win) grp.win.openContext(grp.groupRow)
                }
                onDoubleClicked: function (mouse) {
                    if (mouse.button === Qt.LeftButton && row.modelData.playable && typeof session !== "undefined")
                        session.playByHash(row.modelData.infoHash)
                }
            }
            MouseArea {
                id: playMa
                visible: !!row.modelData.playable
                anchors.fill: playBtn
                anchors.margins: -2
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: if (typeof session !== "undefined") session.playByHash(row.modelData.infoHash)
            }
        }
    }
}
