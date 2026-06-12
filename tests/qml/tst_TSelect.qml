// SPDX-License-Identifier: MIT
// Qt Quick Test for the TSelect dropdown (ComboBox): the model drives count and
// the displayed current text.

import QtQuick
import QtTest
import "qrc:/src/qml/widgets"

Item {
    id: root
    width: 240
    height: 120

    Component {
        id: selectComp
        TSelect { width: 200; height: 30; model: ["Alpha", "Beta", "Gamma"] }
    }

    TestCase {
        name: "TSelect"
        when: windowShown
        width: 240
        height: 120

        function test_modelDrivesCount() {
            var sel = createTemporaryObject(selectComp, root)
            verify(!!sel, "Object exists")
            compare(sel.count, 3)
        }

        function test_currentTextReflectsSelection() {
            var sel = createTemporaryObject(selectComp, root)
            verify(!!sel, "Object exists")
            compare(sel.currentText, "Alpha")
        }
    }
}
