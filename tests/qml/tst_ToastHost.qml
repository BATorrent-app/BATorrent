// SPDX-License-Identifier: MIT
// Qt Quick Test for ToastHost: show() pushes a card onto the stack and maps the
// numeric level to the right kind tag.

import QtQuick
import QtTest
import "qrc:/src/qml/widgets"

Item {
    id: root
    width: 600
    height: 400

    Component {
        id: hostComp
        ToastHost {}
    }

    TestCase {
        name: "ToastHost"
        when: windowShown
        width: 600
        height: 400

        function test_showInsertsToast() {
            var host = createTemporaryObject(hostComp, root)
            verify(!!host, "Object exists")
            var model = findChild(host, "toastModel")
            verify(!!model, "Object exists")
            compare(model.count, 0)

            host.show("Title", "Body", 0)
            tryCompare(model, "count", 1)
            compare(model.get(0).mTitle, "Title")
            compare(model.get(0).mBody, "Body")
        }

        function test_levelMapsToKind() {
            var host = createTemporaryObject(hostComp, root)
            verify(!!host, "Object exists")
            var model = findChild(host, "toastModel")
            verify(!!model, "Object exists")

            host.show("ok", "", 3)        // success is inserted at index 0
            tryCompare(model, "count", 1)
            compare(model.get(0).mKind, "success")

            host.show("err", "", 2)
            tryCompare(model, "count", 2)
            compare(model.get(0).mKind, "error")
        }
    }
}
