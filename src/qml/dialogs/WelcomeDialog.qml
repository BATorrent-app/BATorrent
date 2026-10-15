// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// The two onboarding screens behind one name, as the menus and Main know it:
// mode "update" is What's new (WhatsNewDialog), mode "welcome" hands over to
// the full-window setup wizard.
import QtQuick
import "../theme"

Item {
    id: dlg
    anchors.fill: parent
    visible: whatsNew.opened || whatsNew.anim > 0
    property string mode: "update"          // "welcome" | "update"
    readonly property bool isWelcome: mode === "welcome"
    property var wizard: null
    readonly property bool opened: whatsNew.opened
    readonly property bool wizardOpen: wizard ? wizard.opened : false

    signal accepted()
    signal openReleaseNotes(string version)
    signal actionRequested(string action)

    function open() {
        if (isWelcome && wizard) { wizard.open(); return }
        whatsNew.open()
    }
    function close() { whatsNew.close() }

    WhatsNewDialog {
        id: whatsNew
        onDone: dlg.accepted()
        onReleaseNotesRequested: function (v) { dlg.openReleaseNotes(v) }
        onActionRequested: function (a) { dlg.actionRequested(a) }
    }
}
