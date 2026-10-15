// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

pragma Singleton
import QtQuick
import "palette.js" as Palette

// Colour, type and shape tokens for every theme. The built-in themes live in
// palette.js; a theme of the user's own is built here from its six colours.
// No hex/rgba outside these two files. Saved through QmlThemeBridge
// (QSettings: qmlThemeName, qmlBackgroundMode, qmlCustomProfiles).
QtObject {
    id: theme

    property string name: typeof themeBridge !== "undefined" ? themeBridge.themeName : "default"
    function setName(n) {
        if (typeof themeBridge !== "undefined") themeBridge.themeName = n
        else theme.name = n
    }

    // ---------- which theme ----------
    // "user" is one of the user's own; the rest are palette.js keys.
    readonly property bool isUser: name === "user"
    readonly property var ut: (isUser && typeof themeBridge !== "undefined") ? themeBridge.userTheme : ({})
    readonly property bool userLight: isUser && ut.base === "light"
    // the built-in theme this one takes everything but colour from
    readonly property var p: Palette.build(isUser ? (userLight ? "light" : "default") : name)
    readonly property string key: isUser ? "user" : p.key
    readonly property bool isDefault: key === "default"
    readonly property bool isLight: isUser ? userLight : p.isLight
    readonly property bool isDark: !isLight

    // ---------- spacing (--s1..--s6) ----------
    readonly property int sp1: 4
    readonly property int sp2: 8
    readonly property int sp3: 12
    readonly property int sp4: 16
    readonly property int sp5: 24
    readonly property int sp6: 32

    // ---------- type ----------
    // The interface face stays IBM Plex Sans in most themes (Zen Kaku Gothic
    // New in Sakura, Plex Mono in Matrix); all are bundled, so metrics are
    // the same on every platform. The display face is only for tabs, the
    // details title and headers, never numbers.
    readonly property string fontSans: p.sans
    readonly property string fontDisplay: isUser ? (ut.font || "IBM Plex Sans") : p.display
    readonly property string fontMono: Qt.platform.os === "windows" ? "Consolas" : (Qt.platform.os === "osx" ? "Menlo" : "monospace")
    // tabular figures for numeric readouts (font.features: Theme.tnum)
    readonly property var tnum: ({ "tnum": true })

    // Tab lettering. Default keeps the bar as it was (14 px, DemiBold/Medium).
    readonly property var _tab: p.tab
    readonly property real tabSize: _tab ? _tab.size : 14
    // Instrument Serif has one weight: a theme of the user's that picks it
    // gets it as drawn rather than a bolded copy
    readonly property bool _singleWeight: isUser && fontDisplay === "Instrument Serif"
    readonly property int tabWeightOn: _singleWeight ? Font.Normal : _tab ? _tab.weight : Font.DemiBold
    readonly property int tabWeightOff: _singleWeight ? Font.Normal : _tab ? _tab.offWeight : Font.Medium
    readonly property real tabTracking: _tab ? _tab.track * tabSize : 0
    readonly property int tabCaps: !_tab ? Font.MixedCase
                                   : _tab.caps === "upper" ? Font.AllUppercase
                                   : _tab.caps === "lower" ? Font.AllLowercase : Font.MixedCase
    readonly property bool tabItalic: !!(_tab && _tab.italic)
    // Titles (the details panel, a series' name). titleSize 0 keeps the
    // component's own size, which is what Default does.
    readonly property var _title: p.title
    readonly property real titleSize: _title ? _title.size : 0
    readonly property int titleWeight: _title ? _title.weight : Font.DemiBold
    readonly property real titleTracking: _title ? _title.track * (titleSize || 17) : 0
    readonly property int titleCaps: (_title && _title.caps === "upper") ? Font.AllUppercase : Font.MixedCase
    readonly property bool titleItalic: !!(_title && _title.italic)
    // A title or header in the theme's letter. Each takes what the component
    // asks for and returns it unchanged on Default: the size scaled by the
    // ratio the theme sets its titles at (the boards' 22 px), the theme's
    // weight and tracking, its case and slant.
    function displaySize(px) { return (!isUser && titleSize > 0) ? Math.round(px * titleSize / 22) : px }
    function tWeight(w) {
        if (isDefault) return w
        // Instrument Serif comes in one weight; a bolded copy of it is a smear
        if (isUser) return fontDisplay === "Instrument Serif" ? Font.Normal : w
        return titleWeight
    }
    function tTrack(px, orig) { return (isDefault || isUser || !_title) ? (orig || 0) : _title.track * displaySize(px) }
    readonly property int tCaps: isUser ? Font.MixedCase : titleCaps
    readonly property bool tItalic: !isUser && titleItalic

    // ---------- corners ----------
    // Each theme has a corner (0 in Matrix to 12 in Dark Star); every radius
    // in the app was drawn for Default's 8, so it scales by that ratio. A pill
    // stays a pill, except in Matrix, where nothing is round.
    readonly property real radius: isUser ? ({ square: 0, soft: 4, round: 8 }[ut.corners] || 4) : p.r
    readonly property real radiusCard: isUser ? radius : p.rCard
    readonly property bool _keep: !isUser && p.keepShape
    function rad(px) {
        if (_keep) return px
        if (radius <= 0) return 0
        if (px >= 40) return px
        return Math.min(px * 1.6, px * radius / 8)
    }
    function radCard(px) {
        if (_keep) return px
        if (radiusCard <= 0) return 0
        return Math.min(px * 1.6, px * radiusCard / 8)
    }

    // ---------- the themes, side by side ----------
    // For the onboarding: the built-in themes with a few of their colours.
    readonly property var swatches: Palette.ORDER.map(function (k) {
        var t = Palette.build(k)
        return { key: k, name: t.name, bg: t.bg, panel: t.panel, accent: t.accent, nav: t.nav,
                 t1: t.t1, t3: t.t3, hair: t.hair, done: t.doneFill, ink: t.inkOnAccent }
    })
    readonly property var order: Palette.ORDER

    // ---------- state → colour ----------
    // One home for the language, because the dot, the label and the progress
    // bar have to agree: one colour per meaning. `down` is downloading now or
    // a problem (red in most themes, coral where the accent is not red); done
    // is green, seeding or not (a darker green under a bar, so a row of
    // finished items does not glare); grey is paused or queued. Amber is only
    // the upload arrow.
    function fillFor(k) {
        if (k === "finished" || k === "completed" || k === "seeding") return doneFill
        if (k === "paused" || k === "queued") return pausedFill
        return down
    }
    // States that cannot report progress: the bar carries a moving indicator
    // instead of a fill, because a percentage would be a lie.
    function isTroubleState(k) { return k === "missing" || k === "error" }

    // Ink that survives on top of a filled bar or chip of colour c (WCAG
    // relative luminance; 0.19 is where the two contrast ratios cross).
    // Pass an opaque colour: alpha is ignored.
    function inkOn(c) {
        var col = (typeof c === "string") ? Qt.color(c) : c
        if (!col || col.r === undefined) return "#ffffff"
        function lin(v) { return v <= 0.03928 ? v / 12.92 : Math.pow((v + 0.055) / 1.055, 2.4) }
        return (0.2126 * lin(col.r) + 0.7152 * lin(col.g) + 0.0722 * lin(col.b)) > 0.19
               ? "#101014" : "#ffffff"
    }

    // ---------- a theme of the user's own ----------
    // Six colours, no legibility auto-fixing: the user owns the contrast.
    // Secondary text and lines come from the text colour by opacity. With a
    // window picture, the nav and the panels let it through.
    function _hex(v, fallback) { return (typeof v === "string" && v.length >= 4) ? v : fallback }
    readonly property color uBg:        _hex(ut.bg, "#0e0e10")
    readonly property color uPanel:     _hex(ut.panel, "#18181b")
    readonly property color uText:      _hex(ut.text, "#f3f3f4")
    readonly property color uPrimary:   _hex(ut.primary, "#e5332b")
    readonly property color uSecondary: _hex(ut.secondary, "#d99a2b")
    readonly property color uTertiary:  _hex(ut.tertiary, "#3fb950")
    function _uInk(a) { return Qt.rgba(uText.r, uText.g, uText.b, a) }
    function _over(c, a) { return Qt.rgba(c.r, c.g, c.b, a) }

    // the window picture (user themes only)
    function _fileUrl(path) {
        return path ? (Qt.platform.os === "windows" ? "file:///" : "file://") + encodeURI(path) : ""
    }
    readonly property string windowImage: isUser ? _fileUrl(ut.image || "") : ""
    readonly property bool hasWindowImage: windowImage !== ""
    readonly property string windowFit: ut.fit || "fill"
    readonly property real windowZoom: (ut.zoom || 100) / 100
    readonly property real windowDim: (ut.dim === undefined ? 55 : ut.dim) / 100
    readonly property real windowBlur: (ut.blur || 0) / 20

    // ---------- surfaces ----------
    readonly property color bg:    isUser ? uBg : p.bg
    readonly property color panel: isUser ? (hasWindowImage ? _over(uPanel, 0.78) : uPanel) : p.panel
    readonly property color elev:  isUser ? Qt.tint(uPanel, _uInk(0.05)) : p.elev
    // top navigation surface: the darkest band of the chrome ladder
    readonly property color nav:   isUser ? (hasWindowImage ? _over(uPanel, 0.82) : Qt.darker(uPanel, userLight ? 1.04 : 1.35)) : p.nav
    // the window's frame around the title strip: a step below the nav
    readonly property color frame: isLight ? Qt.darker(nav, 1.08) : Qt.darker(nav, 2.2)
    // a page's own fill: clear when a user theme puts a picture behind the window
    readonly property color pageBg: hasWindowImage ? "transparent" : bg
    // an input or recessed field
    readonly property color field: isUser ? (userLight ? "#ffffff" : Qt.darker(uBg, 1.18)) : p.field

    // ---------- lines and washes ----------
    readonly property color hair:     isUser ? _uInk(0.14) : p.hair
    readonly property color hairSoft: isUser ? _uInk(0.07) : p.hairSoft
    readonly property color hover:    isUser ? _uInk(0.05) : p.hover
    // a step above hover: a pressed/open control, the selected row, a caption button
    readonly property color hover2:   isUser ? _uInk(0.08) : p.hover2
    readonly property color track:    isUser ? _uInk(0.10) : p.track

    // a grid card with no art, and the type label set on it
    readonly property color tileBlank: isUser ? Qt.tint(uPanel, _uInk(0.04)) : p.tileBlank
    readonly property color tileInk: t1
    // the two covers peeking out above a series card, back one darkest
    readonly property color stackBack: isUser ? Qt.tint(uPanel, _uInk(0.07)) : p.stackBack
    readonly property color stackMid:  isUser ? Qt.tint(uPanel, _uInk(0.11)) : p.stackMid

    // the close caption button under the pointer (Windows' own red)
    readonly property color winClose: "#c42b1c"

    // a selected row or an "on" pill, and a stronger accent wash
    readonly property color sel:        isUser ? _over(uPrimary, 0.13) : p.sel
    readonly property color accentTint: isUser ? _over(uPrimary, 0.12) : p.accentTint

    // The ring around a selected card: the theme's ink (white on dark, ink
    // on light), with a violet halo in Dark Star.
    readonly property color selRing: t1
    readonly property bool glow: !isUser && p.glow

    // ---------- text ----------
    readonly property color t1:    isUser ? uText : p.t1
    // between t1 and t2: menu rows, body copy that should not shout
    readonly property color tSoft: isUser ? _uInk(0.86) : p.tSoft
    readonly property color t2:    isUser ? _uInk(0.72) : p.t2
    readonly property color t3:    isUser ? _uInk(0.68) : p.t3
    readonly property color t4:    isUser ? _uInk(0.55) : p.t4

    // ---------- stage (a title's page: art behind, release list beside it) ----------
    // Stays dark on every theme, light ones included: the text sits on a
    // backdrop, and a white page around a film still reads as a broken image.
    // Dark built-in themes keep their own ink; the rest fall back to Default's.
    readonly property bool stageOwnInk: isDark && !isUser && !isDefault
    readonly property var _stage: Palette.build(stageOwnInk ? key : "default")
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
    // ink on an accent-filled button: dark on a bright accent (Midnight's lamp
    // yellow, Matrix's phosphor)
    readonly property color inkOnAccent: isUser ? (Palette.luminance(String(uPrimary).length === 9 ? "#" + String(uPrimary).slice(3) : String(uPrimary)) > 0.4 ? "#101014" : "#ffffff")
                                                : p.inkOnAccent
    // the backdrop's fade into the panel, as alpha over stageBg
    function stageWash(a) { return Qt.rgba(stageBg.r, stageBg.g, stageBg.b, a) }

    // ---------- signal colours ----------
    readonly property color star: up                 // a rating
    // orange, apart from the upload amber: won't fit, slow, port blocked
    readonly property color warn:    isUser ? (userLight ? "#c2621f" : "#e8833a") : p.warn
    readonly property color seedLow: isUser ? (userLight ? "#b4562a" : "#d97640") : p.seedLow   // 1–9 seeds

    // ---------- accents (functional) ----------
    // buttons, the active tab, a selected pill
    readonly property color accent:     isUser ? uPrimary : p.accent
    readonly property color accentDark: isUser ? Qt.darker(uPrimary, 1.35) : p.accentDark
    // accent as text; for a user theme the primary itself (the user owns the contrast)
    readonly property color accentText: isUser ? uPrimary : p.accentText
    // downloading, and a problem
    readonly property color down:       isUser ? uPrimary : p.down
    readonly property color downText:   isUser ? uPrimary : p.downText   // the same, as text
    // the upload arrow
    readonly property color amber: isUser ? uSecondary : p.amber
    readonly property color up:    isUser ? Qt.lighter(uSecondary, userLight ? 1.0 : 1.2) : p.up
    // paused: a neutral, from the user's text in their own theme
    readonly property color pausedFill: isUser ? _uInk(0.42) : p.pausedFill
    // a finished item's bar: the done colour, darker so many of them stay calm
    readonly property color doneFill: isUser ? Qt.darker(uTertiary, userLight ? 1.15 : 1.7) : p.doneFill
    // done as a dot or a word, and health (seeds in search, "Installed")
    readonly property color grn: isUser ? uTertiary : p.grn

    // ---------- what sits behind the library ----------
    // "clean": nothing; "theme": the art drawn for the theme; "user": the
    // user's own picture in the same slot (their theme's window picture
    // replaces all of this). Default has no art of its own.
    readonly property string backgroundMode: typeof themeBridge !== "undefined" ? themeBridge.backgroundMode : "theme"
    readonly property var art: (!isUser && backgroundMode === "theme" && p.art) ? p.art : null
    readonly property string artSource: art ? "qrc:/images/themes/" + art.src : ""
    readonly property string userArtSource:
        (!isUser && backgroundMode === "user" && typeof themeBridge !== "undefined")
            ? _fileUrl(themeBridge.userImage) : ""
    readonly property bool hasArt: artSource !== "" || userArtSource !== ""
    // grain or paper over the library (in every background mode), and the
    // terminal's scanlines over the whole window
    readonly property string texture: isUser ? "" : p.texture
    readonly property bool scanlines: !isUser && p.scan
    // Matrix: a cursor on the active tab, lowercase labels; Sakura: 桜 by the logo
    readonly property bool tabCursor: !isUser && p.cursor
    readonly property bool mono: !isUser && p.mono
    readonly property bool kanji: !isUser && p.kanji
    // the logo with a dark body, for light themes
    readonly property string logoSource: isLight ? "qrc:/images/logo-light.svg" : "qrc:/images/logo.svg"

    // ---------- window chrome ----------
    // The window draws its own title strip, with back/forward: the Back
    // buttons inside pages step aside then (they stay where the system draws
    // the title bar, macOS and Linux, which has no arrows).
    readonly property bool titleStripNav: typeof framelessWindow !== "undefined" && framelessWindow === true
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

    // Ctrl+Shift+T: the next built-in theme
    function cycle() {
        var o = Palette.ORDER, i = o.indexOf(key)
        setName(o[(i + 1) % o.length])
    }
}
