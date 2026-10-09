// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

import QtQuick
import QtQuick.Effects
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

    // Allow-list, not a deny-list: the old form listed every state that is not
    // downloading, so it never excluded "error" (an errored torrent showed a
    // download speed) and still named "finished", which torrentStateKey has
    // never produced. Any state added later would leak through too.
    readonly property bool isDownloading: stateKey === "downloading"
    // Incomplete, not just actively moving. A paused or stalled torrent is the
    // case where Windows already shows the file at its final size, so hiding
    // how much is really on disk is exactly when it misleads most.
    readonly property bool isIncomplete: progress < 1.0 && stateKey !== "fetching"
    readonly property int etaSec: (downRate > 0 && progress < 1.0 && sizeBytes > 0)
        ? Math.round(sizeBytes * (1 - progress) / downRate) : -1

    readonly property string posterUrl: win.fileUrl(posterPath)
    // A poster that fails to decode is no poster. Treating it as one left a
    // hole where the artwork should be: a missing image plugin, a truncated
    // download or a corrupt file all land here.
    readonly property bool artShown: posterUrl !== "" && posterImg.status !== Image.Error
    readonly property bool hovered: tileMa.containsMouse || ptMa.containsMouse

    // What the bar and the percentage draw. The engine reports once a second,
    // so `progress` moves in steps; this follows it linearly over that second.
    // It animates the value, not the bar width, so resizing the window stays
    // instant.
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

    Item {
        id: posterWrap
        width: 178
        height: 237
        transform: Translate {
            y: tile.hovered ? -Theme.travel(4) : 0
            Behavior on y { NumberAnimation { duration: Theme.durSlow; easing.type: Theme.easeOut } }
        }

        Rectangle {
            anchors.fill: parent
            radius: 10
            color: "#161618"
            visible: !tile.artShown
            // Last resort only: no usable extension, no resolved type, nothing
            // to say. The bat goes in the middle only when it is the only
            // thing we know; as a permanent backdrop it was just noise.
            Image {
                anchors.centerIn: parent
                width: parent.width * 0.5
                height: width
                visible: tile.fileKind.length === 0
                source: "qrc:/images/logo.svg"
                sourceSize: Qt.size(width * 2, width * 2)
                fillMode: Image.PreserveAspectFit
                opacity: 0.06
                layer.enabled: Theme.isLight
                layer.effect: MultiEffect { colorization: 1.0; colorizationColor: Theme.t1 }
            }

            // A typographic cover rather than a centred watermark. Centred and
            // symmetrical is what reads as a placeholder; a hero set high and
            // left, with air under it, reads as a decision. The extension is
            // also the most useful thing we know about a torrent with no art.
            Column {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.leftMargin: 12
                anchors.rightMargin: 12
                anchors.topMargin: Math.round(parent.height * 0.17)
                spacing: 7
                visible: tile.fileKind.length > 0

                Text {
                    id: kindText
                    text: tile.fileKind
                    color: "#f5f5f6"
                    // HorizontalFit so a long one (M2TS, WEBM) shrinks to the
                    // tile instead of being clipped or eliding to nonsense.
                    font.pixelSize: Math.round(tile.width * 0.30)
                    fontSizeMode: Text.HorizontalFit
                    minimumPixelSize: 20
                    width: parent.width
                    horizontalAlignment: Text.AlignLeft
                    font.weight: Font.Bold
                    font.letterSpacing: -1.5
                    font.family: Theme.fontSans
                }
                // Red only here, and only under the hero: colour is signal, and
                // this is the one place on a blank tile that earns it.
                Rectangle {
                    width: Math.min(kindText.contentWidth, parent.width)
                    height: 3
                    radius: 1.5
                    color: Theme.accent
                }
            }
            Text {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.leftMargin: 13
                anchors.rightMargin: 13
                // Same as the artwork title: the bar's band is 8 + 9 whether
                // or not there is a poster behind it.
                anchors.bottomMargin: 25
                text: tile.metaTitle || tile.torrentName
                color: "#f5f5f6"
                font.pixelSize: 18
                font.weight: Font.Bold
                font.letterSpacing: -0.3
                font.family: Theme.fontSans
                wrapMode: Text.WordWrap
                maximumLineCount: 3
                elide: Text.ElideRight
            }
        }

        Rectangle {
            id: posterBg
            anchors.fill: parent
            color: "#161618"
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
            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: parent.height * 0.6
                gradient: Gradient {
                    GradientStop { position: 0.0; color: "transparent" }
                    GradientStop { position: 0.55; color: Qt.rgba(0, 0, 0, 0.45) }
                    GradientStop { position: 1.0; color: Qt.rgba(0, 0, 0, 0.92) }
                }
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
        Text {
            visible: tile.artShown
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.leftMargin: 12
            anchors.rightMargin: 12
            // Clears the progress bar's band (8 + 9 high) instead of sitting in it.
            anchors.bottomMargin: 25
            text: tile.metaTitle || tile.torrentName
            color: "#f5f5f6"
            font.pixelSize: 15
            font.weight: Font.Bold
            font.letterSpacing: -0.2
            font.family: Theme.fontSans
            elide: Text.ElideRight
            maximumLineCount: 2
            wrapMode: Text.WordWrap
        }

        // No pill behind it: the poster already carries a scrim that reaches
        // 92% black at this edge, so a second dark plate was black on black
        // and only added weight.
        // Sized for the widest value it can ever hold, so every bar in the
        // grid ends at the same x: letting the text size itself made a tile
        // at 100% stop 20px short of one at 0%.
        TextMetrics {
            id: pctMetrics
            font: pctTxt.font
            text: "100%"
        }
        Text {
            id: pctTxt
            visible: !Theme.isTroubleState(tile.stateKey) && tile.stateKey !== "fetching"
            anchors.right: parent.right
            anchors.rightMargin: 12
            // Centred on the bar, not sharing its bottom edge: a text item's
            // box reserves descender space, so matching bottoms leaves the
            // glyphs sitting high.
            anchors.verticalCenter: progTrack.verticalCenter
            width: pctMetrics.width
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
            anchors.bottomMargin: 8
            // 9, not 14: the extra height only ever existed to seat the
            // percentage, and that moved out beside it.
            height: 9
            progress: tile.shownProgress
            stateKey: tile.stateKey
            sheen: (tile.stateKey === "seeding" && tile.upRate > 0)
                   || (tile.isDownloading && tile.downRate > 0)
        }

        Rectangle {
            // Metadata and a video in it is the whole condition. Playing preps
            // sequential download and prioritises the head of the file, so
            // there is nothing to wait for: the old `progress > 0.02` gate
            // meant 1 GB on a 50 GB torrent and 14 MB on a 700 MB one, and
            // hid the control exactly while someone wanted to start watching.
            visible: tile.playable
                     && (tileMa.containsMouse || ptMa.containsMouse || controller.isRowSelected(tile.index))
            x: (parent.width - width) / 2
            y: (parent.height - height) / 2
            width: 46
            height: 46
            radius: 23
            z: 5
            color: "#cc101014"
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
                onClicked: if (typeof session !== "undefined") session.playByHash(tile.infoHash)
            }
        }

        Rectangle {
            anchors.fill: parent
            radius: 10
            color: "transparent"
            border.color: controller.isRowSelected(tile.index) ? Theme.accent
                          : (tileMa.containsMouse ? Qt.rgba(1, 1, 1, 0.2) : Theme.hair)
            border.width: controller.isRowSelected(tile.index) ? 2 : 1
            Behavior on border.color { ColorAnimation { duration: Theme.durFast; easing.type: Theme.easeOut } }
        }

        // Finished while on screen: the bar fills, then a ring in the done
        // colour grows off the poster edge and fades. Only on the transition,
        // never for a tile that loads already complete.
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

        PosterTileBadges { tile: tile }

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
                    controller.selectRow(tile.index, mouse.modifiers)
                }
            }
            onDoubleClicked: function(mouse) {
                if (mouse.button !== Qt.RightButton) {
                    controller.selectRow(tile.index, 0)
                    session.openSelectedFile()
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
