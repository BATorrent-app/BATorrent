// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// The "General" detail pane, shared by the bottom panel (vertical: false —
// cover · main · three KV columns in a row) and the 340px side inspector
// (vertical: true — everything stacked, scrolls when it outgrows the column).
import QtQuick
import QtQuick.Effects
import QtQuick.Layouts
import "../theme"

Flickable {
    id: gen
    property var win
    property bool vertical: false

    contentWidth: width
    contentHeight: body.implicitHeight + Theme.sp5 * 2
    interactive: vertical && contentHeight > height
    clip: true

    GridLayout {
        id: body
        x: Theme.sp5
        y: Theme.sp5
        width: gen.width - Theme.sp5 * 2
        columns: gen.vertical ? 1 : 4
        columnSpacing: Theme.sp6
        rowSpacing: Theme.sp4

        // .dcover — only when there's a resolved poster. A generic torrent
        // (Ubuntu ISO, a code archive) has no cover; showing a placeholder
        // logo made it look like something failed, so the cover collapses to
        // zero width and the text column takes the whole row instead.
        readonly property bool hasCover: gen.win.hasSel && session.selectedPoster.length > 0

        // recovery banner — the selected download's files were deleted/moved off
        // disk; offer to point it at where they live now (or a fresh folder to
        // re-download into) and re-check, without hunting through the menu.
        Rectangle {
            visible: session.selectedFilesMissing
            Layout.columnSpan: body.columns
            Layout.fillWidth: true
            Layout.preferredHeight: missingRow.implicitHeight + Theme.sp4 * 2
            radius: 10
            color: Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.10)
            border.color: Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.35)
            border.width: 1
            RowLayout {
                id: missingRow
                anchors.fill: parent
                anchors.margins: Theme.sp4
                spacing: Theme.sp4
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2
                    Text {
                        text: (i18n.language, i18n.t("state_files_missing"))
                        color: Theme.accentText; font.pixelSize: 13; font.weight: Font.Bold; font.family: Theme.fontSans
                    }
                    Text {
                        Layout.fillWidth: true
                        text: (i18n.language, i18n.t("missing_recover_msg"))
                        color: Theme.t3; font.pixelSize: 11; font.family: Theme.fontSans
                        wrapMode: Text.WordWrap
                    }
                }
                Rectangle {
                    Layout.preferredHeight: 28
                    Layout.preferredWidth: locTxt.implicitWidth + 22
                    radius: 7
                    color: locMa.containsMouse ? Theme.hover : Theme.elev
                    border.color: Theme.hair; border.width: 1
                    Text { id: locTxt; anchors.centerIn: parent; text: (i18n.language, i18n.t("ctx_move_storage")); color: Theme.t1; font.pixelSize: 11; font.weight: Font.DemiBold; font.family: Theme.fontSans }
                    MouseArea { id: locMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: gen.win.promptSetLocation() }
                }
                Rectangle {
                    Layout.preferredHeight: 28
                    Layout.preferredWidth: reTxt.implicitWidth + 22
                    radius: 7
                    color: reMa.containsMouse ? Qt.darker(Theme.accent, 1.1) : Theme.accent
                    Text { id: reTxt; anchors.centerIn: parent; text: (i18n.language, i18n.t("ctx_force_recheck")); color: "#ffffff"; font.pixelSize: 11; font.weight: Font.DemiBold; font.family: Theme.fontSans }
                    MouseArea { id: reMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: session.forceRecheckSelected() }
                }
            }
        }

        Item {
            visible: body.hasCover
            Layout.preferredWidth: body.hasCover ? 104 : 0
            Layout.preferredHeight: 146
            Layout.alignment: Qt.AlignTop

            Rectangle {
                id: coverContent
                anchors.fill: parent
                color: "#161618"
                visible: false
                layer.enabled: true
                Image {
                    anchors.fill: parent
                    source: gen.win.fileUrl(gen.win.hasSel ? session.selectedPoster : "")
                    fillMode: Image.PreserveAspectCrop
                    asynchronous: true
                    cache: true
                    sourceSize: Qt.size(208, 292)
                }
            }
            Rectangle {
                id: coverMask
                anchors.fill: parent
                radius: 8
                color: "white"
                visible: false
                layer.enabled: true
            }
            MultiEffect {
                source: coverContent
                anchors.fill: parent
                maskEnabled: true
                maskSource: coverMask
            }
            Rectangle {
                anchors.fill: parent
                radius: 8
                color: "transparent"
                border.color: Theme.hair
                border.width: 1
            }
        }

        // .dmain — title, meta, progress, live transfer
        ColumnLayout {
            Layout.preferredWidth: gen.vertical ? -1 : 460
            Layout.maximumWidth: gen.vertical ? Number.POSITIVE_INFINITY : 460
            Layout.fillWidth: gen.vertical
            Layout.alignment: Qt.AlignTop
            spacing: 6

            Text {
                text: gen.win.hasSel ? (session.selectedMetaTitle.length > 0 ? session.selectedMetaTitle : session.selectedName) : (i18n.language, i18n.t("empty_no_selection"))
                color: Theme.t1
                font.pixelSize: 17
                font.weight: Font.DemiBold
                font.letterSpacing: -0.2
                font.family: Theme.fontSans
                elide: Text.ElideRight
                Layout.fillWidth: true
            }
            Text {
                visible: gen.win.hasSel && session.selectedMetaInfo.length > 0
                text: gen.win.hasSel ? session.selectedMetaInfo : ""
                color: Theme.t3
                font.pixelSize: 12
                font.family: Theme.fontSans
                elide: Text.ElideRight
                Layout.fillWidth: true
            }
            // progress bar — % (left) + downloaded / total (right)
            ColumnLayout {
                visible: gen.win.hasSel
                Layout.fillWidth: true
                Layout.topMargin: 10
                spacing: 6
                RowLayout {
                    Layout.fillWidth: true
                    Text {
                        text: Math.round((gen.win.hasSel ? session.selectedProgress : 0) * 100) + "%"
                        color: Theme.t1; font.pixelSize: 13; font.weight: Font.DemiBold; font.family: Theme.fontSans; font.features: Theme.tnum
                    }
                    Item { Layout.fillWidth: true }
                    Text {
                        text: gen.win.hasSel ? (session.selectedDownloaded.split(" (")[0] + "  /  " + session.selectedSize) : ""
                        color: Theme.t4; font.pixelSize: 12; font.family: Theme.fontSans; font.features: Theme.tnum
                    }
                }
                Rectangle {
                    Layout.fillWidth: true; height: 4; radius: 2; color: Theme.track
                    Rectangle {
                        height: parent.height; radius: 2
                        width: parent.width * (gen.win.hasSel ? session.selectedProgress : 0)
                        // same state→colour language as the grid tile and the
                        // status dot: downloading red, seeding amber, done green
                        color: gen.win.fillFor(gen.win.hasSel ? session.selectedStateKey : "")
                        Behavior on width { NumberAnimation { duration: 220; easing.type: Easing.OutCubic } }
                        Behavior on color { ColorAnimation { duration: 200 } }
                    }
                }
            }

            // big live transfer: DOWN · UP · ETA. Only the arrows carry the
            // direction colour; the rates read neutral
            RowLayout {
                visible: gen.win.hasSel
                Layout.fillWidth: true
                Layout.topMargin: 10
                spacing: Theme.sp6
                Repeater {
                    // the arrow is a fixed legend for the direction, not a live
                    // indicator — it stays red/amber whatever the rate. Dimming it
                    // at low speed made the panel look broken at 2 KB/s.
                    model: [
                        { lbl: (i18n.language, i18n.t("graph_download")), arrow: "↓ ", v: gen.win.hasSel ? session.selectedDownSpeed : "—", c: Theme.accent },
                        { lbl: (i18n.language, i18n.t("graph_upload")),   arrow: "↑ ", v: gen.win.hasSel ? session.selectedUpSpeed   : "—", c: Theme.amber },
                        { lbl: (i18n.language, i18n.t("col_eta")),        arrow: "",   v: gen.win.hasSel ? session.selectedEta       : "—", c: Theme.t1 }
                    ]
                    delegate: Column {
                        spacing: 3
                        Text { text: modelData.lbl; color: Theme.t4; font.pixelSize: 9; font.weight: Font.Bold; font.letterSpacing: 0.8; font.capitalization: Font.AllUppercase; font.family: Theme.fontSans }
                        // the arrow carries the direction colour, the number stays
                        // neutral — a whole reading in red/amber shouts louder than
                        // a rate deserves
                        Row {
                            spacing: 4
                            Text {
                                visible: modelData.arrow.length > 0
                                text: modelData.arrow
                                color: modelData.c
                                font.pixelSize: 14; font.weight: Font.DemiBold; font.family: Theme.fontSans
                            }
                            Text {
                                text: modelData.v
                                color: Theme.t1
                                font.pixelSize: 14; font.weight: Font.DemiBold; font.family: Theme.fontSans; font.features: Theme.tnum
                            }
                        }
                    }
                }
            }
        }

        Item { visible: !gen.vertical; Layout.fillWidth: true }

        // .dcols — three KV sections (row in the panel, stacked in the inspector)
        GridLayout {
            Layout.alignment: Qt.AlignTop
            Layout.fillWidth: gen.vertical
            columns: gen.vertical ? 1 : 3
            columnSpacing: Theme.sp6
            rowSpacing: Theme.sp4

            DetailKVSection {
                Layout.preferredWidth: gen.vertical ? -1 : 168
                Layout.fillWidth: gen.vertical
                Layout.alignment: Qt.AlignTop
                title: (i18n.language, i18n.t("detail_section_storage"))
                valueMaxWidth: gen.vertical ? 170 : 110
                model: [
                    { kk: "detail_kv_size",  v: gen.win.hasSel ? session.selectedSize : "—" },
                    { kk: "detail_kv_hash",  v: gen.win.hasSel ? session.selectedHash : "—" },
                    { kk: "detail_kv_added", v: gen.win.hasSel ? session.selectedAdded : "—" },
                    { kk: "detail_kv_path",  v: gen.win.hasSel ? session.selectedPath : "—" }
                ]
            }
            DetailKVSection {
                Layout.preferredWidth: gen.vertical ? -1 : 168
                Layout.fillWidth: gen.vertical
                Layout.alignment: Qt.AlignTop
                title: (i18n.language, i18n.t("detail_section_transfer"))
                valueMaxWidth: gen.vertical ? 170 : 110
                model: [
                    { kk: "detail_kv_downloaded", v: gen.win.hasSel ? session.selectedDownloaded.split(" (")[0] : "—" },
                    { kk: "detail_kv_uploaded",   v: gen.win.hasSel ? session.selectedUploaded : "—" },
                    { kk: "detail_kv_ratio",      v: gen.win.hasSel ? session.selectedRatio : "—" }
                ]
            }
            DetailKVSection {
                Layout.preferredWidth: gen.vertical ? -1 : 168
                Layout.fillWidth: gen.vertical
                Layout.alignment: Qt.AlignTop
                title: (i18n.language, i18n.t("detail_section_health"))
                valueMaxWidth: gen.vertical ? 170 : 110
                model: [
                    { kk: "detail_kv_seeds",        v: gen.win.hasSel ? String(session.selectedSeeds) : "—" },
                    { kk: "detail_kv_peers",        v: gen.win.hasSel ? String(session.selectedPeers) : "—" },
                    { kk: "detail_kv_availability", v: gen.win.hasSel ? session.selectedAvailability : "—" }
                ]
            }
        }
    }
}
