// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Appearance controls for SettingsRow: the theme picker (preview, what goes
// behind the library, the grid of themes), the user's picture, the editor of
// a theme of one's own, and the toggle and app-icon rows. Host is SettingsRow
// (colour/picture requests go out as its signals); field + sw are host props.
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import QtQuick.Effects
import "../theme"
import "../widgets"

// Item (not QtObject): Component children need a default property.
Item {
    id: root
    width: 0; height: 0; visible: false
    property var field
    property var sw
    property var host

    property Component toggle: cToggle
    property Component appicon: cAppIcon
    property Component appearance: cAppearance
    property Component userimage: cUserImage
    property Component editor: cEditor

    Component {
        id: cToggle
        TToggle {
            big: true
            on: (root.field.key === "torrentSearchEnabled" && typeof addons !== "undefined") ? addons.torrentSearchEnabled
                : (root.field.key === "autoTrackers" && typeof addons !== "undefined") ? addons.autoTrackers
                : (root.field.key === "followSystem" && typeof themeBridge !== "undefined") ? themeBridge.followSystem
                : root.sw.boolPref(root.field)
            onToggled: function(v) {
                if (root.field.key === "torrentSearchEnabled" && typeof addons !== "undefined") addons.torrentSearchEnabled = v
                else if (root.field.key === "autoTrackers" && typeof addons !== "undefined") addons.autoTrackers = v
                else if (root.field.key === "followSystem" && typeof themeBridge !== "undefined") themeBridge.followSystem = v
                else if (typeof settings !== "undefined" && root.field.key !== undefined) settings.set(root.field.key, v)
            }
        }
    }
    Component {
        id: cAppIcon
        Row {
            spacing: 10
            Repeater {
                model: (typeof themeBridge !== "undefined") ? themeBridge.appIcons() : []
                delegate: Rectangle {
                    id: tile
                    required property var modelData
                    readonly property bool sel: typeof themeBridge !== "undefined"
                        && (themeBridge.appIconChoice, themeBridge.appIconChoice === modelData.key)
                    width: 46; height: 46; radius: Theme.rad(12)
                    color: tile.sel ? Theme.hover2 : "transparent"
                    border.width: 2
                    border.color: tile.sel ? Theme.selRing : Theme.hair
                    Image {
                        anchors.centerIn: parent
                        width: 34; height: 34
                        source: tile.modelData.preview
                        sourceSize: Qt.size(68, 68)
                        fillMode: Image.PreserveAspectFit
                        smooth: true
                    }
                    MouseArea {
                        id: tileMa
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: if (typeof themeBridge !== "undefined") themeBridge.setAppIcon(tile.modelData.key)
                    }
                    ToolTip.text: tile.modelData.key
                    ToolTip.visible: tileMa.containsMouse
                    ToolTip.delay: 400
                }
            }
        }
    }

    // ---------- pieces ----------
    // A segmented choice: the selected option on a raised fill, like the
    // update channel. `stretch` shares the width out evenly.
    component Seg: Rectangle {
        id: seg
        property var options: []
        property int current: 0
        property bool stretch: false
        property bool enabledAll: true
        property var disabledAt: []          // indices that cannot be picked
        property var tips: []
        signal picked(int i)
        implicitWidth: segRow.implicitWidth + 8
        implicitHeight: 40
        radius: Theme.rad(10)
        color: Theme.field
        border.color: Theme.hair
        border.width: 1
        opacity: enabledAll ? 1 : 0.4
        Row {
            id: segRow
            x: 4
            anchors.verticalCenter: parent.verticalCenter
            spacing: 2
            Repeater {
                model: seg.options
                delegate: Rectangle {
                    id: opt
                    required property var modelData
                    required property int index
                    readonly property bool on: index === seg.current
                    readonly property bool off: !seg.enabledAll || seg.disabledAt.indexOf(index) >= 0
                    height: 32
                    width: seg.stretch ? (seg.width - 8 - segRow.spacing * (seg.options.length - 1)) / seg.options.length
                                       : optLbl.implicitWidth + 26
                    radius: Theme.rad(7)
                    color: on ? Theme.hover2 : (optMa.containsMouse && !off ? Theme.hover : "transparent")
                    opacity: off && seg.enabledAll ? 0.4 : 1
                    Text {
                        id: optLbl
                        anchors.centerIn: parent
                        text: opt.modelData
                        color: opt.on ? Theme.t1 : Theme.t3
                        font.pixelSize: 14
                        font.weight: Font.Medium
                        font.family: Theme.fontSans
                    }
                    MouseArea {
                        id: optMa
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: opt.off ? Qt.ArrowCursor : Qt.PointingHandCursor
                        onClicked: if (!opt.off) seg.picked(opt.index)
                    }
                    ToolTip.visible: optMa.containsMouse && (seg.tips[opt.index] || "").length > 0
                    ToolTip.text: seg.tips[opt.index] || ""
                    ToolTip.delay: 500
                }
            }
        }
    }

    // A labelled slider with its value on the right.
    component Slide: RowLayout {
        id: sl
        property string label
        property real from: 0
        property real to: 100
        property real value: 0
        property string unit: ""
        signal moved(real v)
        spacing: 14
        Text {
            text: sl.label
            color: Theme.t3
            font.pixelSize: 14
            font.family: Theme.fontSans
            Layout.preferredWidth: 84
        }
        Slider {
            id: slider
            Layout.fillWidth: true
            from: sl.from; to: sl.to; stepSize: 1
            onMoved: sl.moved(Math.round(value))
            Binding { target: slider; property: "value"; value: sl.value }
            background: Rectangle {
                x: parent.leftPadding; y: parent.topPadding + parent.availableHeight / 2 - height / 2
                width: parent.availableWidth; height: 4; radius: Theme.rad(2); color: Theme.track
                Rectangle { width: parent.width * slider.visualPosition; height: parent.height; radius: Theme.rad(2); color: Theme.accent }
            }
            handle: Rectangle {
                x: slider.leftPadding + slider.visualPosition * (slider.availableWidth - width)
                y: slider.topPadding + slider.availableHeight / 2 - height / 2
                implicitWidth: 18; implicitHeight: 18; radius: Theme.rad(9)
                color: Theme.accent; border.color: Theme.panel; border.width: 2
            }
        }
        Text {
            text: Math.round(sl.value) + sl.unit
            color: Theme.t2
            font.pixelSize: 14
            font.family: Theme.fontSans
            font.features: Theme.tnum
            horizontalAlignment: Text.AlignRight
            Layout.preferredWidth: 52
        }
    }

    // a small caption over a field
    component Cap: Text {
        color: Theme.t3
        font.pixelSize: 13
        font.family: Theme.fontSans
    }

    // the themes in the order of the boards, with their printed pictures
    readonly property var appThemes: Theme.order
    function themeLabel(k) {
        return k === "default" ? i18n.t("theme_default")
             : k === "cinema" ? i18n.t("theme_cinema")
             : k === "light" ? i18n.t("set_theme_light")
             : k === "midnight" ? "Midnight" : k === "sakura" ? "Sakura"
             : k === "darkstar" ? "Dark Star" : "Matrix"
    }
    // the boards still call Cinema by its first name
    function shotFor(k, withArt) {
        var id = k === "cinema" ? "dark" : k
        return "qrc:/images/theme-shots/" + id + "-" + (withArt && k !== "default" ? "bg" : "clean") + ".png"
    }
    readonly property bool userTheme: Theme.isUser
    readonly property var profiles: typeof themeBridge !== "undefined" ? themeBridge.customProfiles : []
    readonly property int activeIdx: typeof themeBridge !== "undefined" ? themeBridge.activeProfile : 0
    readonly property var active: profiles.length > activeIdx ? profiles[activeIdx] : ({})
    function setActive(key, v) { if (typeof themeBridge !== "undefined") themeBridge.setProfileValue(root.activeIdx, key, v) }
    function newTheme() {
        if (typeof themeBridge === "undefined") return
        var i = themeBridge.addProfile()
        themeBridge.renameProfile(i, i18n.t("ed_new_name").arg(i + 1))
        themeBridge.selectProfile(i)
    }

    // A theme of the user's own, drawn live: page, picture, nav, a few cards
    // and the details panel, in its colours, corners and title font.
    component UserPreview: Rectangle {
        id: up
        property var t: ({})
        readonly property color cBg: t.bg || "#0e0e10"
        readonly property color cPanel: t.panel || "#18181b"
        readonly property color cText: t.text || "#f3f3f4"
        readonly property color cPrimary: t.primary || "#e5332b"
        readonly property color cSecondary: t.secondary || "#d99a2b"
        readonly property color cTertiary: t.tertiary || "#3fb950"
        readonly property real r: ({ square: 0, soft: 4, round: 8 }[t.corners] || 4)
        readonly property bool hasImg: !!t.image
        function ink(a) { return Qt.rgba(cText.r, cText.g, cText.b, a) }
        color: cBg
        clip: true
        Image {
            anchors.fill: parent
            visible: up.hasImg
            source: up.hasImg ? ((Qt.platform.os === "windows" ? "file:///" : "file://") + encodeURI(up.t.image)) : ""
            fillMode: Image.PreserveAspectCrop
            asynchronous: true
            sourceSize.width: 600
        }
        Rectangle { anchors.fill: parent; visible: up.hasImg; color: up.cBg; opacity: (up.t.dim === undefined ? 55 : up.t.dim) / 100 }
        Column {
            anchors.fill: parent
            Rectangle {
                width: parent.width; height: parent.height * 0.11
                color: Qt.rgba(up.cPanel.r, up.cPanel.g, up.cPanel.b, 0.82)
                Row {
                    anchors.verticalCenter: parent.verticalCenter
                    x: parent.width * 0.04
                    spacing: parent.width * 0.05
                    Image { anchors.verticalCenter: parent.verticalCenter; height: parent.parent.height * 0.52; width: height; source: (up.t.base === "light") ? "qrc:/images/logo-light.svg" : "qrc:/images/logo.svg"; sourceSize: Qt.size(40, 40) }
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: i18n.t("nav_downloads")
                        color: up.cText; font.pixelSize: 11; font.weight: Font.DemiBold; font.family: up.t.font || "IBM Plex Sans"
                        Rectangle { anchors.top: parent.bottom; anchors.topMargin: 4; width: parent.width; height: 2; color: up.cPrimary }
                    }
                    Text { anchors.verticalCenter: parent.verticalCenter; text: i18n.t("nav_find"); color: up.ink(0.6); font.pixelSize: 11; font.family: up.t.font || "IBM Plex Sans" }
                }
            }
            Row {
                width: parent.width; height: parent.height * 0.89
                Item {
                    width: parent.width * 0.74; height: parent.height
                    Grid {
                        x: parent.width * 0.05; y: parent.height * 0.06
                        columns: 4
                        spacing: parent.width * 0.035
                        Repeater {
                            model: 4
                            Column {
                                required property int index
                                spacing: 4
                                readonly property real w: (up.width * 0.74 * 0.9 - 3 * up.width * 0.74 * 0.035) / 4
                                Rectangle {
                                    width: parent.w; height: parent.w * 1.5; radius: up.r
                                    color: Qt.tint(up.cPanel, up.ink(0.04 + index * 0.03))
                                    Rectangle {
                                        anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom
                                        anchors.margins: parent.width * 0.08
                                        height: 3; radius: 1.5
                                        color: index === 1 ? up.cPrimary : up.cTertiary
                                    }
                                }
                                Rectangle { width: parent.w * 0.8; height: 4; radius: 2; color: up.ink(0.85) }
                            }
                        }
                    }
                }
                Rectangle {
                    width: parent.width * 0.26; height: parent.height
                    color: Qt.rgba(up.cPanel.r, up.cPanel.g, up.cPanel.b, up.hasImg ? 0.78 : 1)
                    Rectangle { width: 1; height: parent.height; color: up.ink(0.14) }
                    Column {
                        x: parent.width * 0.12; y: parent.height * 0.06
                        width: parent.width * 0.76
                        spacing: 8
                        Text { width: parent.width; text: "Hollow Knight"; elide: Text.ElideRight; color: up.cText; font.pixelSize: 13; font.weight: Font.Bold; font.family: up.t.font || "IBM Plex Sans" }
                        Rectangle {
                            width: parent.width; height: 5; radius: 2.5; color: up.ink(0.14)
                            Rectangle { width: parent.width * 0.62; height: parent.height; radius: 2.5; color: up.cPrimary }
                        }
                        Text { text: "↑ 312 KB/s"; color: up.cSecondary; font.pixelSize: 10; font.family: "IBM Plex Sans" }
                    }
                }
            }
        }
    }

    // ---------- Theme: the preview, what goes behind, the grid ----------
    Component {
        id: cAppearance
        Item {
            id: ap
            width: root.host ? root.host.width : 760
            implicitHeight: Math.max(leftCol.implicitHeight, grid.implicitHeight)
            readonly property real gap: 24
            readonly property real leftW: Math.round((width - gap) * 1.3 / 2.3)
            readonly property string mode: Theme.backgroundMode
            Column {
                id: leftCol
                width: ap.leftW
                spacing: 14
                Item {
                    width: parent.width
                    height: Math.round(width * 352 / 560)
                    Rectangle { id: pvMask; anchors.fill: parent; radius: 10; visible: false; layer.enabled: true }
                    Item {
                        anchors.fill: parent
                        layer.enabled: true
                        layer.effect: MultiEffect { maskEnabled: true; maskSource: pvMask; maskThresholdMin: 0.5; maskSpreadAtMin: 1.0 }
                        Image {
                            anchors.fill: parent
                            visible: !root.userTheme
                            source: root.userTheme ? "" : root.shotFor(Theme.key, ap.mode === "theme")
                            fillMode: Image.PreserveAspectCrop
                            asynchronous: true
                            smooth: true; mipmap: true
                        }
                        UserPreview {
                            anchors.fill: parent
                            visible: root.userTheme
                            t: root.active
                        }
                        // where the user's picture will go
                        Rectangle {
                            visible: ap.mode === "user" && !root.userTheme
                            x: parent.width * 0.42; y: parent.height * 0.14
                            width: parent.width * 0.28; height: parent.height * 0.52
                            radius: 6
                            color: Qt.rgba(0, 0, 0, 0.45)
                            border.width: 1.5; border.color: Qt.rgba(1, 1, 1, 0.85)
                            Image {
                                anchors.fill: parent; anchors.margins: 2
                                visible: Theme.userArtSource !== ""
                                source: Theme.userArtSource
                                fillMode: Image.PreserveAspectFit
                                asynchronous: true
                                sourceSize.width: 300
                            }
                            Text {
                                visible: Theme.userArtSource === ""
                                anchors.centerIn: parent
                                text: i18n.t("set_your_image")
                                color: "#ffffff"; font.pixelSize: 12; font.weight: Font.DemiBold; font.family: Theme.fontSans
                            }
                        }
                    }
                    Rectangle { anchors.fill: parent; radius: 10; color: "transparent"; border.width: 1; border.color: Theme.hair }
                }
                Row {
                    spacing: 14
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: (i18n.language, i18n.t("set_bg_label"))
                        color: Theme.t3; font.pixelSize: 14; font.family: Theme.fontSans
                    }
                    Seg {
                        readonly property var keys: ["clean", "theme", "user"]
                        options: [(i18n.language, i18n.t("set_bg_clean")), (i18n.language, i18n.t("set_bg_theme")), (i18n.language, i18n.t("set_bg_user"))]
                        current: root.userTheme ? -1 : keys.indexOf(ap.mode)
                        enabledAll: !root.userTheme
                        disabledAt: Theme.isDefault ? [1] : []
                        tips: root.userTheme
                              ? [i18n.t("set_bg_user_theme"), i18n.t("set_bg_user_theme"), i18n.t("set_bg_user_theme")]
                              : [i18n.t("set_bg_clean_tip"), Theme.isDefault ? i18n.t("set_bg_default_none") : i18n.t("set_bg_theme_tip"), i18n.t("set_bg_user_tip")]
                        onPicked: function(i) { if (typeof themeBridge !== "undefined") themeBridge.backgroundMode = keys[i] }
                    }
                }
            }
            Grid {
                id: grid
                x: ap.leftW + ap.gap
                width: ap.width - x
                columns: 3
                columnSpacing: 10
                rowSpacing: 12
                readonly property real cellW: (width - columnSpacing * 2) / 3
                // the app's themes
                Repeater {
                    model: root.appThemes
                    delegate: Item {
                        id: tcard
                        required property string modelData
                        readonly property bool on: !root.userTheme && Theme.key === modelData
                        width: grid.cellW
                        height: tthumb.height + 6 + tname.implicitHeight
                        Rectangle {
                            x: -4; y: -4
                            width: tthumb.width + 8; height: tthumb.height + 8
                            radius: 11
                            color: "transparent"
                            border.width: 2
                            border.color: tcard.on ? Theme.selRing : "transparent"
                        }
                        Image {
                            id: tthumb
                            width: parent.width
                            height: Math.round(width * 352 / 560)
                            source: root.shotFor(tcard.modelData, ap.mode === "theme")
                            fillMode: Image.PreserveAspectCrop
                            asynchronous: true
                            smooth: true; mipmap: true
                            Rectangle { anchors.fill: parent; radius: 7; color: "transparent"; border.width: 1; border.color: tcardMa.containsMouse && !tcard.on ? Theme.t4 : Theme.hair }
                        }
                        Text {
                            id: tname
                            anchors.top: tthumb.bottom; anchors.topMargin: 6
                            width: parent.width
                            text: (i18n.language, root.themeLabel(tcard.modelData))
                            color: tcard.on ? Theme.t1 : Theme.t3
                            font.pixelSize: 14
                            font.weight: tcard.on ? Font.DemiBold : Font.Medium
                            font.family: Theme.fontSans
                            elide: Text.ElideRight
                        }
                        MouseArea { id: tcardMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: Theme.setName(tcard.modelData) }
                        Accessible.role: Accessible.RadioButton
                        Accessible.name: tname.text
                        Accessible.checked: tcard.on
                        Accessible.onPressAction: Theme.setName(tcard.modelData)
                    }
                }
                // the user's own
                Repeater {
                    model: root.profiles
                    delegate: Item {
                        id: ucard
                        required property var modelData
                        required property int index
                        readonly property bool on: root.userTheme && root.activeIdx === index
                        width: grid.cellW
                        height: uthumb.height + 6 + uname.implicitHeight
                        Rectangle {
                            x: -4; y: -4
                            width: uthumb.width + 8; height: uthumb.height + 8
                            radius: 11
                            color: "transparent"
                            border.width: 2
                            border.color: ucard.on ? Theme.selRing : "transparent"
                        }
                        Rectangle {
                            id: uthumb
                            width: parent.width
                            height: Math.round(width * 352 / 560)
                            radius: 7
                            clip: true
                            color: ucard.modelData.bg || "#0e0e10"
                            border.width: 1
                            border.color: Theme.hair
                            Image {
                                anchors.fill: parent; anchors.margins: 1
                                visible: !!ucard.modelData.image
                                source: ucard.modelData.image ? ((Qt.platform.os === "windows" ? "file:///" : "file://") + encodeURI(ucard.modelData.image)) : ""
                                fillMode: Image.PreserveAspectCrop
                                asynchronous: true
                                sourceSize.width: 240
                            }
                            Rectangle {
                                anchors.fill: parent; anchors.margins: 1
                                visible: !!ucard.modelData.image
                                color: ucard.modelData.bg || "#0e0e10"
                                opacity: (ucard.modelData.dim === undefined ? 55 : ucard.modelData.dim) / 100
                            }
                            Row {
                                x: 8; anchors.bottom: parent.bottom; anchors.bottomMargin: 8
                                spacing: 3
                                Repeater {
                                    model: [ucard.modelData.panel, ucard.modelData.text, ucard.modelData.primary, ucard.modelData.tertiary]
                                    Rectangle {
                                        required property var modelData
                                        width: 10; height: 10; radius: 5
                                        color: modelData || "#888888"
                                        border.width: 1; border.color: Qt.rgba(1, 1, 1, 0.25)
                                    }
                                }
                            }
                        }
                        Text {
                            id: uname
                            anchors.top: uthumb.bottom; anchors.topMargin: 6
                            width: parent.width
                            text: ucard.modelData.name || ""
                            color: ucard.on ? Theme.t1 : Theme.t3
                            font.pixelSize: 14
                            font.weight: ucard.on ? Font.DemiBold : Font.Medium
                            font.family: Theme.fontSans
                            elide: Text.ElideRight
                        }
                        MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: if (typeof themeBridge !== "undefined") themeBridge.selectProfile(ucard.index) }
                        Accessible.role: Accessible.RadioButton
                        Accessible.name: uname.text
                        Accessible.checked: ucard.on
                    }
                }
                // a new one
                Item {
                    width: grid.cellW
                    height: newBox.height + 6 + newName.implicitHeight
                    Rectangle {
                        id: newBox
                        width: parent.width
                        height: Math.round(width * 352 / 560)
                        radius: 7
                        color: newMa.containsMouse ? Theme.hover : "transparent"
                        border.width: 1.5
                        border.color: Theme.hair
                        Text { anchors.centerIn: parent; text: "+"; color: Theme.t3; font.pixelSize: 22; font.family: Theme.fontSans }
                    }
                    Text {
                        id: newName
                        anchors.top: newBox.bottom; anchors.topMargin: 6
                        text: (i18n.language, i18n.t("set_new_theme"))
                        color: Theme.t3; font.pixelSize: 14; font.weight: Font.Medium; font.family: Theme.fontSans
                    }
                    MouseArea { id: newMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: root.newTheme() }
                    Accessible.role: Accessible.Button
                    Accessible.name: newName.text
                    Accessible.onPressAction: root.newTheme()
                }
            }
        }
    }

    // ---------- Your image: the picture behind a built-in theme ----------
    Component {
        id: cUserImage
        RowLayout {
            spacing: Theme.sp2
            TFld {
                implicitWidth: 300; implicitHeight: 42; radius: Theme.rad(9); fontSize: 14
                text: typeof themeBridge !== "undefined" ? themeBridge.userImage : ""
                placeholder: (i18n.language, i18n.t("set_choose_image"))
                readonly: true
            }
            BtnFlat {
                visible: typeof themeBridge !== "undefined" && themeBridge.userImage !== ""
                text: "×"; lg: true
                onClicked: themeBridge.userImage = ""
            }
            BtnFlat { text: (i18n.language, i18n.t("ed_browse")); lg: true; onClicked: root.host.userImageRequested() }
        }
    }

    // ---------- Edit theme: a theme of the user's own, from scratch ----------
    Component {
        id: cEditor
        Column {
            id: ed
            width: root.host ? root.host.width : 760
            spacing: 22
            readonly property var t: root.active
            readonly property var fonts: ["IBM Plex Sans", "Barlow Condensed", "Newsreader", "Instrument Serif",
                                          "Shippori Mincho", "Space Grotesk", "IBM Plex Mono"]
            // name, base, corners
            Row {
                width: parent.width
                spacing: 12
                readonly property real unit: (width - 24) / 3.4
                Column {
                    width: parent.unit * 1.4
                    spacing: 6
                    Cap { text: (i18n.language, i18n.t("ed_name")) }
                    TFld {
                        width: parent.width; implicitHeight: 40; radius: Theme.rad(9); fontSize: 15
                        text: ed.t.name || ""
                        onEdited: function(v) { if (v.length > 0) root.setActive("name", v) }
                    }
                }
                Column {
                    width: parent.unit
                    spacing: 6
                    Cap { text: (i18n.language, i18n.t("ed_base")) }
                    Seg {
                        width: parent.width; stretch: true
                        options: [(i18n.language, i18n.t("ed_dark")), (i18n.language, i18n.t("ed_light"))]
                        current: ed.t.base === "light" ? 1 : 0
                        onPicked: function(i) { root.setActive("base", i === 1 ? "light" : "dark") }
                    }
                }
                Column {
                    width: parent.unit
                    spacing: 6
                    Cap { text: (i18n.language, i18n.t("ed_corners")) }
                    Seg {
                        readonly property var keys: ["square", "soft", "round"]
                        width: parent.width; stretch: true
                        options: [(i18n.language, i18n.t("ed_square")), (i18n.language, i18n.t("ed_soft")), (i18n.language, i18n.t("ed_round"))]
                        current: Math.max(0, keys.indexOf(ed.t.corners || "soft"))
                        onPicked: function(i) { root.setActive("corners", keys[i]) }
                    }
                }
            }
            // the title font, each name written in its own face
            Column {
                width: parent.width
                spacing: 8
                Cap { text: (i18n.language, i18n.t("ed_title_font")) }
                Flow {
                    width: parent.width
                    spacing: 6
                    Repeater {
                        model: ed.fonts
                        delegate: Rectangle {
                            id: fbtn
                            required property string modelData
                            readonly property bool on: (ed.t.font || "IBM Plex Sans") === modelData
                            width: fLbl.implicitWidth + 28; height: 38
                            radius: Theme.rad(8)
                            color: on ? Theme.hover2 : (fMa.containsMouse ? Theme.hover : Theme.field)
                            border.width: 1
                            border.color: on ? Theme.t4 : Theme.hair
                            Text {
                                id: fLbl
                                anchors.centerIn: parent
                                text: fbtn.modelData
                                color: fbtn.on ? Theme.t1 : Theme.t2
                                font.pixelSize: 16
                                font.family: fbtn.modelData
                                font.italic: fbtn.modelData === "Instrument Serif"
                            }
                            MouseArea { id: fMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: root.setActive("font", fbtn.modelData) }
                        }
                    }
                }
            }
            // the six colours, each with what it paints
            Column {
                width: parent.width
                spacing: 8
                Cap { text: (i18n.language, i18n.t("ed_colours")) }
                Grid {
                    width: parent.width
                    columns: 3
                    columnSpacing: 10
                    rowSpacing: 10
                    Repeater {
                        model: [
                            { k: "bg", l: "ed_bg", r: "ed_bg_role" },
                            { k: "panel", l: "ed_panel", r: "ed_panel_role" },
                            { k: "text", l: "ed_text", r: "ed_text_role" },
                            { k: "primary", l: "ed_primary", r: "ed_primary_role" },
                            { k: "secondary", l: "ed_secondary", r: "ed_secondary_role" },
                            { k: "tertiary", l: "ed_tertiary", r: "ed_tertiary_role" }
                        ]
                        delegate: Rectangle {
                            id: cf
                            required property var modelData
                            readonly property string val: ed.t[modelData.k] || "#000000"
                            width: (ed.width - 20) / 3
                            height: 52
                            radius: Theme.rad(9)
                            color: Theme.field
                            border.width: 1
                            border.color: cfMa.containsMouse ? Theme.t4 : Theme.hair
                            Rectangle {
                                id: sw
                                x: 12; anchors.verticalCenter: parent.verticalCenter
                                width: 26; height: 26; radius: Theme.rad(7)
                                color: cf.val
                                border.width: 1; border.color: Qt.rgba(1, 1, 1, 0.18)
                            }
                            Column {
                                anchors.left: sw.right; anchors.leftMargin: 12
                                anchors.right: hex.left; anchors.rightMargin: 8
                                anchors.verticalCenter: parent.verticalCenter
                                spacing: 1
                                Text { width: parent.width; text: (i18n.language, i18n.t(cf.modelData.l)); color: Theme.t1; font.pixelSize: 14; font.weight: Font.Medium; font.family: Theme.fontSans; elide: Text.ElideRight }
                                Text { width: parent.width; text: (i18n.language, i18n.t(cf.modelData.r)); color: Theme.t4; font.pixelSize: 12; font.family: Theme.fontSans; elide: Text.ElideRight }
                            }
                            Text {
                                id: hex
                                anchors.right: parent.right; anchors.rightMargin: 12
                                anchors.verticalCenter: parent.verticalCenter
                                text: cf.val
                                color: Theme.t3; font.pixelSize: 12; font.family: Theme.fontMono
                            }
                            MouseArea { id: cfMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: root.host.colorPickRequested(cf.modelData.k, cf.val) }
                        }
                    }
                }
                // no auto-fixing, but a word when the text will not read
                Row {
                    readonly property real ratio: {
                        function lum(h) {
                            var c = Qt.color(h)
                            function l(v) { return v <= 0.03928 ? v / 12.92 : Math.pow((v + 0.055) / 1.055, 2.4) }
                            return 0.2126 * l(c.r) + 0.7152 * l(c.g) + 0.0722 * l(c.b)
                        }
                        var a = lum(ed.t.text || "#ffffff"), b = lum(ed.t.bg || "#000000")
                        return (Math.max(a, b) + 0.05) / (Math.min(a, b) + 0.05)
                    }
                    visible: ratio < 4.5
                    spacing: 8
                    IconImg { anchors.verticalCenter: parent.verticalCenter; src: "qrc:/icons/triangle-alert.svg"; tint: Theme.warn; s: 15 }
                    Text { anchors.verticalCenter: parent.verticalCenter; text: (i18n.language, i18n.t("ed_contrast_warn")); color: Theme.t2; font.pixelSize: 13; font.family: Theme.fontSans }
                }
            }
            // a picture behind the whole window
            Rectangle {
                width: parent.width
                implicitHeight: wbCol.implicitHeight + 32
                height: implicitHeight
                radius: Theme.rad(12)
                color: Theme.elev
                border.width: 1
                border.color: Theme.hairSoft
                readonly property bool hasImg: !!ed.t.image
                Column {
                    id: wbCol
                    x: 16; y: 16
                    width: parent.width - 32
                    spacing: 12
                    RowLayout {
                        width: parent.width
                        spacing: 12
                        Text { text: (i18n.language, i18n.t("ed_window_bg")); color: Theme.t1; font.pixelSize: 15; font.weight: Font.Medium; font.family: Theme.fontSans }
                        Text {
                            Layout.fillWidth: true
                            text: ed.t.image ? String(ed.t.image).split(/[\\/]/).pop() : (i18n.language, i18n.t("ed_no_image"))
                            color: ed.t.image ? Theme.t2 : Theme.t4
                            font.pixelSize: 14; font.family: Theme.fontSans
                            elide: Text.ElideMiddle
                        }
                        BtnFlat { visible: !!ed.t.image; text: (i18n.language, i18n.t("ed_remove")); onClicked: root.setActive("image", "") }
                        BtnFlat { text: (i18n.language, i18n.t("ed_browse")); onClicked: root.host.bgImageRequested() }
                    }
                    Column {
                        width: parent.width
                        spacing: 12
                        opacity: parent.parent.hasImg ? 1 : 0.4
                        enabled: parent.parent.hasImg
                        Row {
                            spacing: 14
                            Text { anchors.verticalCenter: parent.verticalCenter; width: 84; text: (i18n.language, i18n.t("ed_fit")); color: Theme.t3; font.pixelSize: 14; font.family: Theme.fontSans }
                            Seg {
                                readonly property var keys: ["fill", "fit", "stretch", "center", "tile"]
                                options: [(i18n.language, i18n.t("ed_fit_fill")), (i18n.language, i18n.t("ed_fit_fit")), (i18n.language, i18n.t("ed_fit_stretch")),
                                          (i18n.language, i18n.t("ed_fit_center")), (i18n.language, i18n.t("ed_fit_tile"))]
                                current: Math.max(0, keys.indexOf(ed.t.fit || "fill"))
                                onPicked: function(i) { root.setActive("fit", keys[i]) }
                            }
                        }
                        Slide { width: parent.width; label: (i18n.language, i18n.t("ed_size")); from: 50; to: 200; unit: "%"; value: ed.t.zoom || 100; onMoved: function(v) { root.setActive("zoom", v) } }
                        Slide { width: parent.width; label: (i18n.language, i18n.t("ed_dim")); from: 0; to: 90; unit: "%"; value: ed.t.dim === undefined ? 55 : ed.t.dim; onMoved: function(v) { root.setActive("dim", v) } }
                        Slide { width: parent.width; label: (i18n.language, i18n.t("ed_blur")); from: 0; to: 20; unit: " px"; value: ed.t.blur || 0; onMoved: function(v) { root.setActive("blur", v) } }
                    }
                }
            }
            // duplicate, delete
            Row {
                anchors.right: parent.right
                spacing: 8
                BtnFlat {
                    text: (i18n.language, i18n.t("ed_duplicate"))
                    onClicked: if (typeof themeBridge !== "undefined") themeBridge.selectProfile(themeBridge.duplicateProfile(root.activeIdx))
                }
                BtnFlat {
                    text: (i18n.language, i18n.t("ed_delete"))
                    danger: true
                    onClicked: if (typeof themeBridge !== "undefined") themeBridge.removeProfile(root.activeIdx)
                }
            }
        }
    }
}
