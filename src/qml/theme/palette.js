// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// The theme palettes. Dark and Light are the two references, tuned by hand
// against the design files. Every other theme is one of them turned to its
// own hue: each surface and text step keeps the reference's lightness (in
// OKLCH, where equal steps look equal), and only hue and a small, fixed
// amount of colour change. That is what keeps the hierarchy the same
// everywhere: the side panel sits one step above the page in Midnight just as
// it does in Dark, secondary text is as far from the body text, and nothing
// has to be re-checked for contrast by eye when a theme is added.
//
// Colour rides on the accent and on the faint tint of the surfaces. Text
// stays almost neutral, because tinted body copy reads as low contrast long
// before it measures as such. The state colours (done, upload, warning) are
// chosen per theme so they never land on the accent's hue: in a green theme
// "finished" cannot be green too, or a finished bar looks like a running one.
.pragma library

// ---------- OKLCH <-> sRGB ----------
function _lin(c) { return c <= 0.04045 ? c / 12.92 : Math.pow((c + 0.055) / 1.055, 2.4) }
function _gam(c) { return c <= 0.0031308 ? 12.92 * c : 1.055 * Math.pow(c, 1 / 2.4) - 0.055 }

function toOklch(hex) {
    var h = hex.replace("#", "")
    var r = _lin(parseInt(h.substr(0, 2), 16) / 255)
    var g = _lin(parseInt(h.substr(2, 2), 16) / 255)
    var b = _lin(parseInt(h.substr(4, 2), 16) / 255)
    var l = Math.cbrt(0.4122214708 * r + 0.5363325363 * g + 0.0514459929 * b)
    var m = Math.cbrt(0.2119034982 * r + 0.6806995451 * g + 0.1073969566 * b)
    var s = Math.cbrt(0.0883024619 * r + 0.2817188376 * g + 0.6299787005 * b)
    var L = 0.2104542553 * l + 0.7936177850 * m - 0.0040720468 * s
    var A = 1.9779984951 * l - 2.4285922050 * m + 0.4505937099 * s
    var B = 0.0259040371 * l + 0.7827717662 * m - 0.8086757660 * s
    return { L: L, C: Math.sqrt(A * A + B * B), H: (Math.atan2(B, A) * 180 / Math.PI + 360) % 360 }
}

function _rgb(L, C, H) {
    var a = C * Math.cos(H * Math.PI / 180), b = C * Math.sin(H * Math.PI / 180)
    var l = Math.pow(L + 0.3963377774 * a + 0.2158037573 * b, 3)
    var m = Math.pow(L - 0.1055613458 * a - 0.0638541728 * b, 3)
    var s = Math.pow(L - 0.0894841775 * a - 1.2914855480 * b, 3)
    return [ 4.0767416621 * l - 3.3077115913 * m + 0.2309699292 * s,
            -1.2684380046 * l + 2.6097574011 * m - 0.3413193965 * s,
            -0.0041960863 * l - 0.7034186147 * m + 1.7076147010 * s]
}

// Out-of-gamut colours lose chroma, never lightness: the step must hold.
function oklch(L, C, H) {
    var rgb = _rgb(L, C, H)
    for (var i = 0; i < 24; i++) {
        if (rgb.every(function (v) { return v >= -0.0005 && v <= 1.0005 })) break
        C *= 0.9
        rgb = _rgb(L, C, H)
    }
    return "#" + rgb.map(function (v) {
        var n = Math.round(Math.max(0, Math.min(1, _gam(Math.max(0, v)))) * 255)
        return (n < 16 ? "0" : "") + n.toString(16)
    }).join("")
}

// #AARRGGBB, which a QML color property takes as is
function alpha(hex, a) {
    var n = Math.round(a * 255)
    return "#" + (n < 16 ? "0" : "") + n.toString(16) + hex.replace("#", "")
}

// ---------- the two references ----------
var REF = {
    dark: {
        light: false,
        nav: "#0c0c0e", bg: "#0e0e10", panel: "#141416", elev: "#18181b", field: "#0b0c0d",
        t1: "#f3f3f4", tSoft: "#d9dade", t2: "#b4b5ba", t3: "#999a9f", t4: "#7d7e87",
        tileBlank: "#161618", stackBack: "#1c1c20", stackMid: "#252529", pausedFill: "#54555c",
        stageBg: "#0a0a0b", stagePanel: "#141416", stageField: "#1a1a1d",
        // alpha of t1 for lines and washes
        aHair: 0.08, aHairSoft: 0.05, aHover: 0.035, aHover2: 0.06, aTrack: 0.09
    },
    light: {
        light: true,
        nav: "#e7e9ec", bg: "#ffffff", panel: "#f4f5f7", elev: "#eef0f2", field: "#ffffff",
        t1: "#16171a", tSoft: "#2a2c31", t2: "#44464d", t3: "#5c5d64", t4: "#727581",
        pausedFill: "#6a6c73",
        aHair: 0.11, aHairSoft: 0.06, aHover: 0.045, aHover2: 0.07, aTrack: 0.10
    }
}

// What each theme sets. `hue` and `tint` (the chroma of the surfaces) turn the
// reference; `ink` is the chroma of the text, kept low on purpose; `dL` shifts
// the whole ladder, so a theme can sit deeper than its reference while every
// step between surfaces stays the same.
var SEEDS = {
    dark: {
        ref: "dark",
        accent: "#e5332b", accentDark: "#c01f18", accentText: "#ec6a64",
        amber: "#d99a2b", up: "#e0b454", doneFill: "#24692f", grn: "#3fb950",
        warn: "#e8833a", seedLow: "#d97640", sel: 0.09, inkOnAccent: "#ffffff"
    },
    light: {
        ref: "light",
        accent: "#e5332b", accentDark: "#c01f18", accentText: "#cf2a22",
        amber: "#d99a2b", up: "#9a6710", doneFill: "#3d9a4c", grn: "#2e9c40",
        warn: "#c2621f", seedLow: "#b4562a", sel: 0.10, inkOnAccent: "#ffffff"
    },
    // deep navy; the red accent stays, it is what makes it BATorrent
    midnight: {
        ref: "dark", hue: 262, tint: 0.040, ink: 0.020, dL: -0.010,
        accent: "#e5332b", accentDark: "#c01f18", accentText: "#ef6a64",
        amber: "#d99a2b", up: "#e0b454", doneFill: "#24692f", grn: "#3fb950",
        warn: "#e8833a", seedLow: "#d97640", sel: 0.12, inkOnAccent: "#ffffff"
    },
    // violet night; upload moves to cyan so it never meets the purple
    darkstar: {
        ref: "dark", hue: 298, tint: 0.046, ink: 0.024, dL: -0.010,
        accent: "#a855f7", accentDark: "#7e22ce", accentText: "#c084fc",
        amber: "#22d3ee", up: "#67e8f9", doneFill: "#24692f", grn: "#3fb950",
        warn: "#e8833a", seedLow: "#d97640", sel: 0.14, inkOnAccent: "#ffffff"
    },
    // terminal green; "done" is teal and upload amber, so neither reads as
    // the green of a running download, and the accent carries dark ink
    matrix: {
        ref: "dark", hue: 155, tint: 0.030, ink: 0.020, dL: -0.016,
        accent: "#2bd968", accentDark: "#18a84c", accentText: "#5ef08f",
        amber: "#d9a63a", up: "#e6c15c", doneFill: "#1e6b6e", grn: "#3cc7c0",
        warn: "#e8833a", seedLow: "#d97640", sel: 0.13, inkOnAccent: "#04140a"
    },
    // blush rather than candy: Light's ladder with a rose tint, plum text
    sakura: {
        ref: "light", hue: 352, tint: 0.032, ink: 0.034, dL: -0.022,
        accent: "#d6336c", accentDark: "#be185d", accentText: "#be185d",
        amber: "#c8881f", up: "#9a6710", doneFill: "#3d9a4c", grn: "#2e9c40",
        warn: "#c2621f", seedLow: "#b4562a", sel: 0.11, inkOnAccent: "#ffffff"
    }
}

function _turn(hex, seed, chroma, shift) {
    var c = toOklch(hex)
    return oklch(Math.max(0, Math.min(1, c.L + shift)), chroma, seed.hue)
}

var _cache = {}

// Every token for one theme, as colour strings. Unknown names get Dark.
function build(name) {
    if (_cache[name]) return _cache[name]
    var seed = SEEDS[name] || SEEDS.dark
    var ref = REF[seed.ref]
    var p = {}
    var surfaces = ["nav", "bg", "panel", "elev", "field", "tileBlank", "stackBack", "stackMid",
                    "pausedFill", "stageBg", "stagePanel", "stageField"]
    var inks = ["t1", "tSoft", "t2", "t3", "t4"]
    if (seed.hue === undefined) {
        surfaces.concat(inks).forEach(function (k) { if (ref[k]) p[k] = ref[k] })
    } else {
        var dL = seed.dL || 0
        surfaces.forEach(function (k) {
            if (!ref[k]) return
            // the input field is the page's own white on light themes
            p[k] = (ref.light && k === "field") ? ref[k] : _turn(ref[k], seed, seed.tint, dL)
        })
        // body text stays nearest neutral; the quieter steps carry a little more hue
        inks.forEach(function (k) {
            var c = (k === "t1" || k === "tSoft") ? seed.ink * 0.6 : seed.ink
            p[k] = _turn(ref[k], seed, c, 0)
        })
    }
    p.isLight = ref.light;
    ["accent", "accentDark", "accentText", "amber", "up", "doneFill", "grn", "warn", "seedLow",
     "inkOnAccent"].forEach(function (k) { p[k] = seed[k] })
    // lines and washes: plain white or black on the references, as tuned;
    // the theme's own (barely tinted) text colour elsewhere
    var ink = seed.hue === undefined ? (ref.light ? "#000000" : "#ffffff") : p.t1
    p.hair = alpha(ink, ref.aHair)
    p.hairSoft = alpha(ink, ref.aHairSoft)
    p.hover = alpha(ink, ref.aHover)
    p.hover2 = alpha(ink, ref.aHover2)
    p.track = alpha(ink, ref.aTrack)
    p.sel = alpha(seed.accent, seed.sel)
    p.accentTint = alpha(seed.accent, 0.12)
    _cache[name] = p
    return p
}

var ORDER = ["dark", "light", "midnight", "sakura", "darkstar", "matrix"]
