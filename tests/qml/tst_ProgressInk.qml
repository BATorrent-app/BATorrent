// SPDX-License-Identifier: MIT
// Qt Quick Test for Theme.inkOn — the percentage that rides inside a filled
// ProgressTrack.
//
// This exists because a hardcoded white percentage shipped into the bar and
// measured 2.44:1 against the seeding amber, and 1.55:1 on the matrix theme.
// The rule now derives the ink from the fill's luminance, so the test asserts
// the thing that actually matters: whatever ink comes back clears the 3:1
// floor for large text against every fill the app can produce.

import QtQuick
import QtTest
import "qrc:/src/qml/theme"

Item {
    id: root
    width: 100
    height: 40

    TestCase {
        name: "ProgressInk"
        when: windowShown

        function lin(v) { return v <= 0.03928 ? v / 12.92 : Math.pow((v + 0.055) / 1.055, 2.4) }
        function lum(c) {
            var col = Qt.color(c)
            return 0.2126 * lin(col.r) + 0.7152 * lin(col.g) + 0.0722 * lin(col.b)
        }
        function contrast(a, b) {
            var la = lum(a), lb = lum(b)
            return (Math.max(la, lb) + 0.05) / (Math.min(la, lb) + 0.05)
        }

        // Every fill Theme.fillFor can return, across the themes that redefine
        // them. Named literals rather than Theme lookups so the expectation
        // does not move when the active theme does.
        function test_inkClearsContrastFloor_data() {
            return [
                { tag: "seeding-amber",      fill: "#d99a2b" },
                { tag: "seeding-sakura",     fill: "#c8881f" },
                { tag: "seeding-matrix",     fill: "#a6e22e" },
                { tag: "seeding-darkstar",   fill: "#22d3ee" },
                { tag: "done-green-dark",    fill: "#3fb950" },
                { tag: "done-green-light",   fill: "#2e9c40" },
                { tag: "done-green-matrix",  fill: "#44e070" },
                { tag: "downloading-accent", fill: "#e5332b" },
                { tag: "accent-sakura",      fill: "#d6336c" },
                { tag: "accent-darkstar",    fill: "#a855f7" },
                { tag: "accent-matrix",      fill: "#2be86a" },
                { tag: "paused-dark",        fill: "#54555c" },
                { tag: "paused-light",       fill: "#6a6c73" }
            ]
        }

        function test_inkClearsContrastFloor(data) {
            var ink = Theme.inkOn(data.fill)
            var c = contrast(ink, data.fill)
            verify(c >= 3.0,
                   data.tag + ": " + ink + " on " + data.fill
                   + " is " + c.toFixed(2) + ":1, under the 3:1 floor")
        }

        // The regression itself: white is the wrong answer on the fills that
        // caused the report, and the rule has to actually say so.
        function test_lightFillsGetDarkInk() {
            compare(Theme.inkOn("#d99a2b"), "#101014", "seeding amber")
            compare(Theme.inkOn("#a6e22e"), "#101014", "matrix amber")
            compare(Theme.inkOn("#3fb950"), "#101014", "completed green")
        }

        function test_darkFillsGetWhiteInk() {
            compare(Theme.inkOn("#54555c"), "#ffffff", "paused grey")
            compare(Theme.inkOn("#6a6c73"), "#ffffff", "paused grey, light theme")
        }

        // Darkstar's violet sits just above the crossover (L 0.2154): it looks
        // dark, but dark ink measures 4.80:1 on it against white's 3.96. Pinned
        // because it is exactly the case where eyeballing the answer loses to
        // the luminance, and someone will be tempted to "fix" it back.
        function test_nearCrossoverFollowsLuminanceNotIntuition() {
            compare(Theme.inkOn("#a855f7"), "#101014")
        }

        // Both call shapes have to agree. A string literal reading as NaN is how
        // the rule silently returned white for every fill it was meant to catch.
        function test_stringAndColourAgree() {
            compare(Theme.inkOn("#d99a2b"), Theme.inkOn(Qt.color("#d99a2b")))
            compare(Theme.inkOn("#54555c"), Theme.inkOn(Qt.color("#54555c")))
        }
    
        // inkOn measures r,g,b and ignores alpha, so a translucent wash reads
        // as its opaque colour. Theme.track is 9% white: feeding it here
        // returned black ink and the 0% label vanished into the bar.
        function test_translucentWashIsNotAValidBackground() {
            compare(Theme.inkOn(Qt.rgba(1, 1, 1, 0.09)).toString(),
                    Theme.inkOn("#ffffff").toString())
        }
}
}
