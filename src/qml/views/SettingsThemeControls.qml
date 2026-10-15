// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Theme / custom-theme SettingsRow control Components. Host is SettingsRow
// (rename/color/bg signals); field + sw are host props: never `field: field`.
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
    property Component anime: cAnime
    property Component theme: cTheme
    property Component appicon: cAppIcon
    property Component profiles: cProfiles
    property Component color: cColor
    property Component bgimage: cBgImage
    property Component slider: cSlider

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
        id: cAnime
        TToggle {
            big: true
            on: Theme.anime
            onToggled: function(v) { Theme.setAnime(v) }
        }
    }
    // The theme picker: a small picture of the app in each theme, under the
    // row's label and across its whole width, so the choice is seen rather
    // than read out of a dropdown.
    Component {
        id: cTheme
        Flow {
            id: grid
            readonly property var names: ["dark", "light", "midnight", "sakura", "darkstar", "matrix", "custom"]
            readonly property int cols: width >= 760 ? 4 : (width >= 520 ? 3 : 2)
            readonly property real cardW: (width - spacing * (cols - 1)) / cols
            width: root.host ? root.host.width - 6 : 600
            spacing: 14
            Repeater {
                model: grid.names
                delegate: Item {
                    id: card
                    required property string modelData
                    required property int index
                    readonly property bool custom: modelData === "custom"
                    readonly property var sw: custom ? null : Theme.swatches[index]
                    readonly property bool on: Theme.name === modelData
                    // custom shows the user's own colours
                    readonly property color cBg:    custom ? Theme.customBgColor : sw.bg
                    readonly property color cPanel: custom ? Theme.customPanelColor : sw.panel
                    readonly property color cNav:   custom ? Qt.darker(Theme.customPanelColor, 1.45) : sw.nav
                    readonly property color cAcc:   custom ? Theme.customPrimaryColor : sw.accent
                    readonly property color cT1:    custom ? Theme.customTextColor : sw.t1
                    readonly property color cT3:    custom ? Qt.rgba(Theme.customTextColor.r, Theme.customTextColor.g, Theme.customTextColor.b, 0.55) : sw.t3
                    readonly property color cDone:  custom ? Qt.darker(Theme.customTertiaryColor, 1.7) : sw.done
                    readonly property color cHair:  custom ? Qt.rgba(Theme.customTextColor.r, Theme.customTextColor.g, Theme.customTextColor.b, 0.14) : sw.hair
                    width: grid.cardW
                    height: thumb.height + 30

                    Rectangle {
                        id: ring
                        x: -3; y: -3
                        width: thumb.width + 6; height: thumb.height + 6
                        radius: 13
                        color: "transparent"
                        border.width: 2
                        border.color: card.on ? Theme.accent : (cardMa.containsMouse ? Theme.hair : "transparent")
                        Behavior on border.color { ColorAnimation { duration: Theme.durFast } }
                    }
                    // the picture is cut to the card's rounded corners (clip alone
                    // keeps square ones, and the side panel showed past them)
                    Rectangle {
                        id: thumbMask
                        anchors.fill: thumb
                        radius: 10
                        visible: false
                        layer.enabled: true
                    }
                    Rectangle {
                        id: thumb
                        width: parent.width
                        height: Math.round(width * 0.58)
                        color: card.cBg
                        layer.enabled: true
                        layer.effect: MultiEffect {
                            maskEnabled: true
                            maskSource: thumbMask
                            maskThresholdMin: 0.5
                            maskSpreadAtMin: 1.0
                        }
                        // top bar: logo dot, the active tab, the add button
                        Rectangle {
                            width: parent.width; height: Math.round(parent.height * 0.17)
                            color: card.cNav
                            Rectangle { x: 8; anchors.verticalCenter: parent.verticalCenter; width: 8; height: 8; radius: 4; color: card.cT1; opacity: 0.85 }
                            Rectangle { x: 22; anchors.verticalCenter: parent.verticalCenter; width: parent.width * 0.16; height: 4; radius: 2; color: card.cT1; opacity: 0.8 }
                            Rectangle { x: 22; anchors.bottom: parent.bottom; width: parent.width * 0.16; height: 2; color: card.cAcc }
                        }
                        // side panel
                        Rectangle {
                            anchors.right: parent.right
                            y: Math.round(parent.height * 0.17)
                            width: Math.round(parent.width * 0.3); height: parent.height - y
                            color: card.cPanel
                            Rectangle { x: 8; y: 10; width: parent.width - 16; height: 4; radius: 2; color: card.cT1; opacity: 0.85 }
                            Rectangle { x: 8; y: 20; width: (parent.width - 16) * 0.6; height: 3; radius: 1.5; color: card.cT3 }
                            Rectangle { x: 8; y: 30; width: parent.width - 16; height: 3; radius: 1.5; color: card.cDone }
                        }
                        // the page: an accent button and three covers
                        Rectangle {
                            x: 8; y: Math.round(parent.height * 0.17) + 8
                            width: 22; height: 9; radius: 3
                            color: card.cAcc
                        }
                        Row {
                            x: 8; y: Math.round(parent.height * 0.17) + 24
                            spacing: 5
                            Repeater {
                                model: 3
                                Column {
                                    required property int index
                                    spacing: 3
                                    readonly property real w: (thumb.width * 0.7 - 16 - 10) / 3
                                    Rectangle { width: parent.w; height: parent.w * 1.25; radius: 3; color: Qt.tint(card.cPanel, Qt.rgba(card.cT1.r, card.cT1.g, card.cT1.b, 0.06 + index * 0.04)) }
                                    Rectangle { width: parent.w * 0.8; height: 3; radius: 1.5; color: card.cT1; opacity: 0.8 }
                                    Rectangle { width: parent.w * 0.5; height: 3; radius: 1.5; color: card.cT3 }
                                }
                            }
                        }
                        // custom: the user's own colours, marked as such
                        Rectangle {
                            visible: card.custom
                            x: Math.round(thumb.width * 0.7) - width - 6
                            y: Math.round(thumb.height * 0.17) + 4
                            width: 20; height: 20; radius: 10
                            color: card.cNav
                            IconImg { anchors.centerIn: parent; src: "qrc:/icons/palette.svg"; s: 14; tint: card.cT1 }
                        }
                        // the active one is marked on the picture, not only by the ring
                        Rectangle {
                            visible: card.on
                            anchors.right: parent.right; anchors.top: parent.top
                            anchors.margins: 6
                            width: 18; height: 18; radius: 9
                            color: Theme.accent
                            IconImg { anchors.centerIn: parent; src: "qrc:/icons/check.svg"; s: 11; tint: Theme.inkOnAccent }
                        }
                    }
                    Rectangle {
                        anchors.fill: thumb
                        radius: 10
                        color: "transparent"
                        border.width: 1
                        border.color: card.cHair
                    }
                    Text {
                        anchors.top: thumb.bottom; anchors.topMargin: 9
                        anchors.left: parent.left; anchors.right: parent.right
                        text: (root.field.options || [])[card.index] || card.modelData
                        color: card.on ? Theme.t1 : Theme.t2
                        font.pixelSize: 14
                        font.weight: card.on ? Font.DemiBold : Font.Medium
                        font.family: Theme.fontSans
                        elide: Text.ElideRight
                    }
                    MouseArea {
                        id: cardMa
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: Theme.setName(card.modelData)
                    }
                    Accessible.role: Accessible.RadioButton
                    Accessible.name: (root.field.options || [])[card.index] || card.modelData
                    Accessible.checked: card.on
                    Accessible.onPressAction: Theme.setName(card.modelData)
                }
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
                    width: 46; height: 46; radius: 12
                    color: tile.sel ? Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.12) : "transparent"
                    border.width: 2
                    border.color: tile.sel ? Theme.accent : Theme.hair
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
    Component {
        id: cProfiles
        RowLayout {
            spacing: Theme.sp2
            TSelect {
                implicitWidth: 180
                implicitHeight: 42
                radius: 9
                font.pixelSize: 15
                model: {
                    var names = []
                    if (typeof themeBridge !== "undefined")
                        for (var i = 0; i < themeBridge.customProfiles.length; i++)
                            names.push(themeBridge.customProfiles[i].name)
                    return names
                }
                currentIndex: themeBridge ? themeBridge.activeProfile : 0
                onActivated: function(i) { if (themeBridge) themeBridge.activeProfile = i }
            }
            BtnFlat { text: "+"; lg: true; onClicked: if (themeBridge) themeBridge.activeProfile = themeBridge.addProfile() }
            BtnFlat {
                text: (i18n.language, i18n.t("set_custom_rename")); lg: true
                onClicked: root.host.renameProfileRequested()
            }
            BtnFlat {
                text: (i18n.language, i18n.t("set_custom_clear")); lg: true
                enabled: themeBridge && themeBridge.customProfiles.length > 1
                onClicked: if (themeBridge) themeBridge.removeProfile(themeBridge.activeProfile)
            }
        }
    }
    Component {
        id: cColor
        RowLayout {
            spacing: Theme.sp2
            readonly property string cur: (root.sw.ap && root.sw.ap[root.field.role]) ? root.sw.ap[root.field.role] : "#000000"
            Rectangle {
                implicitWidth: 32; implicitHeight: 32; radius: 8
                color: parent.cur
                border.color: Theme.hair; border.width: 1
            }
            TFld {
                implicitWidth: 130; implicitHeight: 42; radius: 9; fontSize: 14; mono: true
                text: parent.cur
                placeholder: "#rrggbb"
                onEdited: function(t) {
                    var v = t.charAt(0) === "#" ? t : "#" + t
                    if (/^#[0-9a-fA-F]{6}$/.test(v) && themeBridge)
                        themeBridge.setProfileColor(themeBridge.activeProfile, root.field.role, v.toLowerCase())
                }
            }
            BtnFlat {
                text: "…"; lg: true
                onClicked: root.host.colorPickRequested(root.field.role, parent.cur)
            }
        }
    }
    Component {
        id: cBgImage
        RowLayout {
            spacing: Theme.sp2
            TFld {
                implicitWidth: 240; implicitHeight: 42; radius: 9; fontSize: 14; mono: true
                text: (root.sw.ap && root.sw.ap.image) ? root.sw.ap.image : ""
                placeholder: (i18n.language, i18n.t("set_custom_bgimage"))
                readonly: true
            }
            BtnFlat { text: (i18n.language, i18n.t("settings_browse")); lg: true; onClicked: root.host.bgImageRequested() }
            BtnFlat { text: (i18n.language, i18n.t("set_custom_clear")); lg: true; onClicked: if (themeBridge) themeBridge.setProfileImage(themeBridge.activeProfile, "") }
        }
    }
    Component {
        id: cSlider
        RowLayout {
            spacing: Theme.sp2
            Slider {
                id: opacitySlider
                implicitWidth: 200
                from: 0; to: 100; stepSize: 1
                onMoved: if (themeBridge) themeBridge.setProfileOpacity(themeBridge.activeProfile, Math.round(value))
                // Binding (not a plain `value:`) so dragging doesn't break the link.
                Binding { target: opacitySlider; property: "value"; value: (root.sw.ap && root.sw.ap.opacity !== undefined) ? root.sw.ap.opacity : 55 }
                background: Rectangle {
                    x: parent.leftPadding; y: parent.topPadding + parent.availableHeight / 2 - height / 2
                    width: parent.availableWidth; height: 4; radius: 2; color: Theme.track
                    Rectangle { width: parent.width * parent.parent.visualPosition; height: parent.height; radius: 2; color: Theme.accent }
                }
                handle: Rectangle {
                    x: parent.leftPadding + parent.visualPosition * (parent.availableWidth - width)
                    y: parent.topPadding + parent.availableHeight / 2 - height / 2
                    implicitWidth: 20; implicitHeight: 20; radius: 10
                    color: Theme.accent; border.color: Theme.bg; border.width: 2
                }
            }
            Text {
                text: ((root.sw.ap && root.sw.ap.opacity !== undefined) ? root.sw.ap.opacity : 55) + "%"
                color: Theme.t3; font.pixelSize: 14; font.family: Theme.fontSans; font.features: Theme.tnum
                Layout.preferredWidth: 44
            }
        }
    }
}
