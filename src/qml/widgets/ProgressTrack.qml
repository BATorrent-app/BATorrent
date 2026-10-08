// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// The one progress bar. Every surface that shows how far a torrent has got:
// the grid tile, the list row, the detail panel, the rail slot: draws this, so
// the state language cannot drift between them.
//
// Three states carry no percentage. Missing and failed still have a progress
// number, and painting it would say "we are 62% of the way there" about
// something that has stopped; a magnet without metadata has no number at all.
// Those get travelling bands over a dead track instead: hazard stripes for
// missing and error, a single sweep for fetching.
import QtQuick
import "../theme"

Item {
    id: track
    property real progress: 0
    property string stateKey: ""
    property color fill: Theme.fillFor(track.stateKey)
    // A highlight travelling over the fill: "this is moving right now".
    property bool sheen: false
    // The percentage belongs to the bar, not to a corner of whatever hosts it.
    // The list already read it off the middle of the track while the grid put it
    // in a top-right pill, so the same number lived in two different places
    // depending on the view. Owned here, both views get it in the same spot.
    property bool showPercent: false
    // Ink for the empty part of the bar. Not derived from Theme.track: that is
    // a 9%-alpha wash, so whatever sits behind the bar decides this, and only
    // the caller knows what that is.
    property color emptyInk: Theme.t1
    // Follows whichever background the number is actually over, which changes
    // as the fill grows past it. Callers whose fill is dimmed (the list row)
    // pass their own.
    property color percentColor: pct.overFill ? Theme.inkOn(track.fill) : track.emptyInk
    // Dims the fill without dimming the number on top of it. The list row used
    // to set `opacity` on the whole component to keep its percentage readable,
    // which only worked while the percentage lived outside the bar.
    property real fillOpacity: 1.0

    readonly property bool trouble: Theme.isTroubleState(track.stateKey)
    // Nothing has been measured yet, so there is no percentage to be honest
    // about: the bar says "working" instead of claiming zero.
    readonly property bool indeterminate: track.stateKey === "fetching"
    implicitHeight: 4

    Rectangle {
        id: bed
        anchors.fill: parent
        radius: height / 2
        color: Theme.track
        clip: true

        // normal states: a fill you can read a percentage off
        Rectangle {
            visible: !track.trouble && !track.indeterminate
            opacity: track.fillOpacity
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: parent.width * Math.max(0, Math.min(1, track.progress))
            radius: parent.radius
            color: track.fill
            Behavior on width { NumberAnimation { duration: Theme.durSlow; easing.type: Theme.easeOut } }
            Behavior on color { ColorAnimation { duration: Theme.durBase } }
            clip: true

            Rectangle {
                id: sheenBand
                visible: track.sheen && !Theme.reduceMotion
                width: Math.max(24, parent.width * 0.28)
                height: parent.height
                radius: parent.radius
                color: Qt.rgba(1, 1, 1, 0.35)
                SequentialAnimation on x {
                    running: sheenBand.visible
                    loops: Animation.Infinite
                    NumberAnimation {
                        from: -sheenBand.width
                        to: Math.max(1, sheenBand.parent.width)
                        duration: 2600
                        easing.type: Easing.InOutSine
                    }
                    PauseAnimation { duration: 900 }
                }
            }
        }

        Rectangle {
            id: seekBand
            visible: track.indeterminate
            width: Math.max(28, parent.width * 0.45)
            height: parent.height
            opacity: track.fillOpacity
            // Transparent ends, not rounded ones: a hard-edged pill slides
            // across as an object, where a band that has no edge reads as the
            // bar being searched. The stops carry the fill's own colour at
            // zero alpha — plain "transparent" is black and fringes grey.
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0.0; color: Qt.rgba(track.fill.r, track.fill.g, track.fill.b, 0) }
                GradientStop { position: 0.5; color: track.fill }
                GradientStop { position: 1.0; color: Qt.rgba(track.fill.r, track.fill.g, track.fill.b, 0) }
            }
            SequentialAnimation on x {
                running: seekBand.visible && !Theme.reduceMotion
                loops: Animation.Infinite
                NumberAnimation {
                    from: -seekBand.width
                    to: Math.max(1, seekBand.parent.width)
                    duration: 2000
                    easing.type: Easing.Linear
                }
            }
        }

        // trouble states: the whole track becomes a moving hazard stripe. A small
        // band sliding across read as an object crossing the bar; stripes that
        // travel inside a full bar say "this one is in a special state and still
        // alive", without claiming a percentage it does not have.
        Item {
            anchors.fill: parent
            visible: track.trouble
            opacity: track.fillOpacity
            clip: true

            Row {
                id: stripes
                // Three times the bar's height, hung above it, so a tilted stripe
                // still covers the full band at both ends instead of leaving a
                // triangular gap in the corners.
                height: parent.height * 3
                y: -parent.height
                // One period wider than the track on each side, so the loop can
                // shift by exactly one period and start over invisibly.
                // Wide on purpose: at 6px a 178px tile fits fifteen blocks and
                // reads as a barcode. Half the count reads as a hazard marking.
                readonly property int stripeW: 11
                readonly property int period: stripeW * 2
                x: 0
                Repeater {
                    model: Math.ceil(bed.width / stripes.period) + 2
                    delegate: Row {
                        height: stripes.height
                        // 30°, not 45°: on a 9px bar a 45° stripe is too short to
                        // cross the band and reads as a diamond. And a diagonal
                        // can't be mistaken for progress, since no progress bar
                        // is diagonal. That is what these states need.
                        rotation: 30
                        transformOrigin: Item.Center
                        Rectangle {
                            width: stripes.stripeW; height: parent.height
                            color: track.fill
                        }
                        Rectangle {
                            width: stripes.stripeW; height: parent.height
                            // White for missing, a darker red for error: same
                            // motion, and error still reads as solid red.
                            // Off-white, not white: pure white against the accent
                            // is the highest contrast in the palette and shouted
                            // louder than "your files moved" deserves.
                            color: track.stateKey === "error"
                                   ? Qt.darker(track.fill, 1.9) : "#c9c9cf"
                        }
                    }
                }
                NumberAnimation on x {
                    running: track.trouble && track.visible && !Theme.reduceMotion
                    loops: Animation.Infinite
                    from: 0
                    to: -stripes.period
                    duration: 700
                    // Linear on purpose: any easing turns a steady march into a
                    // pulse, which reads as a heartbeat rather than movement.
                    easing.type: Easing.Linear
                }
            }
        }

        // Sits above both layers, and only where a percentage is honest: the
        // trouble states deliberately have no number to show.
        Text {
            id: pct
            anchors.centerIn: parent
            visible: track.showPercent && !track.trouble && !track.indeterminate
            text: Math.floor(Math.max(0, Math.min(1, track.progress)) * 100) + "%"
            color: track.percentColor

            readonly property real fillEdge:
                parent.width * Math.max(0, Math.min(1, track.progress))
            readonly property real leftX: (parent.width - implicitWidth) / 2
            readonly property real rightX: (parent.width + implicitWidth) / 2
            readonly property bool overFill: fillEdge >= rightX
            // Only while the edge actually runs through the digits: a full bar
            // has nothing to separate, and the halo there is just mud.
            readonly property bool straddles: fillEdge > leftX && fillEdge < rightX

            style: straddles ? Text.Outline : Text.Normal
            styleColor: track.percentColor.hslLightness > 0.5
                        ? Qt.rgba(0, 0, 0, 0.55) : Qt.rgba(1, 1, 1, 0.45)
            font.pixelSize: Math.max(9, Math.round(parent.height * 0.62))
            font.weight: Font.DemiBold
            font.family: Theme.fontSans
            font.features: Theme.tnum
        }
    }
}
