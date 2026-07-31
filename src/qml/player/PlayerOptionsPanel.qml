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
    readonly property int subTrackCount: mediaPlayer ? mediaPlayer.subtitleTracks.length : 0
    // Rough content height: section titles + rows + speed strip. Cap + scroll when long.
    readonly property int panelRows: Math.max(subTrackCount + 3, hasAudioChoice ? mediaPlayer.audioTracks.length : 0)
    readonly property int idealHeight: 100 + Math.min(panelRows, 11) * 36

    // Own UI selection state — OptRow's `checked:` binding was going stale (both
    // "off" and a track could show ✓). Sync from the player, write on click.
    property int uiSubTrack: -1
    property int uiAudioTrack: 0

    function syncTracksFromPlayer() {
        if (!mediaPlayer) return
        uiSubTrack = mediaPlayer.activeSubtitleTrack
        uiAudioTrack = mediaPlayer.activeAudioTrack
    }

    function selectSubOff() {
        if (opts.pw) opts.pw.clearExternalSubs()
        if (mediaPlayer) mediaPlayer.activeSubtitleTrack = -1
        uiSubTrack = -1
        if (typeof settings !== "undefined" && opts.pw)
            settings.set("subTrack_" + opts.pw.infoHash, -1)
    }

    function selectSubTrack(index) {
        if (opts.pw) opts.pw.clearExternalSubs()
        if (mediaPlayer) mediaPlayer.activeSubtitleTrack = index
        uiSubTrack = index
        if (typeof settings !== "undefined" && opts.pw)
            settings.set("subTrack_" + opts.pw.infoHash, index)
    }

    function selectAudioTrack(index) {
        if (mediaPlayer) mediaPlayer.activeAudioTrack = index
        uiAudioTrack = index
        if (typeof settings !== "undefined" && opts.pw)
            settings.set("audioTrack_" + opts.pw.infoHash, index)
    }

    onOpenChanged: if (open) syncTracksFromPlayer()
    Connections {
        target: mediaPlayer
        enabled: opts.open
        function onActiveSubtitleTrackChanged() { opts.uiSubTrack = mediaPlayer.activeSubtitleTrack }
        function onActiveAudioTrackChanged() { opts.uiAudioTrack = mediaPlayer.activeAudioTrack }
    }

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
        // Cap against the player chrome; long subtitle lists scroll inside.
        height: Math.min(idealHeight, Math.max(160, parent.height - 160))
        radius: 12
        color: "#f50a0a0c"
        border.color: Theme.hair; border.width: 1
        clip: true
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
                    Layout.fillHeight: true
                    spacing: 4
                    SectionTitle { text: (i18n.language, i18n.t("player_audio")) }
                    Flickable {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        contentWidth: width
                        contentHeight: audioCol.implicitHeight
                        boundsBehavior: Flickable.StopAtBounds
                        ColumnLayout {
                            id: audioCol
                            width: parent.width
                            spacing: 4
                            Repeater {
                                model: opts.hasAudioChoice ? mediaPlayer.audioTracks.length : 0
                                OptRow {
                                    required property int index
                                    label: opts.pw.trackName(mediaPlayer.audioTracks[index],
                                               (i18n.language, i18n.t("player_audio")) + " " + (index + 1))
                                    checked: opts.uiAudioTrack === index
                                    onActivated: opts.selectAudioTrack(index)
                                }
                            }
                        }
                    }
                }

                // ---- subtitles ----
                ColumnLayout {
                    Layout.alignment: Qt.AlignTop
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: 4
                    SectionTitle { text: (i18n.language, i18n.t("player_subs")) }
                    Flickable {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        contentWidth: width
                        contentHeight: subsCol.implicitHeight
                        boundsBehavior: Flickable.StopAtBounds
                        ColumnLayout {
                            id: subsCol
                            width: parent.width
                            spacing: 4
                            OptRow {
                                label: (i18n.language, i18n.t("player_subs_off"))
                                checked: opts.uiSubTrack < 0 && !opts.pw.extSubsActive
                                onActivated: opts.selectSubOff()
                            }
                            Repeater {
                                model: mediaPlayer ? mediaPlayer.subtitleTracks.length : 0
                                OptRow {
                                    required property int index
                                    label: opts.pw.trackName(mediaPlayer.subtitleTracks[index],
                                               (i18n.language, i18n.t("player_subs")) + " " + (index + 1))
                                    checked: !opts.pw.extSubsActive && opts.uiSubTrack === index
                                    onActivated: opts.selectSubTrack(index)
                                }
                            }
                            OptRow {
                                visible: opts.pw.extSubsActive
                                label: (i18n.language, i18n.t("player_subs_external")) + ": " + opts.pw.extSubName
                                checked: opts.pw.extSubsActive
                                onActivated: opts.selectSubOff()
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
