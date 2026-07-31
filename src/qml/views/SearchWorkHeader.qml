// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Picked-work header — which title (and type) these releases belong to.
import QtQuick
import QtQuick.Layouts
import "../theme"
import "../widgets"

Rectangle {
    id: root
    required property var sv

    Layout.fillWidth: true
    Layout.preferredHeight: 62
    visible: !sv.browse && sv.api && sv.api.singleTitleView && !sv.isEpisodes
             && (sv.api.workTitle || "").length > 0
    color: "transparent"
    Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hairSoft }
    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Theme.sp5
        anchors.rightMargin: Theme.sp5
        spacing: Theme.sp4
        PosterThumb {
            Layout.alignment: Qt.AlignVCenter
            implicitWidth: 34; implicitHeight: 46
            posterUrl: root.sv.fileUrl(root.sv.api.workPoster || "")
            label: root.sv.api.workTitle
        }
        ColumnLayout {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter
            spacing: 2
            Text {
                Layout.fillWidth: true
                text: root.sv.api.workTitle
                color: Theme.t1; font.pixelSize: 15; font.weight: Font.Bold; font.family: Theme.fontSans
                elide: Text.ElideRight
            }
            Text {
                text: {
                    var parts = []
                    if ((root.sv.api.workYear || "").length > 0) parts.push(root.sv.api.workYear)
                    var t = root.sv.typeLabel(root.sv.api.workType || "")
                    if (t.length > 0) parts.push(t)
                    return parts.join("  ·  ")
                }
                color: Theme.t3; font.pixelSize: 11; font.weight: Font.DemiBold; font.family: Theme.fontSans
            }
        }
    }
}
