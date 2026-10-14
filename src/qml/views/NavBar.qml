// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// The one navigation bar, on top. On Windows it is also the window's title
// bar: the empty stretch drags the window (double click maximizes) and the
// window buttons sit at its right end. The logo opens the app menu that used
// to be a menu bar of its own. Tabs on the left; continue chip, alt speed,
// VPN and settings on the right.
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
    // true when the window has no system title bar: draws the window buttons
    property bool ownsTitleBar: false
    property var appWindow: null
    signal pageRequested(int page)
    signal settingsClicked()
    signal vpnClicked()          // open the VPN cockpit (Settings → VPN section)
    signal selectTorrent(string infoHash)
    signal makeRoomRequested()
    signal menuRequested(Item anchor)

    // Everything a click must reach, for the frameless window: the rest of the
    // bar is title bar (drag, double click, the system menu on right click).
    readonly property var hitItems: [logoBtn, navRow, dlChip, altChip, vpnChip, gearBtn]
    readonly property alias minButton: minBtn
    readonly property alias maxButton: maxBtn
    readonly property alias closeButton: closeBtn

    readonly property bool tightChip: width < 1180

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
        NumberAnimation { target: dlChip; property: "contentOpacity"; to: 0; duration: Theme.durBase; easing.type: Easing.InCubic }
        ScriptAction { script: car.dlShown = car.dlIndex }
        NumberAnimation { target: dlChip; property: "contentOpacity"; to: 1; duration: Theme.durSlower; easing.type: Theme.easeOut }
    }

    Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hairSoft }

    readonly property var items: buildItems(i18n.language)
    function buildItems() {
        return [
            { icon: "qrc:/icons/download.svg", label: i18n.t("nav_downloads"), page: 0 },
            { icon: "qrc:/icons/search.svg",   label: i18n.t("nav_find"),      page: 1 },
            { icon: "qrc:/icons/hub.svg",      label: i18n.t("nav_hub"),       page: 2 }
        ]
    }

    // Geometry of a nav target, mapped into `mapTo`'s coords (for the tour
    // spotlight). key: a page number, "settings", or "rail" (the whole bar).
    function itemRect(key, mapTo) {
        var it = null
        if (key === "rail") it = bar
        else if (key === "settings") it = gearBtn
        else {
            for (var i = 0; i < navRepeater.count; i++) {
                var d = navRepeater.itemAt(i)
                if (d && d.modelData && d.modelData.page === key) { it = d; break }
            }
        }
        if (!it) return Qt.rect(0, 0, 0, 0)
        var p = it.mapToItem(mapTo, 0, 0)
        return Qt.rect(p.x, p.y, it.width, it.height)
    }

    // ----- logo: the app menu -----
    Rectangle {
        id: logoBtn
        x: 10
        anchors.verticalCenter: parent.verticalCenter
        width: logoRow.implicitWidth + 16
        height: 40
        radius: 9
        color: bar.menuOpen ? Theme.hover2 : (logoMa.containsMouse ? Theme.hover : "transparent")
        Behavior on color { ColorAnimation { duration: Theme.durFast } }
        Row {
            id: logoRow
            anchors.centerIn: parent
            spacing: 5
            Image {
                anchors.verticalCenter: parent.verticalCenter
                width: 30; height: 30
                source: "qrc:/images/logo.svg"
                sourceSize: Qt.size(60, 60)
                fillMode: Image.PreserveAspectFit
                layer.enabled: Theme.isLight
                layer.effect: MultiEffect { colorization: 1.0; colorizationColor: Theme.t1 }
            }
            IconImg {
                anchors.verticalCenter: parent.verticalCenter
                src: "qrc:/icons/caret-down-fill.svg"
                tint: Theme.t4
                s: 9
            }
        }
        MouseArea {
            id: logoMa
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: bar.menuRequested(logoBtn)
            ToolTip.visible: containsMouse && !bar.menuOpen
            ToolTip.text: (i18n.language, i18n.t("nav_app_menu"))
            ToolTip.delay: 600
        }
        Accessible.role: Accessible.ButtonMenu
        Accessible.name: (i18n.language, i18n.t("nav_app_menu"))
        Accessible.onPressAction: bar.menuRequested(logoBtn)
    }
    property bool menuOpen: false

    // ----- page tabs -----
    Row {
        id: navRow
        anchors.left: logoBtn.right
        anchors.leftMargin: 14
        height: parent.height
        Repeater {
            id: navRepeater
            model: bar.items
            delegate: Item {
                id: navTab
                required property var modelData
                readonly property bool active: bar.currentIndex === modelData.page
                height: navRow.height
                width: tabRow.implicitWidth + 28

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
                    anchors.margins: 6
                    radius: 9
                    color: "transparent"
                    border.color: Theme.focusRing
                    border.width: Theme.focusRingWidth
                }
                readonly property color ink: navTab.active || tabMa.containsMouse ? Theme.t1 : Theme.t3
                Row {
                    id: tabRow
                    anchors.centerIn: parent
                    spacing: 8
                    IconImg {
                        anchors.verticalCenter: parent.verticalCenter
                        src: navTab.modelData.icon
                        tint: navTab.ink
                        s: 17
                        Behavior on tint { ColorAnimation { duration: Theme.durFast } }
                    }
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: navTab.modelData.label
                        color: navTab.ink
                        font.pixelSize: 14
                        font.weight: navTab.active ? Font.DemiBold : Font.Medium
                        font.family: Theme.fontSans
                        Behavior on color { ColorAnimation { duration: Theme.durFast } }
                    }
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
    }

    // One underline shared by the tabs; it slides to the active one.
    Rectangle {
        id: tabLine
        readonly property Item target: {
            var n = navRepeater.count, c = bar.currentIndex
            for (var i = 0; i < n; ++i) {
                var d = navRepeater.itemAt(i)
                if (d && d.modelData.page === c) return d
            }
            return null
        }
        property bool ready: false
        Component.onCompleted: Qt.callLater(function() { tabLine.ready = true })
        x: target ? navRow.x + target.x : x
        width: target ? target.width : width
        anchors.bottom: parent.bottom
        height: 2
        color: Theme.accent
        opacity: target ? 1 : 0
        Behavior on x { enabled: tabLine.ready && !Theme.reduceMotion
            NumberAnimation { duration: Theme.durSlower; easing.type: Theme.easeOut } }
        Behavior on width { enabled: tabLine.ready && !Theme.reduceMotion
            NumberAnimation { duration: Theme.durSlower; easing.type: Theme.easeOut } }
        Behavior on opacity { NumberAnimation { duration: Theme.durBase } }
    }

    // ----- right cluster -----
    RowLayout {
        id: rightRow
        anchors.right: winButtons.visible ? winButtons.left : parent.right
        anchors.rightMargin: winButtons.visible ? 0 : 12
        anchors.verticalCenter: parent.verticalCenter
        spacing: 2

        NavBarDownloadChip { id: dlChip; bar: bar; car: car }

        // alt speed (turtle): a mode with a visible state, next to the VPN
        Rectangle {
            id: altChip
            readonly property bool on: typeof session !== "undefined" && session.altSpeedsActive
            Layout.leftMargin: dlChip.visible ? 6 : 0
            Layout.preferredHeight: 36
            Layout.preferredWidth: altRow.implicitWidth + 24
            radius: 9
            color: on ? Theme.accentTint : (altMa.containsMouse ? Theme.hover : "transparent")
            Behavior on color { ColorAnimation { duration: Theme.durFast } }
            readonly property color ink: on ? Theme.accentText : Theme.t3
            function limitText() {
                var kb = typeof settings !== "undefined" ? Number(settings.get("altDownloadLimit") || 0) : 0
                if (kb <= 0) return ""
                return kb >= 1024 ? (Math.round(kb / 102.4) / 10).toLocaleString(Qt.locale(), 'f', kb % 1024 === 0 ? 0 : 1) + " MB/s"
                                  : kb + " KB/s"
            }
            Row {
                id: altRow
                anchors.centerIn: parent
                spacing: 8
                IconImg {
                    anchors.verticalCenter: parent.verticalCenter
                    src: "qrc:/icons/turtle.svg"
                    tint: altChip.ink
                    s: 17
                }
                Text {
                    visible: !bar.tightChip || altChip.on
                    anchors.verticalCenter: parent.verticalCenter
                    text: (i18n.language, altChip.on
                           ? (altChip.limitText().length > 0 ? i18n.t("nav_alt_on_limit").arg(altChip.limitText())
                                                             : i18n.t("nav_alt_on"))
                           : i18n.t("nav_alt_off"))
                    color: altChip.ink
                    font.pixelSize: 13; font.weight: Font.Medium; font.family: Theme.fontSans
                }
            }
            MouseArea {
                id: altMa
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: if (typeof session !== "undefined") session.setAltSpeedsActive(!session.altSpeedsActive)
            }
            ToolTip.text: (i18n.language, i18n.t("tb_alt_speed"))
            ToolTip.visible: altMa.containsMouse
            ToolTip.delay: 400
            Accessible.role: Accessible.CheckBox
            Accessible.name: (i18n.language, i18n.t("tb_alt_speed"))
            Accessible.checked: altChip.on
        }

        NavBarVpnChip { id: vpnChip; bar: bar }

        Rectangle {
            id: gearBtn
            readonly property bool on: bar.currentIndex === 3
            Layout.preferredWidth: 36
            Layout.preferredHeight: 36
            radius: 9
            color: on ? Theme.hover2 : (gearMa.containsMouse ? Theme.hover : "transparent")
            Behavior on color { ColorAnimation { duration: Theme.durFast } }
            IconImg {
                anchors.centerIn: parent
                src: "qrc:/icons/settings.svg"
                tint: gearBtn.on || gearMa.containsMouse ? Theme.t1 : Theme.t2
                s: 18
            }
            MouseArea {
                id: gearMa
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: bar.settingsClicked()
            }
            ToolTip.text: (i18n.language, i18n.t("tb_settings"))
            ToolTip.visible: gearMa.containsMouse
            ToolTip.delay: 400
            Accessible.role: Accessible.Button
            Accessible.name: (i18n.language, i18n.t("tb_settings"))
            Accessible.onPressAction: bar.settingsClicked()
        }
    }

    // ----- window buttons (frameless only) -----
    Row {
        id: winButtons
        visible: bar.ownsTitleBar
        anchors.right: parent.right
        anchors.top: parent.top
        Rectangle {
            anchors.top: parent.top
            anchors.topMargin: 16
            width: 1; height: 22
            color: Theme.hair
        }
        Item { width: 6; height: 1 }
        WinButton {
            id: minBtn
            glyph: "min"
            onClicked: if (bar.appWindow) bar.appWindow.showMinimized()
        }
        WinButton {
            id: maxBtn
            glyph: bar.appWindow && bar.appWindow.visibility === Window.Maximized ? "restore" : "max"
            onClicked: {
                if (!bar.appWindow) return
                if (bar.appWindow.visibility === Window.Maximized) bar.appWindow.showNormal()
                else bar.appWindow.showMaximized()
            }
        }
        WinButton {
            id: closeBtn
            glyph: "close"
            onClicked: if (bar.appWindow) bar.appWindow.close()
        }
    }

    // Caption buttons drawn the way Windows draws them: 46×32, a hairline
    // glyph, grey wash on hover and red under the close button.
    component WinButton: Rectangle {
        id: wb
        property string glyph: "min"
        signal clicked()
        width: 46; height: 32
        color: wbMa.containsMouse ? (glyph === "close" ? Theme.winClose : Theme.hover2) : "transparent"
        readonly property color ink: wbMa.containsMouse && glyph === "close" ? "#ffffff" : Theme.tSoft
        Rectangle {   // minimize
            visible: wb.glyph === "min"
            anchors.centerIn: parent
            width: 10; height: 1
            color: wb.ink
        }
        Rectangle {   // maximize
            visible: wb.glyph === "max"
            anchors.centerIn: parent
            width: 10; height: 10; radius: 1
            color: "transparent"
            border.width: 1; border.color: wb.ink
        }
        Item {        // restore: two offset squares
            visible: wb.glyph === "restore"
            anchors.centerIn: parent
            width: 10; height: 10
            Rectangle { x: 2; y: 0; width: 8; height: 8; radius: 1; color: "transparent"; border.width: 1; border.color: wb.ink }
            Rectangle { x: 0; y: 2; width: 8; height: 8; radius: 1; color: wb.color.a > 0 ? wb.color : bar.color; border.width: 1; border.color: wb.ink }
        }
        Item {        // close
            visible: wb.glyph === "close"
            anchors.centerIn: parent
            width: 13; height: 13
            Rectangle { anchors.centerIn: parent; width: 14; height: 1; rotation: 45; color: wb.ink; antialiasing: true }
            Rectangle { anchors.centerIn: parent; width: 14; height: 1; rotation: -45; color: wb.ink; antialiasing: true }
        }
        MouseArea {
            id: wbMa
            anchors.fill: parent
            hoverEnabled: true
            onClicked: wb.clicked()
        }
    }
}
