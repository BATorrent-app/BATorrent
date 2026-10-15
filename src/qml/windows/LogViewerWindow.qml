// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// This session's log as rows: time, level, module, message. A level shows
// itself and everything more serious; the copy for a bug report takes the
// last lines with the user's own paths taken out. Wired to QmlLogBridge (`logs`).
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import "../theme"
import "../widgets"

Window {
    id: win
    Shortcut { sequences: [StandardKey.Cancel]; onActivated: win.close() }
    width: 960
    height: 600
    minimumWidth: 680
    minimumHeight: 420
    color: Theme.bg
    flags: Theme.unifiedChrome ? (Qt.Window | Qt.ExpandedClientAreaHint | Qt.NoTitleBarBackgroundHint) : Qt.Window
    title: (i18n.language, i18n.t("logviewer_title2"))

    readonly property bool frameless: typeof framelessWindow !== "undefined" && framelessWindow === true
    Loader {
        id: framelessLoader
        active: win.frameless
        source: win.frameless ? "../AppFramelessAgent.qml" : ""
        onLoaded: { item.window = win; item.navBar = titleBar }
    }
    function prepareFrame() { if (framelessLoader.item) framelessLoader.item.attach() }

    readonly property var api: typeof logs !== "undefined" ? logs : null
    onVisibleChanged: {
        if (!api) return
        if (visible) { api.start(); reload() } else api.stop()
    }

    property int minLevel: 1                   // Errors 3, Warnings 2, Info 1, All 0
    property bool follow: true
    property var rows: []
    property string copied: ""
    function reload() {
        if (!api) return
        rows = api.entries(minLevel, filter.text, 3000)
        if (follow) Qt.callLater(function () { list.positionViewAtEnd() })
    }
    onMinLevelChanged: reload()
    Connections {
        target: win.api
        function onEntriesChanged() { if (win.visible) win.reload() }
    }
    Timer { id: filterTimer; interval: 200; onTriggered: win.reload() }
    Timer { id: copiedTimer; interval: 2500; onTriggered: win.copied = "" }

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

        // ---- level, filter, follow ----
        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 52
            Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hair }
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12; anchors.rightMargin: 14
                spacing: 10
                Rectangle {
                    implicitWidth: lvRow.implicitWidth + 6; implicitHeight: 32
                    radius: Theme.rad(8)
                    color: Theme.field
                    border.color: Theme.hair; border.width: 1
                    Row {
                        id: lvRow
                        x: 3; y: 3
                        spacing: 2
                        Repeater {
                            model: [[3, "log_errors", 0], [2, "log_warnings", 1], [1, "log_info", 2], [0, "log_all", 3]]
                            delegate: Rectangle {
                                readonly property bool on: win.minLevel === modelData[0]
                                width: lvTxt.implicitWidth + 22; height: 26
                                radius: Math.max(0, Theme.rad(8) - 2)
                                color: on ? Qt.rgba(Theme.t1.r, Theme.t1.g, Theme.t1.b, 0.12) : "transparent"
                                Text {
                                    id: lvTxt
                                    anchors.centerIn: parent
                                    textFormat: Text.StyledText
                                    text: (i18n.language, i18n.t(modelData[1])) + "  <font color='" + Theme.t4 + "'>"
                                          + (win.api ? win.api.counts[modelData[2]] : 0) + "</font>"
                                    color: parent.on ? Theme.t1 : Theme.t3
                                    font.pixelSize: 13; font.weight: parent.on ? Font.DemiBold : Font.Normal
                                    font.family: Theme.fontSans; font.features: Theme.tnum
                                }
                                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: win.minLevel = modelData[0] }
                            }
                        }
                    }
                }
                TFld {
                    id: filter
                    Layout.fillWidth: true
                    clearable: true
                    placeholder: (i18n.language, i18n.t("log_filter"))
                    onTextChanged: filterTimer.restart()
                }
                Text { text: (i18n.language, i18n.t("log_follow")); color: Theme.t2; font.pixelSize: 13; font.family: Theme.fontSans }
                TToggle { on: win.follow; onToggled: function (v) { win.follow = v; if (v) list.positionViewAtEnd() } }
            }
        }

        // ---- the lines ----
        ListView {
            id: list
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: win.rows
            boundsBehavior: Flickable.StopAtBounds
            topMargin: 6; bottomMargin: 6
            ScrollBar.vertical: ScrollBar {}
            // scrolling up stops following; back at the end, it follows again
            onMovementEnded: win.follow = atYEnd
            Text {
                anchors.centerIn: parent
                visible: list.count === 0
                text: (i18n.language, i18n.t("log_empty"))
                color: Theme.t3; font.pixelSize: 13; font.family: Theme.fontSans
            }
            delegate: Rectangle {
                id: line
                required property var modelData
                width: ListView.view.width
                height: msg.implicitHeight + 8
                color: modelData.level === 3 ? Qt.rgba(Theme.down.r, Theme.down.g, Theme.down.b, 0.07)
                     : modelData.level === 2 ? Qt.rgba(Theme.warn.r, Theme.warn.g, Theme.warn.b, 0.05) : "transparent"
                readonly property color lvInk: modelData.level === 3 ? Theme.downText
                                              : modelData.level === 2 ? Theme.warn
                                              : modelData.level === 0 ? Theme.t4 : Theme.t2
                Text {
                    x: 12; y: 4
                    width: 72
                    text: line.modelData.time
                    color: Theme.t4; font.pixelSize: 12; font.family: Theme.fontMono
                }
                Text {
                    x: 84; y: 4
                    width: 60
                    text: ["DEBUG", "INFO", "WARN", "ERROR"][line.modelData.level]
                    color: line.lvInk; font.pixelSize: 12; font.weight: line.modelData.level >= 2 ? Font.DemiBold : Font.Normal; font.family: Theme.fontMono
                }
                Text {
                    x: 148; y: 4
                    width: 90
                    text: line.modelData.module
                    color: Theme.t3; font.pixelSize: 12; font.family: Theme.fontMono
                    elide: Text.ElideRight
                }
                Text {
                    id: msg
                    x: 246; y: 4
                    width: line.width - x - 14
                    text: line.modelData.msg
                    color: line.modelData.level === 0 ? Theme.t3 : Theme.t1
                    font.pixelSize: 12; font.family: Theme.fontMono
                    wrapMode: Text.WrapAnywhere
                    textFormat: Text.PlainText
                }
            }
        }

        // ---- footer ----
        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 56
            Rectangle { width: parent.width; height: 1; color: Theme.hair }
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 14; anchors.rightMargin: 14
                spacing: 8
                Text {
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                    color: win.copied !== "" ? Theme.grn : Theme.t3
                    font.pixelSize: 12; font.family: Theme.fontSans; font.features: Theme.tnum
                    text: {
                        i18n.language
                        if (win.copied !== "") return win.copied
                        var s = win.api ? win.api.sessionLine : ""
                        if (s === "") return ""
                        var p = s.split("\t")
                        return i18n.t("log_session").arg(p[0]).arg((p[1] || "").replace(/^BATorrent /, ""))
                    }
                }
                BtnFlat { text: (i18n.language, i18n.t("logviewer_clear")); onClicked: if (win.api) win.api.clearLog() }
                BtnFlat { text: (i18n.language, i18n.t("logviewer_open_folder2")); onClicked: if (win.api) win.api.openLogsFolder() }
                BtnFlat {
                    primary: true
                    text: (i18n.language, i18n.t("log_copy_report"))
                    onClicked: {
                        if (!win.api) return
                        var n = win.api.copyForReport(400)
                        win.copied = i18n.t("log_copied").arg(n)
                        copiedTimer.restart()
                    }
                }
            }
        }
    }
}
