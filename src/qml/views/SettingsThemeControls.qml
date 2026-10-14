// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Theme / custom-theme SettingsRow control Components. Host is SettingsRow
// (rename/color/bg signals); field + sw are host props: never `field: field`.
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
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
    Component {
        id: cTheme
        TSelect {
            readonly property var names: ["dark", "light", "midnight", "sakura", "darkstar", "matrix", "custom"]
            implicitWidth: 220
            implicitHeight: 42
            radius: 9
            font.pixelSize: 15
            model: root.field.options || []
            currentIndex: Math.max(0, names.indexOf(Theme.name))
            onActivated: function(i) { Theme.setName(names[i]) }
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
