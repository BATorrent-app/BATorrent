// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Who the selected torrent is: cover, title, what it is, progress, live rates.
//
// Separate from the tabs so both detail surfaces keep it on screen while the
// tabs swap: switching to Peers or Files keeps the cover and title in sight.
// vertical (the 340px inspector): the cover beside title and actions, then
// progress and rates under both. Horizontal (the bottom panel, 440px): the
// cover beside a column with the title on top and progress and rates at its
// foot, as tall as the cover.
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Effects
import "../theme"

Item {
    id: ident
    property var win
    property bool vertical: false
    readonly property bool hasSel: ident.win && ident.win.hasSel
    readonly property bool hasCover: hasSel && session.selectedPoster.length > 0
    Layout.fillHeight: false
    implicitHeight: vertical ? vCol.implicitHeight : 142

    // Read when the selection changes, not inside the bindings: the call can
    // reach into the engine process, and a binding that re-enters itself
    // while it waits reports a loop.
    property string selCat: ""
    function readCat() { selCat = hasSel ? session.selectedCategory() : "" }
    Component.onCompleted: readCat()
    onHasSelChanged: readCat()
    Connections {
        target: typeof session !== "undefined" ? session : null
        function onSelectionChanged() { ident.readCat() }
    }
    // "Mark as watched / played / done", by what the item is
    readonly property string markKey: {
        if (!hasSel) return "ctx_mark_completed_plain"
        var c = selCat
        if (c === "Games") return "mark_played"
        if (c === "Movies" || c === "Series" || session.selectedHasVideo) return "mark_watched"
        return "ctx_mark_completed_plain"
    }
    readonly property string metaLine: {
        if (!hasSel) return ""
        var parts = []
        if (session.selectedMetaInfo.length > 0) parts.push(session.selectedMetaInfo)
        if (selCat.length > 0) parts.push(ident.win.catLabel(selCat))
        return parts.join(" · ")
    }

    component Cover: Item {
        Item {
            id: coverContent
            anchors.fill: parent
            visible: false
            layer.enabled: true
            Rectangle { anchors.fill: parent; color: Theme.tileBlank }
            Image {
                anchors.fill: parent
                source: ident.win ? ident.win.fileUrl(ident.hasSel ? session.selectedPoster : "") : ""
                fillMode: Image.PreserveAspectCrop
                asynchronous: true
                cache: true
                sourceSize: Qt.size(208, 292)
            }
        }
        Rectangle { id: coverMask; anchors.fill: parent; radius: Theme.rad(7); visible: false; layer.enabled: true }
        MultiEffect { source: coverContent; anchors.fill: parent; maskEnabled: true; maskSource: coverMask }
        Rectangle { anchors.fill: parent; radius: Theme.rad(7); color: "transparent"; border.color: Theme.hair; border.width: 1 }
    }
    component Title: Text {
        text: ident.hasSel ? (session.selectedMetaTitle.length > 0 ? session.selectedMetaTitle : session.selectedName)
                           : (i18n.language, i18n.t("empty_no_selection"))
        color: Theme.t1
        font.pixelSize: Theme.displaySize(17)
        font.weight: Theme.tWeight(Font.DemiBold)
        font.family: Theme.fontDisplay
        font.capitalization: Theme.tCaps
        font.italic: Theme.tItalic
        font.letterSpacing: Theme.tTrack(17, 0)
        lineHeight: 1.15
        wrapMode: Text.WordWrap
        maximumLineCount: 2
        elide: Text.ElideRight
    }
    component Meta: Text {
        visible: text.length > 0
        text: ident.metaLine
        color: Theme.t3
        font.pixelSize: 12
        font.family: Theme.fontSans
        elide: Text.ElideRight
    }
    component ProgressBlock: ColumnLayout {
        spacing: 6
        RowLayout {
            Layout.fillWidth: true
            Text {
                text: Math.round((ident.hasSel ? session.selectedProgress : 0) * 100) + "%"
                color: Theme.t1; font.pixelSize: 13; font.weight: Font.DemiBold; font.family: Theme.fontSans; font.features: Theme.tnum
            }
            Item { Layout.fillWidth: true }
            Text {
                text: !ident.hasSel ? ""
                      : session.selectedProgress >= 0.999 ? session.selectedSize
                      : (session.selectedDownloaded.split(" (")[0] + " / " + session.selectedSize)
                color: Theme.t4; font.pixelSize: 12; font.family: Theme.fontSans; font.features: Theme.tnum
            }
        }
        ProgressTrack {
            Layout.fillWidth: true
            Layout.preferredHeight: 4
            progress: ident.hasSel ? session.selectedProgress : 0
            stateKey: ident.hasSel ? session.selectedStateKey : ""
            fill: ident.hasSel && session.selectedStateKey === "downloading" ? Theme.down : Theme.fillFor(stateKey)
        }
    }
    // DOWN · UP · ETA. The arrows carry the direction colour, the rates stay neutral.
    component Rates: RowLayout {
        spacing: 12
        Repeater {
            model: [
                { lbl: (i18n.language, i18n.t("graph_download")), arrow: "↓ ", v: ident.hasSel ? session.selectedDownSpeed : "—", c: Theme.down },
                { lbl: (i18n.language, i18n.t("graph_upload")),   arrow: "↑ ", v: ident.hasSel ? session.selectedUpSpeed   : "—", c: Theme.amber },
                { lbl: (i18n.language, i18n.t("col_eta")),        arrow: "",   v: ident.hasSel && session.selectedEta.length > 0 ? session.selectedEta : "—", c: Theme.t1 }
            ]
            delegate: Column {
                required property var modelData
                required property int index
                Layout.fillWidth: index < 2
                Layout.preferredWidth: index < 2 ? -1 : 56
                spacing: 2
                Row {
                    spacing: 0
                    Text { visible: modelData.arrow.length > 0; text: modelData.arrow; color: modelData.c; font.pixelSize: 9; font.weight: Font.DemiBold; font.family: Theme.fontSans }
                    Text { text: modelData.lbl; color: Theme.t4; font.pixelSize: 9; font.weight: Font.DemiBold; font.letterSpacing: 0.8; font.capitalization: Font.AllUppercase; font.family: Theme.fontSans }
                }
                Text {
                    text: modelData.v
                    color: Theme.t1
                    font.pixelSize: ident.vertical ? 14 : 14
                    font.weight: ident.vertical ? Font.DemiBold : Font.Medium
                    font.family: Theme.fontSans
                    font.features: Theme.tnum
                }
            }
        }
    }
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
        radius: Theme.rad(8)
        color: ib.active ? Qt.rgba(Theme.grn.r, Theme.grn.g, Theme.grn.b, 0.12)
                         : (ibMa.containsMouse ? Theme.hover2 : Theme.elev)
        border.color: ib.active ? Qt.rgba(Theme.grn.r, Theme.grn.g, Theme.grn.b, 0.42) : Theme.hair
        border.width: 1
        scale: ibMa.pressed ? Theme.pressScale : 1
        Behavior on color { ColorAnimation { duration: Theme.durFast } }
        Behavior on scale { NumberAnimation { duration: Theme.durFast; easing.type: Theme.easeOut } }
        IconImg {
            visible: ib.icon.length > 0
            anchors.centerIn: parent
            src: ib.icon; s: 14
            tint: ib.active ? Theme.grn : (ibMa.containsMouse ? Theme.t1 : Theme.t2)
        }
        Text {
            visible: ib.label.length > 0
            anchors.fill: parent
            anchors.leftMargin: 8
            anchors.rightMargin: 8
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignHCenter
            text: ib.label
            color: ib.active ? Theme.grn : (ibMa.containsMouse ? Theme.t1 : Theme.t2)
            font.pixelSize: 11
            font.weight: Font.DemiBold
            font.family: Theme.fontSans
            elide: Text.ElideRight
        }
        MouseArea { id: ibMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: ib.clicked() }
        ToolTip.visible: ibMa.containsMouse && ib.tip.length > 0
        ToolTip.text: ib.tip
        ToolTip.delay: 400
    }

    // ---- inspector: cover + title/actions, then progress and rates ----
    ColumnLayout {
        id: vCol
        visible: ident.vertical
        width: parent.width
        spacing: 14
        RowLayout {
            Layout.fillWidth: true
            spacing: 16
            Cover {
                visible: ident.hasCover
                Layout.preferredWidth: 104
                Layout.preferredHeight: 146
            }
            ColumnLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: 146
                spacing: 4
                Title { Layout.fillWidth: true }
                Meta { Layout.fillWidth: true }
                Item { Layout.fillHeight: true }
                RowLayout {
                    visible: ident.hasSel
                    Layout.fillWidth: true
                    spacing: 5
                    IdentBtn {
                        wide: true
                        active: ident.hasSel && session.selectedCompleted
                        label: (i18n.language, ident.hasSel && session.selectedCompleted
                                ? i18n.t("ctx_unmark_completed_plain") : i18n.t(ident.markKey))
                        tip: label
                        onClicked: session.selectedCompleted ? session.unmarkSelectedCompleted() : session.markSelectedCompleted()
                    }
                    IdentBtn { icon: "qrc:/icons/folder.svg"; tip: (i18n.language, i18n.t("ctx_open_folder")); onClicked: session.openSaveFolder() }
                    IdentBtn {
                        readonly property bool link: session.selectedCopyKind === "link"
                        icon: link ? "qrc:/icons/link.svg" : "qrc:/icons/magnet.svg"
                        tip: (i18n.language, i18n.t(link ? "ctx_copy_link" : "ctx_copy_magnet"))
                        onClicked: session.copyMagnetLink()
                    }
                }
            }
        }
        ProgressBlock { visible: ident.hasSel; Layout.fillWidth: true }
        Rates { visible: ident.hasSel; Layout.fillWidth: true }
    }

    // ---- bottom panel: cover beside a column as tall as the cover ----
    RowLayout {
        visible: !ident.vertical
        anchors.fill: parent
        spacing: 18
        Cover {
            visible: ident.hasCover
            Layout.preferredWidth: 100
            Layout.preferredHeight: 142
            Layout.alignment: Qt.AlignTop
        }
        ColumnLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 142
            spacing: 3
            Title { Layout.fillWidth: true }
            Meta { Layout.fillWidth: true }
            Item { Layout.fillHeight: true; Layout.minimumHeight: 8 }
            ProgressBlock { visible: ident.hasSel; Layout.fillWidth: true }
            Rates { visible: ident.hasSel; Layout.fillWidth: true; Layout.topMargin: 3 }
        }
    }
}
