// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Optional coach-marks tour. Dims the window and cuts a spotlight around each
// target, with a callout (title + text + arrow). Step 0 is a richer welcome
// (logo + wordmark) shown centered with no spotlight. Steps transition with an
// "iris" cross-fade — the highlight closes, repositions while invisible, then
// re-opens at the new target — so nothing slides across the screen.
import QtQuick
import QtQuick.Layouts
import QtQuick.Effects
import "theme"
import "widgets"

Item {
    id: tour
    anchors.fill: parent
    z: 190
    visible: running || fade > 0

    property var steps: []
    property int index: 0
    property bool running: false

    // fade = whole-overlay (dim) enter/exit; contentVis = per-step spotlight +
    // callout visibility (drives the iris and the callout cross-fade).
    property real fade: 0
    property real contentVis: 0
    Behavior on fade { NumberAnimation { duration: 260; easing.type: Easing.OutCubic } }
    onRunningChanged: {
        fade = running ? 1 : 0
        if (!running) { stepAnim.stop(); openTimer.stop(); contentVis = 0 }
    }

    signal pageRequested(int page)

    readonly property var step: (index >= 0 && index < steps.length) ? steps[index] : null
    property rect spot: Qt.rect(0, 0, 0, 0)
    readonly property int pad: 7
    readonly property bool hasSpot: spot.width > 1 && spot.height > 1

    // stable padded highlight rect — callout/arrow placement reads this so they
    // don't jiggle as the iris animates.
    readonly property rect fullHole: hasSpot
        ? Qt.rect(spot.x - pad, spot.y - pad, spot.width + 2 * pad, spot.height + 2 * pad)
        : Qt.rect(width / 2, height / 2, 0, 0)
    // irised hole (for the dim cut + ring): scales 0→full with contentVis.
    readonly property rect hole: {
        if (!hasSpot) return Qt.rect(width / 2, height / 2, 0, 0)
        var k = Math.max(0, Math.min(1, contentVis))
        var w = fullHole.width * k, h = fullHole.height * k
        var cx = spot.x + spot.width / 2, cy = spot.y + spot.height / 2
        return Qt.rect(cx - w / 2, cy - h / 2, w, h)
    }
    readonly property color dim: Qt.rgba(0, 0, 0, 0.74)

    // ---- step machinery ----
    function start() {
        if (!steps || steps.length === 0) return
        stepAnim.stop()
        index = 0
        spot = Qt.rect(0, 0, 0, 0)
        contentVis = 0
        running = true
        _switchPage()
        openTimer.restart()
    }
    function finish() { running = false }
    function next() { if (index < steps.length - 1) _step(index + 1); else finish() }
    function back() { if (index > 0) _step(index - 1) }

    property int _pending: 0
    function _step(n) { _pending = n; stepAnim.restart() }
    function _switchPage() { if (step && step.page !== undefined) tour.pageRequested(step.page) }
    function _recalc() {
        if (!running || !step || !step.rectFn) { spot = Qt.rect(0, 0, 0, 0); return }
        var r = step.rectFn()
        spot = (r && r.width > 0) ? r : Qt.rect(0, 0, 0, 0)
    }

    // initial open (no close phase): settle layout, measure, iris open
    Timer { id: openTimer; interval: 90; onTriggered: { tour._recalc(); contentOpen.start() } }
    NumberAnimation { id: contentOpen; target: tour; property: "contentVis"; from: 0; to: 1; duration: 280; easing.type: Easing.OutCubic }

    // step change: iris closed → swap (page + measure, while invisible) → open
    SequentialAnimation {
        id: stepAnim
        NumberAnimation { target: tour; property: "contentVis"; to: 0; duration: 150; easing.type: Easing.InCubic }
        ScriptAction { script: { tour.index = tour._pending; tour._switchPage() } }
        PauseAnimation { duration: 130 }
        ScriptAction { script: tour._recalc() }
        NumberAnimation { target: tour; property: "contentVis"; to: 1; duration: 280; easing.type: Easing.OutCubic }
    }

    onWidthChanged: if (running) _recalc()
    onHeightChanged: if (running) _recalc()

    // ---- dim, with the irised hole cut out (4 framing bands) ----
    Rectangle { color: tour.dim; opacity: tour.fade; x: 0; y: 0; width: tour.width; height: Math.max(0, tour.hole.y) }
    Rectangle { color: tour.dim; opacity: tour.fade; x: 0; y: tour.hole.y + tour.hole.height
        width: tour.width; height: Math.max(0, tour.height - (tour.hole.y + tour.hole.height)) }
    Rectangle { color: tour.dim; opacity: tour.fade; x: 0; y: tour.hole.y; width: Math.max(0, tour.hole.x); height: tour.hole.height }
    Rectangle { color: tour.dim; opacity: tour.fade; x: tour.hole.x + tour.hole.width; y: tour.hole.y
        width: Math.max(0, tour.width - (tour.hole.x + tour.hole.width)); height: tour.hole.height }

    // accent ring around the irised hole
    Rectangle {
        visible: tour.hasSpot
        x: tour.hole.x; y: tour.hole.y; width: tour.hole.width; height: tour.hole.height
        radius: 12; color: "transparent"
        border.color: Theme.accent; border.width: 2
        opacity: tour.fade * tour.contentVis
    }

    // swallow interaction with the app behind the tour
    MouseArea { anchors.fill: parent; hoverEnabled: true; enabled: tour.running; onWheel: function(w) { w.accepted = true } }

    // ---- callout ----
    readonly property int coGap: 16
    readonly property string placeSide: {
        if (!hasSpot) return "center"
        var cw = callout.width, ch = callout.height
        if (tour.height - (fullHole.y + fullHole.height) >= ch + coGap + 16) return "below"
        if (fullHole.y >= ch + coGap + 16) return "above"
        if (tour.width - (fullHole.x + fullHole.width) >= cw + coGap + 16) return "right"
        if (fullHole.x >= cw + coGap + 16) return "left"
        return "center"
    }

    Rectangle {
        id: callout
        visible: tour.running && tour.step !== null
        opacity: tour.fade * tour.contentVis
        scale: 0.96 + 0.04 * tour.contentVis
        transformOrigin: Item.Center
        width: 326
        height: calloutCol.implicitHeight + 32
        radius: 14
        color: Theme.panel
        border.color: Theme.hair; border.width: 1
        x: {
            switch (tour.placeSide) {
            case "center": return tour.width / 2 - width / 2
            case "right":  return Math.min(tour.width - width - 16, tour.fullHole.x + tour.fullHole.width + tour.coGap)
            case "left":   return Math.max(16, tour.fullHole.x - tour.coGap - width)
            default:       return Math.max(16, Math.min(tour.width - width - 16,
                                  tour.fullHole.x + tour.fullHole.width / 2 - width / 2))
            }
        }
        y: {
            switch (tour.placeSide) {
            case "center": return tour.height / 2 - height / 2
            case "below":  return tour.fullHole.y + tour.fullHole.height + tour.coGap
            case "above":  return tour.fullHole.y - tour.coGap - height
            default:       return Math.max(16, Math.min(tour.height - height - 16,
                                  tour.fullHole.y + tour.fullHole.height / 2 - height / 2))
            }
        }

        ColumnLayout {
            id: calloutCol
            anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
            anchors.margins: 16
            spacing: 8

            // ---- step counter ----
            Text {
                text: (tour.index + 1) + " / " + tour.steps.length
                color: Theme.t4; font.pixelSize: 11; font.weight: Font.Medium
                font.letterSpacing: 0.5; font.family: Theme.fontMono
            }
            // ---- title ----
            Text {
                Layout.fillWidth: true
                text: tour.step ? tour.step.title : ""
                color: Theme.t1; font.pixelSize: 16; font.weight: Font.Bold; font.family: Theme.fontSans
                wrapMode: Text.WordWrap
            }
            // ---- description ----
            Text {
                Layout.fillWidth: true
                text: tour.step ? tour.step.text : ""
                color: Theme.t2; font.pixelSize: 13; font.family: Theme.fontSans
                wrapMode: Text.WordWrap; lineHeight: 1.35
            }
            // ---- buttons ----
            RowLayout {
                Layout.fillWidth: true; Layout.topMargin: 4; spacing: 8
                BtnFlat { text: (i18n.language, i18n.t("tour_skip")); onClicked: tour.finish() }
                Item { Layout.fillWidth: true }
                BtnFlat {
                    visible: tour.index > 0
                    text: (i18n.language, i18n.t("tour_back")); onClicked: tour.back()
                }
                BtnFlat {
                    primary: true
                    text: tour.index < tour.steps.length - 1
                          ? (i18n.language, i18n.t("tour_next"))
                          : (i18n.language, i18n.t("tour_done"))
                    onClicked: tour.next()
                }
            }
        }
    }

    // ---- arrow (below/above placements only) ----
    Canvas {
        id: arrow
        visible: callout.visible && (tour.placeSide === "below" || tour.placeSide === "above")
        opacity: tour.fade * tour.contentVis
        width: 22; height: 11
        x: Math.max(callout.x + 14, Math.min(callout.x + callout.width - 36,
                    tour.fullHole.x + tour.fullHole.width / 2 - width / 2))
        y: tour.placeSide === "below" ? (callout.y - height + 1) : (callout.y + callout.height - 1)
        rotation: tour.placeSide === "below" ? 0 : 180
        transformOrigin: Item.Center
        onXChanged: requestPaint()
        onYChanged: requestPaint()
        onVisibleChanged: requestPaint()
        onPaint: {
            var ctx = getContext("2d")
            ctx.reset()
            ctx.fillStyle = Theme.panel
            ctx.beginPath()
            ctx.moveTo(0, height)
            ctx.lineTo(width / 2, 0)
            ctx.lineTo(width, height)
            ctx.closePath()
            ctx.fill()
        }
    }
}
