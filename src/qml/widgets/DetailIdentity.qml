// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Who the selected torrent is: cover, title, category, progress, live rates.
//
// Pulled out of DetailGeneral so the side inspector can keep it on screen while
// the tabs below swap. Switching to Peers or Files used to replace the whole
// pane, poster included, so you lost sight of which torrent you were reading.
// The bottom deck is only 270px tall and has no room for a fixed header, so it
// still renders this inside its General tab.
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Effects
import "../theme"

ColumnLayout {
    id: ident
    property var win
    property bool vertical: false
    // Was ident.hasCover, an id that lived on DetailGeneral's grid.
    readonly property bool hasCover: ident.win && ident.win.hasSel
                                     && session.selectedPoster.length > 0
    // A layout nested in a layout defaults to fillHeight TRUE, so this block was
    // splitting the sidebar's spare height with the tab panes. That was the gap,
    // and it survived every fix aimed one level lower.
    Layout.fillHeight: false
    spacing: 6

    // One definition, two homes. Stacked under the cover it reads as the
    // panel's headline; beside it, in the wide deck, it belongs to the title
    // block like any other fact about the release. Which one shows is the
    // only difference, so the two placements cannot drift apart.
    component ProgressBlock: ColumnLayout {
        spacing: 6
        RowLayout {
            Layout.fillWidth: true
            Text {
                text: Math.round((ident.win.hasSel ? session.selectedProgress : 0) * 100) + "%"
                color: Theme.t1; font.pixelSize: 13; font.weight: Font.DemiBold; font.family: Theme.fontSans; font.features: Theme.tnum
            }
            Item { Layout.fillWidth: true }
            Text {
                text: ident.win.hasSel ? (session.selectedDownloaded.split(" (")[0] + "  /  " + session.selectedSize) : ""
                color: Theme.t4; font.pixelSize: 12; font.family: Theme.fontSans; font.features: Theme.tnum
            }
        }
        ProgressTrack {
            Layout.fillWidth: true
            Layout.preferredHeight: 4
            progress: ident.win.hasSel ? session.selectedProgress : 0
            stateKey: ident.win.hasSel ? session.selectedStateKey : ""
        }
    }


        // Cover and the title that names it read as one unit. Stacked, the
        // sidebar spent its first 150px on art with the title pushed below
        // the fold; the progress bar stays full width under both, since it
        // is the thing you actually come here to read.
        RowLayout {
            Layout.fillWidth: true
            // No explicit height on purpose. A RowLayout is already as tall as
            // its tallest child, so the cover's 146 is the floor and the text
            // column raises it when a long title needs more. Setting the height
            // from identCol.implicitHeight made the row depend on a column that
            // depends on the row, and that self-reference inflated it.
            spacing: Theme.sp4

        Item {
            visible: ident.hasCover
            Layout.preferredWidth: ident.hasCover ? 104 : 0
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
                    source: ident.win.fileUrl(ident.win.hasSel ? session.selectedPoster : "")
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

            ColumnLayout {
                id: identCol
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 4

                Text {
                    text: ident.win.hasSel ? (session.selectedMetaTitle.length > 0 ? session.selectedMetaTitle : session.selectedName) : (i18n.language, i18n.t("empty_no_selection"))
                    color: Theme.t1
                    font.pixelSize: 17
                    font.weight: Font.DemiBold
                    font.letterSpacing: -0.2
                    font.family: Theme.fontSans
                    wrapMode: Text.WordWrap
                    maximumLineCount: 3
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }
                Text {
                    visible: ident.win.hasSel && session.selectedMetaInfo.length > 0
                    text: ident.win.hasSel ? session.selectedMetaInfo : ""
                    color: Theme.t3
                    font.pixelSize: 12
                    font.family: Theme.fontSans
                    wrapMode: Text.WordWrap
                    maximumLineCount: 2
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }
                // Category had no surface here at all, despite being the one
                // field the user sets by hand.
                Rectangle {
                    visible: ident.win.hasSel && session.selectedCategory().length > 0
                    Layout.topMargin: 2
                    implicitWidth: catLbl.implicitWidth + 14
                    implicitHeight: 18
                    radius: 9
                    color: Theme.hover
                    Text {
                        id: catLbl
                        anchors.centerIn: parent
                        text: ident.win.hasSel ? ident.win.catLabel(session.selectedCategory()) : ""
                        color: Theme.t2
                        font.pixelSize: 9
                        font.weight: Font.Bold
                        font.letterSpacing: 0.8
                        font.capitalization: Font.AllUppercase
                        font.family: Theme.fontSans
                    }
                }

                Item {
                    visible: ident.vertical
                    Layout.fillHeight: true
                    Layout.minimumHeight: 2
                }

                RowLayout {
                    visible: ident.vertical
                    Layout.fillWidth: true
                    Layout.topMargin: 4
                    spacing: 5

                    component IdentBtn: Rectangle {
                        id: ib
                        property string icon
                        property string label
                        property string tip
                        property bool wide: false
                        property bool active: false
                        signal clicked()
                        Layout.fillWidth: ib.wide
                        Layout.preferredWidth: ib.wide ? -1 : 28
                        Layout.preferredHeight: 28
                        radius: 8
                        color: ib.active
                            ? Qt.rgba(Theme.grn.r, Theme.grn.g, Theme.grn.b, 0.12)
                            : (ibMa.containsMouse ? Theme.hover : Theme.elev)
                        border.color: ib.active
                            ? Qt.rgba(Theme.grn.r, Theme.grn.g, Theme.grn.b, 0.42)
                            : Theme.hair
                        border.width: 1
                        scale: ibMa.pressed ? Theme.pressScale : 1
                        Behavior on color { ColorAnimation { duration: 130 } }
                        Behavior on scale { NumberAnimation { duration: Theme.durFast; easing.type: Easing.OutCubic } }
                        IconImg {
                            visible: ib.icon.length > 0
                            anchors.centerIn: parent
                            src: ib.icon; s: 15
                            tint: ib.active ? Theme.grn
                                  : (ibMa.containsMouse ? Theme.t1 : Theme.t2)
                        }
                        Text {
                            visible: ib.label.length > 0
                            anchors.fill: parent
                            anchors.leftMargin: 10
                            anchors.rightMargin: 10
                            verticalAlignment: Text.AlignVCenter
                            horizontalAlignment: Text.AlignHCenter
                            text: ib.label
                            color: ib.active ? Theme.grn
                                  : (ibMa.containsMouse ? Theme.t1 : Theme.t2)
                            font.pixelSize: 11
                            font.weight: Font.DemiBold
                            font.family: Theme.fontSans
                            elide: Text.ElideRight
                        }
                        MouseArea {
                            id: ibMa
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: ib.clicked()
                        }
                        ToolTip.visible: ibMa.containsMouse && ib.tip.length > 0
                        ToolTip.text: ib.tip
                        ToolTip.delay: 400
                    }

                    IdentBtn {
                        wide: true
                        active: session.selectedCompleted
                        label: session.selectedCompleted
                            ? (i18n.language, i18n.t("ctx_unmark_completed_plain"))
                            : (i18n.language, i18n.t("ctx_mark_completed_plain"))
                        tip: label
                        onClicked: session.selectedCompleted
                            ? session.unmarkSelectedCompleted()
                            : session.markSelectedCompleted()
                    }
                    IdentBtn {
                        icon: "qrc:/icons/folder.svg"
                        tip: (i18n.language, i18n.t("ctx_open_folder"))
                        onClicked: session.openSaveFolder()
                    }
                    IdentBtn {
                        icon: "qrc:/icons/copy.svg"
                        tip: (i18n.language, i18n.t("tb_copy"))
                        onClicked: session.copyMagnetLink()
                    }

                ProgressBlock {
                    visible: !ident.vertical && ident.win.hasSel
                    Layout.fillWidth: true
                    Layout.topMargin: 6
                }
                }
            }
        }
        ProgressBlock {
            visible: ident.vertical && ident.win.hasSel
            Layout.fillWidth: true
            Layout.topMargin: 10
        }

        // big live transfer: DOWN · UP · ETA. Only the arrows carry the
        // direction colour; the rates read neutral
        RowLayout {
            visible: ident.win.hasSel
            Layout.fillWidth: true
            Layout.topMargin: 10
            spacing: Theme.sp4
            Repeater {
                // the arrow is a fixed legend for the direction, not a live
                // indicator — it stays red/amber whatever the rate. Dimming it
                // at low speed made the panel look broken at 2 KB/s.
                model: [
                    { lbl: (i18n.language, i18n.t("graph_download")), arrow: "↓ ", v: ident.win.hasSel ? session.selectedDownSpeed : "—", c: Theme.accent },
                    { lbl: (i18n.language, i18n.t("graph_upload")),   arrow: "↑ ", v: ident.win.hasSel ? session.selectedUpSpeed   : "—", c: Theme.amber },
                    { lbl: (i18n.language, i18n.t("col_eta")),        arrow: "",   v: ident.win.hasSel ? session.selectedEta       : "—", c: Theme.t1 }
                ]
                delegate: Column {
                    // Each takes an equal share instead of bunching against
                    // the left edge with the rest of the row left empty.
                    Layout.fillWidth: true
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
