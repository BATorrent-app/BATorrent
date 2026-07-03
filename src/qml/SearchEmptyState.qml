// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Search page's empty / loading state — a small centered prompt. Three cases:
//   · searching        → spinner + "searching…"
//   · searched, 0 hits  → search icon + the bridge's status + "nothing found" hint
//   · initial (no query)→ search icon + "type something" prompt
// Extracted from SearchView so the identical block isn't duplicated across the
// flat-list and titles-grid views. `sv` is the owning SearchView.
import QtQuick
import QtQuick.Layouts
import "theme"
import "widgets"

Item {
    id: root
    property var sv
    readonly property var api: sv ? sv.api : null
    readonly property bool searching: api && api.searching
    readonly property bool notFound: !searching && api && api.statusText.length > 0
                                     && api.results.length === 0

    ColumnLayout {
        anchors.centerIn: parent
        width: Math.min(parent.width - 80, 360)
        spacing: 10

        Spinner { Layout.alignment: Qt.AlignHCenter; visible: root.searching; s: 30 }
        IconImg {
            Layout.alignment: Qt.AlignHCenter
            visible: !root.searching
            src: "qrc:/icons/search.svg"; tint: Theme.t4; s: 38; opacity: 0.7
        }
        Text {
            Layout.fillWidth: true
            text: root.searching ? (i18n.language, i18n.t("search_searching2"))
                 : (root.notFound ? api.statusText : (i18n.language, i18n.t("search_prompt")))
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
}
