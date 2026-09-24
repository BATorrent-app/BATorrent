// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Featured billboard of the browse surface: ambient blurred backdrop, the
// crisp vertical poster anchoring on the LEFT, title/meta/overview beside it,
// Get & Watch / Trailer / Details CTAs, and rotation dots bottom-right.
// Rotates every 7s with a crossfade; trailer key and source summary are
// fetched lazily per featured item.
import QtQuick
import QtQuick.Effects
import QtQuick.Layouts
import "../theme"
import "../widgets"

Item {
    id: bb
    property var model: []          // featured items (already type-filtered)
    property bool active: true      // gates the rotation timer
    signal detailsRequested(string title)

    property int heroIndex: 0
    property int shownIndex: 0
    readonly property var heroItem: (model.length > shownIndex) ? model[shownIndex] : null
    onModelChanged: { heroIndex = 0; shownIndex = 0 }

    visible: heroItem !== null

    function fmtGB(bytes) {
        if (!bytes || bytes <= 0) return ""
        var gb = bytes / (1024 * 1024 * 1024)
        return (gb >= 10 ? Math.round(gb) : gb.toFixed(1)) + " GB"
    }
    function fmtCount(n) {
        return n >= 1000 ? (n / 1000).toFixed(1).replace(/\.0$/, "") + "k" : String(n)
    }

    // lazily-fetched trailer key + source summary for the current hero
    property string trailerKey: ""
    property var summary: null      // {count, bestSize, maxSeeds} for heroItem.title
    onHeroItemChanged: {
        trailerKey = ""
        summary = null
        if (typeof discovery !== "undefined" && heroItem
            && (heroItem.type === "movie" || heroItem.type === "series") && (heroItem.tmdbId || 0) > 0)
            discovery.fetchTrailer(heroItem.tmdbId, heroItem.type)
        if (typeof search !== "undefined" && heroItem && (heroItem.title || "").length > 0)
            search.summarizeSources(heroItem.title)
    }
    Connections {
        target: typeof discovery !== "undefined" ? discovery : null
        ignoreUnknownSignals: true
        function onTrailerReady(tmdbId, youtubeKey) {
            if (bb.heroItem && (bb.heroItem.tmdbId || 0) === tmdbId) bb.trailerKey = youtubeKey
        }
    }
    Connections {
        target: typeof search !== "undefined" ? search : null
        ignoreUnknownSignals: true
        function onSourceSummary(title, count, bestSize, maxSeeds) {
            if (bb.heroItem && bb.heroItem.title === title)
                bb.summary = { count: count, bestSize: bestSize, maxSeeds: maxSeeds }
        }
    }

    // Cross-fade: the old text leaves first, then a still of the backdrop and
    // poster is put on top, the content underneath switches to the next item
    // and the still fades out while the new text comes in. Only the still is
    // a copy; the live hero is never duplicated, and two titles are never on
    // screen together.
    onHeroIndexChanged: {
        heroTimer.restart()
        if (heroIndex !== shownIndex) heroSwap.restart()
    }
    SequentialAnimation {
        id: heroSwap
        ParallelAnimation {
            NumberAnimation { target: heroTextCol; property: "opacity"; to: 0; duration: 140; easing.type: Theme.easeIn }
            NumberAnimation { target: heroTextShift; property: "x"; to: -Theme.travel(10); duration: 140; easing.type: Theme.easeIn }
        }
        ScriptAction { script: { heroSnap.scheduleUpdate(); heroSnap.opacity = 1 } }
        // one frame so the still is rendered before the content changes
        PauseAnimation { duration: 34 }
        ScriptAction { script: { bb.shownIndex = bb.heroIndex; kenBurns.restart() } }
        ParallelAnimation {
            NumberAnimation { target: heroSnap; property: "opacity"; to: 0; duration: 480; easing.type: Easing.InOutQuad }
            NumberAnimation { target: heroPoster; property: "scale"; from: Theme.grow(1.04); to: 1; duration: 640; easing.type: Theme.easeOut }
            SequentialAnimation {
                PauseAnimation { duration: 120 }
                ParallelAnimation {
                    NumberAnimation { target: heroTextCol; property: "opacity"; to: 1; duration: 320; easing.type: Theme.easeOut }
                    NumberAnimation { target: heroTextShift; property: "x"; from: Theme.travel(22); to: 0; duration: 560; easing.type: Theme.easeOut }
                }
            }
        }
    }
    Timer {
        id: heroTimer
        interval: 7000
        running: bb.active && bb.model.length > 1
        repeat: true
        onTriggered: if (bb.model.length > 0) bb.heroIndex = (bb.heroIndex + 1) % bb.model.length
    }

    Rectangle {
        id: heroFrame
        anchors.fill: parent
        radius: 16
        color: Theme.elev
        border.color: Theme.hair; border.width: 1
        clip: true

        ShaderEffectSource {
            id: heroSnap
            anchors.fill: parent
            sourceItem: heroContent
            live: false
            opacity: 0
            visible: opacity > 0
            z: 4
        }

        Item {
            id: heroContent
            anchors.fill: parent

            // ambient background: backdrop, or the poster when the backdrop is bad/missing (games)
            Image {
                id: heroBg
                anchors.fill: parent
                source: bb.heroItem
                    ? ((bb.heroItem.backdrop && bb.heroItem.backdrop.length > 0)
                       ? bb.heroItem.backdrop : (bb.heroItem.poster || ""))
                    : ""
                fillMode: Image.PreserveAspectCrop
                asynchronous: true; cache: true
                // decode small: it's blurred anyway, and blurring a full-res
                // backdrop every frame is what made scroll crawl on Windows/D3D
                sourceSize: Qt.size(640, 360)
                visible: false
            }
            MultiEffect {
                id: heroBlur
                anchors.fill: heroBg
                source: heroBg
                // slow push-in over the time each item is up; scaling the
                // finished effect doesn't re-run the blur
                NumberAnimation on scale {
                    id: kenBurns
                    running: bb.active && !Theme.reduceMotion
                    from: 1.0; to: 1.08
                    duration: heroTimer.interval
                    easing.type: Easing.Linear
                }
                blurEnabled: true
                blur: 1.0
                blurMax: 32
                brightness: -0.25
                saturation: -0.1
            }
            // horizontal scrim: heavy on the left for text legibility
            Rectangle {
                anchors.fill: parent
                gradient: Gradient {
                    orientation: Gradient.Horizontal
                    GradientStop { position: 0.0;  color: "#f00b0b0d" }
                    GradientStop { position: 0.55; color: "#cc0b0b0d" }
                    GradientStop { position: 1.0;  color: "#700b0b0d" }
                }
            }

            // left: crisp poster (rounded via mask); the vertical anchor
            Item {
                id: heroPoster
                anchors.left: parent.left; anchors.leftMargin: Theme.sp5
                anchors.verticalCenter: parent.verticalCenter
                height: Math.min(parent.height - Theme.sp5 * 2, 268)
                width: height * 0.67
                visible: bb.heroItem && (bb.heroItem.poster || "").length > 0
                Rectangle {
                    id: hpImg
                    anchors.fill: parent; color: "#161618"; visible: false; layer.enabled: true
                    Image {
                        anchors.fill: parent
                        source: bb.heroItem ? (bb.heroItem.poster || "") : ""
                        fillMode: Image.PreserveAspectCrop
                        asynchronous: true; cache: true
                        sourceSize: Qt.size(360, 540)
                    }
                }
                Rectangle { id: hpMask; anchors.fill: parent; radius: 12; color: "white"; visible: false; layer.enabled: true }
                MultiEffect {
                    source: hpImg; anchors.fill: parent
                    maskEnabled: true; maskSource: hpMask
                }
            }

            // beside the poster: text + CTAs
            ColumnLayout {
                id: heroTextCol
                transform: Translate { id: heroTextShift }
                anchors.left: heroPoster.visible ? heroPoster.right : parent.left
                anchors.top: parent.top; anchors.bottom: parent.bottom
                anchors.right: parent.right
                anchors.leftMargin: Theme.sp5; anchors.topMargin: Theme.sp5
                anchors.bottomMargin: Theme.sp5; anchors.rightMargin: Theme.sp5
                spacing: 9

                Item { Layout.fillHeight: true }

                Row {   // eyebrow: ● FEATURED · TYPE
                    spacing: 7
                    Rectangle { width: 6; height: 6; radius: 3; color: Theme.grn; anchors.verticalCenter: parent.verticalCenter }
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: (i18n.language, i18n.t("find_featured")) + "   ·   " + (bb.heroItem ? (bb.heroItem.type || "").toUpperCase() : "")
                        color: Theme.t3; font.pixelSize: 11; font.weight: Font.Bold
                        font.letterSpacing: 1.2; font.family: Theme.fontSans
                    }
                }
                Text {
                    text: bb.heroItem ? bb.heroItem.title : ""
                    color: "#ffffff"
                    font.pixelSize: 32; font.weight: Font.Bold; font.family: Theme.fontSans
                    Layout.fillWidth: true; elide: Text.ElideRight; maximumLineCount: 2; wrapMode: Text.WordWrap
                }
                RowLayout {   // year · ★rating · ●seeds
                    spacing: 12
                    Text {
                        visible: text.length > 0
                        text: {
                            if (!bb.heroItem) return ""
                            var parts = []
                            if ((bb.heroItem.year || "").length > 0) parts.push(bb.heroItem.year)
                            if ((bb.heroItem.rating || 0) > 0) parts.push("★ " + bb.heroItem.rating.toFixed(1))
                            return parts.join("    ·    ")
                        }
                        color: "#c8c8cc"; font.pixelSize: 12; font.weight: Font.DemiBold; font.family: Theme.fontSans
                    }
                    Row {
                        spacing: 6
                        visible: bb.summary && bb.summary.maxSeeds > 0
                        Rectangle { width: 7; height: 7; radius: 4; color: Theme.grn; anchors.verticalCenter: parent.verticalCenter }
                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: bb.summary ? (bb.fmtCount(bb.summary.maxSeeds) + " seeds") : ""
                            color: Theme.grn; font.pixelSize: 12; font.weight: Font.DemiBold; font.family: Theme.fontSans
                        }
                    }
                }
                Text {
                    text: bb.heroItem ? bb.heroItem.overview : ""
                    color: "#d8d8dc"; font.pixelSize: 13; font.family: Theme.fontSans
                    Layout.fillWidth: true; Layout.maximumWidth: 620
                    wrapMode: Text.WordWrap; maximumLineCount: 3; elide: Text.ElideRight
                }
                RowLayout {
                    spacing: 10
                    BtnFlat {
                        visible: bb.heroItem && bb.heroItem.type !== "game"
                        primary: true
                        text: (i18n.language, i18n.t("gw_get_and_watch"))
                        onClicked: if (bb.heroItem && typeof search !== "undefined")
                                       search.getAndWatch(bb.heroItem.title,
                                                          bb.heroItem.year || "",
                                                          bb.heroItem.type || "movie")
                    }
                    BtnFlat {
                        visible: bb.heroItem && bb.heroItem.type === "game"
                        primary: true
                        text: (i18n.language, i18n.t("gi_get_and_install"))
                        onClicked: if (bb.heroItem && typeof search !== "undefined")
                                       search.getAndWatch(bb.heroItem.title,
                                                          bb.heroItem.year || "",
                                                          "game")
                    }
                    BtnFlat {
                        visible: bb.trailerKey !== ""
                        text: (i18n.language, i18n.t("gw_trailer"))
                        onClicked: Qt.openUrlExternally("https://www.youtube.com/watch?v=" + bb.trailerKey)
                    }
                    BtnFlat {
                        primary: false
                        text: (i18n.language, i18n.t("find_details"))
                        onClicked: if (bb.heroItem) bb.detailsRequested(bb.heroItem.title)
                    }
                    Text {   // N sources · best X GB (when the lookup resolves)
                        Layout.leftMargin: 4
                        visible: bb.summary && bb.summary.count > 0
                        text: {
                            if (!bb.summary) return ""
                            var s = bb.summary.count + " sources"
                            if (bb.summary.bestSize > 0) s += "    ·    best " + bb.fmtGB(bb.summary.bestSize)
                            return s
                        }
                        color: Theme.t4; font.pixelSize: 12; font.family: Theme.fontSans; font.features: Theme.tnum
                    }
                }

                Item { Layout.fillHeight: true }
            }
        }

        // carousel dots: bottom-right, click to jump between featured items
        Row {
            anchors.right: parent.right
            anchors.rightMargin: Theme.sp5
            anchors.bottom: parent.bottom
            anchors.bottomMargin: Theme.sp4
            spacing: 7
            z: 5
            visible: bb.model.length > 1
            Repeater {
                model: bb.model.length
                delegate: Rectangle {
                    id: dot
                    required property int index
                    readonly property bool current: index === bb.shownIndex
                    width: current ? 28 : 7
                    height: 7; radius: 3.5
                    color: current ? Qt.rgba(1, 1, 1, 0.22) : Qt.rgba(1, 1, 1, 0.35)
                    Behavior on width { NumberAnimation { duration: 260; easing.type: Theme.easeOut } }
                    Behavior on color { ColorAnimation { duration: 200 } }
                    clip: true
                    // the current dot fills up until the next item comes in
                    Rectangle {
                        height: parent.height; radius: parent.radius
                        color: Theme.accent
                        visible: dot.current
                        width: heroTimer.running && !Theme.reduceMotion ? parent.width * dot.fill : parent.width
                    }
                    property real fill: 0
                    NumberAnimation on fill {
                        id: dotFill
                        running: false
                        from: 0; to: 1
                        duration: heroTimer.interval
                    }
                    onCurrentChanged: if (current) dotFill.restart()
                    Component.onCompleted: if (current) dotFill.restart()
                    Connections {
                        target: heroTimer
                        function onRunningChanged() { if (dot.current && heroTimer.running) dotFill.restart() }
                    }
                    MouseArea {
                        anchors.fill: parent; anchors.margins: -5
                        cursorShape: Qt.PointingHandCursor
                        onClicked: bb.heroIndex = index
                    }
                }
            }
        }
    }
}
