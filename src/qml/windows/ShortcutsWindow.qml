// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Keyboard shortcuts: the one worth remembering (the command palette) on
// top, the rest in two columns, each key in its own cap. Only keys that are
// bound (LibraryShortcuts, Main, the command palette) belong here.
import QtQuick
import QtQuick.Layouts
import "../theme"
import "../widgets"

Window {
    id: win
    Shortcut { sequences: [StandardKey.Cancel]; onActivated: win.close() }
    width: 640
    height: 600
    minimumWidth: 560
    minimumHeight: 440
    color: Theme.elev
    flags: Theme.unifiedChrome ? (Qt.Window | Qt.ExpandedClientAreaHint | Qt.NoTitleBarBackgroundHint) : Qt.Window
    title: (i18n.language, i18n.t("shortcuts_title2"))

    readonly property bool frameless: typeof framelessWindow !== "undefined" && framelessWindow === true
    Loader {
        id: framelessLoader
        active: win.frameless
        source: win.frameless ? "../AppFramelessAgent.qml" : ""
        onLoaded: { item.window = win; item.navBar = titleBar }
    }
    function prepareFrame() { if (framelessLoader.item) framelessLoader.item.attach() }
    onVisibleChanged: if (visible) search.field.forceActiveFocus()

    readonly property bool mac: Qt.platform.os === "osx"
    function keys(s) {
        return s.split("+").map(function (k) {
            if (!win.mac) return k
            return k === "Ctrl" ? "⌘" : k === "Alt" ? "⌥" : k === "Shift" ? "⇧" : k
        })
    }
    readonly property var left: [
        { title: "sk_g_app", rows: [
            ["sk_settings", "Ctrl+,"], ["sk_shortcuts", "F1"], ["sk_logs", "Ctrl+Shift+L"],
            ["sk_refresh", "F5"], ["sk_theme", "Ctrl+Shift+T"], ["sk_quit", "Ctrl+Q"] ] },
        { title: "sk_g_goto", rows: [
            ["sk_back", "Alt+←"], ["sk_forward", "Alt+→"], ["sk_filter", "Ctrl+F"] ] },
        { title: "sk_g_show", rows: [
            ["sk_show_all", "Ctrl+1"], ["sk_show_down", "Ctrl+2"], ["sk_show_seed", "Ctrl+3"],
            ["sk_show_done", "Ctrl+4"], ["sk_show_active", "Ctrl+5"], ["sk_show_paused", "Ctrl+6"] ] }
    ]
    readonly property var right: [
        { title: "sk_g_downloads", rows: [
            ["sk_add", "Ctrl+O"], ["sk_paste", "Ctrl+V"], ["sk_pause", "Space"], ["sk_remove", "Del"],
            ["sk_remove_files", "Shift+Del"], ["sk_rename", "F2"], ["sk_folder", "Ctrl+E"], ["sk_select_all", "Ctrl+A"],
            ["sk_recheck", "Ctrl+R"], ["sk_queue_up", "Ctrl+↑"], ["sk_queue_down", "Ctrl+↓"], ["sk_create", "Ctrl+N"] ] }
    ]
    function matches(row) {
        var q = search.text.trim().toLowerCase()
        if (q.length === 0) return true
        return i18n.t(row[0]).toLowerCase().indexOf(q) >= 0 || row[1].toLowerCase().indexOf(q) >= 0
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        WindowTitleBar {
            id: titleBar
            Layout.fillWidth: true
            Layout.preferredHeight: implicitHeight
            appWindow: win
            ownsTitleBar: win.frameless
            title: win.title
        }

        Flickable {
            id: flick
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            contentWidth: width
            contentHeight: body.implicitHeight + 40
            boundsBehavior: Flickable.StopAtBounds
            WheelScroller { flick: flick }

            ColumnLayout {
                id: body
                x: 18; y: 16
                width: flick.width - 36
                spacing: 14

                TFld {
                    id: search
                    Layout.fillWidth: true
                    icon: "qrc:/icons/search.svg"
                    clearable: true
                    placeholder: (i18n.language, i18n.t("sk_search"))
                }

                // the one to remember
                Rectangle {
                    Layout.fillWidth: true
                    visible: win.matches(["sk_palette", "Ctrl+K"])
                    implicitHeight: 60
                    radius: Theme.radCard(10)
                    color: Theme.panel
                    border.color: Theme.hair; border.width: 1
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 14; anchors.rightMargin: 12
                        spacing: 12
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2
                            Text { text: (i18n.language, i18n.t("sk_palette")); color: Theme.t1; font.pixelSize: 14; font.weight: Font.DemiBold; font.family: Theme.fontSans }
                            Text { Layout.fillWidth: true; text: (i18n.language, i18n.t("sk_palette_d")); color: Theme.t3; font.pixelSize: 12; font.family: Theme.fontSans; elide: Text.ElideRight }
                        }
                        KeyCaps { combo: "Ctrl+K"; big: true }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 20
                    Repeater {
                        model: [win.left, win.right]
                        delegate: ColumnLayout {
                            id: column
                            required property var modelData
                            Layout.fillWidth: true
                            Layout.preferredWidth: 1
                            Layout.alignment: Qt.AlignTop
                            spacing: 14
                            Repeater {
                                model: column.modelData
                                delegate: ColumnLayout {
                                    id: grp
                                    required property var modelData
                                    readonly property var shown: grp.modelData.rows.filter(function (r) { return win.matches(r) })
                                    Layout.fillWidth: true
                                    visible: shown.length > 0
                                    spacing: 0
                                    Text {
                                        Layout.bottomMargin: 4
                                        text: (i18n.language, i18n.t(grp.modelData.title))
                                        color: Theme.t4; font.pixelSize: 11; font.weight: Font.DemiBold; font.letterSpacing: 0.9
                                        font.capitalization: Font.AllUppercase; font.family: Theme.fontSans
                                    }
                                    Repeater {
                                        model: grp.shown
                                        delegate: Item {
                                            required property var modelData
                                            required property int index
                                            Layout.fillWidth: true
                                            Layout.preferredHeight: 34
                                            Rectangle { visible: index > 0; width: parent.width; height: 1; color: Theme.hairSoft }
                                            Text {
                                                anchors.left: parent.left; anchors.right: caps.left; anchors.rightMargin: 10
                                                anchors.verticalCenter: parent.verticalCenter
                                                text: (i18n.language, i18n.t(modelData[0]))
                                                color: Theme.t2; font.pixelSize: 13; font.family: Theme.fontSans
                                                elide: Text.ElideRight
                                            }
                                            KeyCaps { id: caps; anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter; combo: modelData[1] }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                Text {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.topMargin: 20
                    visible: !win.matches(["sk_palette", "Ctrl+K"]) && win.left.concat(win.right).every(function (g) { return g.rows.filter(function (r) { return win.matches(r) }).length === 0 })
                    text: (i18n.language, i18n.t("sk_none"))
                    color: Theme.t3; font.pixelSize: 13; font.family: Theme.fontSans
                }
            }
        }
    }

    // one cap per key
    component KeyCaps: Row {
        id: kc
        property string combo: ""
        property bool big: false
        spacing: 4
        Repeater {
            model: win.keys(kc.combo)
            delegate: Rectangle {
                required property var modelData
                width: Math.max(kc.big ? 30 : 24, capTxt.implicitWidth + (kc.big ? 16 : 12))
                height: kc.big ? 28 : 22
                radius: Theme.rad(5)
                color: Theme.field
                border.color: Theme.hair; border.width: 1
                Text {
                    id: capTxt
                    anchors.centerIn: parent
                    text: modelData === "Space" ? (i18n.language, i18n.t("key_space")) : modelData
                    color: Theme.t2
                    font.pixelSize: kc.big ? 12 : 11
                    font.family: Theme.fontMono
                }
            }
        }
    }
}
