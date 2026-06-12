// SPDX-License-Identifier: MIT
// Qt Quick Test for the TFld text field: the `text` alias round-trips through the
// inner TextField for letters, digits and special characters.

import QtQuick
import QtTest
import "qrc:/src/qml/widgets"

Item {
    id: root
    width: 280
    height: 100

    Component {
        id: fldComp
        TFld { width: 240; height: 34 }
    }

    TestCase {
        name: "TFld"
        when: windowShown
        width: 280
        height: 100

        function test_textAliasRoundTrips() {
            var fld = createTemporaryObject(fldComp, root)
            verify(!!fld, "Object exists")

            fld.text = "Hello"
            compare(fld.text, "Hello")

            fld.text = "1234567890"
            compare(fld.text, "1234567890")

            fld.text = "a-b_c.d/e:f"
            compare(fld.text, "a-b_c.d/e:f")
        }

        function test_placeholderAliasRoundTrips() {
            var fld = createTemporaryObject(fldComp, root, { placeholder: "type here" })
            verify(!!fld, "Object exists")
            compare(fld.placeholder, "type here")
        }
    }
}
