// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// The built-in themes, as the design hands them over: each colour is an
// OKLCH triple (lightness, chroma, hue) copied from the theme boards, and
// each theme also brings its own title face, interface face, corners, tab
// lettering, texture and background art. Default is the app as it was
// before themes had a character of their own and keeps its exact hex values.
//
// What never changes between themes: layout, sizes, and what the state
// colours mean (downloading, done, upload arrow, warning, paused). Numbers
// (speeds, sizes, percentages) always stay in an interface face.
.pragma library

// ---------- OKLCH -> sRGB ----------
function _gam(c) { return c <= 0.0031308 ? 12.92 * c : 1.055 * Math.pow(c, 1 / 2.4) - 0.055 }
function _lin(c) { return c <= 0.04045 ? c / 12.92 : Math.pow((c + 0.055) / 1.055, 2.4) }

function hexOf(L, C, H) {
    var h = H * Math.PI / 180, a = C * Math.cos(h), b = C * Math.sin(h)
    var l = Math.pow(L + 0.3963377774 * a + 0.2158037573 * b, 3)
    var m = Math.pow(L - 0.1055613458 * a - 0.0638541728 * b, 3)
    var s = Math.pow(L - 0.0894841775 * a - 1.2914855480 * b, 3)
    return "#" + [4.0767416621 * l - 3.3077115913 * m + 0.2309699292 * s,
                  -1.2684380046 * l + 2.6097574011 * m - 0.3413193965 * s,
                  -0.0041960863 * l - 0.7034186147 * m + 1.7076147010 * s]
        .map(function (x) {
            var n = Math.round(_gam(Math.max(0, Math.min(1, x))) * 255)
            return (n < 16 ? "0" : "") + n.toString(16)
        }).join("")
}

function toOklch(hex) {
    var h = hex.replace("#", "")
    var r = _lin(parseInt(h.substr(0, 2), 16) / 255)
    var g = _lin(parseInt(h.substr(2, 2), 16) / 255)
    var b = _lin(parseInt(h.substr(4, 2), 16) / 255)
    var l = Math.cbrt(0.4122214708 * r + 0.5363325363 * g + 0.0514459929 * b)
    var m = Math.cbrt(0.2119034982 * r + 0.6806995451 * g + 0.1073969566 * b)
    var s = Math.cbrt(0.0883024619 * r + 0.2817188376 * g + 0.6299787005 * b)
    var A = 1.9779984951 * l - 2.4285922050 * m + 0.4505937099 * s
    var B = 0.0259040371 * l + 0.7827717662 * m - 0.8086757660 * s
    return [0.2104542553 * l + 0.7936177850 * m - 0.0040720468 * s,
            Math.sqrt(A * A + B * B), (Math.atan2(B, A) * 180 / Math.PI + 360) % 360]
}

// #AARRGGBB, which a QML color property takes as is
function alpha(hex, a) {
    var n = Math.round(Math.max(0, Math.min(1, a)) * 255)
    return "#" + (n < 16 ? "0" : "") + n.toString(16) + hex.replace("#", "")
}

function luminance(hex) {
    var h = hex.replace("#", "")
    var v = [0, 2, 4].map(function (i) { return _lin(parseInt(h.substr(i, 2), 16) / 255) })
    return 0.2126 * v[0] + 0.7152 * v[1] + 0.0722 * v[2]
}
function contrast(a, b) {
    var x = luminance(a), y = luminance(b)
    return (Math.max(x, y) + 0.05) / (Math.min(x, y) + 0.05)
}
// dark ink on a bright fill, white on the rest (0.19: where the two contrasts cross)
function inkOn(hex) { return luminance(hex) > 0.19 ? "#101014" : "#ffffff" }

// The accent as text: moved in lightness, hue kept, until it reads at 4.5:1
// on the panel. The boards only give the accent as a fill.
function _readable(lch, against, lighter) {
    var L = lch[0]
    for (var i = 0; i < 40; i++) {
        var hex = hexOf(L, lch[1], lch[2])
        if (contrast(hex, against) >= 4.5) return hex
        L += lighter ? 0.012 : -0.012
    }
    return hexOf(L, lch[1], lch[2])
}

// ---------- the themes ----------
// Colour keys are the boards' own: nav bg panel elev field (surfaces), tile
// (a card with no art), stack1/stack2 (the covers peeking above a series),
// t1-t4 (text), hair (lines; 4th value is its alpha), accent (buttons, the
// active tab), down (downloading), doneDot/doneFill (finished), up (upload
// arrow), warn, paused.
var THEMES = {
    // the app as it was: hex as tuned against the downloads design
    default: {
        name: "Default", light: false,
        hex: {
            nav: "#0c0c0e", bg: "#0e0e10", panel: "#141416", elev: "#18181b", field: "#0b0c0d",
            tile: "#161618", stack1: "#252529", stack2: "#1c1c20",
            t1: "#f3f3f4", tSoft: "#d9dade", t2: "#b4b5ba", t3: "#999a9f", t4: "#7d7e87",
            accent: "#e5332b", accentDark: "#c01f18", accentText: "#ec6a64",
            down: "#e5332b", downText: "#ec6a64", doneDot: "#3fb950", doneFill: "#24692f", up: "#e0b454", amber: "#d99a2b",
            warn: "#e8833a", seedLow: "#d97640", paused: "#54555c"
        },
        // lines and washes: white at these strengths
        washes: { ink: "#ffffff", hair: 0.08, hairSoft: 0.05, hover: 0.035, hover2: 0.06, track: 0.09, sel: 0.09 },
        display: "IBM Plex Sans", sans: "IBM Plex Sans",
        r: 8, rCard: 8, keepShape: true,
        tab: null, title: null,
        art: null, texture: ""
    },
    // the old "dark" art becomes this: a cinema, the Melies moon on the wall
    cinema: {
        name: "Cinema", light: false,
        lch: {
            nav: [.12, 0, 0], bg: [.145, 0, 0], panel: [.18, 0, 0], elev: [.215, 0, 0], field: [.12, 0, 0],
            tile: [.2, 0, 0], stack1: [.24, 0, 0], stack2: [.2, 0, 0],
            t1: [.965, 0, 0], t2: [.81, 0, 0], t3: [.71, 0, 0], t4: [.635, 0, 0], hair: [1, 0, 0, .08],
            accent: [.62, .21, 29], down: [.62, .21, 29], doneDot: [.72, .17, 145], doneFill: [.45, .11, 145],
            up: [.84, .14, 88], warn: [.74, .16, 55], paused: [.5, 0, 0]
        },
        display: "Barlow Condensed", sans: "IBM Plex Sans", r: 4, rCard: 4,
        tab: { size: 15, track: 0.06, caps: "upper", italic: false, weight: 600, offWeight: 500 },
        title: { size: 22, weight: 700, track: 0.02, caps: "upper", italic: false },
        art: { src: "dark-melies.png", width: 0.44, opacity: 0.4, fade: true, fit: "cover", alignY: 0.3 },
        texture: "grain"
    },
    // a natural-history plate: warm paper, Haeckel's medusae in sepia
    light: {
        name: "Light", light: true,
        lch: {
            nav: [.915, .014, 85], bg: [.962, .012, 85], panel: [.99, .006, 85], elev: [.935, .014, 85], field: [1, 0, 0],
            tile: [.92, .016, 85], stack1: [.88, .016, 85], stack2: [.9, .014, 85],
            t1: [.2, .012, 60], t2: [.35, .012, 60], t3: [.44, .012, 60], t4: [.5, .012, 60], hair: [.2, .012, 60, .12],
            accent: [.54, .2, 33], down: [.54, .2, 33], doneDot: [.52, .13, 150], doneFill: [.58, .14, 150],
            up: [.53, .11, 92], warn: [.6, .16, 58], paused: [.64, .012, 85]
        },
        display: "Newsreader", sans: "IBM Plex Sans", r: 3, rCard: 2,
        tab: { size: 16, track: 0, caps: "", italic: false, weight: 600, offWeight: 500 },
        title: { size: 22, weight: 600, track: -0.005, caps: "", italic: false },
        art: { src: "light-haeckel-b.png", width: 0.54, opacity: 0.5, fade: false, fit: "contain", alignY: 0 },
        texture: "paper"
    },
    // Van Gogh's Rhone: all blue, the one accent is the lamp yellow of the
    // quay; downloading is coral, the only warm colour that is not light
    midnight: {
        name: "Midnight", light: false,
        lch: {
            nav: [.13, .04, 266], bg: [.17, .05, 263], panel: [.21, .06, 262], elev: [.25, .065, 262], field: [.14, .045, 263],
            tile: [.24, .06, 262], stack1: [.3, .07, 262], stack2: [.26, .065, 262],
            t1: [.96, .02, 255], t2: [.83, .05, 258], t3: [.73, .06, 260], t4: [.66, .065, 262], hair: [.8, .08, 255, .14],
            accent: [.84, .12, 95], down: [.68, .18, 25], doneDot: [.77, .14, 165], doneFill: [.5, .1, 165],
            up: [.86, .13, 92], warn: [.76, .15, 58], paused: [.5, .045, 262]
        },
        display: "Instrument Serif", sans: "IBM Plex Sans", r: 8, rCard: 6,
        tab: { size: 19, track: 0, caps: "", italic: true, weight: 400, offWeight: 400 },
        title: { size: 26, weight: 400, track: 0, caps: "", italic: true },
        art: { src: "midnight-rhone.jpg", width: 0.70, opacity: 0.6, fade: true, fit: "cover", alignY: 0.2 },
        texture: ""
    },
    // a cherry branch on washi; the strong pink is only the nav band and the
    // kanji beside the logo, done is the blue-green of the leaves
    sakura: {
        name: "Sakura", light: true,
        lch: {
            nav: [.9, .05, 355], bg: [.972, .012, 20], panel: [.993, .005, 20], elev: [.95, .02, 355], field: [1, 0, 0],
            tile: [.93, .03, 355], stack1: [.88, .04, 355], stack2: [.91, .035, 355],
            t1: [.22, .04, 345], t2: [.35, .05, 345], t3: [.44, .05, 345], t4: [.5, .045, 345], hair: [.3, .05, 345, .12],
            accent: [.52, .18, 6], down: [.52, .18, 6], doneDot: [.52, .12, 165], doneFill: [.6, .1, 165],
            up: [.53, .11, 88], warn: [.6, .15, 52], paused: [.66, .03, 355]
        },
        display: "Shippori Mincho", sans: "Zen Kaku Gothic New", r: 10, rCard: 8,
        tab: { size: 16, track: 0.02, caps: "", italic: false, weight: 700, offWeight: 600 },
        title: { size: 22, weight: 700, track: 0, caps: "", italic: false },
        art: { src: "sakura-branch.svg", width: 0.64, opacity: 1, fade: false, fit: "cover", alignY: 0, bleed: 10, tall: 1.04 },
        texture: "paper", kanji: true
    },
    // the Carina nebula in violet; the primary button glows
    darkstar: {
        name: "Dark Star", light: false,
        lch: {
            nav: [.12, .05, 300], bg: [.15, .06, 300], panel: [.19, .075, 302], elev: [.24, .085, 304], field: [.12, .05, 300],
            tile: [.23, .08, 304], stack1: [.29, .09, 304], stack2: [.25, .085, 304],
            t1: [.96, .025, 300], t2: [.83, .06, 300], t3: [.73, .075, 302], t4: [.66, .08, 304], hair: [.8, .1, 300, .13],
            accent: [.68, .23, 318], down: [.64, .2, 25], doneDot: [.75, .16, 150], doneFill: [.48, .11, 150],
            up: [.86, .14, 88], warn: [.76, .15, 58], paused: [.5, .05, 300]
        },
        display: "Space Grotesk", sans: "IBM Plex Sans", r: 12, rCard: 10,
        tab: { size: 14.5, track: 0.01, caps: "", italic: false, weight: 600, offWeight: 500 },
        title: { size: 22, weight: 600, track: -0.01, caps: "", italic: false },
        art: { src: "darkstar-nebula2.png", width: 0.74, opacity: 0.9, fade: true, fit: "cover", alignY: 0.4, alignX: 0.3, screen: true },
        texture: "", glow: true
    },
    // a phosphor terminal: everything one green, katakana rain behind the
    // cards, scanlines over the window, square corners, a cursor on the tab
    matrix: {
        name: "Matrix", light: false,
        lch: {
            nav: [.09, .03, 150], bg: [.12, .04, 150], panel: [.16, .05, 150], elev: [.21, .06, 150], field: [.09, .03, 150],
            tile: [.2, .06, 150], stack1: [.26, .07, 150], stack2: [.22, .065, 150],
            t1: [.93, .11, 150], t2: [.81, .13, 150], t3: [.71, .13, 150], t4: [.64, .12, 150], hair: [.8, .15, 150, .16],
            accent: [.87, .2, 148], down: [.64, .21, 27], doneDot: [.94, .13, 150], doneFill: [.62, .15, 150],
            up: [.88, .15, 105], warn: [.78, .16, 70], paused: [.48, .06, 150]
        },
        display: "IBM Plex Mono", sans: "IBM Plex Mono", r: 0, rCard: 0,
        tab: { size: 13.5, track: 0, caps: "lower", italic: false, weight: 600, offWeight: 400 },
        title: { size: 18, weight: 600, track: 0, caps: "", italic: false },
        art: { src: "matrix-rain.png", width: 1.0, opacity: 0.22, fade: false, fit: "cover", alignY: 0.5, screen: true },
        texture: "", scan: true, cursor: true, mono: true
    }
}

var ORDER = ["default", "cinema", "light", "midnight", "sakura", "darkstar", "matrix"]

// Old saved names: "dark" was the app as it is now (Default here), and the
// old Custom theme is now a theme of the user's own.
function canonical(name) {
    if (name === "dark") return "default"
    return THEMES[name] ? name : "default"
}

var _cache = {}

// Every colour and style token for one built-in theme.
function build(name) {
    name = canonical(name)
    if (_cache[name]) return _cache[name]
    var t = THEMES[name]
    var p = { key: name, name: t.name, isLight: t.light }
    if (t.hex) {
        for (var k in t.hex) p[k] = t.hex[k]
        var w = t.washes
        p.hair = alpha(w.ink, w.hair); p.hairSoft = alpha(w.ink, w.hairSoft)
        p.hover = alpha(w.ink, w.hover); p.hover2 = alpha(w.ink, w.hover2)
        p.track = alpha(w.ink, w.track); p.sel = alpha(p.accent, w.sel)
    } else {
        var c = t.lch
        for (var key in c) if (key !== "hair") p[key] = hexOf(c[key][0], c[key][1], c[key][2])
        // between t1 and t2: menu rows, body copy that should not shout
        p.tSoft = hexOf((c.t1[0] + c.t2[0]) / 2, (c.t1[1] + c.t2[1]) / 2, c.t1[2])
        var acc = c.accent
        p.accentDark = hexOf(Math.max(0, acc[0] - 0.09), acc[1], acc[2])
        p.accentText = _readable(acc, p.panel, !t.light)
        p.downText = _readable(c.down, p.panel, !t.light)
        p.amber = p.up
        p.seedLow = hexOf(c.warn[0] - 0.04, c.warn[1], c.warn[2])
        // the boards' line colour and its strengths for the softer variants
        var hairInk = hexOf(c.hair[0], c.hair[1], c.hair[2]), ha = c.hair[3]
        p.hair = alpha(hairInk, ha); p.hairSoft = alpha(hairInk, ha * 0.55)
        p.track = alpha(hairInk, ha * 1.2); p.hover2 = alpha(hairInk, 0.08)
        p.hover = alpha(hairInk, 0.045); p.sel = alpha(p.accent, 0.12)
    }
    p.accentTint = alpha(p.accent, 0.12)
    // The label on an accent button. Not inkOn()'s crossing point: a saturated
    // red or violet sits just past it and came out with black text, where a
    // button reads white. Dark ink only on an accent that is light outright
    // (Midnight's lamp yellow, Matrix's phosphor).
    if (!p.inkOnAccent) p.inkOnAccent = luminance(p.accent) > 0.4 ? "#101014" : "#ffffff"
    p.grn = p.doneDot
    p.tileBlank = p.tile; p.stackMid = p.stack1; p.stackBack = p.stack2
    p.pausedFill = p.paused
    // A title's page stays dark on every theme; dark themes give it their
    // own surfaces, light ones borrow Default's.
    var stageFrom = t.light ? THEMES.default.hex : p
    p.stageBg = t.light || t.hex ? "#0a0a0b" : hexOf(Math.max(0, c.bg[0] - 0.04), c.bg[1], c.bg[2])
    p.stagePanel = t.light ? "#141416" : stageFrom.panel
    p.stageField = t.light ? "#1a1a1d" : (t.hex ? "#1a1a1d" : stageFrom.elev)
    // style
    p.display = t.display; p.sans = t.sans
    p.r = t.r; p.rCard = t.rCard; p.keepShape = !!t.keepShape
    p.tab = t.tab; p.title = t.title
    p.art = t.art; p.texture = t.texture
    p.kanji = !!t.kanji; p.glow = !!t.glow; p.scan = !!t.scan; p.cursor = !!t.cursor; p.mono = !!t.mono
    _cache[name] = p
    return p
}
