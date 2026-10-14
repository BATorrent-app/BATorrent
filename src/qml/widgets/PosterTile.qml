// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// A Downloads grid card. The art is only art: nothing sits on it but what is
// in progress or wrong (a 9px bar with its percentage, hazard stripes, the
// metadata sweep) and, on a series, the episode count. The title goes under
// the poster, and the state lives in one line below it: a dot, what is going
// on, and why (PosterTileMeta). A finished item has no bar at all.
import QtQuick
import QtQuick.Effects
import QtQuick.Controls.Basic
import "../theme"

Item {
    property var win
    property var controller
    id: tile
    width: 178
    height: 286

    required property int index
    required property string torrentName
    required property string metaTitle
    required property string stateKey
    required property real progress
    required property string posterPath
    required property string stateString
    required property string stateDetail
    required property string stateDetailShort
    required property string fileKind
    required property string category
    required property string size
    required property string downSpeed
    required property string upSpeed
    required property real downRate
    required property real upRate
    required property var sizeBytes
    required property string infoHash
    required property bool playable
    required property string downloaded
    required property int year
    required property string genres
    required property int queuePos
    // A series group (QmlLibraryModel): one card for every episode of a show;
    // the inspector lists the episodes.
    required property bool isGroup
    required property int groupCount
    required property bool groupExpanded
    required property bool groupChild
    readonly property string groupCaption: {
        if (!tile.isGroup || !controller || !controller.rowModel) return ""
        var s = (tile.groupCount, controller.rowModel.groupSummary(tile.index)).seasons || ""
        var n = (i18n.language, i18n.t("search_episodes_n")).arg(tile.groupCount)
        return s.length > 0 ? s + " · " + n : n
    }
    // A group plays the episode last watched, else its first one to watch.
    function groupPlayHash() {
        var eps = controller.rowModel.groupEpisodes(tile.index)
        var resume = typeof session !== "undefined" ? session.resumeItems : []
        for (var r = 0; r < resume.length; r++)
            for (var e = 0; e < eps.length; e++)
                if (eps[e].infoHash === resume[r].infoHash) return eps[e].infoHash
        for (var i = 0; i < eps.length; i++)
            if (eps[i].playable) return eps[i].infoHash
        return tile.infoHash
    }

    readonly property bool isDownloading: stateKey === "downloading"
    readonly property bool isIncomplete: progress < 1.0 && stateKey !== "fetching"
    readonly property int etaSec: (downRate > 0 && progress < 1.0 && sizeBytes > 0)
        ? Math.round(sizeBytes * (1 - progress) / downRate) : -1
    readonly property bool selected: controller.isRowSelected(tile.index)
    // Over the art only what is unfinished or wrong. A finished item, seeding
    // or not, has nothing to report there.
    readonly property bool trouble: Theme.isTroubleState(stateKey)
    readonly property bool fetching: stateKey === "fetching"
    readonly property bool showBar: !tile.isGroup
        && (trouble || fetching || (progress < 0.999 && stateKey !== "completed" && stateKey !== "seeding"))

    readonly property string posterUrl: win.fileUrl(posterPath)
    readonly property bool artShown: posterUrl !== "" && posterImg.status !== Image.Error
    readonly property bool hovered: tileMa.containsMouse || ptMa.containsMouse

    // What the bar and the percentage draw. The engine reports once a second,
    // so `progress` moves in steps; this follows it linearly over that second.
    property real shownProgress: 0
    property string seenHash: ""
    property bool wasDone: false
    Component.onCompleted: {
        seenHash = infoHash
        shownProgress = progress
        wasDone = progress >= 0.999
        if (GridView.view && GridView.view.introOn) dealIn.restart()
    }

    // first-show cascade, started by the grid (see LibraryView introOn)
    transform: Translate { id: dealShift }
    Connections {
        target: tile.GridView.view
        ignoreUnknownSignals: true
        function onIntroOnChanged() { if (tile.GridView.view.introOn) dealIn.restart() }
    }
    SequentialAnimation {
        id: dealIn
        PropertyAction { target: tile; property: "opacity"; value: 0 }
        PropertyAction { target: dealShift; property: "y"; value: 22 }
        PauseAnimation { duration: 60 + Math.min(tile.index, 16) * 34 }
        ParallelAnimation {
            NumberAnimation { target: tile; property: "opacity"; to: 1; duration: Theme.durSlower; easing.type: Theme.easeOut }
            NumberAnimation { target: dealShift; property: "y"; to: 0; duration: 520; easing.type: Easing.OutExpo }
        }
    }
    // callLater: on a re-sort the tile gets another torrent and the roles
    // change one at a time. Waiting until they all have avoids animating to
    // the other torrent's progress or playing its finish ring here.
    onProgressChanged: Qt.callLater(syncProgress)
    onInfoHashChanged: Qt.callLater(syncProgress)
    function syncProgress() {
        var done = progress >= 0.999
        if (infoHash !== seenHash) {
            progGlide.stop()
            seenHash = infoHash
            shownProgress = progress
            wasDone = done
            return
        }
        if (done && !wasDone) finishPulse.restart()
        wasDone = done
        // backwards (a recheck) or a big leap is a correction, not motion
        if (Theme.reduceMotion || progress < shownProgress || progress - shownProgress > 0.2) {
            progGlide.stop()
            shownProgress = progress
            return
        }
        progGlide.duration = done ? 320 : 1000
        progGlide.easing.type = done ? Theme.easeOut : Easing.Linear
        progGlide.to = progress
        progGlide.restart()
    }
    NumberAnimation { id: progGlide; target: tile; property: "shownProgress" }

    // A series reads as a stack: two covers peek out above its own.
    Rectangle {
        visible: tile.isGroup
        x: 16; y: -10
        width: 178 - 32; height: 30
        radius: 10
        color: Theme.stackBack
        border.color: Theme.hair; border.width: 1
        transform: Translate { y: posterShift.y }
    }
    Rectangle {
        visible: tile.isGroup
        x: 8; y: -5
        width: 178 - 16; height: 30
        radius: 10
        color: Theme.stackMid
        border.color: Theme.hair; border.width: 1
        transform: Translate { y: posterShift.y }
    }

    Item {
        id: posterWrap
        width: 178
        height: 237
        transform: Translate {
            id: posterShift
            y: tile.hovered ? -Theme.travel(3) : 0
            Behavior on y { NumberAnimation { duration: Theme.durSlow; easing.type: Theme.easeOut } }
        }

        // no art: what kind of file it is, set large at the top left
        Rectangle {
            anchors.fill: parent
            radius: 10
            color: Theme.tileBlank
            visible: !tile.artShown
            Image {
                // a magnet without metadata yet: only the bat, faint
                x: 45; y: 70
                width: 88; height: 88
                visible: tile.fileKind.length === 0
                source: "qrc:/images/logo.svg"
                sourceSize: Qt.size(176, 176)
                fillMode: Image.PreserveAspectFit
                opacity: 0.07
                layer.enabled: Theme.isLight
                layer.effect: MultiEffect { colorization: 1.0; colorizationColor: Theme.t1 }
            }
            Text {
                visible: tile.fileKind.length > 0
                x: 12; y: 40
                width: parent.width - 24
                text: tile.fileKind
                color: Theme.tileInk
                font.pixelSize: 53
                fontSizeMode: Text.HorizontalFit
                minimumPixelSize: 20
                font.weight: Font.Bold
                font.letterSpacing: -1.5
                font.family: Theme.fontSans
            }
        }

        Item {
            id: posterBg
            anchors.fill: parent
            visible: false
            layer.enabled: true
            Image {
                id: posterImg
                anchors.fill: parent
                source: tile.posterUrl
                fillMode: Image.PreserveAspectCrop
                asynchronous: true
                sourceSize: Qt.size(356, 474)
                cache: true
            }
        }
        Rectangle {
            id: posterMask
            anchors.fill: parent
            radius: 10
            color: "white"
            visible: false
            layer.enabled: true
        }
        MultiEffect {
            source: posterBg
            anchors.fill: parent
            maskEnabled: true
            maskSource: posterMask
            visible: tile.artShown
        }

        // the bar, only while unfinished or in trouble, over a short scrim
        Item {
            id: barBand
            visible: tile.showBar
            anchors.fill: parent
            Rectangle {
                anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom
                height: 56
                radius: 10
                gradient: Gradient {
                    GradientStop { position: 0.0; color: "transparent" }
                    GradientStop { position: 1.0; color: Qt.rgba(0, 0, 0, 0.85) }
                }
            }
            Text {
                id: pctTxt
                visible: !tile.trouble && !tile.fetching
                anchors.right: parent.right
                anchors.rightMargin: 12
                anchors.verticalCenter: progTrack.verticalCenter
                width: 30
                horizontalAlignment: Text.AlignRight
                text: Math.floor(Math.max(0, Math.min(1, tile.shownProgress)) * 100) + "%"
                color: "#ffffff"
                font.pixelSize: 11
                font.weight: Font.Bold
                font.family: Theme.fontSans
                font.features: Theme.tnum
            }
            ProgressTrack {
                id: progTrack
                anchors.left: parent.left
                anchors.right: pctTxt.visible ? pctTxt.left : parent.right
                anchors.bottom: parent.bottom
                anchors.leftMargin: 12
                anchors.rightMargin: pctTxt.visible ? 8 : 12
                anchors.bottomMargin: tile.trouble ? 13 : 10
                height: 9
                onArt: true
                progress: tile.shownProgress
                stateKey: tile.stateKey
                fill: tile.isDownloading ? Theme.accent : Theme.fillFor(tile.stateKey)
                sheen: tile.isDownloading && tile.downRate > 0
                MouseArea {
                    id: barMa
                    anchors.fill: parent
                    anchors.margins: -6
                    acceptedButtons: Qt.NoButton
                    hoverEnabled: tile.isIncomplete
                }
                ToolTip.visible: barMa.containsMouse
                ToolTip.delay: 250
                ToolTip.text: tile.downloaded + " " + (i18n.language, i18n.t("word_of")) + " " + tile.size
            }
        }

        // play: on the selected card, or under the pointer
        Rectangle {
            visible: tile.playable && (tile.selected || tileMa.containsMouse || ptMa.containsMouse)
            x: (parent.width - width) / 2
            y: 95
            width: 46
            height: 46
            radius: 23
            z: 5
            color: Qt.rgba(16 / 255, 16 / 255, 20 / 255, 0.8)
            border.color: ptMa.containsMouse ? Theme.accent : Qt.rgba(1, 1, 1, 0.25)
            border.width: 1
            scale: ptMa.containsMouse ? 1.08 : 1.0
            Behavior on border.color { ColorAnimation { duration: Theme.durFast } }
            Behavior on scale { NumberAnimation { duration: Theme.durFast; easing.type: Theme.easeOut } }
            IconImg {
                anchors.centerIn: parent
                anchors.horizontalCenterOffset: 1
                src: "qrc:/icons/play.svg"
                tint: ptMa.containsMouse ? Theme.accent : "#ffffff"
                s: 18
            }
            MouseArea {
                id: ptMa
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: if (typeof session !== "undefined") session.playByHash(tile.isGroup ? tile.groupPlayHash() : tile.infoHash)
            }
        }

        // a series: how many episodes, top left, nothing else
        Rectangle {
            visible: tile.isGroup
            x: 8; y: 8
            z: 6
            radius: 10
            height: 20
            width: countTxt.implicitWidth + 16
            color: Qt.rgba(0, 0, 0, 0.8)
            Text {
                id: countTxt
                anchors.centerIn: parent
                text: tile.groupCount
                color: "#ffffff"
                font.pixelSize: 11
                font.weight: Font.Bold
                font.family: Theme.fontSans
                font.features: Theme.tnum
            }
        }

        // Selection is neutral: a white ring outside the edge, never the accent
        // (red already means downloading and trouble). Otherwise a hairline.
        Rectangle {
            anchors.fill: parent
            anchors.margins: tile.selected ? -2 : 0
            radius: tile.selected ? 12 : 10
            color: "transparent"
            border.color: tile.selected ? Theme.t1
                          : (tileMa.containsMouse ? Qt.rgba(1, 1, 1, 0.18) : Theme.hair)
            border.width: tile.selected ? 2 : 1
            Behavior on border.color { ColorAnimation { duration: Theme.durFast; easing.type: Theme.easeOut } }
        }

        // Finished while on screen: a ring in the done colour grows off the
        // poster edge and fades. Only on the transition, never for a card
        // that loads already complete.
        Rectangle {
            id: finishHalo
            anchors.fill: parent
            anchors.margins: -3
            radius: 13
            color: "transparent"
            border.color: Qt.rgba(Theme.grn.r, Theme.grn.g, Theme.grn.b, 0.35)
            border.width: 8
            opacity: 0
            antialiasing: true
        }
        Rectangle {
            id: finishRing
            anchors.fill: parent
            radius: 10
            color: "transparent"
            border.color: Theme.grn
            border.width: 2
            opacity: 0
            antialiasing: true
        }
        SequentialAnimation {
            id: finishPulse
            PauseAnimation { duration: Theme.durSlower }
            ParallelAnimation {
                NumberAnimation { target: finishHalo; property: "opacity"; from: 0.9; to: 0
                    duration: Theme.durShow + 400; easing.type: Easing.OutQuad }
                NumberAnimation { target: finishHalo; property: "scale"; from: 1; to: Theme.grow(1.12)
                    duration: Theme.durShow + 400; easing.type: Theme.easeOut }
                NumberAnimation { target: finishRing; property: "opacity"; from: 0.95; to: 0
                    duration: Theme.durShow + 200; easing.type: Easing.OutQuad }
                NumberAnimation { target: finishRing; property: "scale"; from: 1; to: Theme.grow(1.07)
                    duration: Theme.durShow + 200; easing.type: Theme.easeOut }
                NumberAnimation { target: finishRing; property: "border.width"; from: 4; to: 1
                    duration: Theme.durShow + 200; easing.type: Theme.easeOut }
            }
        }

        MouseArea {
            id: tileMa
            anchors.fill: parent
            hoverEnabled: true
            acceptedButtons: Qt.LeftButton | Qt.RightButton
            cursorShape: Qt.PointingHandCursor
            onClicked: function(mouse) {
                if (mouse.button === Qt.RightButton) {
                    if (!controller.isRowSelected(tile.index)) controller.selectRow(tile.index, 0)
                    win.openContext(tile.index)
                } else {
                    // A plain click on the card that is already the only one
                    // selected opens its page; the first click just selects it.
                    const again = mouse.modifiers === Qt.NoModifier && controller.isRowSelected(tile.index)
                                  && controller.selectedRows.length <= 1
                    controller.selectRow(tile.index, mouse.modifiers)
                    if (again && win.openTorrentPage(tile.infoHash)) tileMa.pageOpenedMs = Date.now()
                }
            }
            // A film, show or game opens on its own page; with no page to
            // open, the file is revealed. A double-click on the selected card:
            // its first click already opened the page.
            property real pageOpenedMs: 0
            onDoubleClicked: function(mouse) {
                if (mouse.button !== Qt.RightButton && Date.now() - pageOpenedMs >= 800) {
                    controller.selectRow(tile.index, 0)
                    if (!win.openTorrentPage(tile.infoHash)) session.openSelectedFile()
                }
            }
        }
    }

    PosterTileMeta {
        id: meta
        tile: tile
        win: tile.win
        anchors.top: posterWrap.bottom
        anchors.topMargin: 10
        anchors.left: posterWrap.left
        anchors.right: posterWrap.right
    }
}
