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

        // .dcover (radius 8 — mask via MultiEffect)
        Item {
            Layout.preferredWidth: 104
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
                visible: gen.win.hasSel && session.selectedPoster.length > 0
            }
            // placeholder logo (no poster / no selection)
            Image {
                anchors.centerIn: parent
                visible: !(gen.win.hasSel && session.selectedPoster.length > 0)
                width: 52; height: 52
                source: "qrc:/images/logo.svg"
                sourceSize: Qt.size(104, 104)
                fillMode: Image.PreserveAspectFit
                opacity: 0.4
                layer.enabled: Theme.isLight
                layer.effect: MultiEffect { colorization: 1.0; colorizationColor: Theme.t1 }
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
                        // red exists at exactly one intensity: while the
                        // download is moving. A finished bar goes neutral —
                        // translucent accent over a dark panel reads as a
                        // third, muddy red, not a quieter one; "done" already
                        // lives in the ✓ badge.
                        color: (gen.win.hasSel && session.selectedProgress >= 0.999)
                               ? Qt.rgba(1, 1, 1, 0.16)
                               : Theme.accent
                        Behavior on width { NumberAnimation { duration: 220; easing.type: Easing.OutCubic } }
                        Behavior on color { ColorAnimation { duration: 200 } }
                    }
                }
            }

            // big live transfer: DOWN (accent) · UP (amber) · ETA — the one
            // place per-direction color earns its keep (single instance)
            RowLayout {
                visible: gen.win.hasSel
                Layout.fillWidth: true
                Layout.topMargin: 10
                spacing: Theme.sp6
                Repeater {
                    // per-direction colour only while that direction is actually
                    // moving (>1 KB/s) — an idle 2 KB/s trickle in bold red is
                    // inverted hierarchy; at rest the numbers read neutral
                    model: [
                        { lbl: (i18n.language, i18n.t("graph_download")), arrow: "↓ ", v: gen.win.hasSel ? session.selectedDownSpeed : "—", c: Theme.accent,
                          on: gen.win.hasSel && session.selectedDownRate > 1024 },
                        { lbl: (i18n.language, i18n.t("graph_upload")),   arrow: "↑ ", v: gen.win.hasSel ? session.selectedUpSpeed   : "—", c: Theme.amber,
                          on: gen.win.hasSel && session.selectedUpRate > 1024 },
                        { lbl: (i18n.language, i18n.t("col_eta")),        arrow: "",   v: gen.win.hasSel ? session.selectedEta       : "—", c: Theme.t1, on: true }
                    ]
                    delegate: Column {
                        spacing: 3
                        Text { text: modelData.lbl; color: Theme.t4; font.pixelSize: 9; font.weight: Font.Bold; font.letterSpacing: 0.8; font.capitalization: Font.AllUppercase; font.family: Theme.fontSans }
                        Text {
                            text: modelData.arrow + modelData.v
                            color: modelData.on ? modelData.c : Theme.t3
                            font.pixelSize: 14; font.weight: Font.DemiBold; font.family: Theme.fontSans; font.features: Theme.tnum
                            Behavior on color { ColorAnimation { duration: 200 } }
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
