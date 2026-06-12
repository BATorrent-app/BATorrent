// SPDX-License-Identifier: MIT
// Qt Quick Test for the TArea multiline field: the `text` alias round-trips
// through the inner TextArea, including multi-line content.

import QtQuick
import QtTest
import "qrc:/src/qml/widgets"

Item {
    id: root
    width: 420
    height: 140

    Component {
        id: areaComp
        TArea { width: 400; height: 88 }
    }

    TestCase {
        name: "TArea"
        when: windowShown
        width: 420
        height: 140

        function test_textAliasRoundTrips() {
            var ta = createTemporaryObject(areaComp, root)
            verify(!!ta, "Object exists")

            ta.text = "single line 123"
            compare(ta.text, "single line 123")

            ta.text = "line one\nline two\nline three"
            compare(ta.text, "line one\nline two\nline three")
        }

        function test_placeholderAliasRoundTrips() {
            var ta = createTemporaryObject(areaComp, root, { placeholder: "notes…" })
            verify(!!ta, "Object exists")
            compare(ta.placeholder, "notes…")
        }
    }
}
