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
                { tag: "completed",   key: "completed",   symbol: "✓" },
                { tag: "queued",      key: "queued",      symbol: "⋯" },
                { tag: "paused",      key: "paused",      symbol: "‖" },
                { tag: "missing",     key: "missing",     symbol: "✗" },
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
            var m = mk({ stateKey: "finished" })
            verify(m.symbol !== "✓", "an unknown key must not read as completed")
            var blank = mk({ stateKey: "" })
            verify(blank.symbol !== "✓", "an empty key must not read as completed")
        }

        // The label is the engine's own stateString; both views pass it through
        // untouched, which is what keeps them from disagreeing.
        function test_labelHiddenWhenEmpty() {
            var m = mk({ stateKey: "seeding", label: "" })
            compare(m.children.length, 2)
            verify(!m.children[1].visible, "no label, no second glyph")

            var l = mk({ stateKey: "seeding", label: "Seeding" })
            verify(l.children[1].visible)
            compare(l.children[1].text, "Seeding")
        }

        // The grid badge has to give way to the year/category pill beside it.
        function test_maxLabelWidthClampsLabel() {
            var wide = mk({ stateKey: "downloading", label: "Downloading" })
            var clamped = mk({ stateKey: "downloading", label: "Downloading",
                               maxLabelWidth: 20 })
            verify(wide.children[1].width > clamped.children[1].width)
            compare(clamped.children[1].width, 20)
        }
    }
}
