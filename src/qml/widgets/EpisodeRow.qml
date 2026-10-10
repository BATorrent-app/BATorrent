// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// One episode on the series screen.
//
// The second line is the point. Stremio can only show an air date there, since
// it has no library and every episode is equally hypothetical until you click.
// We know which ones are on this disk, so the row says whether it plays right
// now, is still coming down, or is not here at all — and the synopsis takes
// that line on hover, so the information is reachable without turning the list
// into a wall of text.
import QtQuick
import QtQuick.Layouts
import "../theme"

Rectangle {
    id: row
    property var ep
    property bool isNext: false
    // What having an episode means depends on who is asking: on this disk in
    // the library, offered by an indexer in a search. The row should not have
    // an opinion about which.
    property string haveLabel: (i18n.language, i18n.t("hub_ep_ready"))
    property string missingLabel: (i18n.language, i18n.t("hub_ep_missing"))
    signal playRequested()

    readonly property bool ready: ep.have && (ep.progress || 0) >= 1
    readonly property bool coming: ep.have && (ep.progress || 0) < 1

    implicitHeight: 92
    color: rowMa.containsMouse && row.ep.have ? Theme.hover : "transparent"
    Behavior on color { ColorAnimation { duration: Theme.durFast } }
    opacity: row.ep.have ? 1 : 0.42

    // Where you left off. The show should say that without being asked.
    Rectangle {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: 2
        visible: row.isNext
        color: Theme.accent
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 10
        anchors.leftMargin: 14
        spacing: 12

        Rectangle {
            Layout.preferredWidth: 124
            Layout.preferredHeight: 70
            radius: 7
            color: Theme.track
            clip: true
            Image {
                id: still
                anchors.fill: parent
                source: row.ep.still || ""
                fillMode: Image.PreserveAspectCrop
                asynchronous: true
                cache: true
                visible: status === Image.Ready
                opacity: row.ep.watched ? 0.55 : 1
            }
            IconImg {
                anchors.centerIn: parent
                visible: still.status !== Image.Ready
                src: "qrc:/icons/play.svg"
                tint: Theme.t4
                s: 17
            }
            Rectangle {
                anchors.centerIn: parent
                visible: row.ep.have && rowMa.containsMouse
                width: 32; height: 32; radius: 16
                color: "#cc101014"
                border.width: 1
                border.color: Theme.accent
                IconImg {
                    anchors.centerIn: parent
                    anchors.horizontalCenterOffset: 1
                    src: "qrc:/icons/play.svg"; tint: "#ffffff"; s: 14
                }
            }
            // A partial episode shows how partial, on the thumbnail itself.
            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: 3
                visible: row.coming
                color: Qt.rgba(1, 1, 1, 0.18)
                Rectangle {
                    anchors.left: parent.left
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    width: parent.width * Math.max(0.02, row.ep.progress || 0)
                    color: Theme.accent
                }
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 4

            Text {
                Layout.fillWidth: true
                text: (row.ep.watched ? "✓  " : "") + row.ep.episode + ". "
                      + (row.ep.title.length > 0 ? row.ep.title
                         : (i18n.language, i18n.t("hub_ep_n")).arg(row.ep.episode))
                color: Theme.t1
                font.pixelSize: 13
                font.weight: row.isNext ? Font.DemiBold : Font.Medium
                font.family: Theme.fontSans
                elide: Text.ElideRight
            }

            // One line, two jobs: what you can do with this episode, and —
            // while the cursor is on it — what it is about.
            Text {
                Layout.fillWidth: true
                Layout.preferredHeight: 32
                text: {
                    if (rowMa.containsMouse && (row.ep.overview || "").length > 0)
                        return row.ep.overview
                    var bits = []
                    if (row.ready)       bits.push(row.haveLabel)
                    else if (row.coming) bits.push((i18n.language, i18n.t("hub_ep_coming"))
                                                   .arg(Math.floor((row.ep.progress || 0) * 100)))
                    else                 bits.push(row.missingLabel)
                    if (row.ep.runtime > 0)
                        bits.push((i18n.language, i18n.t("hub_ep_minutes")).arg(row.ep.runtime))
                    if (row.ep.airDate.length >= 4) bits.push(row.ep.airDate.substring(0, 4))
                    return bits.join("  ·  ")
                }
                color: row.ready && !rowMa.containsMouse ? Theme.t2 : Theme.t3
                font.pixelSize: 12
                font.family: Theme.fontSans
                wrapMode: Text.WordWrap
                elide: Text.ElideRight
                maximumLineCount: 2
            }
        }
    }

    Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hairSoft }

    MouseArea {
        id: rowMa
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: row.ep.have ? Qt.PointingHandCursor : Qt.ArrowCursor
        onClicked: if (row.ep.have) row.playRequested()
    }
}
