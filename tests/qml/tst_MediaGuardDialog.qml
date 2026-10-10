// SPDX-License-Identifier: MIT
// MediaGuard dialog contract: both modes load, the way out (another release)
// is offered only when there is something to search for, and every kind of
// fake has a phrase.

import QtQuick
import QtTest
import "qrc:/src/qml/dialogs"

Item {
    id: root
    width: 800
    height: 600

    Component {
        id: dlgComp
        MediaGuardDialog {}
    }

    SignalSpy { id: searchSpy; signalName: "searchRequested" }

    TestCase {
        name: "MediaGuardDialog"
        when: windowShown

        function test_quarantineOffersAnotherRelease() {
            var d = createTemporaryObject(dlgComp, root)
            verify(!!d)
            d.show("quarantine", "abc", "Movie.mkv", "program", "Movie 2026")
            verify(d.opened)
            verify(d.held)
            verify(d.showOk)
            searchSpy.target = d
            searchSpy.clear()
            d.accepted()
            compare(searchSpy.count, 1)
            compare(searchSpy.signalArguments[0][0], "Movie 2026")
        }

        function test_noQueryHidesSearch() {
            var d = createTemporaryObject(dlgComp, root)
            d.show("quarantine", "abc", "x.mkv", "archive", "")
            verify(!d.showOk)
        }

        function test_lureIsNotQuarantine() {
            var d = createTemporaryObject(dlgComp, root)
            d.show("lure", "abc", "x.wmv", "", "X")
            verify(d.opened)
            verify(!d.held)
            compare(d.cancelText, "guard_l_ok")
        }

        function test_everyKindHasAPhrase() {
            var d = createTemporaryObject(dlgComp, root)
            compare(d.kindPhrase("program"), "guard_kind_program")
            compare(d.kindPhrase("archive"), "guard_kind_archive")
            compare(d.kindPhrase("document"), "guard_kind_document")
            compare(d.kindPhrase("webpage"), "guard_kind_webpage")
            compare(d.kindPhrase("something new"), "guard_kind_unknown")
        }
    }
}
