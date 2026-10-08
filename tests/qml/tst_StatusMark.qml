// SPDX-License-Identifier: MIT
// Qt Quick Test for StatusMark — the single state→symbol/colour mapping the
// grid badge and the list row both draw.
//
// This is worth pinning because the bug it exists to prevent already shipped:
// the two views each built their own status glyph, and 4.8.0 could show SEEDING
// on a tile while the list row for the same torrent said DOWNLOADING. The keys
// under test are exactly the ones torrentStateKey() produces (torrent/types.h);
// a key that is not in that set must never render as a confident state.

import QtQuick
import QtTest
import "qrc:/src/qml/widgets"
import "qrc:/src/qml/theme"

Item {
    id: root
    width: 200
    height: 60

    Component {
        id: markComp
        StatusMark {}
    }

    TestCase {
        name: "StatusMark"
        when: windowShown

        function mk(props) { return createTemporaryObject(markComp, root, props) }

        function test_symbolPerState_data() {
            return [
                { tag: "downloading", key: "downloading", symbol: "↓" },
                { tag: "seeding",     key: "seeding",     symbol: "↑" },
                { tag: "queued",      key: "queued",      symbol: "…" },
                { tag: "missing",     key: "missing",     symbol: "×" },
                { tag: "error",       key: "error",       symbol: "!" }
            ]
        }

        function test_symbolPerState(data) {
            var m = mk({ stateKey: data.key })
            verify(!!m, "Object exists")
            compare(m.symbol, data.symbol)
        }

        // The three Sherwan named, and the three the colour language turns on:
        // up is amber, down is the brand accent, done is green.
        function test_colourPerState_data() {
            return [
                { tag: "seeding",   key: "seeding",   colour: Theme.amber },
                { tag: "completed", key: "completed", colour: Theme.grn },
                { tag: "downloading", key: "downloading", colour: Theme.accent },
                { tag: "error",     key: "error",     colour: Theme.accent }
            ]
        }

        function test_colourPerState(data) {
            var m = mk({ stateKey: data.key })
            compare(m.symbolColor.toString(), data.colour.toString())
        }

        // A download that is nominally running but has nothing arriving keeps
        // its arrow — the state has not changed — and only drops to amber.
        function test_stalledTintsWithoutChangingSymbol() {
            var m = mk({ stateKey: "downloading", stalled: true })
            compare(m.symbol, "↓")
            compare(m.symbolColor.toString(), Theme.amber.toString())
        }

        // "finished" is a key torrentStateKey() has never produced. It must not
        // quietly resolve to a tick: a phantom state rendering as a confident
        // one is how a wrong status becomes invisible.
        function test_unknownKeyDoesNotClaimCompletion() {
            var done = mk({ stateKey: "completed" })
            var tick = done.symbolIcon
            verify(tick !== "", "completed is drawn by an icon")

            var m = mk({ stateKey: "finished" })
            verify(m.symbolIcon !== tick, "an unknown key must not read as completed")
            var blank = mk({ stateKey: "" })
            verify(blank.symbolIcon !== tick, "an empty key must not read as completed")
        }

        // The label is the engine's own stateString; both views pass it through
        // untouched, which is what keeps them from disagreeing.
        function test_labelHiddenWhenEmpty() {
            var m = mk({ stateKey: "seeding", label: "" })
            verify(!findChild(m, "statusLabel").visible, "no label, no second glyph")

            var l = mk({ stateKey: "seeding", label: "Seeding" })
            var lbl = findChild(l, "statusLabel")
            verify(lbl.visible)
            compare(lbl.text, "Seeding")
        }

        // Pause is the one mark drawn as an icon: no glyph in the bundled font
        // gives two stubby bars, and ‖ was being resolved by a fallback
        // typeface that differs per platform.
        function test_shapeStatesDrawIconsAndNoGlyph_data() {
            return [
                { tag: "paused",    key: "paused",    spins: false },
                { tag: "completed", key: "completed", spins: false },
                { tag: "fetching",  key: "fetching",  spins: true }
            ]
        }

        function test_shapeStatesDrawIconsAndNoGlyph(data) {
            var m = mk({ stateKey: data.key })
            verify(m.symbolIcon !== "", data.key + " has an icon")
            compare(m.symbol, "")
            compare(m.spinning, data.spins)
            verify(!findChild(m, "statusGlyph").visible, "the glyph stands down")
        }

        function test_glyphStatesDoNotSpin() {
            var m = mk({ stateKey: "seeding" })
            compare(m.symbolIcon, "")
            verify(!m.spinning)
            verify(findChild(m, "statusGlyph").visible)
        }

        // The grid badge has to give way to the year/category pill beside it.
        function test_maxLabelWidthClampsLabel() {
            var wide = mk({ stateKey: "downloading", label: "Downloading" })
            var clamped = mk({ stateKey: "downloading", label: "Downloading",
                               maxLabelWidth: 20 })
            verify(findChild(wide, "statusLabel").width
                   > findChild(clamped, "statusLabel").width)
            compare(findChild(clamped, "statusLabel").width, 20)
        }
    }
}
