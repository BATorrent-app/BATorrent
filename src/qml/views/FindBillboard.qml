// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Featured billboard of the browse surface, in the language of a title's own
// page: the backdrop full bleed, the title's lettering on its darkest corner,
// one way in. Rotates every 7s with a crossfade; dots jump between items.
import QtQuick
import "../theme"
import "../widgets"

Item {
    id: bb
    property var model: []          // featured items (already type-filtered)
    property bool active: true      // gates the rotation timer
    signal openRequested(var item)
    signal getWatchRequested(var item)

    property int heroIndex: 0
    property int shownIndex: 0
    readonly property var heroItem: (model.length > shownIndex) ? model[shownIndex] : null
    onModelChanged: { heroIndex = 0; shownIndex = 0 }

    visible: heroItem !== null

    function ctaKey(t) {
        return t === "series" ? "find_see_episodes" : t === "game" ? "find_see_builds" : "find_see_releases"
    }

    // Cross-fade: the old text leaves first, then a still of the art is put
    // on top, the content underneath switches to the next item and the still
    // fades out while the new text comes in. Two titles are never on screen
    // together.
    onHeroIndexChanged: {
        heroTimer.restart()
        if (heroIndex !== shownIndex) heroSwap.restart()
    }
    SequentialAnimation {
        id: heroSwap
        ParallelAnimation {
            NumberAnimation { target: content.textBlock; property: "opacity"; to: 0; duration: Theme.durFast; easing.type: Theme.easeIn }
            NumberAnimation { target: content.textShift; property: "x"; to: -Theme.travel(10); duration: Theme.durFast; easing.type: Theme.easeIn }
        }
        ScriptAction { script: { snap.scheduleUpdate(); snap.opacity = 1 } }
        // one frame so the still is rendered before the content changes
        PauseAnimation { duration: 34 }
        ScriptAction { script: { bb.shownIndex = bb.heroIndex; kenBurns.restart() } }
        ParallelAnimation {
            NumberAnimation { target: snap; property: "opacity"; to: 0; duration: 480; easing.type: Easing.InOutQuad }
            SequentialAnimation {
                PauseAnimation { duration: Theme.durFast }
                ParallelAnimation {
                    NumberAnimation { target: content.textBlock; property: "opacity"; to: 1; duration: 320; easing.type: Theme.easeOut }
                    NumberAnimation { target: content.textShift; property: "x"; from: Theme.travel(22); to: 0; duration: 560; easing.type: Theme.easeOut }
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
        id: frame
        anchors.fill: parent
        radius: 8
        color: Theme.stageBg
        clip: true

        ShaderEffectSource {
            id: snap
            anchors.fill: parent
            sourceItem: content
            live: false
            opacity: 0
            visible: opacity > 0
            z: 4
        }

        TitleBanner {
            id: content
            anchors.fill: parent
            item: bb.heroItem
            kicker: bb.heroItem ? (i18n.language, i18n.t("find_featured")) + "  ·  "
                                  + content.typeLabel(bb.heroItem.type || "").toUpperCase() : ""
            body: bb.heroItem ? (bb.heroItem.overview || "") : ""
            primaryLabel: bb.heroItem ? (i18n.language, i18n.t(bb.ctaKey(bb.heroItem.type || ""))) : ""
            secondaryLabel: bb.heroItem && bb.heroItem.type === "game"
                            ? (i18n.language, i18n.t("gi_get_and_install"))
                            : (i18n.language, i18n.t("gw_get_and_watch"))
            onPrimaryClicked: if (bb.heroItem) bb.openRequested(bb.heroItem)
            onSecondaryClicked: if (bb.heroItem) bb.getWatchRequested(bb.heroItem)
            NumberAnimation {
                id: kenBurns
                target: content.art
                property: "scale"
                running: bb.active && !Theme.reduceMotion
                from: 1.0; to: 1.05
                duration: heroTimer.interval
                easing.type: Easing.Linear
            }
        }

        // carousel dots: bottom-right, click to jump between featured items
        Row {
            anchors.right: parent.right
            anchors.rightMargin: 28
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 28
            spacing: 6
            z: 5
            visible: bb.model.length > 1
            Repeater {
                model: bb.model.length
                delegate: Rectangle {
                    id: dot
                    required property int index
                    readonly property bool current: index === bb.shownIndex
                    width: current ? 22 : 8
                    height: 4; radius: 2
                    color: current ? Qt.rgba(1, 1, 1, 0.22) : Qt.rgba(1, 1, 1, 0.35)
                    Behavior on width { NumberAnimation { duration: Theme.durSlower; easing.type: Theme.easeOut } }
                    Behavior on color { ColorAnimation { duration: Theme.durBase } }
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
                        anchors.fill: parent; anchors.margins: -6
                        cursorShape: Qt.PointingHandCursor
                        onClicked: bb.heroIndex = index
                    }
                }
            }
        }
    }
}
