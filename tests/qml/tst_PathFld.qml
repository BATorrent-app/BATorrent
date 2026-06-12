// SPDX-License-Identifier: MIT
// Qt Quick Test for PathFld: the `text` alias plumbs through to the inner field.
// (The browse button label binds to i18n, which is absent under test — that
// binding stays empty here; the path text is what matters and is asserted.)

import QtQuick
import QtTest
import "qrc:/src/qml/widgets"

Item {
    id: root
    width: 360
    height: 100

    Component {
        id: pathComp
        PathFld { width: 340 }
    }

    TestCase {
        name: "PathFld"
        when: windowShown
        width: 360
        height: 100

        function test_textAliasRoundTrips() {
            var pth = createTemporaryObject(pathComp, root)
            verify(!!pth, "Object exists")

            pth.text = "/Users/me/Downloads"
            compare(pth.text, "/Users/me/Downloads")

            pth.text = "C:\\Users\\me\\Downloads"
            compare(pth.text, "C:\\Users\\me\\Downloads")
        }
    }
}
