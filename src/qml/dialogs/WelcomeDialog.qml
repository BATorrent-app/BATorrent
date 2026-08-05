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
import QtQuick.Shapes
import "../theme"
import "../widgets"
import "../views"

BatDialog {
    id: dlg
    property string mode: "update"          // "welcome" | "update"
    readonly property bool isWelcome: mode === "welcome"

    uiPalette: isWelcome ? WizardPalette : Theme
    title: (i18n.language, i18n.t(isWelcome ? "welcome_window_title" : "whatsnew_title"))
    cardW: isWelcome ? Math.min(960, Math.max(320, width - 48)) : 540
    cardH: isWelcome ? Math.min(640, Math.max(480, height - 48)) : 584
    fitContent: !isWelcome
    backdropColor: isWelcome ? "#000000" : standardBackdropColor

    // Welcome is a multi-step setup; "update" keeps its single OK. holdOnOk
    // routes the footer to nextRequested() so accepted() still means "done" —
    // the host starts the tour on it.
    property int step: 0
    okText: !isWelcome ? (i18n.language, i18n.t("whatsnew_ok"))
          : (step < setup.lastStep ? (i18n.language, i18n.t("welcome_next"))
                                   : (i18n.language, i18n.t("welcome_start")))
    cancelText: (i18n.language, i18n.t("welcome_back"))
    showCancel: isWelcome && step > 0
    holdOnOk: isWelcome && step < setup.lastStep
    holdOnCancel: isWelcome
    footHint: isWelcome
        ? (i18n.language, i18n.t("welcome_step_progress")).arg(step + 1).arg(setup.lastStep + 1)
        : ""
    onNextRequested: if (step < setup.lastStep) step++
    onBackRequested: if (step > 0) step--
    // Reopened from Help → Setup wizard, so it must not resume on whatever step
    // it was left at the last time it ran.
    onOpenedChanged: if (opened && isWelcome) step = 0

    signal openReleaseNotes()

    readonly property string appVer: (typeof themeBridge !== "undefined" && themeBridge.appVersion) ? themeBridge.appVersion : ""

    // ---- per-release dev note + highlights (SINGLE LANGUAGE; edit per release) ----
    // Key by "major.minor" so hotfix bumps (4.3.0 → 4.3.1) keep the release's
    // message; an exact-version key still wins when a patch needs its own note.
    readonly property var releaseContent: ({
        "4.8": {
            note: "You can now point BATorrent at your own VPN. Set it up in Settings and only this app goes through it, and if the tunnel drops transfers stop instead of falling back to your normal connection.<br>It also downloads any direct link now, not just torrents. Paste it or drop it on the window.<br>First run has a wizard, so the app looks the way you want from the start. There is a new typeface, and a long list of design fixes throughout.<br>A hug to <a href=\"https://github.com/teoveo\">@teoveo</a>, who is where most of the ideas in this version came from.<br><b>Found a bug or have an idea? <a href=\"https://docs.google.com/forms/d/e/1FAIpQLScdwLxWC-LB4wLuMI6_D3-QNPLNJPpzbob5LU0Y2yMnhaBFrg/viewform\">Tell me here</a></b>, I read everything.<br>Mateus"
            , highlights: [
                "Point BATorrent at your own VPN — only this app goes through it",
                "Download any direct link, not just torrents",
                "A wizard on first run: language, theme and layout, applied as you click",
                "New typeface, and a design pass over the whole app",
                "Every state has its own progress bar, including one for files that vanished",
                "The detail panel keeps the torrent on screen while you change tabs",
                "Search warns about password bait, impossible file sizes and cam rips",
                "Watch more than one video at once, each in its own window"
            ]
        },
        "4.7": {
            note: "This one's called Cinema, and it's mostly about the player. Audio, subtitles and playback speed used to be spread across three menus — now they're one panel. Hover the seek bar for a frame preview, get a next-episode countdown as a movie ends, skip intros and credits when the file has chapters, and the picture's colors spill softly into the black bars. Less \"torrent client playing a file\", more just watching something.<br><br>Also: Linux stopped crashing at launch (a packaging slip on my end — sorry), waiting downloads finally say they're queued, and the save dialogs remember your favorite folders.<br><br><b>Found a bug or have an idea? <a href=\"https://docs.google.com/forms/d/e/1FAIpQLScdwLxWC-LB4wLuMI6_D3-QNPLNJPpzbob5LU0Y2yMnhaBFrg/viewform\">Tell me here</a></b> — I read everything.<br><br>— Mateus"
            , highlights: [
                "Reworked player: audio, subtitles and speed in one panel",
                "Frame preview on the seek bar, next-episode countdown, skip intro/credits",
                "Ambient glow — the video's colors bleed into the black bars (optional)",
                "Waiting downloads show a Queued filter and status",
                "Favorite folders in the save dialogs, each with its free space",
                "Linux: fixed the launch crash (shipped the wrong libtorrent)",
                "Windows: per-type file associations + a toolbar Refresh button"
            ]
        },
        "4.5": {
            note: "First thing you'll notice: navigation moved to a bar at the top. If you prefer the old sidebar, Settings > Appearance brings it back.<br><br>Search and Discover are now one page. Browse the catalog or just start typing, it all lives in the same place.<br><br>Games in the HUB got better too: Play works like you expect, and if a launch fails the app tells you instead of staying quiet. A bunch of smaller bugs are gone as well.<br><br><b>Found a bug or have an idea? <a href=\"https://docs.google.com/forms/d/e/1FAIpQLScdwLxWC-LB4wLuMI6_D3-QNPLNJPpzbob5LU0Y2yMnhaBFrg/viewform\">Tell me here</a></b>, I read everything.<br><br>Mateus"
            , highlights: [
                "New look: navigation moved to the top (the classic sidebar is in Settings)",
                "Search and Discover merged into one Find page",
                "Better games experience in the HUB, Play works like you expect",
                "Windows: faster wheel scrolling and a tray menu that opens where you click",
                "Seeding limits now mark torrents as completed automatically",
                "Crash protection for broken updates",
                "Light theme contrast fixes"
            ]
        },
        "4.4.1": {
            note: "Quick one: 4.4.0 wasn't opening on Windows for anyone. Sorry about that — fixed now in 4.4.1.<br><br>Everything below is what 4.4 was actually about.<br><br>This release came from my own annoyance: finding stuff dubbed or subtitled in my language was always a fight. Not anymore — turn on \"Prefer my language\" and releases in YOUR language (dubbed included) show up first. Running Jackett? Your indexers now plug straight into search too.<br><br><b>Found a bug or have an idea? <a href=\"https://docs.google.com/forms/d/e/1FAIpQLScdwLxWC-LB4wLuMI6_D3-QNPLNJPpzbob5LU0Y2yMnhaBFrg/viewform\">Tell me here</a></b> — I read everything.<br><br>— Mateus"
            , highlights: [
                "The app opens again on Windows (4.4.0 didn't, for anyone)",
                "Dubbed / your-language releases first in streams and search",
                "Jackett preset — your local indexers inside BATorrent search",
                "Free up space without leaving the app — one click from the sidebar",
                "Progress shows 99.9% until it's truly done — 100% is a promise again",
                "Web UI password now stored hardened (PBKDF2)"
            ]
        },
        "4.4": {
            note: "This release started with my own frustration: finding anything dubbed or subtitled in my language was a fight. Not anymore — with \"Prefer my language\" on, releases in YOUR language (dubbed included) now lead the list, whatever language the app speaks. And if you run Jackett, your own indexers now plug straight into search.<br><br>Also fixed a couple of crashes people ran into. Got a bug or an idea? <a href=\"https://docs.google.com/forms/d/e/1FAIpQLScdwLxWC-LB4wLuMI6_D3-QNPLNJPpzbob5LU0Y2yMnhaBFrg/viewform\">tell me here</a>.<br><br>— Mateus"
            , highlights: [
                "Dubbed / your-language releases first in streams and search",
                "Jackett preset — your local indexers inside BATorrent search",
                "Free up space without leaving the app — one click from the sidebar",
                "Fixed a couple of real crashes found via crash reporting",
                "Progress shows 99.9% until it's truly done — 100% is a promise again",
                "Web UI password now stored hardened (PBKDF2)"
            ]
        },
        "4.3": {
            note: "First, the elephant: 4.3.0 refused to start on Windows — a packaging mistake on my side, fixed in 4.3.1. Everything below is what 4.3 was meant to bring you.\n\nOver 3,000 of you have downloaded BATorrent — thank you, genuinely. I build this solo in my spare time, and I'm now looking for contributors to help it grow: if you write C++/QML (or want to translate it), come say hi on GitHub. New here? Press Ctrl/⌘+K anywhere — it's the fastest way around the whole app.\n\n— Mateus"
            , highlights: [
                "Resume your last movie or game right from the HUB",
                "Rebuilt player with resume that finally sticks",
                "Real seeds + best size in Discover and Search",
                "Paused torrents stay paused after a restart",
                "\"Delete permanently\" for when you're low on disk"
            ]
        },
        "4.2": {
            note: "This one is about feel. No new pages \u2014 hundreds of small fixes instead: every dialog answers Esc, stalled torrents finally explain WHY, and deleting sends files to the trash, not the void. Press Ctrl/\u2318+K \u2014 that one's my favorite.\n\n\u2014 Mateus"
            , highlights: [
                "Command palette (Ctrl/\u2318+K) \u2014 fuzzy-find any torrent or action",
                "Stalled torrents explain why on hover",
                "\"Remove with files\" now uses the system trash \u2014 recoverable",
                "Unified window look on macOS + keyboard focus everywhere"
            ]
        },
        "4.0": {
            note: "Over 2,000 of you have downloaded BATorrent — thank you, it means a lot. I build this in my spare time; your support keeps it going.\n\n— Mateus"
            , highlights: [
                "Guided first-run tour of the whole app",
                "Choose your own app icon — independent of the theme",
                "This welcome / what's-new screen, so I can actually reach you between releases"
            ]
        }
    })
    readonly property var content: releaseContent[appVer]
        || releaseContent[appVer.split(".").slice(0, 2).join(".")]
        || ({ note: "", highlights: [] })
    readonly property string noteText: content.note.length > 0
        ? content.note : (i18n.language, i18n.t("whatsnew_generic_note"))

    // Update mode keeps the poster hero; welcome drops it so the first step
    // opens on the question itself instead of a second brand billboard.
    Item {
        visible: !dlg.isWelcome
        Layout.fillWidth: true
        Layout.preferredHeight: visible ? 132 : 0
        clip: true

        // Red bleed behind the numerals. It has to be a Shape/RadialGradient —
        // QML's plain Gradient is LINEAR, and using it here painted a hard-edged
        // slab instead of a glow. The falloff still meets the block's top margin
        // before it reaches zero; that edge is acceptable, a flat slab was not.
        Shape {
            anchors.fill: parent
            preferredRendererType: Shape.CurveRenderer
            ShapePath {
                strokeColor: "transparent"
                fillGradient: RadialGradient {
                    centerX: 74; centerY: parent.height * 0.46; centerRadius: 190
                    focalX: centerX; focalY: centerY
                    GradientStop { position: 0.0; color: Qt.rgba(dlg.uiPalette.accent.r, dlg.uiPalette.accent.g, dlg.uiPalette.accent.b, 0.32) }
                    GradientStop { position: 0.5; color: Qt.rgba(dlg.uiPalette.accent.r, dlg.uiPalette.accent.g, dlg.uiPalette.accent.b, 0.11) }
                    GradientStop { position: 1.0; color: "transparent" }
                }
                PathMove { x: 0; y: 0 }
                PathLine { x: dlg.cardW; y: 0 }
                PathLine { x: dlg.cardW; y: 150 }
                PathLine { x: 0; y: 150 }
                PathLine { x: 0; y: 0 }
            }
        }

        Column {
            anchors.left: parent.left
            anchors.leftMargin: 2
            anchors.verticalCenter: parent.verticalCenter
            spacing: 0

            Eyebrow { text: (i18n.language, i18n.t("whatsnew_eyebrow")); red: true; uiPalette: dlg.uiPalette }
            Text {
                visible: dlg.appVer.length > 0
                text: dlg.appVer.split(".").slice(0, 2).join(".")
                color: dlg.uiPalette.t1
                font.family: "New Rocker"
                font.pixelSize: 88
                topPadding: -4; bottomPadding: -16
            }
            Row {
                spacing: 0
                Text { text: "BAT"; color: dlg.uiPalette.accent; font.family: "New Rocker"; font.pixelSize: 24 }
                Text { text: "orrent"; color: dlg.uiPalette.t1; font.family: "New Rocker"; font.pixelSize: 24 }
            }
        }
    }
    Rectangle {
        visible: !dlg.isWelcome
        Layout.fillWidth: true
        Layout.topMargin: 6
        Layout.preferredHeight: visible ? 2 : 0
        height: 2
        color: dlg.uiPalette.accent
        opacity: 0.9
    }

    // ===== WELCOME body =====
    WelcomeSteps {
        id: setup
        visible: dlg.isWelcome
        step: dlg.step
        uiPalette: dlg.uiPalette
    }

    // ===== UPDATE body =====
    Text {
        visible: !dlg.isWelcome
        Layout.fillWidth: true; Layout.topMargin: Theme.sp2
        text: (i18n.language, i18n.t("whatsnew_heading"))
        color: dlg.uiPalette.t1; font.pixelSize: 20; font.weight: Font.Bold; font.family: dlg.uiPalette.fontSans
        wrapMode: Text.WordWrap
    }
    // dev note card
    Rectangle {
        visible: !dlg.isWelcome
        Layout.fillWidth: true
        radius: 11; color: dlg.uiPalette.panel; border.color: dlg.uiPalette.hair; border.width: 1
        Layout.preferredHeight: noteCol.implicitHeight + 28
        ColumnLayout {
            id: noteCol
            anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top; anchors.margins: 14
            spacing: 7
            Text {
                text: (i18n.language, i18n.t("whatsnew_devnote_label"))
                color: dlg.uiPalette.accent; font.pixelSize: 11; font.weight: Font.Bold; font.letterSpacing: 0.4; font.family: dlg.uiPalette.fontSans
            }
            Text {
                Layout.fillWidth: true
                text: dlg.noteText
                // AutoText (the default): plain notes stay plain (their \n\n
                // survives as real line breaks); a note with a link like 4.4's
                // gets auto-detected as rich text — which is why THAT entry
                // uses <br> instead of \n\n for its paragraph breaks.
                linkColor: dlg.uiPalette.accentText
                color: dlg.uiPalette.t2; font.pixelSize: 13; font.family: dlg.uiPalette.fontSans
                wrapMode: Text.WordWrap; lineHeight: 1.45
                onLinkActivated: function(link) { Qt.openUrlExternally(link) }
            }
        }
    }
    // highlights
    ColumnLayout {
        visible: !dlg.isWelcome && dlg.content.highlights.length > 0
        Layout.fillWidth: true; Layout.topMargin: Theme.sp1; spacing: 8
        Text {
            text: (i18n.language, i18n.t("whatsnew_highlights"))
            color: dlg.uiPalette.t1; font.pixelSize: 14; font.weight: Font.DemiBold; font.family: dlg.uiPalette.fontSans
        }
        Repeater {
            model: dlg.content.highlights
            // Billed, not bulleted: the first line is the release's headline act
            // and gets real size; the rest sit under it as supporting type. A flat
            // list of identical rows says every change matters equally, which is
            // never true and reads as a changelog dump.
            delegate: RowLayout {
                id: hRow
                required property var modelData
                required property int index
                readonly property bool lead: index === 0
                Layout.fillWidth: true
                Layout.topMargin: lead ? 2 : 0
                spacing: 10
                Text {
                    text: hRow.lead ? "▸" : "›"
                    color: dlg.uiPalette.accent
                    font.pixelSize: hRow.lead ? 15 : 13
                    font.weight: Font.Bold
                    Layout.alignment: Qt.AlignTop
                    Layout.topMargin: hRow.lead ? 4 : 0
                }
                Text {
                    Layout.fillWidth: true
                    // Shorter measure for the lead. Display type wants FEWER
                    // characters per line than body copy, not more — at full width
                    // it ran past every bullet below and dropped two words onto a
                    // second line, so the two blocks looked unrelated.
                    Layout.rightMargin: hRow.lead ? 64 : 0
                    text: hRow.modelData
                    color: hRow.lead ? dlg.uiPalette.t1 : dlg.uiPalette.t2
                    font.pixelSize: hRow.lead ? 19 : 13
                    font.weight: hRow.lead ? Font.Bold : Font.Normal
                    font.letterSpacing: hRow.lead ? -0.3 : 0
                    font.family: dlg.uiPalette.fontSans
                    wrapMode: Text.WordWrap; lineHeight: hRow.lead ? 1.2 : 1.35
                }
            }
        }
    }
    // The wizard is new in 4.8, and someone updating would never meet it — it
    // only opens on a fresh install. Offered rather than forced: every answer in
    // it applies the moment it is clicked, so dropping an existing user into it
    // would rewrite a layout they already chose, with no way back.
    RowLayout {
        visible: !dlg.isWelcome
        Layout.topMargin: Theme.sp2
        spacing: Theme.sp2
        BtnFlat {
            text: (i18n.language, i18n.t("whatsnew_full_notes"))
            onClicked: { dlg.openReleaseNotes(); dlg.close() }
        }
        BtnFlat {
            text: (i18n.language, i18n.t("whatsnew_open_wizard"))
            onClicked: { dlg.step = 0; dlg.mode = "welcome" }
        }
    }
}
