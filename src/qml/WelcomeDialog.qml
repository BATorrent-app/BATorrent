// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Unified onboarding screen. mode "welcome" (first install) shows a greeting and
// leads into the mandatory tour; mode "update" (version changed) shows a personal
// dev note + this version's highlights + a link to the full release notes.
//
// The per-release dev note / highlights are SINGLE-LANGUAGE literals in
// releaseContent below — auto-translating a personal message reads as fake. Edit
// (or add) the entry for each release; everything else (chrome) stays i18n'd.
import QtQuick
import QtQuick.Layouts
import QtQuick.Effects
import "theme"
import "widgets"

BatDialog {
    id: dlg
    property string mode: "update"          // "welcome" | "update"
    readonly property bool isWelcome: mode === "welcome"

    title: (i18n.language, i18n.t(isWelcome ? "welcome_window_title" : "whatsnew_title"))
    cardW: 540
    cardH: isWelcome ? 452 : 584
    okText: (i18n.language, i18n.t(isWelcome ? "welcome_start" : "whatsnew_ok"))
    showCancel: false

    signal openReleaseNotes()

    readonly property string appVer: (typeof themeBridge !== "undefined" && themeBridge.appVersion) ? themeBridge.appVersion : ""

    // ---- per-release dev note + highlights (SINGLE LANGUAGE; edit per release) ----
    // Key by the exact app version. Rename/add the entry when you bump the version.
    readonly property var releaseContent: ({
        "4.2.0": {
            note: "This one is about feel. No new pages \u2014 hundreds of small fixes instead: every dialog answers Esc, stalled torrents finally explain WHY, and deleting sends files to the trash, not the void. Press Ctrl/\u2318+K \u2014 that one's my favorite.\n\n\u2014 Mateus"
            , highlights: [
                "Command palette (Ctrl/\u2318+K) \u2014 fuzzy-find any torrent or action",
                "Stalled torrents explain why on hover",
                "\"Remove with files\" now uses the system trash \u2014 recoverable",
                "Unified window look on macOS + keyboard focus everywhere"
            ]
        },
        "4.0.0": {
            note: "Over 2,000 of you have downloaded BATorrent — thank you, it means a lot. I build this in my spare time; your support keeps it going.\n\n— Mateus"
            , highlights: [
                "Guided first-run tour of the whole app",
                "Choose your own app icon — independent of the theme",
                "This welcome / what's-new screen, so I can actually reach you between releases"
            ]
        }
    })
    readonly property var content: releaseContent[appVer] || ({ note: "", highlights: [] })
    readonly property string noteText: content.note.length > 0
        ? content.note : (i18n.language, i18n.t("whatsnew_generic_note"))

    // ===== hero header =====
    RowLayout {
        Layout.fillWidth: true
        spacing: 14
        Image {
            Layout.preferredWidth: 52; Layout.preferredHeight: 52; Layout.alignment: Qt.AlignVCenter
            source: "qrc:/images/logo.svg"; sourceSize: Qt.size(104, 104); fillMode: Image.PreserveAspectFit
            layer.enabled: Theme.isLight
            layer.effect: MultiEffect { colorization: 1.0; colorizationColor: Theme.t1 }
        }
        ColumnLayout {
            Layout.fillWidth: true; spacing: 5
            RowLayout {
                spacing: 8
                Eyebrow { text: (i18n.language, i18n.t(dlg.isWelcome ? "welcome_eyebrow" : "whatsnew_eyebrow")); red: true }
                TChip { visible: dlg.appVer.length > 0; text: "v" + dlg.appVer }
            }
            Row {
                spacing: 0
                Text { text: "BAT"; color: Theme.accent; font.family: "New Rocker"; font.pixelSize: 27 }
                Text { text: "orrent"; color: Theme.t1; font.family: "New Rocker"; font.pixelSize: 27 }
            }
        }
    }
    Rectangle { Layout.fillWidth: true; Layout.topMargin: 2; height: 1; color: Theme.hairSoft }

    // ===== WELCOME body =====
    Text {
        visible: dlg.isWelcome
        Layout.fillWidth: true; Layout.topMargin: Theme.sp2
        text: (i18n.language, i18n.t("welcome_heading"))
        color: Theme.t1; font.pixelSize: 20; font.weight: Font.Bold; font.family: Theme.fontSans
        wrapMode: Text.WordWrap
    }
    Text {
        visible: dlg.isWelcome
        Layout.fillWidth: true
        text: (i18n.language, i18n.t("welcome_blurb2"))
        color: Theme.t2; font.pixelSize: 13; font.family: Theme.fontSans
        wrapMode: Text.WordWrap; lineHeight: 1.45
    }
    RowLayout {
        visible: dlg.isWelcome
        Layout.fillWidth: true; Layout.topMargin: Theme.sp2; spacing: 10
        Rectangle { Layout.alignment: Qt.AlignTop; width: 28; height: 28; radius: 8; color: Theme.field
            IconImg { anchors.centerIn: parent; src: "qrc:/icons/discover.svg"; tint: Theme.accentText; s: 15 } }
        Text {
            Layout.fillWidth: true; Layout.alignment: Qt.AlignVCenter
            text: (i18n.language, i18n.t("welcome_tour_hint"))
            color: Theme.t3; font.pixelSize: 12; font.family: Theme.fontSans; wrapMode: Text.WordWrap; lineHeight: 1.35
        }
    }

    // ===== UPDATE body =====
    Text {
        visible: !dlg.isWelcome
        Layout.fillWidth: true; Layout.topMargin: Theme.sp2
        text: (i18n.language, i18n.t("whatsnew_heading"))
        color: Theme.t1; font.pixelSize: 20; font.weight: Font.Bold; font.family: Theme.fontSans
        wrapMode: Text.WordWrap
    }
    // dev note card
    Rectangle {
        visible: !dlg.isWelcome
        Layout.fillWidth: true
        radius: 11; color: Theme.panel; border.color: Theme.hair; border.width: 1
        Layout.preferredHeight: noteCol.implicitHeight + 28
        ColumnLayout {
            id: noteCol
            anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top; anchors.margins: 14
            spacing: 7
            Text {
                text: (i18n.language, i18n.t("whatsnew_devnote_label"))
                color: Theme.accent; font.pixelSize: 11; font.weight: Font.Bold; font.letterSpacing: 0.4; font.family: Theme.fontSans
            }
            Text {
                Layout.fillWidth: true
                text: dlg.noteText
                color: Theme.t2; font.pixelSize: 13; font.family: Theme.fontSans
                wrapMode: Text.WordWrap; lineHeight: 1.45
            }
        }
    }
    // highlights
    ColumnLayout {
        visible: !dlg.isWelcome && dlg.content.highlights.length > 0
        Layout.fillWidth: true; Layout.topMargin: Theme.sp1; spacing: 8
        Text {
            text: (i18n.language, i18n.t("whatsnew_highlights"))
            color: Theme.t1; font.pixelSize: 14; font.weight: Font.DemiBold; font.family: Theme.fontSans
        }
        Repeater {
            model: dlg.content.highlights
            delegate: RowLayout {
                id: hRow
                required property var modelData
                Layout.fillWidth: true; spacing: 10
                Text { text: "›"; color: Theme.accent; font.pixelSize: 15; font.weight: Font.Bold; Layout.alignment: Qt.AlignTop }
                Text {
                    Layout.fillWidth: true
                    text: hRow.modelData
                    color: Theme.t2; font.pixelSize: 13; font.family: Theme.fontSans
                    wrapMode: Text.WordWrap; lineHeight: 1.35
                }
            }
        }
    }
    BtnFlat {
        visible: !dlg.isWelcome
        Layout.topMargin: Theme.sp2
        text: (i18n.language, i18n.t("whatsnew_full_notes"))
        onClicked: { dlg.openReleaseNotes(); dlg.close() }
    }
}
