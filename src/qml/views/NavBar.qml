// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// The one navigation bar, on top. On Windows it is also the window's title
// bar: a thin strip above it carries back/forward, the app's name and help,
// with the window buttons at its end, so the row below has its whole width
// for the tabs; the empty stretch of either drags the window (double click
// maximizes). The logo opens the app menu that used to be a menu bar of its
// own. Tabs on the left; continue chip, VPN and settings on the right.
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Effects
import "../theme"
import "../widgets"

Rectangle {
    id: bar
    // the title strip exists only where the bar is the window's title bar
    readonly property real stripH: ownsTitleBar ? 32 : 0
    implicitHeight: 52 + stripH
    // With the title strip the bar is the window's frame, a step darker, and
    // a thin accent line sets it apart from the app below.
    color: ownsTitleBar ? Theme.frame : Theme.nav

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
    // page history, like a browser's
    property bool canBack: false
    property bool canForward: false
    signal backRequested()
    signal forwardRequested()
    signal helpRequested(Item anchor)

    // Everything a click must reach, for the frameless window: the rest of the
    // bar is title bar (drag, double click, the system menu on right click).
    readonly property var hitItems: [logoBtn, navRow, dlChip, vpnChip, gearBtn, backBtn, fwdBtn, helpBtn]
    readonly property alias minButton: minBtn
    readonly property alias maxButton: maxBtn
    readonly property alias closeButton: closeBtn

    readonly property bool tightChip: width < 1000

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

    Rectangle {
        id: sheet
        visible: bar.ownsTitleBar
        y: bar.stripH
        width: parent.width
        height: parent.height - y
        color: Theme.nav
        // one thin line in the accent marks where the app begins
        Rectangle { width: parent.width; height: 1; color: Theme.accent }
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
        anchors.verticalCenterOffset: bar.stripH / 2
        width: logoRow.implicitWidth + 16
        height: 40
        radius: Theme.rad(9)
        color: bar.menuOpen ? Theme.hover2 : (logoMa.containsMouse ? Theme.hover : "transparent")
        Behavior on color { ColorAnimation { duration: Theme.durFast } }
        Row {
            id: logoRow
            anchors.centerIn: parent
            spacing: 5
            Image {
                anchors.verticalCenter: parent.verticalCenter
                width: 30; height: 30
                source: Theme.logoSource   // dark body on the light themes
                sourceSize: Qt.size(60, 60)
                fillMode: Image.PreserveAspectFit
            }
            // Sakura: the kanji for cherry blossom beside the logo
            Text {
                visible: Theme.kanji
                anchors.verticalCenter: parent.verticalCenter
                text: "桜"
                color: Theme.accent
                font.pixelSize: 18
                font.weight: Font.Bold
                font.family: "Shippori Mincho"
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
        y: bar.stripH
        height: parent.height - bar.stripH
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
                    radius: Theme.rad(9)
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
                        // Matrix reads like a prompt: "> downloads_"
                        text: (Theme.tabCursor && navTab.active ? "> " : "") + navTab.modelData.label
                        color: navTab.ink
                        font.pixelSize: Math.round(Theme.tabSize)
                        font.weight: navTab.active ? Theme.tabWeightOn : Theme.tabWeightOff
                        font.family: Theme.fontDisplay
                        font.capitalization: Theme.tabCaps
                        font.italic: Theme.tabItalic
                        font.letterSpacing: Theme.tabTracking
                        Behavior on color { ColorAnimation { duration: Theme.durFast } }
                    }
                    Rectangle {
                        visible: Theme.tabCursor && navTab.active
                        anchors.verticalCenter: parent.verticalCenter
                        width: 8; height: 15
                        color: Theme.accent
                        SequentialAnimation on opacity {
                            running: Theme.tabCursor && navTab.active && !Theme.reduceMotion
                            loops: Animation.Infinite
                            NumberAnimation { to: 0; duration: 1; }
                            PauseAnimation { duration: 530 }
                            NumberAnimation { to: 1; duration: 1 }
                            PauseAnimation { duration: 530 }
                        }
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
        visible: !Theme.tabCursor   // Matrix marks the tab with a cursor instead
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
        anchors.right: parent.right
        anchors.rightMargin: 12
        anchors.verticalCenter: parent.verticalCenter
        anchors.verticalCenterOffset: bar.stripH / 2
        spacing: 2

        NavBarDownloadChip { id: dlChip; bar: bar; car: car }

        NavBarVpnChip { id: vpnChip; bar: bar }

        Rectangle {
            id: gearBtn
            readonly property bool on: bar.currentIndex === 3
            Layout.preferredWidth: gearRow.implicitWidth + 22
            Layout.preferredHeight: 36
            radius: Theme.rad(9)
            color: on ? Theme.hover2 : (gearMa.containsMouse ? Theme.hover : "transparent")
            Behavior on color { ColorAnimation { duration: Theme.durFast } }
            readonly property color ink: gearBtn.on || gearMa.containsMouse ? Theme.t1 : Theme.t2
            Row {
                id: gearRow
                anchors.centerIn: parent
                spacing: 7
                IconImg {
                    anchors.verticalCenter: parent.verticalCenter
                    src: "qrc:/icons/settings.svg"
                    tint: gearBtn.ink
                    s: 17
                }
                Text {
                    visible: !bar.tightChip
                    anchors.verticalCenter: parent.verticalCenter
                    text: (i18n.language, i18n.t("tb_settings"))
                    color: gearBtn.ink
                    font.pixelSize: 13; font.weight: Font.Medium; font.family: Theme.fontSans
                }
            }
            MouseArea {
                id: gearMa
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: bar.settingsClicked()
            }
            ToolTip.text: (i18n.language, i18n.t("tb_settings"))
            ToolTip.visible: gearMa.containsMouse && bar.tightChip
            ToolTip.delay: 400
            Accessible.role: Accessible.Button
            Accessible.name: (i18n.language, i18n.t("tb_settings"))
            Accessible.onPressAction: bar.settingsClicked()
        }
    }

    // ----- title strip (frameless only) -----
    // back/forward on the left, the app's name in the middle, help and the
    // window buttons on the right
    Item {
        id: titleStrip
        visible: bar.ownsTitleBar
        width: parent.width
        height: bar.stripH
        Row {
            x: 8
            anchors.verticalCenter: parent.verticalCenter
            spacing: 2
            StripButton {
                id: backBtn
                icon: "qrc:/icons/arrow-left.svg"
                active: bar.canBack
                tip: (i18n.language, i18n.t("nav_back"))
                onClicked: bar.backRequested()
            }
            StripButton {
                id: fwdBtn
                icon: "qrc:/icons/arrow-right.svg"
                active: bar.canForward
                tip: (i18n.language, i18n.t("nav_forward"))
                onClicked: bar.forwardRequested()
            }
        }
        Row {
            anchors.centerIn: parent
            spacing: 7
            Image {
                anchors.verticalCenter: parent.verticalCenter
                width: 16; height: 16
                source: Theme.logoSource
                sourceSize: Qt.size(32, 32)
                fillMode: Image.PreserveAspectFit
            }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: "BATorrent"
                color: Theme.t2
                font.pixelSize: 12
                font.weight: Font.DemiBold
                font.family: Theme.fontSans
            }
        }
        Row {
            id: winButtons
            anchors.right: parent.right
            anchors.top: parent.top
            StripButton {
                id: helpBtn
                icon: "qrc:/icons/help.svg"
                tip: (i18n.language, i18n.t("menu_help_title"))
                onClicked: bar.helpRequested(helpBtn)
            }
            Item {
                width: 13; height: bar.stripH
                Rectangle { anchors.centerIn: parent; width: 1; height: 16; color: Theme.hair }
            }
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
    }

    // A small icon button of the title strip: back, forward, help.
    component StripButton: Rectangle {
        id: sbtn
        property string icon
        property string tip
        property bool active: true
        signal clicked()
        width: 34; height: bar.stripH
        radius: Theme.rad(6)
        color: sbtnMa.containsMouse && sbtn.active ? Theme.hover2 : "transparent"
        IconImg {
            anchors.centerIn: parent
            src: sbtn.icon
            s: 18
            tint: !sbtn.active ? Theme.t4 : (sbtnMa.containsMouse ? Theme.t1 : Theme.t3)
        }
        MouseArea {
            id: sbtnMa
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: sbtn.active ? Qt.PointingHandCursor : Qt.ArrowCursor
            onClicked: if (sbtn.active) sbtn.clicked()
        }
        ToolTip.visible: sbtnMa.containsMouse && sbtn.tip.length > 0
        ToolTip.text: sbtn.tip
        ToolTip.delay: 500
        Accessible.role: Accessible.Button
        Accessible.name: sbtn.tip
    }

    // Caption buttons drawn the way Windows draws them: 46 wide, the strip's
    // height, a hairline glyph, grey wash on hover and red under the close button.
    component WinButton: Rectangle {
        id: wb
        property string glyph: "min"
        signal clicked()
        width: 46; height: bar.stripH
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
            width: 10; height: 10; radius: Theme.rad(1)
            color: "transparent"
            border.width: 1; border.color: wb.ink
        }
        Item {        // restore: two offset squares
            visible: wb.glyph === "restore"
            anchors.centerIn: parent
            width: 10; height: 10
            Rectangle { x: 2; y: 0; width: 8; height: 8; radius: Theme.rad(1); color: "transparent"; border.width: 1; border.color: wb.ink }
            Rectangle { x: 0; y: 2; width: 8; height: 8; radius: Theme.rad(1); color: wb.color.a > 0 ? wb.color : bar.color; border.width: 1; border.color: wb.ink }
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
