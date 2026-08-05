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

        // trouble states: a travelling band, because there is no honest number
        Item {
            id: band
            visible: track.trouble
            width: Math.max(28, bed.width * 0.22)
            height: bed.height
            // Repeating sweep. Reduced motion turns it into a static band parked
            // at the left — the colour still says what happened.
            SequentialAnimation on x {
                running: track.trouble && track.visible && !Theme.reduceMotion
                loops: Animation.Infinite
                NumberAnimation { from: -band.width; to: bed.width; duration: 1900; easing.type: Easing.InOutSine }
                PauseAnimation { duration: 260 }
            }
            x: Theme.reduceMotion ? 0 : -band.width

            Rectangle {
                anchors.fill: parent
                radius: bed.radius
                color: track.fill
                visible: track.stateKey === "error"
            }
            // missing: red and white verticals, drawn rather than tiled so the
            // stripe width holds at any bar height
            Row {
                anchors.fill: parent
                visible: track.stateKey === "missing"
                spacing: 0
                Repeater {
                    model: Math.max(2, Math.floor(band.width / 6))
                    delegate: Rectangle {
                        required property int index
                        width: 6
                        height: band.height
                        color: index % 2 === 0 ? track.fill : "#f2f2f4"
                    }
                }
            }
        }
    }
}
