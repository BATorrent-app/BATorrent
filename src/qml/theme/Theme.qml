// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

pragma Singleton
import QtQuick
import "palette.js" as Palette

// Colour and font tokens for every theme. The built-in palettes live in
// palette.js; no hex/rgba outside these two files.
// Theme.name troca o tema; Theme.anime liga a arte de acento.
// Salvo via QmlThemeBridge (QSettings: qmlThemeName, qmlAnime).
QtObject {
    id: theme

    property string name: typeof themeBridge !== "undefined" ? themeBridge.themeName : "dark"
    property bool anime: typeof themeBridge !== "undefined" ? themeBridge.anime : false

    function setName(n) {
        if (typeof themeBridge !== "undefined") themeBridge.themeName = n
        else theme.name = n
    }
    function setAnime(on) {
        if (typeof themeBridge !== "undefined") themeBridge.anime = on
        else theme.anime = on
    }

    // light-mode flag (sakura e light são claros → color-scheme: light)
    readonly property bool isLight: name === "light" || name === "sakura"
    readonly property bool isDark: !isLight

    // ---------- spacing (--s1..--s6) ----------
    readonly property int sp1: 4
    readonly property int sp2: 8
    readonly property int sp3: 12
    readonly property int sp4: 16
    readonly property int sp5: 24
    readonly property int sp6: 32

    // ---------- fonts ----------
    // IBM Plex Sans is bundled (added via QFontDatabase in main.cpp) and used on
    // every platform so the UI is pixel-identical across macOS/Windows/Linux:
    // the system fonts (SF / Segoe UI) diverge in family and metrics otherwise.
    readonly property string fontSans: "IBM Plex Sans"
    readonly property string fontMono: Qt.platform.os === "windows" ? "Consolas" : (Qt.platform.os === "osx" ? "Menlo" : "monospace")
    // tabular figures for numeric readouts (font.features: Theme.tnum); Plex
    // aligns digits like a mono font without the terminal texture; mono stays
    // only for raw data (hashes, logs)
    readonly property var tnum: ({ "tnum": true })

    // Preview triplets for a theme you are NOT currently in: the onboarding and
    // the theme picker show all six side by side, and bg/panel/accent below can
    // only answer for the active one.
    readonly property var swatches: Palette.ORDER.map(function (k) {
        var t = Palette.build(k)
        return { key: k, bg: t.bg, panel: t.panel, accent: t.accent, nav: t.nav,
                 t1: t.t1, t3: t.t3, hair: t.hair, done: t.doneFill, ink: t.inkOnAccent }
    })

    // ---------- state → colour ----------
    // One home for the language, because the dot, the label and the progress
    // bar have to agree: one colour per meaning. Red is downloading now or a
    // problem; green is done, seeding or not (a darker green under a bar, so
    // a row of finished items does not glare); grey is paused or queued. Amber
    // is only the upload arrow.
    function fillFor(k) {
        if (k === "finished" || k === "completed" || k === "seeding") return doneFill
        if (k === "paused" || k === "queued") return pausedFill
        if (k === "missing" || k === "error") return accent
        return accent
    }
    // States that cannot report progress: the bar carries a moving indicator
    // instead of a fill, because a percentage would be a lie.
    function isTroubleState(k) { return k === "missing" || k === "error" }

    // Ink that survives on top of a filled bar or chip of colour c. Relative
    // luminance (WCAG), not a per-state table: six themes plus a user's custom
    // accent is more combinations than anyone will keep a list correct for.
    // White over the seeding amber measures 2.44:1 — under the 3:1 floor, and
    // 1.55:1 on matrix — which is how a hardcoded white percentage got shipped
    // into a bar nobody could read it on.
    //
    // 0.19 is where the two contrast ratios cross, so either side of it is
    // genuinely the better of the two rather than a guess.
    // Pass an opaque colour: alpha is ignored, so a 9%-white wash measures as
    // white and comes back with black ink on it.
    function inkOn(c) {
        // A string literal is the natural way to call this ("#d99a2b"), and on a
        // string .r is undefined — the luminance goes NaN, every comparison
        // against it is false, and the function quietly returns white for
        // everything. That is precisely the bug it exists to prevent, so it
        // normalises rather than trusting the caller to pass a color.
        var col = (typeof c === "string") ? Qt.color(c) : c
        if (!col || col.r === undefined) return "#ffffff"
        function lin(v) { return v <= 0.03928 ? v / 12.92 : Math.pow((v + 0.055) / 1.055, 2.4) }
        return (0.2126 * lin(col.r) + 0.7152 * lin(col.g) + 0.0722 * lin(col.b)) > 0.19
               ? "#101014" : "#ffffff"
    }

    // ---------- palette ----------
    // Every built-in theme comes from palette.js: Dark and Light as tuned, the
    // rest as those two turned to their own hue (the reasoning is at the top of
    // that file). Custom stays below, on the user's six colours.
    readonly property bool isCustom: name === "custom"
    readonly property var p: Palette.build(isCustom ? "dark" : name)

    // ---------- surfaces ----------
    readonly property color bg:    isCustom ? customBgColor : p.bg
    readonly property color panel: isCustom ? customPanelColor : p.panel
    // elevated/recessed surfaces derive from the user's panel so they track it
    readonly property color elev:  isCustom ? Qt.darker(customPanelColor, 1.12) : p.elev
    // top navigation surface: the darkest band of the chrome ladder
    // (nav < bg < panel), so the global bar reads one level below the page
    readonly property color nav:   isCustom ? Qt.darker(customPanelColor, 1.45) : p.nav
    // an input or recessed field
    readonly property color field: isCustom ? Qt.darker(customBgColor, 1.18) : p.field

    // ---------- lines and washes (custom: from the user's text colour) ----------
    function _customInk(a) { return Qt.rgba(customTextColor.r, customTextColor.g, customTextColor.b, a) }
    readonly property color hair:     isCustom ? _customInk(0.14) : p.hair
    readonly property color hairSoft: isCustom ? _customInk(0.07) : p.hairSoft
    readonly property color hover:    isCustom ? _customInk(0.05) : p.hover
    // a step above hover: a pressed/open control, the selected row, a caption button
    readonly property color hover2:   isCustom ? _customInk(0.08) : p.hover2
    readonly property color track:    isCustom ? _customInk(0.10) : p.track

    // a grid card with no art, and the type label set on it
    readonly property color tileBlank: (isLight || isCustom) ? Qt.darker(panel, 1.04) : p.tileBlank
    readonly property color tileInk: (isLight || isCustom) ? t1 : p.t1
    // the two covers peeking out above a series card, back one darkest
    readonly property color stackBack: (isLight || isCustom) ? Qt.darker(panel, 1.10) : p.stackBack
    readonly property color stackMid:  (isLight || isCustom) ? Qt.darker(panel, 1.05) : p.stackMid

    // the close caption button under the pointer (Windows' own red)
    readonly property color winClose: "#c42b1c"

    // a selected row or an "on" pill, and a stronger accent wash
    readonly property color sel:
        isCustom ? Qt.rgba(customPrimaryColor.r, customPrimaryColor.g, customPrimaryColor.b, 0.13) : p.sel
    readonly property color accentTint:
        isCustom ? Qt.rgba(customPrimaryColor.r, customPrimaryColor.g, customPrimaryColor.b, 0.12) : p.accentTint

    // ---------- text (custom: t1 = user colour, t2-t4 fade by opacity) ----------
    readonly property color t1:    isCustom ? customTextColor : p.t1
    // between t1 and t2: menu rows, body copy that should not shout
    readonly property color tSoft: isCustom ? customTextColor : p.tSoft
    readonly property color t2:    isCustom ? _customInk(0.72) : p.t2
    readonly property color t3:    isCustom ? _customInk(0.68) : p.t3
    readonly property color t4:    isCustom ? _customInk(0.55) : p.t4

    // ---------- stage (a title's page: art behind, release list beside it) ----------
    // Stays dark on every theme, light ones included: the text sits on a
    // backdrop, and a white page around a film still reads as a broken image.
    // Dark themes keep their own ink; light and custom fall back to Dark's.
    readonly property bool stageOwnInk: isDark && !isCustom && name !== "dark"
    readonly property var _stage: Palette.build(stageOwnInk ? name : "dark")
    readonly property color stageBg:    _stage.stageBg
    readonly property color stagePanel: _stage.stagePanel
    readonly property color stageField: _stage.stageField
    readonly property color stageT1:    _stage.t1
    readonly property color stageInk:   _stage.tSoft   // hero body copy
    readonly property color stageRaw:   stageOwnInk ? _stage.t2 : "#c9cace"   // a release's raw name
    readonly property color stageT2:    _stage.t2
    readonly property color stageMuted: stageOwnInk ? _stage.t3 : "#8c8d93"
    readonly property color stageT4:    _stage.t4
    readonly property color stageDim:   stageOwnInk ? Qt.darker(_stage.t4, 1.3) : "#5f6068"
    readonly property color stageHair:  Qt.rgba(1, 1, 1, 0.06)
    readonly property color stageEdge:  Qt.rgba(1, 1, 1, 0.14)   // field outline
    readonly property color stageHover: Qt.rgba(1, 1, 1, 0.045)
    readonly property color stageGhost: Qt.rgba(1, 1, 1, 0.4)    // round icon button outline
    // A filled action button with nothing to do yet (Extracting…), and its hover.
    readonly property color stageIdleFill:      "#2b2b30"
    readonly property color stageIdleFillHover: "#35353b"
    // ink on an accent-filled button: dark on a bright accent (Matrix's green)
    readonly property color inkOnAccent: isCustom ? inkOn(customPrimaryColor) : p.inkOnAccent
    // the backdrop's fade into the panel, as alpha over stageBg
    function stageWash(a) { return Qt.rgba(stageBg.r, stageBg.g, stageBg.b, a) }

    // ---------- signal colours that are not theme accents ----------
    readonly property color star: up                 // a rating
    // orange, apart from the seeding amber: won't fit, slow, port blocked
    readonly property color warn: isCustom ? "#e8833a" : p.warn
    readonly property color seedLow: isCustom ? "#d97640" : p.seedLow   // 1–9 seeds

    // ---------- custom theme (name === "custom"): active profile colors ----------
    // Six user colors (bg/panel/text + 3 accents). No legibility auto-fixing:
    // the user owns the contrast. Secondary text tones (t2-t4) and hairlines are
    // derived from the text/bg colors by opacity so they track the user's palette.
    readonly property color customBgColor:        typeof themeBridge !== "undefined" ? themeBridge.cBg        : "#0e0e10"
    readonly property color customPanelColor:     typeof themeBridge !== "undefined" ? themeBridge.cPanel     : "#141416"
    readonly property color customTextColor:      typeof themeBridge !== "undefined" ? themeBridge.cText      : "#f3f3f4"
    readonly property color customPrimaryColor:   typeof themeBridge !== "undefined" ? themeBridge.cPrimary   : "#e5332b"
    readonly property color customSecondaryColor: typeof themeBridge !== "undefined" ? themeBridge.cSecondary : "#d99a2b"
    readonly property color customTertiaryColor:  typeof themeBridge !== "undefined" ? themeBridge.cTertiary  : "#3fb950"

    // ---------- accents (functional) ----------
    readonly property color accent:     isCustom ? customPrimaryColor : p.accent
    readonly property color accentDark: isCustom ? Qt.darker(customPrimaryColor, 1.35) : p.accentDark
    // accent as text; for custom just the primary itself (the user owns the contrast)
    readonly property color accentText: isCustom ? customPrimaryColor : p.accentText
    // seeding and upload
    readonly property color amber: isCustom ? customSecondaryColor : p.amber
    readonly property color up:    isCustom ? Qt.lighter(customSecondaryColor, 1.2) : p.up
    // paused: a neutral, from the user's text in custom
    readonly property color pausedFill: isCustom ? _customInk(0.42) : p.pausedFill
    // a finished item's bar: the done colour, darker so many of them stay calm
    readonly property color doneFill: isCustom ? Qt.darker(customTertiaryColor, 1.7) : p.doneFill
    // health: seeds in search, "Installed" in add-ons, the RSS "Auto" badge
    readonly property color grn: isCustom ? customTertiaryColor : p.grn

    // ---------- window chrome ----------
    // macOS: secondary windows merge the native titlebar into their own header
    // band (ExpandedClientAreaHint + NoTitleBarBackgroundHint) so the title
    // isn't drawn twice. Off elsewhere until tested per-platform.
    readonly property bool unifiedChrome: Qt.platform.os === "osx"

    // ---------- interaction tokens ----------
    // Keyboard-focus ring (accent-tinted, drawn as a border 2px outside the
    // control) and the shared motion scale. Press feedback is scale-only so it
    // stays on the GPU.
    readonly property color focusRing: Qt.rgba(accent.r, accent.g, accent.b, 0.9)
    readonly property int focusRingWidth: 2
    readonly property real pressScale: 0.97
    readonly property int durFast: 120
    readonly property int durBase: 160
    readonly property int durSlow: 240
    // Geometry covers distance, so it needs a step above the one a tint uses:
    // 23 animations had piled up at 260-300 with nowhere on the scale to land.
    readonly property int durSlower: 300
    // Exits run shorter than entrances. durShow is for one-off moments (a
    // torrent finishing), not for anything that happens often.
    readonly property int durExit: 100
    readonly property int durShow: 520
    readonly property int stagger: 22
    // Entrances ease out, exits ease in.
    readonly property int easeIn: Easing.InCubic
    readonly property int easeOut: Easing.OutQuart
    readonly property int easePop: Easing.OutBack

    // Reduced motion: fewer and gentler animations, NOT zero. Opacity and colour
    // transitions stay: they explain what changed. What goes is displacement and
    // scale, which is what actually triggers vestibular discomfort. Qt exposes no
    // OS-level preference, so this has to be our own switch.
    readonly property bool reduceMotion:
        typeof settings !== "undefined" && settings.getBool("reduceMotion")
    // Distance for a slide or lift, 0 under reduced motion.
    function travel(px) { return reduceMotion ? 0 : px }
    // Starting scale for a grow-in, 1 under reduced motion.
    function grow(from) { return reduceMotion ? 1 : from }

    // ---------- anime accent art (per theme) ----------
    readonly property string animeSource:
        name === "dark"     ? "qrc:/images/eyes-dark.png" :
        name === "midnight" ? "qrc:/images/eyes-midnight.png" :
        name === "sakura"   ? "qrc:/images/eyes-sakura.png" :
        name === "darkstar" ? "qrc:/images/spider.png" : ""   // light: nenhuma
    readonly property bool animeBottom: name === "darkstar"   // aranha no canto inferior-direito
    readonly property bool hasAnime: anime && animeSource !== ""

    // ---------- custom full background image (custom theme only) ----------
    // QSettings stores a plain path; build a file URL (Windows needs file:///).
    readonly property string bgImageSource:
        (name === "custom" && typeof themeBridge !== "undefined" && themeBridge.cBgImage !== "")
            ? (Qt.platform.os === "windows" ? "file:///" : "file://") + encodeURI(themeBridge.cBgImage)
            : ""
    readonly property real bgImageOpacity:
        typeof themeBridge !== "undefined" ? themeBridge.cBgOpacity / 100 : 0.55
    readonly property bool hasBgImage: bgImageSource !== ""

    // troca de tema (chamada pela SettingsWindow: NUNCA por demo control na subbar)
    function cycle() {
        var order = ["dark", "light", "midnight", "sakura", "darkstar", "custom"]
        setName(order[(order.indexOf(name) + 1) % order.length])
    }
}
