// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Overlay for Get & Watch (movie/series) and Get & Install (games): searching →
// buffering/downloading/installing → auto-open, with Cancel. Driven by Main.qml.
import QtQuick
import QtQuick.Layouts
import "theme"
import "widgets"

Item {
    id: ov
    anchors.fill: parent
    z: 350
    visible: phase !== ""

    // "" | searching | buffering | downloading | installing | failed
    property string phase: ""
    property string title: ""
    property string hash: ""
    property real percent: 0          // 0..1
    property string failMessage: ""
    property bool forGame: false      // picks gi_* phase copy when true

    signal canceled()

    function show(p, t) { phase = p; title = t; hash = ""; percent = 0; failMessage = "" }
    function hide() { phase = ""; hash = ""; percent = 0; failMessage = ""; forGame = false; autoHide.stop() }
    function fail(msg) { phase = "failed"; failMessage = msg; autoHide.restart() }

    readonly property bool showSpinner: phase === "searching" || phase === "installing"
    readonly property bool showPct: phase === "buffering" || phase === "downloading"
    readonly property bool showBar: phase === "buffering" || phase === "downloading" || phase === "installing"

    Timer { id: autoHide; interval: 3500; onTriggered: ov.hide() }

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
                Rectangle {
                    id: spinner
                    anchors.fill: parent
                    radius: width / 2
                    color: "transparent"
                    border.color: Theme.accent
                    border.width: 4
                    opacity: ov.showSpinner ? 1 : 0.25
                    RotationAnimation on rotation {
                        from: 0; to: 360; duration: 900
                        loops: Animation.Infinite; running: ov.showSpinner
                    }
                    Rectangle {
                        width: parent.width / 2; height: parent.height / 2
                        color: Theme.bg
                        anchors.right: parent.right; anchors.top: parent.top
                    }
                }
                Text {
                    anchors.centerIn: parent
                    visible: ov.showPct
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
                text: {
                    if (ov.phase === "failed") return ov.failMessage
                    if (ov.phase === "searching")
                        return i18n.t(ov.forGame ? "gi_phase_searching" : "gw_phase_searching")
                    if (ov.phase === "buffering") return i18n.t("gw_phase_buffering")
                    if (ov.phase === "downloading") return i18n.t("gi_phase_downloading")
                    if (ov.phase === "installing") return i18n.t("gi_phase_installing")
                    return ""
                }
                color: Theme.t3
                font.pixelSize: 12; font.family: Theme.fontSans
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 4
                radius: 2
                visible: ov.showBar
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
