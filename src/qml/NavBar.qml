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
import "theme"
import "widgets"

Rectangle {
    id: bar
    implicitHeight: 54
    color: Theme.nav

    property int currentIndex: 0            // bound down from the window; never self-assigned
    property bool discoverVisible: true     // gated off in store builds (step ⑦)
    signal pageRequested(int page)
    signal settingsClicked()
    signal selectTorrent(string infoHash)
    signal makeRoomRequested()

    // responsive degradation: the chip loses its text first, then the disk
    // gauge loses its labels — nothing ever clips
    readonly property bool tightChip: width < 1260
    readonly property bool tightDisk: width < 1140

    readonly property DownloadCarousel carousel: DownloadCarousel {
        id: car
        currentPage: bar.currentIndex
        hovered: chipHover.hovered
        active: car.dlList.length > 0
    }
    Connections {
        target: car
        function onDlIndexChanged() { chipFade.restart() }
    }
    SequentialAnimation {
        id: chipFade
        NumberAnimation { target: chipContent; property: "opacity"; to: 0; duration: 160; easing.type: Easing.InCubic }
        ScriptAction { script: car.dlShown = car.dlIndex }
        NumberAnimation { target: chipContent; property: "opacity"; to: 1; duration: 300; easing.type: Easing.OutCubic }
    }

    // most-pressured save volume drives the gauge; the tooltip lists them all
    readonly property var diskVolumes: (typeof session !== "undefined") ? session.diskVolumes : []
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
            { icon: "qrc:/icons/discover.svg", label: i18n.t("nav_discover"),  page: 1 },
            { icon: "qrc:/icons/search.svg",   label: i18n.t("nav_search"),    page: 2 },
            { icon: "qrc:/icons/hub.svg",      label: i18n.t("nav_hub"),       page: 3 }
        ]
        // Store builds hide Discover (page 1); other pages keep their indices.
        if (typeof isStoreBuild !== "undefined" && isStoreBuild)
            return all.filter(function (x) { return x.page !== 1 })
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

        // ----- brand (logo + New Rocker wordmark, as on the rail/splash) -----
        Row {
            Layout.alignment: Qt.AlignVCenter
            Layout.rightMargin: Theme.sp4
            spacing: 8
            Image {
                anchors.verticalCenter: parent.verticalCenter
                width: 26; height: 26
                source: "qrc:/images/logo.svg"
                sourceSize: Qt.size(52, 52)
                fillMode: Image.PreserveAspectFit
                layer.enabled: Theme.isLight
                layer.effect: MultiEffect { colorization: 1.0; colorizationColor: Theme.t1 }
            }
            Row {
                anchors.verticalCenter: parent.verticalCenter
                spacing: 0
                Text { text: "BAT"; color: Theme.accent; font.family: "New Rocker"; font.pixelSize: 19 }
                Text { text: "orrent"; color: Theme.t1; font.family: "New Rocker"; font.pixelSize: 19 }
            }
        }

        // ----- page tabs -----
        Repeater {
            id: navRepeater
            model: bar.items
            delegate: Item {
                id: navTab
                required property var modelData
                readonly property bool active: bar.currentIndex === modelData.page
                visible: !(modelData.page === 1 && !bar.discoverVisible)
                Layout.fillHeight: true
                Layout.preferredWidth: visible ? tabRow.implicitWidth + 30 : 0

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

        // ----- download chip (the rail carousel, condensed) -----
        Rectangle {
            id: dlChip
            visible: car.dlList.length > 0
            Layout.alignment: Qt.AlignVCenter
            Layout.preferredHeight: 40
            Layout.preferredWidth: chipContent.implicitWidth + 20
            radius: 9
            color: chipHover.hovered ? Theme.hover : "transparent"
            Behavior on color { ColorAnimation { duration: 130 } }
            HoverHandler { id: chipHover }

            RowLayout {
                id: chipContent
                anchors.centerIn: parent
                spacing: 10

                PosterThumb {
                    Layout.preferredWidth: 24; Layout.preferredHeight: 32
                    posterUrl: car.dlItem ? (car.dlItem.poster || "") : ""
                    label: car.dlItem ? (car.dlItem.title || "") : ""
                }
                ColumnLayout {
                    visible: !bar.tightChip
                    Layout.alignment: Qt.AlignVCenter
                    spacing: 3
                    Text {
                        Layout.maximumWidth: 130
                        text: car.dlItem ? (car.dlItem.title || "") : ""
                        color: Theme.t2
                        font.pixelSize: 12; font.weight: Font.DemiBold; font.family: Theme.fontSans
                        elide: Text.ElideRight
                    }
                    RowLayout {
                        spacing: 6
                        Text {
                            text: !car.dlItem ? ""
                                  : car.slotResume ? ("▶ " + i18n.t("hub_resume"))
                                  : (car.dlItem.paused === true) ? ("⏸ " + i18n.t("state_paused"))
                                  : car.slotSeed ? ("↑ " + (car.dlItem.upSpeed || ""))
                                  : ("↓ " + (car.dlItem.downSpeed || ""))
                            color: Theme.accent
                            font.pixelSize: 10; font.family: Theme.fontSans; font.features: Theme.tnum
                        }
                        Rectangle {
                            visible: !car.slotResume
                            Layout.preferredWidth: 46; Layout.preferredHeight: 2
                            Layout.alignment: Qt.AlignVCenter
                            radius: 1; color: Theme.track
                            Rectangle {
                                anchors.left: parent.left; anchors.top: parent.top; anchors.bottom: parent.bottom
                                width: parent.width * Math.min(1, car.dlItem ? (car.dlItem.progress || 0) : 0)
                                radius: 1; color: Theme.accent
                            }
                        }
                    }
                }
                Text {
                    visible: car.dlList.length > 1
                    text: (car.dlShown + 1) + "/" + car.dlList.length
                    color: Theme.t4
                    font.pixelSize: 10; font.family: Theme.fontSans; font.features: Theme.tnum
                }
            }
            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    if (!car.dlItem) return
                    if (car.slotResume) {
                        if (car.dlItem.kind === "game") session.launchGame(car.dlItem.infoHash)
                        else session.playByHash(car.dlItem.infoHash)
                    } else {
                        bar.selectTorrent(car.dlItem.infoHash); bar.pageRequested(0)
                    }
                }
            }
            ToolTip.visible: chipHover.hovered
            ToolTip.text: (i18n.language, i18n.t(car.slotResume ? "nav_continue" : (car.slotSeed ? "nav_seeding" : "nav_downloading")))
            ToolTip.delay: 400
        }

        // ----- disk gauge (worst save volume; click → free up space) -----
        Item {
            id: diskGauge
            visible: bar.diskWorst !== null
            Layout.alignment: Qt.AlignVCenter
            Layout.preferredHeight: 40
            Layout.preferredWidth: bar.tightDisk ? 76 : diskCol.implicitWidth + 24
            Rectangle {
                anchors.fill: parent
                radius: 9
                color: diskMa.containsMouse ? Theme.hover : "transparent"
                Behavior on color { ColorAnimation { duration: 130 } }
            }
            ColumnLayout {
                id: diskCol
                anchors.centerIn: parent
                spacing: 5
                RowLayout {
                    visible: !bar.tightDisk
                    spacing: 8
                    Text {
                        Layout.maximumWidth: 110
                        text: bar.diskWorst ? bar.diskWorst.name : ""
                        color: Theme.t4; elide: Text.ElideRight
                        font.pixelSize: 9; font.weight: Font.Bold; font.letterSpacing: 1.0
                        font.capitalization: Font.AllUppercase; font.family: Theme.fontSans
                    }
                    Text {
                        text: bar.diskWorst ? (i18n.language, i18n.t("status_free_space")).arg(bar.diskWorst.free) : ""
                        color: Theme.t3
                        font.pixelSize: 11; font.family: Theme.fontSans; font.features: Theme.tnum
                    }
                }
                Rectangle {
                    Layout.fillWidth: true
                    Layout.minimumWidth: 56
                    Layout.preferredHeight: 3
                    radius: 2; color: Theme.track
                    Rectangle {
                        readonly property real used: bar.diskWorst ? Math.max(0.02, Math.min(1, bar.diskWorst.usedFraction)) : 0
                        anchors.left: parent.left; anchors.top: parent.top; anchors.bottom: parent.bottom
                        width: parent.width * used
                        radius: 2
                        // neutral gauge; color only signals real disk pressure
                        color: used > 0.95 ? Theme.accent : used > 0.85 ? Theme.amber : Theme.t4
                    }
                }
            }
            MouseArea {
                id: diskMa
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: bar.makeRoomRequested()
            }
            ToolTip.visible: diskMa.containsMouse
            ToolTip.text: {
                var lines = []
                for (var i = 0; i < bar.diskVolumes.length; ++i)
                    lines.push(bar.diskVolumes[i].name + " — " + (i18n.language, i18n.t("status_free_space")).arg(bar.diskVolumes[i].free))
                return lines.join("\n")
            }
            ToolTip.delay: 400
        }

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
            readonly property bool active: bar.currentIndex === 4
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
