// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// What's new, shown once after an update. A letter on the left (the
// version, the developer's note, a way to write back) and what changed on
// the right: the main change with a picture of it and a way to try it, the
// others in a line each, and the fixes apart from the news.
//
// The note and the release text are SINGLE-LANGUAGE literals in
// releaseContent: auto-translating a personal message reads as fake. Key by
// "major.minor" so hotfix bumps keep the release's message; an exact-version
// key still wins. A release can carry date, lead {title, body, action},
// items [{title, body, showMe}] and fixes []; older ones only have
// highlights, which show as plain items.
import QtQuick
import QtQuick.Layouts
import "../theme"
import "../widgets"
import "../widgets/TimeAgo.js" as TimeAgo

ModalShell {
    id: dlg
    cardWidth: 1040
    cardHeight: 830
    cardRadius: 14
    // leaving any way counts as having read it
    function dismiss() { close(); done() }

    signal done()                                  // Got it
    signal actionRequested(string action)          // "add", "downloads"
    signal releaseNotesRequested(string version)   // "" = this one

    readonly property string appVer: (typeof themeBridge !== "undefined" && themeBridge.appVersion) ? themeBridge.appVersion : ""
    readonly property string shortVer: appVer.split(".").slice(0, 2).join(".")
    readonly property string feedbackUrl: "https://docs.google.com/forms/d/e/1FAIpQLScdwLxWC-LB4wLuMI6_D3-QNPLNJPpzbob5LU0Y2yMnhaBFrg/viewform"

    readonly property var releaseContent: ({
        "5.0": {
            date: "2026-10-19",
            lead: { title: "A new BATorrent",
                    body: "Every screen was redone, with seven themes to pick from. Downloads, the player, Find and the HUB all look and work better.",
                    action: "downloads" },
            items: [
                { title: "A VPN inside the app", body: "Import a WireGuard file and only BATorrent goes through it. Nothing is installed on your computer, and it reconnects on its own." },
                { title: "Series, the way you watch them", body: "A show has its own page, episodes fold into one row, and the next one plays in the same window." },
                { title: "A better player", body: "Picture in picture, languages, skip intro, and the wait before a stream happens inside it." },
                { title: "Games have a page", body: "Saves with a backup, the repack's extras, launch options and newer builds." },
                { title: "Seven themes", body: "Each with its own type and art, picked from pictures of the app." }
            ],
            fixes: [
                "Deleted data could start downloading again on its own",
                "Ratio and availability on the cards were not always right"
            ],
            note: "This is the biggest one yet. Almost every screen was redrawn, and a lot changed underneath.<br><br>The VPN is new from the ground up: import a WireGuard file and only BATorrent goes through it. It needs no administrator rights and installs nothing, so it can never take your computer's connection down with it. If it drops, it comes back on its own.<br><br>Series, the player and games all got a lot of attention too.<br><br><b>Found a bug or have an idea? <a href=\"https://docs.google.com/forms/d/e/1FAIpQLScdwLxWC-LB4wLuMI6_D3-QNPLNJPpzbob5LU0Y2yMnhaBFrg/viewform\">Tell me here</a></b>, I read everything.<br>Mateus"
            , highlights: [
                "Every screen redone, with seven themes",
                "A VPN inside the app: only BATorrent goes through it",
                "Series have their own page, and episodes fold into one row",
                "Picture in picture and skip intro in the player",
                "Games have a page: saves, extras, launch options",
                "Fixed: deleted data downloading again on its own"
            ]
        },
        "4.9": {
            date: "2026-10-09",
            lead: { title: "One Add button for everything",
                    body: "Magnet, link, info-hash or file, several at a time. Each one says what it is and where it goes before you add it.",
                    action: "add" },
            items: [
                { title: "Folders in the file list", body: "One checkbox takes everything inside it, and the size follows what you tick." },
                { title: "A magnet still searching says so", body: "With how long it has been looking, instead of sitting at 0%." },
                { title: "Tiles say why something isn't moving", body: "No peers, no seeds or choked, right on the card.", showMe: "downloads" },
                { title: "Categories count what's in them", body: "And mark the one you are in." },
                { title: "Motion throughout", body: "Tabs slide, rows settle in, progress glides between updates." }
            ],
            fixes: [
                "Opening a magnet from the browser could do nothing, or close the app",
                "The setup wizard came back on every launch"
            ],
            note: "We are back! It's been a few quiet months. Most of this one is things you reported, plus a long list of requests from one user who had clearly been keeping notes.<br><br>Adding is one button now. Paste a magnet, a link, an info-hash or a path, or pick a .torrent file. The app works out the rest, and folders in the file list finally behave like folders.<br><br>A magnet still looking for its file list says so, and a torrent that isn't moving tells you why.<br><br>Also fixed: opening a magnet from your browser could do nothing at all, or close the app.<br><br><b>Found a bug or have an idea? <a href=\"https://docs.google.com/forms/d/e/1FAIpQLScdwLxWC-LB4wLuMI6_D3-QNPLNJPpzbob5LU0Y2yMnhaBFrg/viewform\">Tell me here</a></b>, I read everything.<br>Mateus"
            , highlights: [
                "One Add button: magnet, link, info-hash or file, several at a time",
                "Folders in the file list, with a checkbox that takes its contents",
                "A magnet hunting for its file list says so, and for how long",
                "Tiles say why a torrent is stalled: no peers, no seeds, choked",
                "The category menu counts what is in each one",
                "Fixed: opening a magnet from the browser could do nothing, or crash",
                "Fixed: the setup wizard coming back on every launch",
                "Motion throughout, and a design pass over bars, icons and the top bar"
            ]
        },
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
        || releaseContent[shortVer]
        || ({ note: "", highlights: [] })
    // the letter without its own sign-off and feedback line: the column has both
    readonly property string noteText: {
        var n = content.note || ""
        if (n.length === 0) return (i18n.language, i18n.t("whatsnew_generic_note"))
        n = n.replace(/(<br>\s*)*<b>Found a bug[\s\S]*$/i, "")
             .replace(/(<br>|\n)+\s*(—\s*)?Mateus\s*$/, "")
        var paras = n.split(/(?:<br>\s*){2,}|\n\n/).map(function (p) { return p.replace(/^(<br>)+|(<br>)+$/g, "").trim() })
        return paras.filter(function (p) { return p.length > 0 }).join(" ")
    }
    readonly property var noteParas: noteText.split(" ")
    readonly property var items: content.items ? content.items
                                : (content.highlights || []).slice(content.lead ? 0 : 1).map(function (h) { return { title: h, body: "" } })
    readonly property var lead: content.lead ? content.lead
                               : (content.highlights && content.highlights.length > 0 ? { title: content.highlights[0], body: "" } : null)
    function fmtBytes(b) { return typeof session !== "undefined" ? session.humanSize(b) : "" }
    function fmtDate(d) { return TimeAgo.date(d, i18n.localeName, true) }


    RowLayout {
        anchors.fill: parent
        spacing: 0

        // ================= the letter =================
        ColumnLayout {
            Layout.preferredWidth: Math.min(420, dlg.card.width * 0.4)
            Layout.fillWidth: false
            Layout.fillHeight: true
            spacing: 0
            Flickable {
                id: letter
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                contentWidth: width
                contentHeight: letterCol.implicitHeight + 60
                boundsBehavior: Flickable.StopAtBounds
                ColumnLayout {
                    id: letterCol
                    x: 36; y: 34
                    width: letter.width - 72
                    spacing: 0
                    Text { text: dlg.shortVer; color: Theme.t1; font.family: "New Rocker"; font.pixelSize: 112; lineHeight: 0.85 }
                    RowLayout {
                        Layout.fillWidth: true
                        Row {
                            Text { text: "BAT"; color: Theme.down; font.family: "New Rocker"; font.pixelSize: 26 }
                            Text { text: "orrent"; color: Theme.t1; font.family: "New Rocker"; font.pixelSize: 26 }
                        }
                        Item { Layout.fillWidth: true }
                        Text { text: (i18n.language, dlg.fmtDate(dlg.content.date)); color: Theme.t3; font.pixelSize: 13; font.family: Theme.fontSans }
                    }
                    Rectangle { Layout.topMargin: 14; Layout.bottomMargin: 26; width: 40; height: 2; color: Theme.down }
                    Repeater {
                        model: dlg.noteParas
                        delegate: Text {
                            required property var modelData
                            required property int index
                            Layout.fillWidth: true
                            Layout.topMargin: index > 0 ? 12 : 0
                            text: modelData
                            textFormat: Text.StyledText
                            linkColor: Theme.accentText
                            color: Theme.t1; font.pixelSize: 15; font.family: Theme.fontSans
                            wrapMode: Text.WordWrap; lineHeight: 1.65
                            onLinkActivated: function (link) { Qt.openUrlExternally(link) }
                        }
                    }
                    RowLayout {
                        Layout.topMargin: 24
                        spacing: 12
                        RoundedImage {
                            width: 38; height: 38
                            radius: 19
                            sources: ["qrc:/images/avatar-mateus.jpg"]
                            sourceWidth: 76
                        }
                        ColumnLayout {
                            spacing: 0
                            Text { text: "Mateus"; color: Theme.t1; font.family: "New Rocker"; font.pixelSize: 19 }
                            Text { text: (i18n.language, i18n.t("wn_builds_solo")); color: Theme.t3; font.pixelSize: 12; font.family: Theme.fontSans }
                        }
                    }
                }
            }
            Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: Theme.hairSoft }
            ColumnLayout {
                Layout.fillWidth: true
                Layout.margins: 30
                Layout.topMargin: 22
                spacing: 12
                Text { text: (i18n.language, i18n.t("wn_feedback")); color: Theme.t2; font.pixelSize: 13; font.family: Theme.fontSans }
                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: 38
                    radius: Theme.rad(8)
                    color: tellMa.containsMouse ? Theme.hover : "transparent"
                    border.color: Theme.hair; border.width: 1
                    Text { anchors.centerIn: parent; text: (i18n.language, i18n.t("wn_tell_me")) + "  ↗"; color: Theme.t1; font.pixelSize: 14; font.weight: Font.DemiBold; font.family: Theme.fontSans }
                    MouseArea { id: tellMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: Qt.openUrlExternally(dlg.feedbackUrl) }
                }
            }
        }
        Rectangle { Layout.preferredWidth: 1; Layout.fillHeight: true; color: Theme.hair }

        // ================= what changed =================
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0
            RowLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: 60
                Layout.leftMargin: 32; Layout.rightMargin: 14
                Text { Layout.fillWidth: true; text: (i18n.language, i18n.t("whatsnew_title")); color: Theme.t1; font.pixelSize: 17; font.weight: Font.DemiBold; font.family: Theme.fontSans }
                CloseButton {
                    onClicked: dlg.dismiss()
                }
            }
            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Flickable {
                    id: news
                    anchors.fill: parent
                    clip: true
                    contentWidth: width
                    contentHeight: newsCol.implicitHeight + 24
                    boundsBehavior: Flickable.StopAtBounds
                    WheelScroller { flick: news }
                    ColumnLayout {
                        id: newsCol
                        x: 32
                        width: news.width - 64
                        spacing: 0

                        // the main change
                        Rectangle {
                            Layout.fillWidth: true
                            visible: dlg.lead !== null
                            implicitHeight: leadRow.implicitHeight + 40
                            radius: Theme.radCard(12)
                            color: Theme.panel
                            border.color: Theme.hair; border.width: 1
                            GridLayout {
                                id: leadRow
                                x: 20; y: 20
                                width: parent.width - 40
                                columns: width > 560 ? 2 : 1
                                columnSpacing: 18
                                rowSpacing: 16
                                ColumnLayout {
                                    Layout.fillWidth: true
                                    Layout.alignment: Qt.AlignTop
                                    spacing: 10
                                    Text {
                                        Layout.fillWidth: true
                                        text: dlg.lead ? dlg.lead.title : ""
                                        color: Theme.t1; font.pixelSize: 23; font.weight: Font.Bold; font.family: Theme.fontSans
                                        wrapMode: Text.WordWrap; lineHeight: 1.15
                                    }
                                    Text {
                                        Layout.fillWidth: true
                                        visible: text !== ""
                                        text: dlg.lead ? dlg.lead.body : ""
                                        color: Theme.t2; font.pixelSize: 14; font.family: Theme.fontSans
                                        wrapMode: Text.WordWrap; lineHeight: 1.45
                                    }
                                    BtnFlat {
                                        Layout.topMargin: 4
                                        visible: dlg.lead && dlg.lead.action
                                        primary: true
                                        text: (i18n.language, i18n.t("wn_try_it"))
                                        onClicked: { var a = dlg.lead.action; dlg.close(); dlg.done(); dlg.actionRequested(a) }
                                    }
                                }
                                // a picture of it: the Add window's own rows, with made-up items
                                Rectangle {
                                    visible: dlg.lead && dlg.lead.action === "add"
                                    Layout.alignment: Qt.AlignTop
                                    Layout.fillWidth: leadRow.columns === 1
                                    Layout.preferredWidth: leadRow.columns === 1 ? -1 : 300
                                    implicitHeight: pvCol.implicitHeight + 2
                                    radius: Theme.rad(9)
                                    color: Theme.elev
                                    border.color: Theme.hair; border.width: 1
                                    clip: true
                                    Column {
                                        id: pvCol
                                        x: 1; y: 1
                                        width: parent.width - 2
                                        AddItemRow {
                                            width: pvCol.width; first: true; removable: false
                                            name: "Hollow Knight"; tag: "HK"
                                            source: (i18n.language, i18n.t("addw_src_torrent"))
                                            meta: (i18n.language, i18n.t("addw_type_game"))
                                            size: dlg.fmtBytes(7.4e9)
                                        }
                                        AddItemRow {
                                            width: pvCol.width; removable: false
                                            name: "ubuntu-24.04-desktop-amd64.iso"; tag: "ISO"
                                            source: (i18n.language, i18n.t("addw_src_direct"))
                                            meta: (i18n.language, "releases.ubuntu.com · " + i18n.t("addw_resumable"))
                                            size: dlg.fmtBytes(5.7e9)
                                        }
                                    }
                                }
                            }
                        }

                        // the others, a line each
                        Repeater {
                            model: dlg.items
                            delegate: Item {
                                id: it
                                required property var modelData
                                required property int index
                                Layout.fillWidth: true
                                Layout.preferredHeight: itCol.implicitHeight + 30
                                Rectangle { visible: it.index > 0; width: parent.width; height: 1; color: Theme.hairSoft }
                                RowLayout {
                                    anchors.fill: parent
                                    spacing: 12
                                    ColumnLayout {
                                        id: itCol
                                        Layout.fillWidth: true
                                        spacing: 3
                                        Text { Layout.fillWidth: true; text: it.modelData.title; color: Theme.t1; font.pixelSize: 15; font.weight: Font.DemiBold; font.family: Theme.fontSans; wrapMode: Text.WordWrap }
                                        Text { Layout.fillWidth: true; visible: text !== ""; text: it.modelData.body || ""; color: Theme.t3; font.pixelSize: 13; font.family: Theme.fontSans; wrapMode: Text.WordWrap }
                                    }
                                    BtnFlat {
                                        visible: !!it.modelData.showMe
                                        text: (i18n.language, i18n.t("wn_show_me"))
                                        onClicked: { var a = it.modelData.showMe; dlg.close(); dlg.done(); dlg.actionRequested(a) }
                                    }
                                }
                            }
                        }

                        // fixed, apart from the news
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.topMargin: 10
                            visible: (dlg.content.fixes || []).length > 0
                            implicitHeight: fxCol.implicitHeight + 30
                            radius: Theme.radCard(10)
                            color: "transparent"
                            border.color: Theme.hair; border.width: 1
                            ColumnLayout {
                                id: fxCol
                                x: 16; y: 15
                                width: parent.width - 32
                                spacing: 8
                                Text { text: (i18n.language, i18n.t("rn_fixed")); color: Theme.t3; font.pixelSize: 11; font.weight: Font.DemiBold; font.letterSpacing: 0.9; font.capitalization: Font.AllUppercase; font.family: Theme.fontSans }
                                Repeater {
                                    model: dlg.content.fixes || []
                                    delegate: RowLayout {
                                        required property var modelData
                                        Layout.fillWidth: true
                                        spacing: 10
                                        Text { Layout.fillWidth: true; text: modelData; color: Theme.t2; font.pixelSize: 14; font.family: Theme.fontSans; wrapMode: Text.WordWrap }
                                    }
                                }
                            }
                        }
                    }
                }
                Rectangle {
                    anchors.bottom: parent.bottom
                    width: parent.width; height: 40
                    visible: news.contentHeight > news.height && !news.atYEnd
                    gradient: Gradient {
                        GradientStop { position: 0; color: "transparent" }
                        GradientStop { position: 1; color: Theme.elev }
                    }
                }
            }
            Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: Theme.hairSoft }
            RowLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: 68
                Layout.leftMargin: 24; Layout.rightMargin: 20
                spacing: 6
                BtnFlat { text: (i18n.language, i18n.t("whatsnew_full_notes")); onClicked: { dlg.close(); dlg.done(); dlg.releaseNotesRequested(dlg.appVer) } }
                Text { text: "·"; color: Theme.t4; font.pixelSize: 13 }
                BtnFlat { text: (i18n.language, i18n.t("wn_previous")); onClicked: { dlg.close(); dlg.done(); dlg.releaseNotesRequested("previous") } }
                Item { Layout.fillWidth: true }
                BtnFlat { primary: true; text: (i18n.language, i18n.t("wn_got_it")); onClicked: { dlg.close(); dlg.done() } }
            }
        }
    }
}
