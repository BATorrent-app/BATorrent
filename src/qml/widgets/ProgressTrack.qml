// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// The one progress bar. Every surface that shows how far a torrent has got —
// the grid tile, the list row, the detail panel, the rail slot — draws this, so
// the state language cannot drift between them.
//
// Two of the five states carry no percentage. A torrent whose files went
// missing, or whose storage failed, still has a progress number, and painting
// it would say "we are 62% of the way there" about something that has stopped.
// Those get a band travelling left to right over a dead track instead: stripes
// for missing, solid for error. Sherwan's proposal, and he is right that a
// stalled bar and a broken one should not look alike.
import QtQuick
import "../theme"

Item {
    id: track
    property real progress: 0
    property string stateKey: ""
    property color fill: Theme.fillFor(track.stateKey)
    // A highlight travelling over the fill: "this is moving right now". The
    // seeding bar used to be a separate 2px line at the poster's edge, which
    // made seeding and downloading two different shapes for the same fact.
    property bool sheen: false

    readonly property bool trouble: Theme.isTroubleState(track.stateKey)
    implicitHeight: 4

    Rectangle {
        id: bed
        anchors.fill: parent
        radius: height / 2
        color: Theme.track
        clip: true

        // normal states: a fill you can read a percentage off
        Rectangle {
            visible: !track.trouble
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: parent.width * Math.max(0, Math.min(1, track.progress))
            radius: parent.radius
            color: track.fill
            Behavior on width { NumberAnimation { duration: 220; easing.type: Easing.OutCubic } }
            Behavior on color { ColorAnimation { duration: 200 } }
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

        // trouble states: the whole track becomes a moving hazard stripe. A small
        // band sliding across read as an object crossing the bar; stripes that
        // travel inside a full bar say "this one is in a special state and still
        // alive", without claiming a percentage it does not have.
        Item {
            anchors.fill: parent
            visible: track.trouble
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
                        // cannot be mistaken for a progress bar — no progress bar
                        // is diagonal — which is exactly what these states need.
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
    }
}
