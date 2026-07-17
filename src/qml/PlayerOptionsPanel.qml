// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Unified playback options — audio · subtitles · speed in one dark-glass
// popover above the control bar (streaming-app pattern), replacing the three
// separate context menus. Color is a signal: the active row/chip wears the
// accent, surfaces stay dark.
import QtQuick
import QtQuick.Layouts
import "theme"

Item {
    id: opts
    property var pw           // PlayerWindow root
    property var mediaPlayer
    property bool open: false
    signal searchOnline()
    signal loadFile()

    function openPanel() { open = true }
    function closePanel() { open = false }

    visible: opacity > 0
    opacity: open ? 1 : 0
    Behavior on opacity { NumberAnimation { duration: 130; easing.type: Easing.OutCubic } }

    readonly property bool hasAudioChoice: mediaPlayer && mediaPlayer.audioTracks.length > 1

    // click-away
    MouseArea { anchors.fill: parent; enabled: opts.open; onClicked: opts.closePanel() }

    component OptRow: Rectangle {
        id: row
        property string label
        property bool checked: false
        signal activated()
        Layout.fillWidth: true
        implicitHeight: 32
        radius: 7
        color: rowMa.containsMouse ? "#14ffffff" : "transparent"
        Behavior on color { ColorAnimation { duration: 80 } }
        Text {
            anchors.left: parent.left; anchors.leftMargin: 10
            anchors.verticalCenter: parent.verticalCenter
            text: "✓"; visible: row.checked
            color: Theme.accent; font.pixelSize: 12; font.weight: Font.Bold
        }
        Text {
            anchors.left: parent.left; anchors.leftMargin: 28
            anchors.right: parent.right; anchors.rightMargin: 10
            anchors.verticalCenter: parent.verticalCenter
            text: row.label
            color: row.checked ? Theme.t1 : Theme.t2
            font.pixelSize: 12; font.family: Theme.fontSans
            elide: Text.ElideMiddle
        }
        MouseArea {
            id: rowMa; anchors.fill: parent
            hoverEnabled: true; cursorShape: Qt.PointingHandCursor
            onClicked: row.activated()
        }
    }

    component SectionTitle: Text {
        color: Theme.t4; font.pixelSize: 10; font.weight: Font.Bold
        font.letterSpacing: 0.8; font.capitalization: Font.AllUppercase
        font.family: Theme.fontSans
        Layout.leftMargin: 10
    }

    Rectangle {
        id: card
        anchors.right: parent.right; anchors.bottom: parent.bottom
        anchors.rightMargin: 18; anchors.bottomMargin: 104
        width: Math.min(opts.hasAudioChoice ? 520 : 320, parent.width - 36)
        height: Math.min(body.implicitHeight + 28, parent.height - 160)
        radius: 12
        color: "#f50a0a0c"
        border.color: Theme.hair; border.width: 1
        // rise with the fade, matching the app's menu transitions
        transform: Translate {
            y: opts.open ? 0 : 6
            Behavior on y { NumberAnimation { duration: 130; easing.type: Easing.OutCubic } }
        }

        MouseArea { anchors.fill: parent }   // swallow click-away inside the card

        ColumnLayout {
            id: body
            anchors.fill: parent
            anchors.margins: 14
            spacing: 10

            RowLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 18

                // ---- audio (only when there's a real choice) ----
                ColumnLayout {
                    visible: opts.hasAudioChoice
                    Layout.alignment: Qt.AlignTop
                    Layout.fillWidth: true
                    spacing: 4
                    SectionTitle { text: (i18n.language, i18n.t("player_audio")) }
                    Repeater {
                        model: opts.hasAudioChoice ? mediaPlayer.audioTracks.length : 0
                        OptRow {
                            required property int index
                            label: opts.pw.trackName(mediaPlayer.audioTracks[index],
                                       (i18n.language, i18n.t("player_audio")) + " " + (index + 1))
                            checked: mediaPlayer.activeAudioTrack === index
                            onActivated: {
                                mediaPlayer.activeAudioTrack = index
                                if (typeof settings !== "undefined")
                                    settings.set("audioTrack_" + opts.pw.infoHash, index)
                            }
                        }
                    }
                }

                // ---- subtitles ----
                ColumnLayout {
                    Layout.alignment: Qt.AlignTop
                    Layout.fillWidth: true
                    spacing: 4
                    SectionTitle { text: (i18n.language, i18n.t("player_subs")) }
                    OptRow {
                        label: (i18n.language, i18n.t("player_subs_off"))
                        checked: mediaPlayer.activeSubtitleTrack < 0 && !opts.pw.extSubsActive
                        onActivated: { opts.pw.clearExternalSubs(); mediaPlayer.activeSubtitleTrack = -1 }
                    }
                    Repeater {
                        model: mediaPlayer ? mediaPlayer.subtitleTracks.length : 0
                        OptRow {
                            required property int index
                            label: opts.pw.trackName(mediaPlayer.subtitleTracks[index],
                                       (i18n.language, i18n.t("player_subs")) + " " + (index + 1))
                            checked: mediaPlayer.activeSubtitleTrack === index
                            onActivated: {
                                opts.pw.clearExternalSubs()
                                mediaPlayer.activeSubtitleTrack = index
                                if (typeof settings !== "undefined")
                                    settings.set("subTrack_" + opts.pw.infoHash, index)
                            }
                        }
                    }
                    OptRow {
                        visible: opts.pw.extSubsActive
                        label: (i18n.language, i18n.t("player_subs_external")) + ": " + opts.pw.extSubName
                        checked: opts.pw.extSubsActive
                        onActivated: opts.pw.clearExternalSubs()
                    }
                    Rectangle { Layout.fillWidth: true; Layout.topMargin: 2; Layout.bottomMargin: 2; height: 1; color: Theme.hairSoft }
                    OptRow {
                        label: (i18n.language, i18n.t("subsearch_menu"))
                        onActivated: { opts.closePanel(); opts.searchOnline() }
                    }
                    OptRow {
                        label: (i18n.language, i18n.t("player_load_subtitle"))
                        onActivated: { opts.closePanel(); opts.loadFile() }
                    }
                }
            }

            Rectangle { Layout.fillWidth: true; height: 1; color: Theme.hairSoft }

            // ---- speed: segmented chips, not a dropdown ----
            RowLayout {
                Layout.fillWidth: true
                spacing: 6
                SectionTitle { text: (i18n.language, i18n.t("player_speed")); Layout.leftMargin: 10 }
                Item { Layout.fillWidth: true }
                Repeater {
                    model: [0.5, 0.75, 1.0, 1.25, 1.5, 2.0]
                    Rectangle {
                        required property var modelData
                        readonly property bool cur: mediaPlayer.playbackRate === modelData
                        implicitWidth: spT.implicitWidth + 16; implicitHeight: 24
                        radius: 12
                        color: spMa.containsMouse && !cur ? "#14ffffff" : "transparent"
                        border.color: cur ? Theme.accent : Theme.hair; border.width: 1
                        Behavior on border.color { ColorAnimation { duration: 100 } }
                        Text {
                            id: spT; anchors.centerIn: parent
                            text: modelData + "×"
                            color: parent.cur ? Theme.accent : Theme.t2
                            font.pixelSize: 11; font.weight: Font.DemiBold
                            font.family: Theme.fontSans; font.features: Theme.tnum
                        }
                        MouseArea {
                            id: spMa; anchors.fill: parent
                            hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                            onClicked: mediaPlayer.playbackRate = parent.modelData
                        }
                    }
                }
            }
        }
    }
}
