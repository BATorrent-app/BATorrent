// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// What the search page says when there is nothing to show, and always a next
// step from there:
//   · searching          → spinner + "searching…"
//   · offline            → what still works, Try again (and every 30 s by itself)
//   · catalog down       → the internet is fine, search the sources directly
//   · no titles          → the nearest real title if it was a typo, the sources, a magnet
//   · none of this type  → how many of the other types, Show all
//   · torrent list, 0 hits → the bridge's status
//   · initial (no query) → "type something"
// Shared by the flat-list and titles-grid views. `sv` is the owning SearchView.
import QtQuick
import QtQuick.Layouts
import "../theme"
import "../widgets"

Item {
    id: root
    property var sv
    readonly property var api: sv ? sv.api : null
    readonly property bool searching: !!api && api.searching
    readonly property string query: api ? api.activeQuery : ""
    readonly property bool asked: !searching && !!api && api.statusText.length > 0
    readonly property bool titles: !!sv && sv.isTitles
    // what the catalogs did find, before the type chips filter it
    readonly property var found: titles && sv.results ? sv.results : []
    readonly property bool offline: asked && !!api && (api.failure === "offline" || !api.online)
    readonly property bool catalogDown: asked && !offline && titles && api.failure === "catalog"
    readonly property bool noTitles: asked && !offline && !catalogDown && titles && found.length === 0
    readonly property bool noneOfType: asked && !offline && titles && found.length > 0
    readonly property bool notFound: asked && !offline && !titles && sv.results.length === 0
    readonly property var suggestion: noTitles && api.suggestion && api.suggestion.name ? api.suggestion : null

    function q(s) { return "“" + s + "”" }
    function retry() { if (sv) sv.runSearch() }

    // offline: try again every 30 s, and at once when the system says it is back
    Timer {
        interval: 30000; repeat: true
        running: root.visible && root.offline && !!root.sv && root.sv.visible
        onTriggered: root.retry()
    }
    Connections {
        target: root.api
        ignoreUnknownSignals: true
        function onOnlineChanged() { if (root.api.online && root.visible && root.offline) root.retry() }
    }

    // ---- the plain states: searching, nothing typed yet, a torrent list with no hits ----
    ColumnLayout {
        anchors.centerIn: parent
        width: Math.min(parent.width - 80, 360)
        spacing: 10
        visible: root.searching || (!root.asked) || root.notFound

        Spinner { Layout.alignment: Qt.AlignHCenter; visible: root.searching; s: 30 }
        IconImg {
            Layout.alignment: Qt.AlignHCenter
            visible: !root.searching
            src: "qrc:/icons/search.svg"; tint: Theme.t4; s: 38; opacity: 0.7
        }
        Text {
            Layout.fillWidth: true
            text: root.searching ? (i18n.language, i18n.t("search_searching2"))
                 : (root.notFound ? root.api.statusText : (i18n.language, i18n.t("search_prompt")))
            color: Theme.t2; font.pixelSize: 14; font.weight: Font.DemiBold; font.family: Theme.fontSans
            horizontalAlignment: Text.AlignHCenter; wrapMode: Text.WordWrap
        }
        Text {
            Layout.fillWidth: true
            visible: !root.searching
            text: root.notFound ? (i18n.language, i18n.t("search_empty_hint"))
                                : (i18n.language, i18n.t("search_prompt_sub"))
            color: Theme.t4; font.pixelSize: 12; font.family: Theme.fontSans
            horizontalAlignment: Text.AlignHCenter; wrapMode: Text.WordWrap
        }
    }

    // ---- the states that explain themselves: a title, a reason, the next step ----
    ColumnLayout {
        anchors.centerIn: parent
        anchors.verticalCenterOffset: -30
        width: Math.min(parent.width - 80, 500)
        spacing: 0
        visible: root.offline || root.catalogDown || root.noTitles || root.noneOfType

        Text {
            Layout.fillWidth: true
            textFormat: Text.PlainText
            wrapMode: Text.WordWrap
            color: Theme.t1; font.pixelSize: 22; font.weight: Font.DemiBold; font.family: Theme.fontSans
            text: {
                i18n.language
                if (root.offline) return i18n.t("find_offline")
                if (root.catalogDown) return i18n.t("find_catalog_down")
                if (root.noTitles) return i18n.t("find_no_titles").arg(root.q(root.query))
                var t = root.sv ? root.sv.typeFilter : ""
                return i18n.t(t === "movie" ? "find_no_movies" : t === "series" ? "find_no_series" : "find_no_games")
                           .arg(root.q(root.query))
            }
        }
        Text {
            Layout.fillWidth: true
            Layout.topMargin: 14
            textFormat: Text.PlainText
            wrapMode: Text.WordWrap
            lineHeight: 1.15
            color: Theme.t2; font.pixelSize: 15; font.family: Theme.fontSans; font.features: Theme.tnum
            text: {
                i18n.language
                if (root.offline) return i18n.t("find_offline_d")
                if (root.catalogDown) return i18n.t("find_catalog_down_d")
                if (root.noTitles) return i18n.t("find_no_titles_d")
                return root.othersLine()
            }
        }

        // a typo: the nearest real title, one click away
        Rectangle {
            visible: !!root.suggestion
            Layout.fillWidth: true
            Layout.topMargin: 18
            implicitHeight: 64
            radius: Theme.rad(8)
            color: sgMa.containsMouse ? Theme.hover : "transparent"
            border.color: Theme.hair; border.width: 1
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 8; anchors.rightMargin: 14
                spacing: 12
                RoundedImage {
                    Layout.preferredWidth: 86; Layout.preferredHeight: 48
                    radius: Theme.rad(4)
                    fill: Theme.hover
                    sourceWidth: 172
                    sources: {
                        var s = root.suggestion
                        if (!s) return []
                        return (s.backdrop ? [s.backdrop] : []).concat(s.poster ? [s.poster] : [])
                    }
                }
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2
                    Text { text: (i18n.language, i18n.t("find_did_you_mean")); color: Theme.t3; font.pixelSize: 12; font.family: Theme.fontSans }
                    Text {
                        Layout.fillWidth: true
                        textFormat: Text.PlainText
                        text: root.suggestion ? root.suggestion.name : ""
                        color: Theme.t1; font.pixelSize: 15; font.weight: Font.DemiBold; font.family: Theme.fontSans
                        elide: Text.ElideRight
                    }
                }
                Text {
                    text: {
                        var s = root.suggestion
                        if (!s) return ""
                        var bits = []
                        if (s.year) bits.push(s.year)
                        if (s.type && root.sv) bits.push(root.sv.typeLabel(s.type))
                        return bits.join(" · ")
                    }
                    color: Theme.t3; font.pixelSize: 13; font.family: Theme.fontSans; font.features: Theme.tnum
                }
            }
            MouseArea {
                id: sgMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                onClicked: if (root.sv && root.suggestion) root.sv.openTitle(root.suggestion)
            }
        }

        Row {
            Layout.topMargin: 20
            spacing: 8
            LightBtn {
                text: {
                    i18n.language
                    if (root.offline) return i18n.t("find_try_again")
                    if (root.catalogDown) return i18n.t("find_search_direct")
                    if (root.noTitles) return i18n.t("find_search_sources_q").arg(root.q(root.query))
                    return i18n.t("find_show_all").arg(root.found.length)
                }
                onClicked: {
                    if (root.offline) root.retry()
                    else if (root.noneOfType) root.sv.typeFilter = "all"
                    else if (root.api) root.api.searchRaw()
                }
            }
            BtnFlat {
                visible: !root.catalogDown
                text: {
                    i18n.language
                    if (root.offline) return i18n.t("find_open_hub")
                    if (root.noTitles) return i18n.t("find_paste_magnet")
                    return i18n.t("find_search_sources")
                }
                onClicked: {
                    if (root.offline) root.sv.backTo(2)
                    else if (root.noTitles) root.sv.addMagnetRequested()
                    else if (root.api) root.api.searchRaw()
                }
            }
        }

        Text {
            visible: root.offline
            Layout.fillWidth: true
            Layout.topMargin: 18
            wrapMode: Text.WordWrap
            text: (i18n.language, i18n.t("find_offline_retry"))
            color: Theme.t4; font.pixelSize: 13; font.family: Theme.fontSans
        }
    }

    // "There are 1 game and 1 series with this name."
    function othersLine() {
        var n = { movie: 0, series: 0, game: 0 }
        for (var i = 0; i < found.length; ++i) if (n[found[i].type] !== undefined) n[found[i].type]++
        var bits = []
        var keys = ["movie", "series", "game"]
        for (var k = 0; k < keys.length; ++k) {
            var c = n[keys[k]]
            if (c > 0) bits.push(i18n.t(c === 1 ? "find_cnt_" + keys[k] + "_1" : "find_cnt_" + keys[k]).arg(c))
        }
        if (bits.length === 0) return ""
        var list = bits.length === 1 ? bits[0] : i18n.t("find_and").arg(bits.slice(0, -1).join(", ")).arg(bits[bits.length - 1])
        return i18n.t(found.length === 1 ? "find_others_1" : "find_others").arg(list)
    }
}
