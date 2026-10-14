// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// The inspector for a series group: the show at the top, then its episodes as
// short rows (code, the episode's own name from TMDB, size, state, play). The
// grid keeps one card per show; this is where the episodes are. Picking one
// makes it what the tabs below show and the toolbar acts on.
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import "../theme"

ColumnLayout {
    id: grp
    property var win
    property var controller
    property int groupRow: -1
    spacing: 0

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
    function stateLine(e) {
        var p = Math.round(Math.max(0, Math.min(1, e.progress || 0)) * 100)
        if (e.stateKey === "seeding") return "↑ " + i18n.t("state_seeding")
        if (e.stateKey === "downloading") return "↓ " + p + "%"
        return e.stateString || ""
    }
    onGroupRowChanged: { names = ({}); asked = ({}); reload() }
    onVisibleChanged: if (visible) reload()
    // sizes and states move while it is open
    Timer { interval: 1000; repeat: true; running: grp.visible && grp.groupRow >= 0; onTriggered: grp.reload() }
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
    RowLayout {
        Layout.fillWidth: true
        Layout.margins: Theme.sp4
        spacing: 14
        Rectangle {
            Layout.preferredWidth: 84
            Layout.preferredHeight: 118
            radius: 7
            color: Theme.track
            clip: true
            Image {
                anchors.fill: parent
                source: grp.win && grp.summary.poster ? grp.win.fileUrl(grp.summary.poster) : ""
                fillMode: Image.PreserveAspectCrop
                asynchronous: true
                sourceSize.width: 168
            }
        }
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 4
            Text {
                Layout.fillWidth: true
                text: grp.summary.title || ""
                color: Theme.t1
                font.pixelSize: 18
                font.weight: Font.DemiBold
                font.family: Theme.fontSans
                wrapMode: Text.WordWrap
                maximumLineCount: 2
                elide: Text.ElideRight
            }
            Text {
                Layout.fillWidth: true
                visible: text.length > 0
                text: [grp.summary.year > 0 ? String(grp.summary.year) : "", grp.summary.genres || ""]
                      .filter(function (x) { return x.length > 0 }).join("  ·  ")
                color: Theme.t3
                font.pixelSize: 13
                font.family: Theme.fontSans
                elide: Text.ElideRight
            }
            Item { Layout.fillHeight: true }
            Text {
                text: (i18n.language, i18n.t("search_episodes_n")).arg(grp.summary.count || 0)
                      + (grp.summary.size ? "  ·  " + grp.summary.size : "")
                color: Theme.t2
                font.pixelSize: 13
                font.family: Theme.fontSans
                font.features: Theme.tnum
            }
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 4
                radius: 2
                color: Theme.track
                Rectangle {
                    readonly property real p: Math.max(0, Math.min(1, grp.summary.progress || 0))
                    width: parent.width * p; height: parent.height; radius: 2
                    color: p >= 0.999 ? Theme.amber : Theme.accent
                }
            }
        }
    }

    Text {
        Layout.leftMargin: Theme.sp4
        Layout.bottomMargin: Theme.sp2
        text: (i18n.language, i18n.t("detail_episodes"))
        color: Theme.t4
        font.pixelSize: 11
        font.weight: Font.Bold
        font.letterSpacing: 0.8
        font.capitalization: Font.AllUppercase
        font.family: Theme.fontSans
    }

    // ---- the episodes ----
    ListView {
        id: epList
        Layout.fillWidth: true
        Layout.leftMargin: 10
        Layout.rightMargin: 10
        // a long season scrolls here instead of pushing the tabs off the panel
        Layout.preferredHeight: Math.min(contentHeight, 46 * 6)
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
            height: 46
            radius: 8
            color: focused ? Theme.sel : (rowMa.containsMouse || playMa.containsMouse ? Theme.hover : "transparent")
            Behavior on color { ColorAnimation { duration: Theme.durFast } }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 10
                spacing: 10
                Rectangle {
                    Layout.preferredWidth: 3
                    Layout.preferredHeight: 22
                    radius: 2
                    color: row.focused ? Theme.accent : "transparent"
                }
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 1
                    Text {
                        Layout.fillWidth: true
                        text: (row.modelData.label || row.modelData.name)
                              + (row.title.length > 0 ? "  ·  " + row.title : "")
                        color: Theme.t1
                        font.pixelSize: 14
                        font.weight: Font.Medium
                        font.family: Theme.fontSans
                        font.features: Theme.tnum
                        elide: Text.ElideRight
                    }
                    Text {
                        Layout.fillWidth: true
                        text: row.modelData.size + "  ·  " + grp.stateLine(row.modelData)
                        color: Theme.t3
                        font.pixelSize: 12
                        font.family: Theme.fontSans
                        font.features: Theme.tnum
                        elide: Text.ElideRight
                    }
                }
                Rectangle {
                    visible: !!row.modelData.playable
                    Layout.preferredWidth: 28
                    Layout.preferredHeight: 28
                    radius: 14
                    color: playMa.containsMouse ? Theme.accent : Theme.field
                    IconImg { anchors.centerIn: parent; anchors.horizontalCenterOffset: 1; src: "qrc:/icons/play.svg"; s: 11; tint: playMa.containsMouse ? "#ffffff" : Theme.t1 }
                }
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
                anchors.right: parent.right
                anchors.rightMargin: 8
                anchors.verticalCenter: parent.verticalCenter
                width: 32; height: 32
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: if (typeof session !== "undefined") session.playByHash(row.modelData.infoHash)
            }
        }
    }
}
