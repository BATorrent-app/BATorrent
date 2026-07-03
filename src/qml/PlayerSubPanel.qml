// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Online-subtitles side panel: language toggle chips, sync nudges, result
// list wired to the subsearch bridge. `pw` is the owning PlayerWindow
// (subtitle state + loadExternalSubs); `mediaPlayer` the MediaPlayer.
import QtQuick
import QtQuick.Layouts
import "theme"
import "widgets"

Rectangle {
    id: subPanel
    property var pw
    property var mediaPlayer

    visible: false
    color: "#f2101013"
    Rectangle { anchors.left: parent.left; width: 1; height: parent.height; color: "#22ffffff" }

    // languages offered as toggle chips; selection persists and drives search
    readonly property var langCodes: ["pt", "en", "es", "fr", "de", "it", "ru", "ja", "zh"]
    property var subLangs: {
        var s = (typeof settings !== "undefined") ? settings.get("subtitleLangs") : ""
        if (s && s.length > 0) return ("" + s).split(",")
        var ui = ["en", "pt", "zh", "ja", "ru", "es", "de", "uk"][i18n.language] || "en"
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

    function openPanel() {
        visible = true
        if (typeof subsearch !== "undefined")
            subsearch.searchFor(pw.infoHash, pw.fileIndex, pw.mediaTitle, subPanel.subLangs)
    }

    property string panelError: ""
    property int downloadingIdx: -1
    Connections {
        target: typeof subsearch !== "undefined" ? subsearch : null
        ignoreUnknownSignals: true
        function onSubtitleReady(path) {
            subPanel.downloadingIdx = -1
            subPanel.pw.loadExternalSubs(path)
        }
        function onSearchError(message) {
            subPanel.downloadingIdx = -1
            subPanel.panelError = message === "no_sources"
                ? (i18n.language, i18n.t("subsearch_no_sources"))
                : ((i18n.language, i18n.t("subsearch_dl_failed")) + (message.length > 0 ? " — " + message : ""))
        }
        function onSearchingChanged() { if (subsearch.searching) subPanel.panelError = "" }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 14
        spacing: 10

        RowLayout {
            Layout.fillWidth: true
            Text {
                Layout.fillWidth: true
                text: (i18n.language, i18n.t("subsearch_title"))
                color: Theme.t1; font.pixelSize: 14; font.weight: Font.DemiBold; font.family: Theme.fontSans
            }
            PChip { label: "✕"; onClicked: subPanel.visible = false }
        }

        // language picker — toggle which languages to search for
        Flow {
            Layout.fillWidth: true
            spacing: 6
            Repeater {
                model: subPanel.langCodes
                delegate: PChip {
                    required property string modelData
                    label: subPanel.langLabel(modelData)
                    active: subPanel.subLangs.indexOf(modelData) >= 0
                    onClicked: subPanel.toggleLang(modelData)
                }
            }
        }

        // sync controls (visible once a subtitle is active)
        RowLayout {
            Layout.fillWidth: true
            visible: subPanel.pw.extSubsActive
            spacing: 8
            Text {
                text: (i18n.language, i18n.t("subsearch_sync"))
                color: Theme.t3; font.pixelSize: 11; font.family: Theme.fontSans
            }
            Item { Layout.fillWidth: true }
            PChip { label: "−0.5s"; onClicked: subPanel.pw.bumpSubOffset(-500) }
            Text {
                text: (subPanel.pw.extSubOffset >= 0 ? "+" : "") + (subPanel.pw.extSubOffset / 1000).toFixed(1) + "s"
                color: subPanel.pw.extSubOffset === 0 ? Theme.t4 : Theme.t1
                font.pixelSize: 12; font.family: Theme.fontMono
                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: { subPanel.pw.extSubOffset = 0; subPanel.pw.updateCue(subPanel.mediaPlayer.position) } }
            }
            PChip { label: "+0.5s"; onClicked: subPanel.pw.bumpSubOffset(500) }
        }
        Rectangle { Layout.fillWidth: true; visible: subPanel.pw.extSubsActive; implicitHeight: 1; color: "#22ffffff" }

        Spinner {
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: 8
            visible: typeof subsearch !== "undefined" && subsearch.searching
            s: 24
        }
        Text {
            Layout.fillWidth: true
            visible: subPanel.panelError.length > 0
            text: subPanel.panelError
            color: Theme.t3; font.pixelSize: 11; font.family: Theme.fontSans
            wrapMode: Text.WordWrap
        }
        Text {
            Layout.fillWidth: true
            visible: typeof subsearch !== "undefined" && !subsearch.searching
                     && subsearch.results.length === 0 && subPanel.panelError.length === 0
            text: (i18n.language, i18n.t("subsearch_empty"))
            color: Theme.t3; font.pixelSize: 12; font.family: Theme.fontSans
            wrapMode: Text.WordWrap
        }

        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 4
            model: typeof subsearch !== "undefined" ? subsearch.results : []
            boundsBehavior: Flickable.StopAtBounds
            delegate: Rectangle {
                required property var modelData
                required property int index
                width: ListView.view.width
                height: 46
                radius: 8
                color: rowMa.containsMouse ? "#26ffffff" : "#14ffffff"
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 10; anchors.rightMargin: 10
                    spacing: 8
                    Rectangle {
                        implicitWidth: langText.width + 12; implicitHeight: 18; radius: 5
                        color: Theme.accentTint
                        Text { id: langText; anchors.centerIn: parent; text: modelData.language; color: Theme.accentText; font.pixelSize: 10; font.weight: Font.DemiBold; font.family: Theme.fontSans }
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 1
                        Text {
                            Layout.fillWidth: true
                            text: modelData.name.length > 0 ? modelData.name : modelData.provider
                            color: Theme.t1; font.pixelSize: 12; font.family: Theme.fontSans
                            elide: Text.ElideMiddle
                        }
                        Text { text: modelData.provider; color: Theme.t4; font.pixelSize: 10; font.family: Theme.fontSans }
                    }
                    Spinner { visible: subPanel.downloadingIdx === index; s: 14 }
                }
                MouseArea {
                    id: rowMa
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        subPanel.downloadingIdx = index
                        subPanel.panelError = ""
                        subsearch.download(index)
                    }
                }
            }
        }
    }
}
