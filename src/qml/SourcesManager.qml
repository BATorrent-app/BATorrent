// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Search-source manager: toggle the installed JSON torrent providers and add
// localized ones from a curated catalog, grouped by region. `sv` is the owning
// SearchView (showSourcesMgr, api). Provider data comes from the `addons` bridge.
import QtQuick
import QtQuick.Layouts
import "theme"
import "widgets"

Rectangle {
    id: root
    property var sv

    readonly property var api: typeof addons !== "undefined" ? addons : null

    // region tag → localized display label (+ a leading glyph for scanability)
    function regionLabel(r) {
        if (r === "ptbr") return "🇧🇷  " + i18n.t("src_region_ptbr")
        if (r === "cis") return "🇷🇺  " + i18n.t("src_region_cis")
        if (r === "es") return "🇪🇸  " + i18n.t("src_region_es")
        if (r === "anime") return "🌸  " + i18n.t("src_region_anime")
        if (r === "self") return "🛠  " + i18n.t("src_region_self")
        return "🌐  " + i18n.t("src_region_global")
    }

    // catalog rows grouped into [{region, items:[...]}], regions in a stable order
    readonly property var catalogGroups: {
        if (!api) return []
        var order = ["ptbr", "cis", "es", "anime", "self", "global"]
        var byReg = {}
        var cat = api.sourceCatalog
        for (var i = 0; i < cat.length; i++) {
            var r = cat[i].region || "global"
            if (!byReg[r]) byReg[r] = []
            byReg[r].push(cat[i])
        }
        var out = []
        for (var k = 0; k < order.length; k++)
            if (byReg[order[k]]) out.push({ region: order[k], items: byReg[order[k]] })
        return out
    }

    anchors.fill: parent
    visible: root.sv.showSourcesMgr
    color: "#aa000000"
    MouseArea { anchors.fill: parent; onClicked: root.sv.showSourcesMgr = false }

    Rectangle {
        anchors.centerIn: parent
        width: Math.min(parent.width - 48, 560)
        height: Math.min(parent.height - 48, 560)
        radius: 12
        color: Theme.elev
        border.color: Theme.hair
        border.width: 1
        MouseArea { anchors.fill: parent }   // swallow clicks so the dim layer doesn't close it

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: Theme.sp3

            Text { text: (i18n.language, i18n.t("src_title")); color: Theme.t1; font.pixelSize: 15; font.weight: Font.DemiBold; font.family: Theme.fontSans }
            Text {
                Layout.fillWidth: true
                text: (i18n.language, i18n.t("src_hint"))
                color: Theme.t4; font.pixelSize: 11; font.family: Theme.fontSans
                wrapMode: Text.WordWrap
            }

            Flickable {
                id: flick
                Layout.fillWidth: true
                Layout.fillHeight: true
                contentHeight: body.implicitHeight
                clip: true
                boundsBehavior: Flickable.StopAtBounds
                WheelScroller { flick: flick }

                ColumnLayout {
                    id: body
                    width: flick.width
                    spacing: Theme.sp3

                    // ── installed sources ────────────────────────────────────
                    Text {
                        text: (i18n.language, i18n.t("src_installed"))
                        color: Theme.t3; font.pixelSize: 10; font.weight: Font.Bold
                        font.letterSpacing: 0.8; font.family: Theme.fontSans
                        Layout.topMargin: 4
                    }
                    Repeater {
                        model: root.api ? root.api.searchProviders : []
                        delegate: Rectangle {
                            required property var modelData
                            Layout.fillWidth: true
                            implicitHeight: 46
                            radius: 8
                            color: Theme.field
                            border.color: Theme.hair; border.width: 1
                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 12; anchors.rightMargin: 8
                                spacing: Theme.sp3
                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 1
                                    Text {
                                        text: modelData.name
                                        color: Theme.t1; font.pixelSize: 13; font.family: Theme.fontSans
                                        elide: Text.ElideRight; Layout.fillWidth: true
                                    }
                                    Text {
                                        visible: (modelData.note || "").length > 0
                                        text: modelData.note || ""
                                        color: Theme.t4; font.pixelSize: 10; font.family: Theme.fontSans
                                        elide: Text.ElideRight; Layout.fillWidth: true
                                    }
                                }
                                Text {
                                    text: root.regionLabel(modelData.region)
                                    color: Theme.t3; font.pixelSize: 10; font.weight: Font.DemiBold; font.family: Theme.fontSans
                                }
                                TToggle {
                                    on: modelData.enabled
                                    onToggled: function (v) { if (root.api) root.api.setSearchProviderEnabled(modelData.index, v) }
                                }
                                IconImg {
                                    src: "qrc:/icons/close.svg"; s: 14
                                    tint: rmMa.containsMouse ? Theme.accent : Theme.t4
                                    visible: !modelData.builtIn
                                    MouseArea {
                                        id: rmMa; anchors.fill: parent; anchors.margins: -6
                                        hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                                        onClicked: if (root.api) root.api.removeSearchProvider(modelData.index)
                                    }
                                }
                            }
                        }
                    }

                    // ── curated catalog, grouped by region ───────────────────
                    Text {
                        visible: root.catalogGroups.length > 0
                        text: (i18n.language, i18n.t("src_add_localized"))
                        color: Theme.t3; font.pixelSize: 10; font.weight: Font.Bold
                        font.letterSpacing: 0.8; font.family: Theme.fontSans
                        Layout.topMargin: 10
                    }
                    Repeater {
                        model: root.catalogGroups
                        delegate: ColumnLayout {
                            required property var modelData
                            Layout.fillWidth: true
                            spacing: Theme.sp2
                            Text {
                                text: root.regionLabel(modelData.region)
                                color: Theme.t2; font.pixelSize: 12; font.weight: Font.DemiBold; font.family: Theme.fontSans
                                Layout.topMargin: 4
                            }
                            Repeater {
                                model: modelData.items
                                delegate: Rectangle {
                                    required property var modelData
                                    Layout.fillWidth: true
                                    implicitHeight: noteTxt.visible ? 50 : 38
                                    radius: 8
                                    color: "transparent"
                                    border.color: Theme.hairSoft; border.width: 1
                                    RowLayout {
                                        anchors.fill: parent
                                        anchors.leftMargin: 12; anchors.rightMargin: 8
                                        spacing: Theme.sp3
                                        ColumnLayout {
                                            Layout.fillWidth: true
                                            spacing: 1
                                            Text {
                                                text: modelData.name
                                                color: Theme.t1; font.pixelSize: 12; font.family: Theme.fontSans
                                                elide: Text.ElideRight; Layout.fillWidth: true
                                            }
                                            Text {
                                                id: noteTxt
                                                visible: (modelData.note || "").length > 0
                                                text: modelData.note || ""
                                                color: Theme.t4; font.pixelSize: 10; font.family: Theme.fontSans
                                                wrapMode: Text.WordWrap; Layout.fillWidth: true
                                            }
                                        }
                                        BtnFlat {
                                            text: modelData.needsConfig ? (i18n.language, i18n.t("src_configure"))
                                                                        : (i18n.language, i18n.t("src_add"))
                                            onClicked: if (root.api) root.api.addCatalogSource(modelData.id)
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Text {
                    Layout.fillWidth: true
                    text: (i18n.language, i18n.t("src_footer_note"))
                    color: Theme.t4; font.pixelSize: 10; font.family: Theme.fontSans
                    wrapMode: Text.WordWrap
                }
                BtnFlat { primary: true; text: (i18n.language, i18n.t("release_notes_close")); onClicked: root.sv.showSourcesMgr = false }
            }
        }
    }
}
