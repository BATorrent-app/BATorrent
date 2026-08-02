// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// The first-run setup, asked as three short steps instead of one long form:
// language, theme, layout. Every answer is written immediately, so the app and
// this very dialog re-dress themselves while the user is still deciding — the
// preview IS the product, which no swatch or caption can match.
import QtQuick
import QtQuick.Layouts
import "../theme"
import "../widgets"
import "../views"

ColumnLayout {
    id: steps
    property int step: 0
    readonly property int lastStep: 3

    Layout.fillWidth: true
    spacing: Theme.sp3

    SettingsSchema { id: schema }

    // Mirrors of the two layout settings. A function call inside a binding would
    // not re-run when the setting changes, and these tiles have to restate the
    // choice the moment it is made — the same reason Main.qml keeps its own
    // copies and refreshes them on settings.changed.
    property bool navLeft: false
    property bool detailBottom: false
    property bool classicView: false
    function boolPref(key) {
        if (typeof settings === "undefined") return false
        var v = settings.get(key)
        return v === true || v === 1 || v === "1" || v === "true"
    }
    function refreshLayout() {
        navLeft = boolPref("layoutClassic")
        detailBottom = boolPref("detailBottom")
        classicView = boolPref("classicMode")
    }
    Component.onCompleted: refreshLayout()
    Connections {
        target: typeof settings !== "undefined" ? settings : null
        function onChanged() { steps.refreshLayout() }
    }

    Text {
        Layout.fillWidth: true
        text: (i18n.language, i18n.t(steps.step === 0 ? "welcome_heading"
                                   : steps.step === 1 ? "welcome_theme_title"
                                   : steps.step === 2 ? "welcome_view_title"
                                   : "welcome_layout_title"))
        color: Theme.t1
        font.pixelSize: 20; font.weight: Font.Bold; font.family: Theme.fontSans
        wrapMode: Text.WordWrap
    }
    Text {
        Layout.fillWidth: true
        text: (i18n.language, i18n.t(steps.step === 0 ? "welcome_blurb2"
                                   : steps.step === 1 ? "welcome_theme_note"
                                   : steps.step === 2 ? "welcome_view_note"
                                   : "welcome_layout_note"))
        color: Theme.t2
        font.pixelSize: 13; font.family: Theme.fontSans
        wrapMode: Text.WordWrap; lineHeight: 1.45
    }

    // ---- step 0: language ----
    // Interface and content are two questions on purpose: reading English menus
    // while wanting Portuguese audio is a normal combination, and getting it
    // wrong is why a new user leaves without finding a dub.
    Rectangle {
        visible: steps.step === 0
        Layout.fillWidth: true; Layout.topMargin: Theme.sp1
        implicitHeight: langCol.implicitHeight + 2 * Theme.sp3
        radius: 12
        color: Theme.field
        border.color: Theme.hair; border.width: 1

        ColumnLayout {
            id: langCol
            anchors.fill: parent
            anchors.margins: Theme.sp3
            spacing: 10

            Text {
                text: (i18n.language, i18n.t("welcome_lang_title"))
                color: Theme.accent; font.pixelSize: 10; font.weight: Font.Bold
                font.letterSpacing: 1.2; font.family: Theme.fontSans
            }
            RowLayout {
                Layout.fillWidth: true
                spacing: 12
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 5
                    Text {
                        text: (i18n.language, i18n.t("set_language2"))
                        color: Theme.t3; font.pixelSize: 11; font.family: Theme.fontSans
                    }
                    TSelect {
                        Layout.fillWidth: true
                        model: schema.languageNames
                        icons: schema.languageFlags
                        currentIndex: i18n.language
                        onActivated: function (i) { i18n.setLanguage(i) }
                    }
                }
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 5
                    Text {
                        text: (i18n.language, i18n.t("set_content_language"))
                        color: Theme.t3; font.pixelSize: 11; font.family: Theme.fontSans
                    }
                    TSelect {
                        Layout.fillWidth: true
                        model: [(i18n.language, i18n.t("set_content_language_same"))].concat(schema.languageNames)
                        icons: [""].concat(schema.languageFlags)
                        currentIndex: {
                            if (typeof settings === "undefined") return 0
                            var cv = settings.get("contentLanguage")
                            return (cv === undefined || cv === null || cv === "" || cv < 0) ? 0 : cv + 1
                        }
                        onActivated: function (i) {
                            if (typeof settings !== "undefined") settings.set("contentLanguage", i - 1)
                        }
                    }
                }
            }
        }
    }

    // ---- step 1: theme ----
    Grid {
        visible: steps.step === 1
        Layout.fillWidth: true; Layout.topMargin: Theme.sp1
        columns: 3
        spacing: 10
        Repeater {
            model: Theme.swatches
            delegate: Rectangle {
                id: sw
                required property var modelData
                readonly property bool sel: Theme.name === sw.modelData.key
                width: 154; height: 62
                radius: 10
                color: sw.modelData.bg
                border.width: sw.sel ? 2 : 1
                border.color: sw.sel ? Theme.accent : (swMa.containsMouse ? Theme.t4 : Theme.hair)
                Behavior on border.color { ColorAnimation { duration: 140 } }

                Rectangle {
                    x: 10; y: 10; width: 44; height: 42; radius: 6
                    color: sw.modelData.panel
                    Rectangle { x: 7; y: 8; width: 22; height: 4; radius: 2; color: sw.modelData.accent }
                    Rectangle { x: 7; y: 18; width: 30; height: 3; radius: 1.5
                                color: sw.modelData.accent; opacity: 0.35 }
                }
                Text {
                    x: 64; anchors.verticalCenter: parent.verticalCenter
                    text: sw.modelData.key === "dark" ? (i18n.language, i18n.t("set_theme_dark"))
                        : sw.modelData.key === "light" ? (i18n.language, i18n.t("set_theme_light"))
                        : sw.modelData.key === "midnight" ? "Midnight"
                        : sw.modelData.key === "sakura" ? "Sakura"
                        : sw.modelData.key === "darkstar" ? "Dark Star" : "Matrix"
                    // Reads on the swatch's own background, not the app's — this
                    // tile is a window into another theme.
                    color: sw.modelData.key === "light" || sw.modelData.key === "sakura"
                           ? "#16171a" : "#f3f3f4"
                    font.pixelSize: 13
                    font.weight: sw.sel ? Font.DemiBold : Font.Normal
                    font.family: Theme.fontSans
                }
                MouseArea {
                    id: swMa
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: Theme.setName(sw.modelData.key)
                }
            }
        }
    }

    // ---- step 2: how the library reads ----
    ColumnLayout {
        visible: steps.step === 2
        Layout.fillWidth: true; Layout.topMargin: Theme.sp1
        spacing: Theme.sp3

        Text {
            text: (i18n.language, i18n.t("welcome_view_q"))
            color: Theme.accent; font.pixelSize: 10; font.weight: Font.Bold
            font.letterSpacing: 1.2; font.family: Theme.fontSans
        }
        RowLayout {
            Layout.fillWidth: true
            spacing: 14
            LayoutChoiceTile {
                asking: "view"
                classic: false
                navLeft: steps.navLeft
                detailBottom: steps.detailBottom
                selected: !steps.classicView
                label: (i18n.language, i18n.t("welcome_view_grid"))
                onPicked: if (typeof settings !== "undefined") settings.set("classicMode", false)
            }
            LayoutChoiceTile {
                asking: "view"
                classic: true
                navLeft: steps.navLeft
                detailBottom: steps.detailBottom
                selected: steps.classicView
                label: (i18n.language, i18n.t("welcome_view_classic"))
                onPicked: if (typeof settings !== "undefined") settings.set("classicMode", true)
            }
            Item { Layout.fillWidth: true }
        }
    }

    // ---- step 3: layout ----
    ColumnLayout {
        visible: steps.step === 3
        Layout.fillWidth: true; Layout.topMargin: Theme.sp1
        spacing: Theme.sp3

        Text {
            text: (i18n.language, i18n.t("welcome_nav_q"))
            color: Theme.accent; font.pixelSize: 10; font.weight: Font.Bold
            font.letterSpacing: 1.2; font.family: Theme.fontSans
        }
        RowLayout {
            Layout.fillWidth: true
            spacing: 14
            LayoutChoiceTile {
                asking: "nav"
                navLeft: false
                detailBottom: steps.detailBottom
                selected: !steps.navLeft
                label: (i18n.language, i18n.t("welcome_nav_top"))
                onPicked: if (typeof settings !== "undefined") settings.set("layoutClassic", false)
            }
            LayoutChoiceTile {
                asking: "nav"
                navLeft: true
                detailBottom: steps.detailBottom
                selected: steps.navLeft
                label: (i18n.language, i18n.t("welcome_nav_left"))
                onPicked: if (typeof settings !== "undefined") settings.set("layoutClassic", true)
            }
            Item { Layout.fillWidth: true }
        }

        Text {
            Layout.topMargin: Theme.sp2
            text: (i18n.language, i18n.t("welcome_detail_q"))
            color: Theme.accent; font.pixelSize: 10; font.weight: Font.Bold
            font.letterSpacing: 1.2; font.family: Theme.fontSans
        }
        RowLayout {
            Layout.fillWidth: true
            spacing: 14
            LayoutChoiceTile {
                asking: "detail"
                navLeft: steps.navLeft
                detailBottom: false
                selected: !steps.detailBottom
                label: (i18n.language, i18n.t("welcome_detail_right"))
                onPicked: if (typeof settings !== "undefined") settings.set("detailBottom", false)
            }
            LayoutChoiceTile {
                asking: "detail"
                navLeft: steps.navLeft
                detailBottom: true
                selected: steps.detailBottom
                label: (i18n.language, i18n.t("welcome_detail_bottom"))
                onPicked: if (typeof settings !== "undefined") settings.set("detailBottom", true)
            }
            Item { Layout.fillWidth: true }
        }
    }

    // the tour promise, kept on the last step so it lands right before Start
    RowLayout {
        visible: steps.step === steps.lastStep
        Layout.fillWidth: true; Layout.topMargin: Theme.sp2
        spacing: 10
        Rectangle {
            Layout.alignment: Qt.AlignTop
            Layout.preferredWidth: 28; Layout.preferredHeight: 28
            radius: 8; color: Theme.field
            IconImg { anchors.centerIn: parent; src: "qrc:/icons/discover.svg"; tint: Theme.accentText; s: 15 }
        }
        Text {
            Layout.fillWidth: true; Layout.alignment: Qt.AlignVCenter
            text: (i18n.language, i18n.t("welcome_tour_hint"))
            color: Theme.t3; font.pixelSize: 12; font.family: Theme.fontSans
            wrapMode: Text.WordWrap; lineHeight: 1.35
        }
    }
}
