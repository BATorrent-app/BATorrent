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
import ".."

BatDialog {
    id: dlg
    property string mode: "update"          // "welcome" | "update"
    readonly property bool isWelcome: mode === "welcome"

    title: (i18n.language, i18n.t(isWelcome ? "welcome_window_title" : "whatsnew_title"))
    cardW: 540
    cardH: isWelcome ? 560 : 584
    okText: (i18n.language, i18n.t(isWelcome ? "welcome_start" : "whatsnew_ok"))
    showCancel: false

    signal openReleaseNotes()

    readonly property string appVer: (typeof themeBridge !== "undefined" && themeBridge.appVersion) ? themeBridge.appVersion : ""

    // ---- per-release dev note + highlights (SINGLE LANGUAGE; edit per release) ----
    // Key by "major.minor" so hotfix bumps (4.3.0 → 4.3.1) keep the release's
    // message; an exact-version key still wins when a patch needs its own note.
    readonly property var releaseContent: ({
        "4.8": {
            note: "I built the VPN twice. The first one had BATorrent running its own WireGuard tunnel — and it picked a fight with every VPN app you already had, asked for your admin password every single time, and on Windows it could kill your internet outright. A tester lost his connection for two days because of me. So I deleted it.<br><br>This one just uses the VPN you already pay for. Point BATorrent at it in Settings and only this app goes through it. If it drops, downloads stop.<br><br>Search got suspicious, too — it'll tell you when a release wants a password or is too small to be what it claims, before you waste the bandwidth.<br><br>The rest is a lot of small things you'll feel more than notice.<br><br><b>Found a bug or have an idea? <a href=\"https://docs.google.com/forms/d/e/1FAIpQLScdwLxWC-LB4wLuMI6_D3-QNPLNJPpzbob5LU0Y2yMnhaBFrg/viewform\">Tell me here</a></b> — I read everything.<br><br>— Mateus"
            , highlights: [
                "VPN: bind BATorrent to the VPN you already run",
                "Search warns about password bait, impossible file sizes and cam rips",
                "Download from a direct link (Ctrl+D) and from file hosts",
                "Blocklist of known bad peers, on by default",
                "Copy magnet and Open folder are now in the toolbar, not just the menu",
                "Speed graph with a real scale — read the numbers off the gridlines",
                "New icon set and typeface; text contrast now passes WCAG AA",
                "Categories you create stick around, and show up in every menu"
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

    // ===== hero header — billed like a poster =====
    // This screen is seen once per release and is the only moment the app gets
    // to announce itself, so it stops looking like a settings row: the version
    // is the headliner, set in the wordmark face at a size nothing else uses.
    Item {
        Layout.fillWidth: true
        Layout.preferredHeight: dlg.isWelcome ? 92 : 132
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
                    GradientStop { position: 0.0; color: Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.32) }
                    GradientStop { position: 0.5; color: Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.11) }
                    GradientStop { position: 1.0; color: "transparent" }
                }
                PathRectangle { width: 520; height: 150 }
            }
        }

        Column {
            anchors.left: parent.left
            anchors.leftMargin: 2
            anchors.verticalCenter: parent.verticalCenter
            spacing: 0

            Row {
                spacing: 8
                Eyebrow { text: (i18n.language, i18n.t(dlg.isWelcome ? "welcome_eyebrow" : "whatsnew_eyebrow")); red: true }
            }
            // the headliner: version numerals in New Rocker, oversized on purpose
            Text {
                visible: dlg.appVer.length > 0
                text: dlg.appVer.split(".").slice(0, 2).join(".")
                color: Theme.t1
                font.family: "New Rocker"
                font.pixelSize: dlg.isWelcome ? 52 : 88
                topPadding: -4; bottomPadding: dlg.isWelcome ? -8 : -16
            }
            Row {
                spacing: 0
                Text { text: "BAT"; color: Theme.accent; font.family: "New Rocker"; font.pixelSize: 24 }
                Text { text: "orrent"; color: Theme.t1; font.family: "New Rocker"; font.pixelSize: 24 }
            }
        }
    }
    // a solid accent rule, not a hairline — it separates the billing from the mark
    Rectangle { Layout.fillWidth: true; Layout.topMargin: 6; height: 2; color: Theme.accent; opacity: 0.9 }

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
    // Asked here, before anything else, because this is exactly where a new user
    // is lost: the app opens in the OS language and they can't find a dub or a
    // subtitle, so they leave. Interface and content are two questions — reading
    // English menus while wanting Portuguese audio is a normal combination.
    Rectangle {
        visible: dlg.isWelcome
        Layout.fillWidth: true; Layout.topMargin: Theme.sp2
        implicitHeight: langCol.implicitHeight + 2 * Theme.sp3
        radius: 12
        color: Theme.field
        border.color: Theme.hair; border.width: 1

        SettingsSchema { id: langSchema }

        ColumnLayout {
            id: langCol
            anchors.fill: parent
            anchors.margins: Theme.sp3
            spacing: 10

            Text {
                text: (i18n.language, i18n.t("welcome_lang_title"))
                color: Theme.accent; font.pixelSize: 10; font.weight: Font.Bold
                font.letterSpacing: 1.2; font.family: Theme.fontSans
            }
            RowLayout {
                Layout.fillWidth: true
                spacing: 12
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 5
                    Text {
                        text: (i18n.language, i18n.t("set_language2"))
                        color: Theme.t3; font.pixelSize: 11; font.family: Theme.fontSans
                    }
                    TSelect {
                        Layout.fillWidth: true
                        model: langSchema.languageNames
                        icons: langSchema.languageFlags
                        currentIndex: i18n.language
                        onActivated: function (i) { i18n.setLanguage(i) }
                    }
                }
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 5
                    Text {
                        text: (i18n.language, i18n.t("set_content_language"))
                        color: Theme.t3; font.pixelSize: 11; font.family: Theme.fontSans
                    }
                    TSelect {
                        Layout.fillWidth: true
                        model: [(i18n.language, i18n.t("set_content_language_same"))].concat(langSchema.languageNames)
                        icons: [""].concat(langSchema.languageFlags)
                        currentIndex: {
                            if (typeof settings === "undefined") return 0
                            var cv = settings.get("contentLanguage")
                            return (cv === undefined || cv === null || cv === "" || cv < 0) ? 0 : cv + 1
                        }
                        onActivated: function (i) {
                            if (typeof settings !== "undefined") settings.set("contentLanguage", i - 1)
                        }
                    }
                }
            }
        }
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
                // AutoText (the default): plain notes stay plain (their \n\n
                // survives as real line breaks); a note with a link like 4.4's
                // gets auto-detected as rich text — which is why THAT entry
                // uses <br> instead of \n\n for its paragraph breaks.
                linkColor: Theme.accentText
                color: Theme.t2; font.pixelSize: 13; font.family: Theme.fontSans
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
            color: Theme.t1; font.pixelSize: 14; font.weight: Font.DemiBold; font.family: Theme.fontSans
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
                    color: Theme.accent
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
                    color: hRow.lead ? Theme.t1 : Theme.t2
                    font.pixelSize: hRow.lead ? 19 : 13
                    font.weight: hRow.lead ? Font.Bold : Font.Normal
                    font.letterSpacing: hRow.lead ? -0.3 : 0
                    font.family: Theme.fontSans
                    wrapMode: Text.WordWrap; lineHeight: hRow.lead ? 1.2 : 1.35
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
