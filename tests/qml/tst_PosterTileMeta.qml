// SPDX-License-Identifier: MIT
// The state line under a Downloads card. On a narrow card "Downloading · 3m"
// beside "2.11/6.29 GB" cut the state itself to "Down…"; the line now gives
// way in order: the time left, then the size, and the state is never cut.

import QtQuick
import QtTest
import "qrc:/src/qml/widgets"

Item {
    id: root
    width: 400
    height: 100

    Component {
        id: metaComp
        PosterTileMeta {
            tile: ({
                stateKey: "downloading", isDownloading: true, isGroup: false,
                stateDetail: "", stateDetailShort: "", progress: 0.33, etaSec: 180,
                downloaded: "2.11 GB", size: "6.29 GB", year: 0, genres: "",
                metaTitle: "Some Film", torrentName: "Some.Film.2024", fileKind: "mkv",
                upRate: 0, upSpeed: "", queuePos: 0, groupCaption: "", groupRepeats: 0
            })
            win: ({})
        }
    }

    TextMetrics { id: tm }

    TestCase {
        name: "PosterTileMeta"
        when: windowShown

        function lineText(m) {
            // the second row holds the dot, the state line and the size
            var row = m.children[1]
            return { left: row.children[1].text, right: row.children[2].text }
        }
        function advance(m, text, col) {
            tm.font = m.children[1].children[col === undefined ? 1 : col].font
            tm.text = text
            return tm.advanceWidth
        }

        function test_wideShowsEverything() {
            var m = createTemporaryObject(metaComp, root, { width: 360 })
            verify(m.fitsFull)
            compare(lineText(m).left, "state_downloading · 3m")
            compare(lineText(m).right, "2.11/6.29 GB")
        }

        function test_narrowDropsTheTimeFirst() {
            var m = createTemporaryObject(metaComp, root, { width: 360 })
            // just too narrow for the time, wide enough for state and size
            m.width = advance(m, "state_downloading") + advance(m, "2.11/6.29 GB", 2) + 6 + 13 + 4
            verify(!m.fitsFull)
            verify(m.fitsShort)
            compare(lineText(m).left, "state_downloading")
            compare(lineText(m).right, "2.11/6.29 GB")
        }

        function test_narrowerDropsTheSizeAndKeepsTheState() {
            var m = createTemporaryObject(metaComp, root, { width: 360 })
            m.width = advance(m, "state_downloading") + 13 + 2
            verify(!m.fitsShort)
            compare(lineText(m).left, "state_downloading")
            compare(lineText(m).right, "")
        }
    }
}
