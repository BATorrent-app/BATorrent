// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Online-subtitles side panel: a right-edge drawer with a dimming scrim,
// language toggle chips, sync nudges and a result list wired to the subsearch
// bridge. `pw` is the owning PlayerWindow (subtitle state + loadExternalSubs);
// `mediaPlayer` the MediaPlayer. Fills the player; scrim + Esc + a real close
// button all dismiss it (the old tiny ✕ was impossible to find).
import QtQuick
import QtQuick.Layouts
import QtQuick.Effects
import "theme"
import "widgets"

Item {
    id: root
    property var pw
    property var mediaPlayer
    property bool open: false

    // stay in the scene during the slide-out so the animation can finish
    visible: open || drawer.x < root.width

    function openPanel() {
        open = true
        if (typeof subsearch !== "undefined")
            subsearch.searchFor(pw.infoHash, pw.fileIndex, pw.mediaTitle, root.subLangs)
    }
    function closePanel() { open = false }

    // ---- language state ----
    readonly property var langCodes: ["pt", "en", "es", "fr", "de", "it", "ru", "ja", "zh"]
    property var subLangs: {
        var s = (typeof settings !== "undefined") ? settings.get("subtitleLangs") : ""
        if (s && s.length > 0) return ("" + s).split(",")
        var li = (typeof i18n !== "undefined") ? i18n.language : 0
        var ui = ["en", "pt", "zh", "ja", "ru", "es", "de", "uk"][li] || "en"
        return ui === "en" ? ["en"] : [ui, "en"]
    }
    function langLabel(c) {
        return ({ pt: "PT", en: "EN", es: "ES", fr: "FR", de: "DE",
                  it: "IT", ru: "RU", ja: "JA", zh: "ZH" })[c] || c.toUpperCase()
    }
    function toggleLang(code) {
        var a = subLangs.slice()
        var i = a.indexOf(code)
        if (i >= 0) a.splice(i, 1); else a.push(code)
        if (a.length === 0) a = ["en"]
        subLangs = a
        if (typeof settings !== "undefined") settings.set("subtitleLangs", a.join(","))
        if (typeof subsearch !== "undefined")
            subsearch.searchFor(pw.infoHash, pw.fileIndex, pw.mediaTitle, a)
    }

    property string panelError: ""
    property int downloadingIdx: -1
    Connections {
        target: typeof subsearch !== "undefined" ? subsearch : null
        ignoreUnknownSignals: true
        function onSubtitleReady(path) {
            root.downloadingIdx = -1
            root.pw.loadExternalSubs(path)
            root.closePanel()          // subtitle picked → get out of the way
        }
        function onSearchError(message) {
            root.downloadingIdx = -1
            root.panelError = message === "no_sources"
                ? (i18n.language, i18n.t("subsearch_no_sources"))
                : ((i18n.language, i18n.t("subsearch_dl_failed")) + (message.length > 0 ? " — " + message : ""))
        }
        function onSearchingChanged() { if (subsearch.searching) root.panelError = "" }
    }

    // ---- dimming scrim: click anywhere off the drawer to close ----
    Rectangle {
        anchors.fill: parent
        color: "#000000"
        opacity: root.open ? 0.5 : 0
        Behavior on opacity { NumberAnimation { duration: 200; easing.type: Easing.OutCubic } }
        MouseArea { anchors.fill: parent; onClicked: root.closePanel() }
    }

    // ---- the drawer ----
    Item {
        id: drawer
        width: Math.min(384, root.width * 0.5)
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        x: root.open ? root.width - width : root.width
        Behavior on x { NumberAnimation { duration: 260; easing.type: Easing.OutCubic } }

        // soft shadow cast onto the video to the left of the drawer
        RectangularShadow {
            anchors.fill: card
            radius: card.radius
            blur: 46
            color: "#cc000000"
            offset.x: -10
        }

        Rectangle {
            id: card
            anchors.fill: parent
            color: "#f2111216"       // near-opaque charcoal (gradient paints over it)
            // subtle top-to-bottom sheen so it reads as a lit surface, not a slab
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#f01a191f" }
                GradientStop { position: 1.0; color: "#f60d0c10" }
            }
            // accent hairline on the leading edge — the drawer's "spine"
            Rectangle { anchors.left: parent.left; width: 2; height: parent.height
                gradient: Gradient {
                    GradientStop { position: 0.0; color: "#00b3001b" }
                    GradientStop { position: 0.5; color: "#66ff2e3d" }
                    GradientStop { position: 1.0; color: "#00b3001b" }
                }
            }

            // swallow clicks so they don't fall through to the scrim
            MouseArea { anchors.fill: parent }

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 18
                spacing: 12

                // ---- header: title + big obvious close ----
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10
                    IconImg { src: "qrc:/icons/subtitles.svg"; tint: "#ff2e3d"; s: 18; Layout.alignment: Qt.AlignVCenter }
                    Text {
                        Layout.fillWidth: true
                        text: (i18n.language, i18n.t("subsearch_title"))
                        color: "#f3f3f4"; font.pixelSize: 16; font.weight: Font.DemiBold; font.family: Theme.fontSans
                    }
                    Rectangle {
                        Layout.alignment: Qt.AlignVCenter
                        implicitWidth: 30; implicitHeight: 30; radius: 8
                        color: closeMa.containsMouse ? "#26ffffff" : "#14ffffff"
                        border.color: "#22ffffff"; border.width: 1
                        Behavior on color { ColorAnimation { duration: 120 } }
                        Text { anchors.centerIn: parent; text: "✕"; color: "#e8e8ea"; font.pixelSize: 14 }
                        MouseArea { id: closeMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: root.closePanel() }
                    }
                }

                Rectangle { Layout.fillWidth: true; height: 1; color: "#18ffffff" }

                // ---- language picker ----
                Flow {
                    Layout.fillWidth: true
                    spacing: 6
                    Repeater {
                        model: root.langCodes
                        delegate: Rectangle {
                            id: chip
                            required property string modelData
                            readonly property bool on: (root.subLangs || []).indexOf(modelData) >= 0
                            implicitWidth: lc.implicitWidth + 20; implicitHeight: 28; radius: 7
                            color: chip.on ? "#ff2e3d" : (lcMa.containsMouse ? "#26ffffff" : "#16ffffff")
                            border.color: chip.on ? "#ff2e3d" : "#22ffffff"; border.width: 1
                            Behavior on color { ColorAnimation { duration: 120 } }
                            Text {
                                id: lc; anchors.centerIn: parent; text: root.langLabel(chip.modelData)
                                color: chip.on ? "#ffffff" : "#c6c6cc"
                                font.pixelSize: 12; font.weight: chip.on ? Font.DemiBold : Font.Medium
                                font.letterSpacing: 0.4; font.family: Theme.fontSans
                            }
                            MouseArea { id: lcMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: root.toggleLang(chip.modelData) }
                        }
                    }
                }

                // ---- sync controls (once a subtitle is active) ----
                RowLayout {
                    Layout.fillWidth: true
                    visible: root.pw.extSubsActive
                    spacing: 8
                    Text { text: (i18n.language, i18n.t("subsearch_sync")); color: "#9a9aa0"; font.pixelSize: 11; font.family: Theme.fontSans }
                    Item { Layout.fillWidth: true }
                    PChip { label: "−0.5s"; onClicked: root.pw.bumpSubOffset(-500) }
                    Text {
                        text: (root.pw.extSubOffset >= 0 ? "+" : "") + (root.pw.extSubOffset / 1000).toFixed(1) + "s"
                        color: root.pw.extSubOffset === 0 ? "#77777d" : "#f3f3f4"
                        font.pixelSize: 12; font.family: Theme.fontMono
                        MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: { root.pw.extSubOffset = 0; root.pw.updateCue(root.mediaPlayer.position) } }
                    }
                    PChip { label: "+0.5s"; onClicked: root.pw.bumpSubOffset(500) }
                }

                // ---- states: spinner / error / empty ----
                Spinner {
                    Layout.alignment: Qt.AlignHCenter; Layout.topMargin: 8
                    visible: typeof subsearch !== "undefined" && subsearch.searching
                    s: 24
                }
                Text {
                    Layout.fillWidth: true
                    visible: root.panelError.length > 0
                    text: root.panelError; color: "#c6c6cc"; font.pixelSize: 11; font.family: Theme.fontSans; wrapMode: Text.WordWrap
                }
                Text {
                    Layout.fillWidth: true
                    visible: typeof subsearch !== "undefined" && !subsearch.searching
                             && subsearch.results.length === 0 && root.panelError.length === 0
                    text: (i18n.language, i18n.t("subsearch_empty"))
                    color: "#9a9aa0"; font.pixelSize: 12; font.family: Theme.fontSans; wrapMode: Text.WordWrap
                }

                // ---- result list ----
                ListView {
                    Layout.fillWidth: true; Layout.fillHeight: true
                    clip: true; spacing: 6
                    model: typeof subsearch !== "undefined" ? subsearch.results : []
                    boundsBehavior: Flickable.StopAtBounds
                    delegate: Rectangle {
                        required property var modelData
                        required property int index
                        width: ListView.view.width; height: 50; radius: 9
                        color: rowMa.containsMouse ? "#22ffffff" : "#12ffffff"
                        border.color: rowMa.containsMouse ? "#2effffff" : "transparent"; border.width: 1
                        Behavior on color { ColorAnimation { duration: 100 } }
                        RowLayout {
                            anchors.fill: parent; anchors.leftMargin: 11; anchors.rightMargin: 11; spacing: 9
                            Rectangle {
                                implicitWidth: lt.width + 14; implicitHeight: 19; radius: 5
                                color: "#33ff2e3d"; border.color: "#55ff2e3d"; border.width: 1
                                Text { id: lt; anchors.centerIn: parent; text: modelData.language; color: "#ff8a92"; font.pixelSize: 10; font.weight: Font.DemiBold; font.family: Theme.fontSans }
                            }
                            ColumnLayout {
                                Layout.fillWidth: true; spacing: 1
                                Text { Layout.fillWidth: true; text: modelData.name.length > 0 ? modelData.name : modelData.provider; color: "#f0f0f2"; font.pixelSize: 12; font.family: Theme.fontSans; elide: Text.ElideMiddle }
                                Text { text: modelData.provider; color: "#77777d"; font.pixelSize: 10; font.family: Theme.fontSans }
                            }
                            Spinner { visible: root.downloadingIdx === index; s: 14 }
                            IconImg { visible: root.downloadingIdx !== index; src: "qrc:/icons/download.svg"; tint: rowMa.containsMouse ? "#e8e8ea" : "#77777d"; s: 15; Layout.alignment: Qt.AlignVCenter }
                        }
                        MouseArea {
                            id: rowMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                            onClicked: { root.downloadingIdx = index; root.panelError = ""; subsearch.download(index) }
                        }
                    }
                }
            }
        }
    }
}
