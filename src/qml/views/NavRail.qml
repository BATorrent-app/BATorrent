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
import "../theme"
import "../widgets"

Rectangle {
    id: rail
    implicitWidth: collapsed ? 64 : 188
    color: Theme.panel
    clip: true

    property int currentIndex: 0            // bound down from the window; never self-assigned
    property bool collapsed: false
    signal pageRequested(int page)
    signal settingsClicked()
    signal vpnClicked()          // open the VPN cockpit (Settings → VPN section)
    signal selectTorrent(string infoHash)
    signal makeRoomRequested()
    signal aboutRequested()      // the brand mark is the way into About

    // Contextual rail slot (rotating carousel): the state lives in the shared
    // DownloadCarousel so the top-bar chip drives the exact same logic.
    property bool showDownloadChip: true    // host setting gate
    readonly property DownloadCarousel carousel: DownloadCarousel {
        id: car
        currentPage: rail.currentIndex
        hovered: dlSlot.slotHovered
        active: rail.showDl
    }
    // Shown on every page, Downloads included. It used to be gated off page 0
    // on the theory that the list says it better, but that left the rail with a
    // hole exactly where users spend their time — and it also meant the
    // carousel's resume mode, which only arms on page 0, could never appear.
    readonly property bool showDl: !collapsed && car.dlList.length > 0 && showDownloadChip
    Connections {
        target: car
        function onDlIndexChanged() { dlFade.restart() }
    }
    SequentialAnimation {
        id: dlFade
        NumberAnimation { target: dlSlot; property: "contentOpacity"; to: 0; duration: 160; easing.type: Easing.InCubic }
        ScriptAction { script: car.dlShown = car.dlIndex }
        NumberAnimation { target: dlSlot; property: "contentOpacity"; to: 1; duration: 300; easing.type: Easing.OutCubic }
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
            Layout.preferredHeight: 66
            Image {
                anchors.verticalCenter: parent.verticalCenter
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
                anchors.left: parent.left; anchors.leftMargin: 52
                spacing: 0
                opacity: rail.collapsed ? 0 : 1
                Behavior on opacity { NumberAnimation { duration: 140 } }
                Text { text: "BAT"; color: Theme.accent; font.family: "New Rocker"; font.pixelSize: 21 }
                Text { text: "orrent"; color: Theme.t1; font.family: "New Rocker"; font.pixelSize: 21 }
            }
            // the whole brand block opens About — the conventional home for a
            // logotype, and the only place version/credits were reachable from
            MouseArea {
                id: brandMa
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: rail.aboutRequested()
                ToolTip.visible: containsMouse
                ToolTip.text: (i18n.language, i18n.t("menu_about"))
                ToolTip.delay: 500
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
                        tint: navItem.active ? Theme.t1 : Theme.t2
                        s: 20
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

        Item { Layout.fillHeight: true }   // the slot and the bottom group settle together at the foot

        NavRailDownloadSlot { id: dlSlot; rail: rail; car: car }
        NavRailMiniDownloads { rail: rail; car: car }


        Rectangle {
            Layout.fillWidth: true; Layout.leftMargin: 16; Layout.rightMargin: 16
            Layout.topMargin: 10; Layout.bottomMargin: 4
            implicitHeight: 1; color: Theme.hairSoft
            visible: !rail.collapsed
        }

        // ----- VPN (status + door to the cockpit: Settings → VPN) -----
        Item {
            id: vpnItem
            visible: typeof vpn !== "undefined"
            readonly property int st: (typeof vpn !== "undefined") ? vpn.connState : 0
            readonly property color stColor: st === 2 ? Theme.grn : st === 1 ? Theme.amber
                                           : st === 3 ? Theme.accent : Theme.t4
            Layout.fillWidth: true
            Layout.preferredHeight: visible ? 46 : 0
            Layout.leftMargin: 10
            Layout.rightMargin: 10
            Rectangle {
                anchors.fill: parent
                radius: 10
                color: vpnMa.containsMouse ? Qt.rgba(1, 1, 1, 0.05) : "transparent"
                Behavior on color { ColorAnimation { duration: 140 } }
            }
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: rail.collapsed ? 13 : 17
                anchors.rightMargin: 12
                spacing: 13
                Item {
                    Layout.alignment: Qt.AlignVCenter
                    Layout.preferredWidth: 18; Layout.preferredHeight: 18
                    IconImg { anchors.centerIn: parent; src: "qrc:/icons/set-vpn.svg"; tint: vpnItem.stColor; s: 20 }
                    Rectangle {
                        width: 7; height: 7; radius: 3.5
                        anchors.right: parent.right; anchors.bottom: parent.bottom
                        anchors.rightMargin: -2; anchors.bottomMargin: -1
                        color: vpnItem.stColor
                        border.color: Theme.panel; border.width: 1.5
                        SequentialAnimation on opacity {
                            running: vpnItem.st === 1; loops: Animation.Infinite
                            NumberAnimation { from: 1; to: 0.3; duration: 600 }
                            NumberAnimation { from: 0.3; to: 1; duration: 600 }
                        }
                    }
                }
                Text {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignVCenter
                    text: "VPN"
                    color: vpnItem.st === 2 ? Theme.t1 : Theme.t2
                    font.pixelSize: 14; font.weight: Font.Medium; font.family: Theme.fontSans
                    opacity: rail.collapsed ? 0 : 1
                    Behavior on opacity { NumberAnimation { duration: 140 } }
                }
            }
            MouseArea {
                id: vpnMa
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: rail.vpnClicked()
            }
            ToolTip.text: (i18n.language, "VPN — " +
                (vpnItem.st === 2 ? i18n.t("vpn_state_on") : vpnItem.st === 1 ? i18n.t("vpn_state_connecting")
                 : vpnItem.st === 3 ? i18n.t("vpn_state_failed") : i18n.t("vpn_state_off")))
            ToolTip.visible: rail.collapsed && vpnMa.containsMouse
            ToolTip.delay: 400
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
                IconImg { Layout.alignment: Qt.AlignVCenter; src: "qrc:/icons/heart.svg"; tint: donMa.containsMouse ? Theme.accent : Theme.t2; s: 20 }
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
                IconImg { Layout.alignment: Qt.AlignVCenter; src: "qrc:/icons/settings.svg"; tint: settingsItem.active ? Theme.t1 : Theme.t2; s: 20; Behavior on tint { ColorAnimation { duration: 140 } } }
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

        Item { Layout.preferredHeight: 8 }
    }
}
