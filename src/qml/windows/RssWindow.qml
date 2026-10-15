// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// RSS: feeds that download new releases on their own. With no feed yet it is
// one empty state with the field to add one; with feeds, the feeds on the
// left and, on the right, the selected feed's rule in plain words and what
// became of each of its items.
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Dialogs
import "../theme"
import "../widgets"

Window {
    id: win
    Shortcut { sequences: [StandardKey.Cancel]; onActivated: win.close() }
    width: 900
    height: 600
    minimumWidth: 640
    minimumHeight: 440
    color: Theme.bg
    flags: Theme.unifiedChrome ? (Qt.Window | Qt.ExpandedClientAreaHint | Qt.NoTitleBarBackgroundHint) : Qt.Window
    title: (i18n.language, i18n.t("tb_rss"))

    // Windows builds with QWindowKit: the same title bar as the main window
    readonly property bool frameless: typeof framelessWindow !== "undefined" && framelessWindow === true
    Loader {
        id: framelessLoader
        active: win.frameless
        source: win.frameless ? "../AppFramelessAgent.qml" : ""
        onLoaded: { item.window = win; item.navBar = titleBar }
    }
    // called before the first show (MainWindowLoaders.showWin), so the frame never flashes
    function prepareFrame() { if (framelessLoader.item) framelessLoader.item.attach() }

    property int selectedFeed: 0
    readonly property var feedList: typeof rss !== "undefined" ? rss.feeds : []
    readonly property bool hasFeeds: feedList.length > 0
    readonly property var itemList: (typeof rss !== "undefined" && feedList.length > selectedFeed && selectedFeed >= 0)
        ? rss.itemsForFeed(selectedFeed) : []
    readonly property var curFeed: (feedList.length > selectedFeed && selectedFeed >= 0) ? feedList[selectedFeed] : null

    onFeedListChanged: if (selectedFeed >= feedList.length) selectedFeed = Math.max(0, feedList.length - 1)
    // a feed counts as looked at once you move off it or close the window
    function pick(i) {
        if (i === selectedFeed) return
        if (typeof rss !== "undefined" && hasFeeds) rss.markFeedSeen(selectedFeed)
        selectedFeed = i
    }
    onClosing: if (typeof rss !== "undefined" && hasFeeds) rss.markFeedSeen(selectedFeed)

    property string statusMsg: ""
    Connections {
        target: typeof rss !== "undefined" ? rss : null
        ignoreUnknownSignals: true
        function onErrorOccurred(message) { win.statusMsg = (i18n.language, i18n.t("create_error_prefix")) + message }
    }

    // "12 min ago", "yesterday", "3 days ago"
    function ago(secs) {
        if (!secs || secs <= 0) return ""
        var d = Math.max(0, Date.now() / 1000 - secs)
        if (d < 60) return i18n.t("rss_ago_now")
        if (d < 3600) return i18n.t("rss_ago_min").arg(Math.floor(d / 60))
        if (d < 86400) return i18n.t("rss_ago_h").arg(Math.floor(d / 3600))
        if (d < 2 * 86400) return i18n.t("freeup_yesterday")
        return i18n.t("freeup_days_ago").arg(Math.floor(d / 86400))
    }
    function esc(s) { return String(s).replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;") }
    function mono(s) { return "<font face='" + Theme.fontMono + "' color='" + Theme.t1 + "'>" + esc(s) + "</font>" }
    // the rule in plain words
    function ruleText(f) {
        i18n.language
        if (!f) return ""
        if (!f.autoDownload) return i18n.t("rss_rule_off")
        var dest = f.savePath && f.savePath.length > 0 ? f.savePath : (typeof session !== "undefined" ? session.defaultSavePath() : "")
        if (f.filterPattern && f.filterPattern.length > 0)
            return i18n.t("rss_rule_match").arg(mono(f.filterPattern)).arg(mono(dest))
        return i18n.t("rss_rule_all").arg(mono(dest))
    }
    function addFeed(url) {
        if (typeof rss !== "undefined" && url.trim().length > 0) rss.addFeed(url.trim())
    }

    // ---- feed settings ----
    Item {
        id: editOverlay
        anchors.fill: parent
        z: 110
        visible: false
        property int feedIdx: -1
        function openFor(idx) {
            var f = win.feedList[idx]
            if (!f) return
            feedIdx = idx
            edFilter.text = f.filterPattern || ""
            edPath.text = f.savePath || ""
            edInterval.text = String(f.checkInterval || 30)
            edEnabled.on = f.enabled === true
            edAuto.on = f.autoDownload === true
            visible = true
        }
        Rectangle { anchors.fill: parent; color: Qt.rgba(0, 0, 0, 0.6); MouseArea { anchors.fill: parent; onClicked: editOverlay.visible = false } }
        Rectangle {
            anchors.centerIn: parent
            width: 480; height: edCol.implicitHeight + 44
            radius: Theme.rad(12)
            color: Theme.elev; border.color: Theme.hair; border.width: 1
            MouseArea { anchors.fill: parent }
            ColumnLayout {
                id: edCol
                x: 22; y: 22
                width: parent.width - 44
                spacing: 12
                Text { text: (i18n.language, i18n.t("rss_feed_settings2")); color: Theme.t1; font.pixelSize: 17; font.weight: Font.DemiBold; font.family: Theme.fontSans }
                Text { text: (i18n.language, i18n.t("rss_filter2")); color: Theme.t3; font.pixelSize: 13; font.family: Theme.fontSans }
                TFld { id: edFilter; Layout.fillWidth: true; mono: true; placeholder: "1080p|2160p" }
                Text { text: (i18n.language, i18n.t("rss_save_path2")); color: Theme.t3; font.pixelSize: 13; font.family: Theme.fontSans }
                PathFld { id: edPath; Layout.fillWidth: true; onBrowseClicked: edFolderDlg.open() }
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 16
                    ColumnLayout {
                        spacing: 6
                        Text { text: (i18n.language, i18n.t("rss_interval2")); color: Theme.t3; font.pixelSize: 13; font.family: Theme.fontSans }
                        TFld { id: edInterval; Layout.preferredWidth: 100; placeholder: "30" }
                    }
                    Item { Layout.fillWidth: true }
                    ColumnLayout {
                        spacing: 8
                        RowLayout { spacing: 10; Text { text: (i18n.language, i18n.t("rss_enabled")); color: Theme.t2; font.pixelSize: 13; font.family: Theme.fontSans } Item { Layout.fillWidth: true } TToggle { id: edEnabled } }
                        RowLayout { spacing: 10; Text { text: (i18n.language, i18n.t("rss_auto2")); color: Theme.t2; font.pixelSize: 13; font.family: Theme.fontSans } Item { Layout.fillWidth: true } TToggle { id: edAuto } }
                    }
                }
                RowLayout {
                    Layout.fillWidth: true
                    Layout.topMargin: 6
                    Item { Layout.fillWidth: true }
                    BtnFlat { text: (i18n.language, i18n.t("btn_cancel")); onClicked: editOverlay.visible = false }
                    BtnFlat {
                        primary: true; text: (i18n.language, i18n.t("logviewer_save2"))
                        onClicked: {
                            if (typeof rss !== "undefined" && editOverlay.feedIdx >= 0)
                                rss.updateFeedSettings(editOverlay.feedIdx, edFilter.text, edPath.text,
                                    parseInt(edInterval.text) || 30, edEnabled.on, edAuto.on)
                            editOverlay.visible = false
                        }
                    }
                }
            }
        }
        FolderDialog {
            id: edFolderDlg
            onAccepted: edPath.text = session.urlToLocalPath(edFolderDlg.selectedFolder.toString())
        }
    }

    // ---- add a feed (when there are feeds already) ----
    Item {
        id: addOverlay
        anchors.fill: parent
        z: 100
        visible: false
        onVisibleChanged: if (visible) addUrl.forceActiveFocus()
        Rectangle { anchors.fill: parent; color: Qt.rgba(0, 0, 0, 0.6); MouseArea { anchors.fill: parent; onClicked: addOverlay.visible = false } }
        Rectangle {
            anchors.centerIn: parent
            width: 480; height: addCol.implicitHeight + 44
            radius: Theme.rad(12)
            color: Theme.elev; border.color: Theme.hair; border.width: 1
            MouseArea { anchors.fill: parent }
            ColumnLayout {
                id: addCol
                x: 22; y: 22
                width: parent.width - 44
                spacing: 14
                Text { text: (i18n.language, i18n.t("rss_add_feed")); color: Theme.t1; font.pixelSize: 17; font.weight: Font.DemiBold; font.family: Theme.fontSans }
                TFld { id: addUrl; Layout.fillWidth: true; mono: true; placeholder: (i18n.language, i18n.t("rss_paste_url")) }
                RowLayout {
                    Layout.fillWidth: true
                    Item { Layout.fillWidth: true }
                    BtnFlat { text: (i18n.language, i18n.t("btn_cancel")); onClicked: { addUrl.text = ""; addOverlay.visible = false } }
                    BtnFlat {
                        primary: true; text: (i18n.language, i18n.t("rss_add"))
                        onClicked: { win.addFeed(addUrl.text); addUrl.text = ""; addOverlay.visible = false }
                    }
                }
            }
        }
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

        // ======== no feed yet ========
        Item {
            visible: !win.hasFeeds
            Layout.fillWidth: true
            Layout.fillHeight: true
            ColumnLayout {
                anchors.centerIn: parent
                width: Math.min(parent.width - 64, 580)
                spacing: 20
                Rectangle {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.preferredWidth: 48; Layout.preferredHeight: 48
                    radius: Theme.rad(12)
                    color: Theme.panel
                    border.color: Theme.hair; border.width: 1
                    IconImg { anchors.centerIn: parent; src: "qrc:/icons/rss.svg"; tint: Theme.t2; s: 22 }
                }
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    Text { Layout.alignment: Qt.AlignHCenter; text: (i18n.language, i18n.t("rss_empty_title2")); color: Theme.t1; font.pixelSize: 20; font.weight: Font.DemiBold; font.family: Theme.fontSans }
                    Text {
                        Layout.fillWidth: true
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.WordWrap
                        text: (i18n.language, i18n.t("rss_empty_body"))
                        color: Theme.t2; font.pixelSize: 14; font.family: Theme.fontSans; lineHeight: 1.4
                    }
                }
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    TFld {
                        id: emptyUrl
                        Layout.fillWidth: true
                        placeholder: (i18n.language, i18n.t("rss_paste_url"))
                        Connections {
                            target: emptyUrl.field
                            function onAccepted() { win.addFeed(emptyUrl.text); emptyUrl.text = "" }
                        }
                    }
                    BtnFlat {
                        primary: true; lg: true
                        text: (i18n.language, i18n.t("rss_add"))
                        onClicked: { win.addFeed(emptyUrl.text); emptyUrl.text = "" }
                    }
                }
                Text {
                    Layout.alignment: Qt.AlignHCenter
                    textFormat: Text.StyledText
                    text: (i18n.language, i18n.t("rss_empty_tip")).arg("<font color='" + Theme.t1 + "'>RSS</font>")
                    color: Theme.t3; font.pixelSize: 13; font.family: Theme.fontSans
                }
            }
        }

        // ======== with feeds ========
        Item {
            visible: win.hasFeeds
            Layout.fillWidth: true
            Layout.preferredHeight: 60
            Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hair }
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 20; anchors.rightMargin: 16
                spacing: 8
                Text { text: (i18n.language, i18n.t("rss_feeds2")); color: Theme.t1; font.pixelSize: 17; font.weight: Font.DemiBold; font.family: Theme.fontSans }
                Text {
                    Layout.fillWidth: true
                    Layout.leftMargin: 8
                    visible: win.statusMsg.length > 0
                    text: win.statusMsg
                    color: Theme.downText; font.pixelSize: 12; font.family: Theme.fontSans
                    elide: Text.ElideRight
                }
                Item { Layout.fillWidth: true; visible: win.statusMsg.length === 0 }
                BtnFlat { text: (i18n.language, i18n.t("settings_refresh")); onClicked: { win.statusMsg = ""; if (typeof rss !== "undefined") rss.checkAllFeeds() } }
                BtnFlat { primary: true; text: (i18n.language, i18n.t("rss_add_feed_btn")); onClicked: addOverlay.visible = true }
            }
        }

        RowLayout {
            visible: win.hasFeeds
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            // feeds
            Item {
                Layout.preferredWidth: 250
                Layout.fillHeight: true
                Rectangle { anchors.right: parent.right; width: 1; height: parent.height; color: Theme.hair }
                ListView {
                    anchors.fill: parent
                    anchors.margins: 8
                    anchors.rightMargin: 9
                    clip: true
                    model: win.feedList
                    spacing: 2
                    boundsBehavior: Flickable.StopAtBounds
                    delegate: Rectangle {
                        id: feedRow
                        readonly property bool on: win.selectedFeed === index
                        width: ListView.view.width
                        height: 56
                        radius: Theme.rad(6)
                        color: on ? Qt.rgba(Theme.t1.r, Theme.t1.g, Theme.t1.b, 0.08) : (feedMa.containsMouse ? Theme.hover : "transparent")
                        Rectangle { visible: feedRow.on; width: 2; height: parent.height; color: Theme.t1 }
                        ColumnLayout {
                            x: 12
                            width: parent.width - 24
                            anchors.verticalCenter: parent.verticalCenter
                            spacing: 3
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8
                                Text {
                                    Layout.fillWidth: true
                                    text: modelData.name
                                    color: modelData.enabled ? Theme.t1 : Theme.t3
                                    font.pixelSize: 14; font.weight: Font.DemiBold; font.family: Theme.fontSans
                                    elide: Text.ElideRight
                                }
                                Text {
                                    visible: modelData.newCount > 0
                                    text: (i18n.language, modelData.newCount === 1 ? i18n.t("rss_new_one") : i18n.t("rss_new_n").arg(modelData.newCount))
                                    color: Theme.t2; font.pixelSize: 12; font.weight: Font.DemiBold; font.family: Theme.fontSans; font.features: Theme.tnum
                                }
                            }
                            Text {
                                Layout.fillWidth: true
                                text: {
                                    i18n.language
                                    if (!modelData.enabled) return i18n.t("rss_feed_off")
                                    return modelData.lastCheckedSecs > 0 ? i18n.t("rss_updated").arg(win.ago(modelData.lastCheckedSecs)) : i18n.t("rss_not_checked")
                                }
                                color: Theme.t3; font.pixelSize: 12; font.family: Theme.fontSans
                                elide: Text.ElideRight
                            }
                        }
                        MouseArea {
                            id: feedMa; anchors.fill: parent; hoverEnabled: true
                            acceptedButtons: Qt.LeftButton | Qt.RightButton
                            cursorShape: Qt.PointingHandCursor
                            onClicked: function (mouse) {
                                win.pick(index)
                                if (mouse.button === Qt.RightButton && typeof rss !== "undefined") feedMenu.popup()
                            }
                        }
                        DMenu {
                            id: feedMenu
                            DMenuItem {
                                text: modelData.enabled ? (i18n.language, i18n.t("rss_disable")) : (i18n.language, i18n.t("rss_enable"))
                                onTriggered: rss.setFeedEnabled(index, !modelData.enabled)
                            }
                            DMenuItem { text: (i18n.language, i18n.t("rss_refresh_now")); onTriggered: rss.checkFeed(index) }
                            DMenuItem { text: (i18n.language, i18n.t("rss_edit")); onTriggered: editOverlay.openFor(index) }
                            DMenuSep {}
                            DMenuItem { tone: "danger"; text: (i18n.language, i18n.t("rss_remove_feed")); onTriggered: rss.removeFeed(index) }
                        }
                    }
                }
            }

            // the selected feed
            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 0

                // the rule, in plain words
                Item {
                    Layout.fillWidth: true
                    Layout.preferredHeight: ruleCol.implicitHeight + 32
                    Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hair }
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 20; anchors.rightMargin: 16
                        spacing: 12
                        ColumnLayout {
                            id: ruleCol
                            Layout.fillWidth: true
                            spacing: 4
                            Text { text: (i18n.language, i18n.t("rss_rule_label")); color: Theme.t3; font.pixelSize: 11; font.weight: Font.DemiBold; font.letterSpacing: 0.9; font.capitalization: Font.AllUppercase; font.family: Theme.fontSans }
                            Text {
                                Layout.fillWidth: true
                                textFormat: Text.StyledText
                                text: win.ruleText(win.curFeed)
                                color: Theme.t2; font.pixelSize: 14; font.family: Theme.fontSans
                                elide: Text.ElideRight
                            }
                        }
                        TToggle {
                            on: win.curFeed ? win.curFeed.autoDownload === true : false
                            onToggled: function (value) {
                                if (typeof rss !== "undefined" && win.curFeed) rss.setAutoDownload(win.selectedFeed, value)
                            }
                        }
                        BtnFlat { text: (i18n.language, i18n.t("rss_edit")); onClicked: editOverlay.openFor(win.selectedFeed) }
                    }
                }

                ListView {
                    id: itemView
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: win.itemList
                    boundsBehavior: Flickable.StopAtBounds
                    Text {
                        anchors.centerIn: parent
                        visible: itemView.count === 0
                        text: (i18n.language, i18n.t("rss_no_items"))
                        color: Theme.t3; font.pixelSize: 13; font.family: Theme.fontSans
                    }
                    delegate: Rectangle {
                        id: itemRow
                        width: ListView.view.width
                        height: 57
                        color: itemMa.containsMouse ? Theme.hover : "transparent"
                        Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hairSoft }
                        readonly property color dot: modelData.state === "downloading" ? Theme.down
                                                    : modelData.state === "done" ? Theme.grn : Theme.t4
                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 20; anchors.rightMargin: 16
                            spacing: 12
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 3
                                Text {
                                    Layout.fillWidth: true
                                    text: modelData.title
                                    color: Theme.t1; font.pixelSize: 14; font.weight: Font.DemiBold; font.family: Theme.fontSans
                                    elide: Text.ElideRight
                                }
                                Row {
                                    visible: modelData.state !== "" || modelData.size !== ""
                                    spacing: 6
                                    Rectangle { visible: modelData.state !== ""; anchors.verticalCenter: parent.verticalCenter; width: 6; height: 6; radius: 3; color: itemRow.dot }
                                    Text {
                                        text: {
                                            i18n.language
                                            var s = modelData.state === "downloading" ? i18n.t("rss_st_downloading")
                                                  : modelData.state === "done" ? i18n.t("rss_st_done")
                                                  : modelData.state === "ignored" ? i18n.t("rss_st_ignored")
                                                  : ""
                                            if (modelData.size !== "") s = s.length > 0 ? s + " · " + modelData.size : modelData.size
                                            return s
                                        }
                                        color: modelData.state === "" || modelData.state === "ignored" ? Theme.t3 : Theme.t2
                                        font.pixelSize: 12; font.family: Theme.fontSans; font.features: Theme.tnum
                                    }
                                }
                            }
                            Text {
                                visible: !dlBtn.visible
                                text: (i18n.language, win.ago(modelData.pubSecs))
                                color: Theme.t3; font.pixelSize: 12; font.family: Theme.fontSans; font.features: Theme.tnum
                            }
                            // a manual download, for what the rule left
                            BtnFlat {
                                id: dlBtn
                                visible: itemMa.containsMouse && !modelData.downloaded
                                text: (i18n.language, i18n.t("rss_download"))
                                onClicked: if (typeof rss !== "undefined") rss.downloadItem(win.selectedFeed, index)
                            }
                        }
                        MouseArea { id: itemMa; anchors.fill: parent; hoverEnabled: true; acceptedButtons: Qt.NoButton }
                    }
                }
            }
        }
    }
}
