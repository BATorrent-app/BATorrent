// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Left navigation rail for the 4.0 hub. Switches the main content stack between
// Downloads / Discover / Search / HUB; Settings (bottom) opens its window.
// Animated: selection accent bar, hover/active color fades, collapse/expand.
// Collapsible: a chevron at the bottom toggles icon-only mode (state persisted).
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Effects
import "theme"
import "widgets"

Rectangle {
    id: rail
    implicitWidth: collapsed ? 64 : 188
    color: Theme.panel
    clip: true

    property int currentIndex: 0            // bound down from the window; never self-assigned
    property bool collapsed: false
    signal pageRequested(int page)
    signal settingsClicked()
    signal selectTorrent(string infoHash)
    signal makeRoomRequested()

    // Contextual rail slot (rotating carousel): the state lives in the shared
    // DownloadCarousel so the top-bar chip drives the exact same logic.
    readonly property DownloadCarousel carousel: DownloadCarousel {
        id: car
        currentPage: rail.currentIndex
        hovered: dlHov.hovered
        active: rail.showDl
    }
    property bool showDownloadChip: true    // host setting gate
    readonly property bool showDl: !collapsed && car.dlList.length > 0 && showDownloadChip
    Connections {
        target: car
        function onDlIndexChanged() { dlFade.restart() }
    }
    SequentialAnimation {
        id: dlFade
        NumberAnimation { target: dlContent; property: "opacity"; to: 0; duration: 160; easing.type: Easing.InCubic }
        ScriptAction { script: car.dlShown = car.dlIndex }
        NumberAnimation { target: dlContent; property: "opacity"; to: 1; duration: 300; easing.type: Easing.OutCubic }
    }

    Behavior on implicitWidth { NumberAnimation { duration: 200; easing.type: Easing.OutCubic } }

    // QSettings stores bool differently per platform (macOS plist=bool, Windows
    // registry=int, Linux INI=string), so persist as 0/1 and read all forms.
    Component.onCompleted: {
        if (typeof settings === "undefined") return
        var v = settings.get("navRailCollapsed")
        collapsed = (v === true || v === 1 || v === "1" || v === "true")
    }
    function toggleCollapsed() {
        collapsed = !collapsed
        if (typeof settings !== "undefined") settings.set("navRailCollapsed", collapsed ? 1 : 0)
    }

    // right hairline
    Rectangle { anchors.right: parent.right; width: 1; height: parent.height; color: Theme.hair }

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
    // spotlight). key: a page number, "settings", or "rail" (the whole rail).
    function itemRect(key, mapTo) {
        if (key === "rail") {
            var rp = rail.mapToItem(mapTo, 0, 0)
            return Qt.rect(rp.x, rp.y, rail.width, rail.height)
        }
        var it = (key === "settings") ? settingsItem : null
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

    ColumnLayout {
        anchors.fill: parent
        spacing: 2

        // ----- brand header -----
        // Standard collapsible-rail pattern: icon + wordmark when expanded, icon
        // only when collapsed. Wordmark matches the splash brand (BAT accent +
        // orrent t1) so it reads as the logotype, not generic text.
        Item {
            Layout.fillWidth: true
            // unified chrome: the traffic lights live in this corner — push the
            // brand below them instead of underneath
            Layout.preferredHeight: Theme.unifiedChrome ? 88 : 66
            Image {
                anchors.verticalCenter: parent.verticalCenter
                anchors.verticalCenterOffset: Theme.unifiedChrome ? 11 : 0
                anchors.left: parent.left; anchors.leftMargin: 18
                width: 30; height: 30
                source: "qrc:/images/logo.svg"
                sourceSize: Qt.size(60, 60)
                fillMode: Image.PreserveAspectFit
                layer.enabled: Theme.isLight
                layer.effect: MultiEffect { colorization: 1.0; colorizationColor: Theme.t1 }
            }
            // wordmark in New Rocker (OFL) — a real logotype, not a UI font.
            // Two-tone: BAT in accent (echoes the bat's red), orrent in t1.
            Row {
                anchors.verticalCenter: parent.verticalCenter
                anchors.verticalCenterOffset: Theme.unifiedChrome ? 11 : 0
                anchors.left: parent.left; anchors.leftMargin: 52
                spacing: 0
                opacity: rail.collapsed ? 0 : 1
                Behavior on opacity { NumberAnimation { duration: 140 } }
                Text { text: "BAT"; color: Theme.accent; font.family: "New Rocker"; font.pixelSize: 21 }
                Text { text: "orrent"; color: Theme.t1; font.family: "New Rocker"; font.pixelSize: 21 }
            }
        }

        Item { Layout.preferredHeight: 6 }

        // ----- nav items -----
        Repeater {
            id: navRepeater
            model: rail.items
            delegate: Item {
                id: navItem
                required property var modelData
                readonly property bool active: rail.currentIndex === modelData.page
                Layout.fillWidth: true
                Layout.preferredHeight: visible ? 46 : 0
                Layout.leftMargin: 10
                Layout.rightMargin: 10

                Rectangle {
                    anchors.fill: parent
                    radius: 10
                    color: navItem.active ? Theme.hover
                         : (itemMa.containsMouse ? Qt.rgba(1, 1, 1, 0.05) : "transparent")
                    Behavior on color { ColorAnimation { duration: 140 } }

                    // animated active accent bar
                    Rectangle {
                        anchors.left: parent.left; anchors.leftMargin: 3
                        anchors.verticalCenter: parent.verticalCenter
                        width: 3
                        height: navItem.active ? 22 : 0
                        radius: 2
                        color: Theme.accent
                        Behavior on height { NumberAnimation { duration: 200; easing.type: Easing.OutBack } }
                    }
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: rail.collapsed ? 13 : 17
                    anchors.rightMargin: 12
                    spacing: 13
                    IconImg {
                        Layout.alignment: Qt.AlignVCenter
                        src: navItem.modelData.icon
                        tint: navItem.active ? Theme.t1 : Theme.t3
                        s: 18
                        Behavior on tint { ColorAnimation { duration: 140 } }
                    }
                    Text {
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignVCenter
                        text: navItem.modelData.label
                        color: navItem.active ? Theme.t1 : Theme.t2
                        font.pixelSize: 14
                        font.weight: navItem.active ? Font.DemiBold : Font.Medium
                        font.family: Theme.fontSans
                        opacity: rail.collapsed ? 0 : 1
                        Behavior on color { ColorAnimation { duration: 140 } }
                        Behavior on opacity { NumberAnimation { duration: 140 } }
                    }
                }
                MouseArea {
                    id: itemMa
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: rail.pageRequested(navItem.modelData.page)
                }
                ToolTip.text: navItem.modelData.label
                ToolTip.visible: rail.collapsed && itemMa.containsMouse
                ToolTip.delay: 400
            }
        }

        Item { Layout.fillHeight: true }   // push disk + donate + Settings + collapse to the bottom

        // ----- disk usage: one block per volume torrents save to (multi-HD) -----
        Column {
            Layout.fillWidth: true
            Layout.leftMargin: 18; Layout.rightMargin: 18
            spacing: 11
            visible: !rail.collapsed && typeof session !== "undefined" && session.diskVolumes.length > 0
            Repeater {
                model: typeof session !== "undefined" ? session.diskVolumes : []
                delegate: Item {
                    id: dvItem
                    required property var modelData
                    width: parent.width
                    height: 30
                    Text {
                        id: dvName
                        anchors.left: parent.left; anchors.top: parent.top
                        anchors.right: dvFree.left; anchors.rightMargin: 8
                        text: modelData.name
                        color: dvMa.containsMouse ? Theme.t2 : Theme.t4; elide: Text.ElideRight
                        font.pixelSize: 9; font.weight: Font.Bold; font.letterSpacing: 1.0
                        font.capitalization: Font.AllUppercase; font.family: Theme.fontSans
                    }
                    Text {
                        id: dvFree
                        anchors.right: parent.right; anchors.top: parent.top
                        text: (i18n.language, i18n.t("status_free_space")).arg(modelData.free)
                        color: Theme.t3; font.pixelSize: 11; font.family: Theme.fontSans; font.features: Theme.tnum
                    }
                    Rectangle {
                        anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom
                        height: 3; radius: 2; color: Theme.track
                        Rectangle {
                            height: parent.height; radius: 2
                            width: parent.width * Math.max(0.02, Math.min(1, modelData.usedFraction))
                            // neutral gauge; color only signals real disk pressure
                            color: modelData.usedFraction > 0.95 ? Theme.accent
                                 : modelData.usedFraction > 0.85 ? Theme.amber : Theme.t4
                        }
                    }
                    // the bar was purely decorative — make it the entry point to
                    // free up space, since disk pressure is the one signal the
                    // app never let the user act on
                    MouseArea {
                        id: dvMa
                        anchors.fill: parent
                        anchors.margins: -4
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: rail.makeRoomRequested()
                    }
                }
            }
        }
        Rectangle {
            Layout.fillWidth: true; Layout.leftMargin: 16; Layout.rightMargin: 16
            Layout.topMargin: 10; Layout.bottomMargin: 4
            implicitHeight: 1; color: Theme.hairSoft
            visible: !rail.collapsed
        }

        // ----- donate (heart: gray at rest, red on hover) -----
        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 46
            Layout.leftMargin: 10
            Layout.rightMargin: 10
            Rectangle {
                anchors.fill: parent
                radius: 10
                color: donMa.containsMouse ? Qt.rgba(229/255, 51/255, 43/255, 0.12) : "transparent"
                Behavior on color { ColorAnimation { duration: 140 } }
            }
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: rail.collapsed ? 13 : 17
                anchors.rightMargin: 12
                spacing: 13
                IconImg { Layout.alignment: Qt.AlignVCenter; src: "qrc:/icons/heart.svg"; tint: donMa.containsMouse ? Theme.accent : Theme.t3; s: 18 }
                Text {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignVCenter
                    text: (i18n.language, i18n.t("action_donate"))
                    color: Theme.t2
                    font.pixelSize: 14
                    font.weight: Font.Medium
                    font.family: Theme.fontSans
                    opacity: rail.collapsed ? 0 : 1
                    Behavior on opacity { NumberAnimation { duration: 140 } }
                }
            }
            MouseArea {
                id: donMa
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: Qt.openUrlExternally("https://github.com/sponsors/Mateuscruz19")
            }
            ToolTip.text: (i18n.language, i18n.t("action_donate"))
            ToolTip.visible: rail.collapsed && donMa.containsMouse
            ToolTip.delay: 400
        }

        // ----- settings (page 4 — fullscreen tab, not a separate window) -----
        Item {
            id: settingsItem
            readonly property bool active: rail.currentIndex === 3
            Layout.fillWidth: true
            Layout.preferredHeight: 46
            Layout.leftMargin: 10
            Layout.rightMargin: 10
            Rectangle {
                anchors.fill: parent
                radius: 10
                color: settingsItem.active ? Theme.hover
                     : (setMa.containsMouse ? Qt.rgba(1, 1, 1, 0.05) : "transparent")
                Behavior on color { ColorAnimation { duration: 140 } }
                Rectangle {
                    anchors.left: parent.left; anchors.leftMargin: 3
                    anchors.verticalCenter: parent.verticalCenter
                    width: 3
                    height: settingsItem.active ? 22 : 0
                    radius: 2
                    color: Theme.accent
                    Behavior on height { NumberAnimation { duration: 200; easing.type: Easing.OutBack } }
                }
            }
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: rail.collapsed ? 13 : 17
                anchors.rightMargin: 12
                spacing: 13
                IconImg { Layout.alignment: Qt.AlignVCenter; src: "qrc:/icons/settings.svg"; tint: settingsItem.active ? Theme.t1 : Theme.t3; s: 18; Behavior on tint { ColorAnimation { duration: 140 } } }
                Text {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignVCenter
                    text: (i18n.language, i18n.t("tb_settings"))
                    color: settingsItem.active ? Theme.t1 : Theme.t2
                    font.pixelSize: 14
                    font.weight: settingsItem.active ? Font.DemiBold : Font.Medium
                    font.family: Theme.fontSans
                    opacity: rail.collapsed ? 0 : 1
                    Behavior on color { ColorAnimation { duration: 140 } }
                    Behavior on opacity { NumberAnimation { duration: 140 } }
                }
            }
            MouseArea {
                id: setMa
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: rail.settingsClicked()
            }
            ToolTip.text: (i18n.language, i18n.t("tb_settings"))
            ToolTip.visible: rail.collapsed && setMa.containsMouse
            ToolTip.delay: 400
        }

        // ----- collapse / expand toggle (back to just under Settings, as it always was) -----
        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 46
            Layout.leftMargin: 10
            Layout.rightMargin: 10
            Rectangle {
                anchors.fill: parent
                radius: 10
                color: tglMa.containsMouse ? Qt.rgba(1, 1, 1, 0.05) : "transparent"
                Behavior on color { ColorAnimation { duration: 140 } }
            }
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: rail.collapsed ? 13 : 17
                anchors.rightMargin: 12
                spacing: 13
                IconImg {
                    Layout.alignment: Qt.AlignVCenter
                    src: "qrc:/icons/chevron.svg"
                    tint: Theme.t3
                    s: 18
                    rotation: rail.collapsed ? -90 : 90   // down chevron → right (expand) / left (collapse)
                    Behavior on rotation { NumberAnimation { duration: 200; easing.type: Easing.OutCubic } }
                }
                Text {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignVCenter
                    text: (i18n.language, i18n.t("nav_collapse"))
                    color: Theme.t2
                    font.pixelSize: 14
                    font.weight: Font.Medium
                    font.family: Theme.fontSans
                    opacity: rail.collapsed ? 0 : 1
                    Behavior on opacity { NumberAnimation { duration: 140 } }
                }
            }
            MouseArea {
                id: tglMa
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: rail.toggleCollapsed()
            }
            ToolTip.text: (i18n.language, i18n.t("nav_expand"))
            ToolTip.visible: rail.collapsed && tglMa.containsMouse
            ToolTip.delay: 400
        }

        // ----- contextual rail slot (bottom): downloading off Downloads, continue/resume on it -----
        // height animates 0↔full so switching tabs slides instead of jumping, and a
        // finished/empty list leaves no empty hole.
        ColumnLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 18; Layout.rightMargin: 18
            Layout.bottomMargin: 4
            spacing: 9
            clip: true
            Layout.preferredHeight: rail.showDl ? implicitHeight : 0
            opacity: rail.showDl ? 1 : 0
            Behavior on Layout.preferredHeight { NumberAnimation { duration: 220; easing.type: Easing.OutCubic } }
            Behavior on opacity { NumberAnimation { duration: 180 } }

            Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: Theme.hair }

            RowLayout {
                Layout.fillWidth: true
                Text { text: (i18n.language, i18n.t(car.slotResume ? "nav_continue" : (car.slotSeed ? "nav_seeding" : "nav_downloading"))); color: Theme.t4; font.pixelSize: 9; font.weight: Font.Bold; font.letterSpacing: 1.0; font.capitalization: Font.AllUppercase; font.family: Theme.fontSans }
                Item { Layout.fillWidth: true }
                Text { visible: car.dlList.length > 1; text: (car.dlShown + 1) + "/" + car.dlList.length; color: Theme.t4; font.pixelSize: 10; font.family: Theme.fontSans; font.features: Theme.tnum }
            }

            // loose content (no box) — uses the full rail width
            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: 60
                HoverHandler { id: dlHov }
                RowLayout {
                    id: dlContent
                    anchors.fill: parent
                    spacing: 12
                    PosterThumb {
                        Layout.preferredWidth: 50; Layout.preferredHeight: 60; Layout.alignment: Qt.AlignVCenter
                        posterUrl: car.dlItem ? (car.dlItem.poster || "") : ""
                        label: car.dlItem ? (car.dlItem.title || "") : ""
                    }
                    ColumnLayout {
                        Layout.fillWidth: true; Layout.alignment: Qt.AlignVCenter
                        spacing: 6
                        Text { Layout.fillWidth: true; text: car.dlItem ? (car.dlItem.title || "") : ""; color: Theme.t1; font.pixelSize: 14; font.weight: Font.DemiBold; font.family: Theme.fontSans; elide: Text.ElideRight }
                        RowLayout {
                            Layout.fillWidth: true; spacing: 6
                            // svg play/pause glyphs — ▶/⏸ text renders as
                            // colored system emoji on Windows
                            IconImg {
                                visible: car.dlItem && (car.slotResume || car.dlItem.paused === true)
                                src: car.slotResume ? "qrc:/icons/play.svg" : "qrc:/icons/pause.svg"
                                tint: Theme.accent; s: 11
                                Layout.alignment: Qt.AlignVCenter
                            }
                            Text {
                                text: !car.dlItem ? ""
                                      : car.slotResume ? i18n.t("hub_resume")
                                      : (car.dlItem.paused === true) ? i18n.t("state_paused")
                                      : car.slotSeed ? ("↑ " + (car.dlItem.upSpeed || ""))
                                      : ("↓ " + (car.dlItem.downSpeed || ""))
                                color: Theme.accent; font.pixelSize: 13; font.family: Theme.fontSans; font.features: Theme.tnum
                            }
                            Item { Layout.fillWidth: true }
                            Text {
                                text: !car.dlItem ? ""
                                      : car.slotResume ? (car.dlItem.metric || "")
                                      : car.slotSeed ? ("⇅ " + (car.dlItem.ratio || "0.00"))
                                      : (Math.floor((car.dlItem.progress || 0) * 100) + "%")
                                color: Theme.t2; font.pixelSize: 13; font.weight: Font.DemiBold; font.family: Theme.fontSans; font.features: Theme.tnum
                            }
                        }
                        Rectangle {
                            Layout.fillWidth: true; Layout.preferredHeight: 4; radius: 2; color: Theme.track
                            Rectangle { anchors.left: parent.left; anchors.top: parent.top; anchors.bottom: parent.bottom; width: parent.width * Math.min(1, car.dlItem ? (car.dlItem.progress || 0) : 0); radius: 2; color: Theme.accent }
                        }
                    }
                }
                MouseArea {
                    id: dlMa; anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        if (!car.dlItem) return
                        if (car.slotResume) {
                            if (car.dlItem.kind === "game") session.launchGame(car.dlItem.infoHash)
                            else session.playByHash(car.dlItem.infoHash)
                        } else {
                            rail.selectTorrent(car.dlItem.infoHash); rail.pageRequested(0)
                        }
                    }
                }

                // hover nav arrows — scale to any number of downloads (no dot explosion)
                Rectangle {
                    anchors.left: parent.left; anchors.verticalCenter: parent.verticalCenter
                    width: 24; height: 24; radius: 12
                    color: "#ee15151a"; border.color: Theme.hair; border.width: 1
                    visible: car.dlList.length > 1
                    opacity: dlHov.hovered ? 1 : 0
                    Behavior on opacity { NumberAnimation { duration: 130 } }
                    Text { anchors.centerIn: parent; text: "‹"; color: Theme.t1; font.pixelSize: 16; font.family: Theme.fontSans }
                    MouseArea { anchors.fill: parent; enabled: dlHov.hovered; cursorShape: Qt.PointingHandCursor
                        onClicked: car.prev() }
                }
                Rectangle {
                    anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter
                    width: 24; height: 24; radius: 12
                    color: "#ee15151a"; border.color: Theme.hair; border.width: 1
                    visible: car.dlList.length > 1
                    opacity: dlHov.hovered ? 1 : 0
                    Behavior on opacity { NumberAnimation { duration: 130 } }
                    Text { anchors.centerIn: parent; text: "›"; color: Theme.t1; font.pixelSize: 16; font.family: Theme.fontSans }
                    MouseArea { anchors.fill: parent; enabled: dlHov.hovered; cursorShape: Qt.PointingHandCursor
                        onClicked: car.next() }
                }
            }

            // separator under the card
            Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: Theme.hair }
        }

        // (version moved to the status bar)
        Item { Layout.preferredHeight: 8 }
    }
}
