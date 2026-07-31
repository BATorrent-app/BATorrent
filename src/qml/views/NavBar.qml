// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Top navigation bar — the nav rail laid horizontally (default layout since
// 4.5; the rail survives behind the "classic layout" setting). Brand + page
// tabs on the left; download chip, disk gauge, donate and settings on the
// right. Same itemRect() contract as NavRail so the tour targets both.
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Effects
import "../theme"
import "../widgets"

Rectangle {
    id: bar
    implicitHeight: 54
    color: Theme.nav

    property int currentIndex: 0            // bound down from the window; never self-assigned
    property bool showDownloadChip: true    // host setting gate
    signal pageRequested(int page)
    signal settingsClicked()
    signal vpnClicked()          // open the VPN cockpit (Settings → VPN section)
    signal selectTorrent(string infoHash)
    signal makeRoomRequested()
    signal aboutRequested()      // the brand mark is the way into About

    // responsive degradation: the chip loses its text first, then the disk
    // gauge loses its labels — nothing ever clips
    readonly property bool tightChip: width < 1260
    readonly property bool tightDisk: width < 1140

    readonly property DownloadCarousel carousel: DownloadCarousel {
        id: car
        currentPage: bar.currentIndex
        hovered: dlChip.chipHovered
        active: car.dlList.length > 0
    }
    Connections {
        target: car
        function onDlIndexChanged() { chipFade.restart() }
    }
    SequentialAnimation {
        id: chipFade
        NumberAnimation { target: dlChip; property: "contentOpacity"; to: 0; duration: 160; easing.type: Easing.InCubic }
        ScriptAction { script: car.dlShown = car.dlIndex }
        NumberAnimation { target: dlChip; property: "contentOpacity"; to: 1; duration: 300; easing.type: Easing.OutCubic }
    }

    readonly property var diskVolumes: (typeof session !== "undefined") ? session.diskVolumes : []
    // gauge rotates through every disk (like the game card), pausing on each;
    // default-save volume leads the list
    property int diskIdx: 0
    readonly property var diskShown: diskVolumes.length > 0
        ? diskVolumes[Math.min(diskIdx, diskVolumes.length - 1)] : null
    Timer {
        interval: 5000; repeat: true
        running: bar.diskVolumes.length > 1
        onTriggered: bar.diskIdx = (bar.diskIdx + 1) % bar.diskVolumes.length
    }
    readonly property var diskWorst: {
        var w = null
        for (var i = 0; i < diskVolumes.length; ++i)
            if (!w || diskVolumes[i].usedFraction > w.usedFraction) w = diskVolumes[i]
        return w
    }

    Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hair }

    readonly property var items: buildItems(i18n.language)
    function buildItems() {
        var all = [
            { icon: "qrc:/icons/download.svg", label: i18n.t("nav_downloads"), page: 0 },
            { icon: "qrc:/icons/search.svg",   label: i18n.t("nav_find"),      page: 1 },
            { icon: "qrc:/icons/hub.svg",      label: i18n.t("nav_hub"),       page: 2 }
        ]
        return all
    }

    // Geometry of a nav target, mapped into `mapTo`'s coords (for the tour
    // spotlight). key: a page number, "settings", or "rail" (the whole bar).
    function itemRect(key, mapTo) {
        if (key === "rail") {
            var rp = bar.mapToItem(mapTo, 0, 0)
            return Qt.rect(rp.x, rp.y, bar.width, bar.height)
        }
        var it = (key === "settings") ? settingsBtn : null
        if (!it) {
            for (var i = 0; i < navRepeater.count; i++) {
                var d = navRepeater.itemAt(i)
                if (d && d.modelData && d.modelData.page === key && d.visible) { it = d; break }
            }
        }
        if (!it) return Qt.rect(0, 0, 0, 0)
        var p = it.mapToItem(mapTo, 0, 0)
        return Qt.rect(p.x, p.y, it.width, it.height)
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Theme.sp4
        anchors.rightMargin: Theme.sp3
        spacing: 2

        // ----- brand — glyph only; the wordmark lives where the brand
        // introduces itself (splash, About, expanded rail) -----
        Image {
            id: brandGlyph
            Layout.alignment: Qt.AlignVCenter
            Layout.rightMargin: 2
            Layout.preferredWidth: 32
            Layout.preferredHeight: 32
            source: "qrc:/images/logo.svg"
            sourceSize: Qt.size(64, 64)
            fillMode: Image.PreserveAspectFit
            layer.enabled: Theme.isLight
            layer.effect: MultiEffect { colorization: 1.0; colorizationColor: Theme.t1 }

            // clicking the logotype opens About — the conventional home for it.
            // (The rail's brand block does the same; both chromes need it, since
            // only one of the two is on screen at a time.)
            MouseArea {
                id: brandMa
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: bar.aboutRequested()
                ToolTip.visible: containsMouse
                ToolTip.text: (i18n.language, i18n.t("menu_about"))
                ToolTip.delay: 500
            }
            Accessible.role: Accessible.Button
            Accessible.name: (i18n.language, i18n.t("menu_about"))
            Accessible.onPressAction: bar.aboutRequested()
        }

        // ----- page tabs -----
        Repeater {
            id: navRepeater
            model: bar.items
            delegate: Item {
                id: navTab
                required property var modelData
                readonly property bool active: bar.currentIndex === modelData.page
                Layout.fillHeight: true
                Layout.preferredWidth: visible ? tabRow.implicitWidth + 30 : 0

                // page switching was mouse-only and silent to a screen reader
                Accessible.role: Accessible.PageTab
                Accessible.name: navTab.modelData.label
                Accessible.checked: navTab.active
                Accessible.onPressAction: bar.pageRequested(navTab.modelData.page)
                activeFocusOnTab: true
                Keys.onReturnPressed: bar.pageRequested(navTab.modelData.page)
                Keys.onSpacePressed: bar.pageRequested(navTab.modelData.page)
                Rectangle {
                    visible: navTab.activeFocus
                    anchors.fill: parent
                    anchors.margins: 4
                    radius: 9
                    color: "transparent"
                    border.color: Theme.focusRing
                    border.width: Theme.focusRingWidth
                }

                Row {
                    id: tabRow
                    anchors.centerIn: parent
                    spacing: 9
                    IconImg {
                        anchors.verticalCenter: parent.verticalCenter
                        src: navTab.modelData.icon
                        tint: navTab.active ? Theme.t1 : Theme.t3
                        s: 16
                        Behavior on tint { ColorAnimation { duration: 140 } }
                    }
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: navTab.modelData.label
                        color: navTab.active ? Theme.t1 : (tabMa.containsMouse ? Theme.t2 : Theme.t3)
                        font.pixelSize: 14
                        font.weight: navTab.active ? Font.DemiBold : Font.Medium
                        font.family: Theme.fontSans
                        Behavior on color { ColorAnimation { duration: 140 } }
                    }
                }
                // active accent underline
                Rectangle {
                    anchors.left: parent.left; anchors.right: parent.right
                    anchors.leftMargin: 13; anchors.rightMargin: 13
                    anchors.bottom: parent.bottom
                    height: navTab.active ? 3 : 0
                    radius: 3
                    color: Theme.accent
                    Behavior on height { NumberAnimation { duration: 200; easing.type: Easing.OutBack } }
                }
                MouseArea {
                    id: tabMa
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: bar.pageRequested(navTab.modelData.page)
                }
            }
        }

        Item { Layout.fillWidth: true }

        NavBarDownloadChip { id: dlChip; bar: bar; car: car }
        NavBarDiskGauge { bar: bar }
        NavBarVpnChip { bar: bar }

        // ----- donate (heart: gray at rest, red on hover) -----
        Item {
            Layout.alignment: Qt.AlignVCenter
            Layout.preferredWidth: 34; Layout.preferredHeight: 34
            Rectangle {
                anchors.fill: parent
                radius: 8
                color: donMa.containsMouse ? Theme.accentTint : "transparent"
                Behavior on color { ColorAnimation { duration: 140 } }
            }
            IconImg {
                anchors.centerIn: parent
                src: "qrc:/icons/heart-line.svg"
                tint: donMa.containsMouse ? Theme.accent : Theme.t3
                s: 16
            }
            MouseArea {
                id: donMa
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: Qt.openUrlExternally("https://github.com/sponsors/Mateuscruz19")
            }
            ToolTip.text: (i18n.language, i18n.t("action_donate"))
            ToolTip.visible: donMa.containsMouse
            ToolTip.delay: 400
        }

        // ----- settings (page 4) -----
        Item {
            id: settingsBtn
            readonly property bool active: bar.currentIndex === 3
            Layout.alignment: Qt.AlignVCenter
            Layout.preferredWidth: 34; Layout.preferredHeight: 34
            Rectangle {
                anchors.fill: parent
                radius: 8
                color: settingsBtn.active ? Theme.hover : (setMa.containsMouse ? Theme.hover : "transparent")
                Behavior on color { ColorAnimation { duration: 140 } }
            }
            IconImg {
                anchors.centerIn: parent
                src: "qrc:/icons/sliders.svg"
                tint: settingsBtn.active || setMa.containsMouse ? Theme.t1 : Theme.t3
                s: 16
                Behavior on tint { ColorAnimation { duration: 140 } }
            }
            MouseArea {
                id: setMa
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: bar.settingsClicked()
            }
            ToolTip.text: (i18n.language, i18n.t("tb_settings"))
            ToolTip.visible: setMa.containsMouse
            ToolTip.delay: 400
        }
    }
}
