// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// "Preparing to watch" overlay for the one-click Get & Watch flow: shows the
// phase (searching a release → buffering with live %), and a Cancel. Driven by
// Main.qml from the search/session bridge signals.
import QtQuick
import QtQuick.Layouts
import "theme"
import "widgets"

Item {
    id: ov
    anchors.fill: parent
    z: 350
    visible: phase !== ""

    // "" hidden | "searching" | "buffering" | "failed"
    property string phase: ""
    property string title: ""
    property string hash: ""
    property real percent: 0          // 0..1, buffering
    property string failMessage: ""

    signal canceled()

    function show(p, t) { phase = p; title = t; hash = ""; percent = 0 }
    function hide() { phase = ""; hash = ""; percent = 0; autoHide.stop() }
    function fail(msg) { phase = "failed"; failMessage = msg; autoHide.restart() }

    Timer { id: autoHide; interval: 3500; onTriggered: ov.hide() }

    // dim + swallow clicks behind the card
    Rectangle {
        anchors.fill: parent
        color: Qt.rgba(0, 0, 0, 0.55)
        MouseArea { anchors.fill: parent; onWheel: function(w){ w.accepted = true } }
    }

    Rectangle {
        anchors.centerIn: parent
        width: 420
        height: col.implicitHeight + 44
        radius: 14
        color: Theme.bg
        border.color: Theme.hair
        border.width: 1

        ColumnLayout {
            id: col
            anchors.centerIn: parent
            width: parent.width - 48
            spacing: 16

            // spinner (searching) or progress ring text (buffering)
            Item {
                Layout.alignment: Qt.AlignHCenter
                Layout.preferredWidth: 54; Layout.preferredHeight: 54
                visible: ov.phase !== "failed"
                Rectangle {
                    anchors.fill: parent
                    radius: width / 2
                    color: "transparent"
                    border.color: Theme.hairSoft
                    border.width: 4
                }
                // rotating arc for "searching"; filling arc would need Canvas — keep
                // it simple: a spinning accent dot ring.
                Rectangle {
                    id: spinner
                    anchors.fill: parent
                    radius: width / 2
                    color: "transparent"
                    border.color: Theme.accent
                    border.width: 4
                    opacity: ov.phase === "searching" ? 1 : 0.25
                    // mask to a quarter via a rotating overlay isn't trivial; use a
                    // simple rotation of a notched ring drawn with a conic-ish trick:
                    RotationAnimation on rotation {
                        from: 0; to: 360; duration: 900
                        loops: Animation.Infinite; running: ov.phase === "searching"
                    }
                    Rectangle {   // the "gap" that makes the ring look like a spinner
                        width: parent.width / 2; height: parent.height / 2
                        color: Theme.bg
                        anchors.right: parent.right; anchors.top: parent.top
                    }
                }
                Text {
                    anchors.centerIn: parent
                    visible: ov.phase === "buffering"
                    text: Math.round(ov.percent * 100) + "%"
                    color: Theme.t1; font.pixelSize: 13; font.weight: Font.Bold; font.family: Theme.fontMono
                }
            }

            IconImg {
                Layout.alignment: Qt.AlignHCenter
                visible: ov.phase === "failed"
                src: "qrc:/icons/close.svg"
                tint: Theme.accent
                s: 30
            }

            Text {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                text: ov.title
                color: Theme.t1
                font.pixelSize: 15; font.weight: Font.Bold; font.family: Theme.fontSans
                elide: Text.ElideRight
            }
            Text {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
                text: ov.phase === "searching" ? i18n.t("gw_phase_searching")
                    : ov.phase === "buffering" ? i18n.t("gw_phase_buffering")
                    : ov.failMessage
                color: Theme.t3
                font.pixelSize: 12; font.family: Theme.fontSans
            }

            // thin progress bar while buffering
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 4
                radius: 2
                visible: ov.phase === "buffering"
                color: Theme.track
                Rectangle {
                    height: parent.height; radius: 2
                    width: parent.width * Math.max(0.02, Math.min(1, ov.percent))
                    color: Theme.accent
                    Behavior on width { NumberAnimation { duration: 300; easing.type: Easing.OutCubic } }
                }
            }

            BtnFlat {
                Layout.alignment: Qt.AlignHCenter
                visible: ov.phase !== "failed"
                text: (i18n.language, i18n.t("gw_cancel"))
                onClicked: { ov.canceled(); ov.hide() }
            }
        }
    }
}
