// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// The first-run setup, asked as four short steps instead of one long form:
// language, theme, library view and layout. Every answer is written immediately, so the app and
// this very dialog re-dress themselves while the user is still deciding — the
// preview IS the product, which no swatch or caption can match.
import QtQuick
import QtQuick.Layouts
import "../theme"
import "../widgets"
import "../views"

GridLayout {
    id: steps
    property int step: 0
    property var uiPalette: Theme
    readonly property int lastStep: 4
    readonly property bool narrow: width < 760

    Layout.fillWidth: true
    Layout.preferredHeight: narrow ? controls.implicitHeight + previewPane.implicitHeight + Theme.sp4
                                   : Math.max(controls.implicitHeight, previewPane.implicitHeight)
    columns: narrow ? 1 : 2
    columnSpacing: Theme.sp5
    rowSpacing: Theme.sp4

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

    ColumnLayout {
        id: controls
        Layout.fillWidth: steps.narrow
        Layout.preferredWidth: steps.narrow ? -1 : 316
        Layout.alignment: Qt.AlignTop
        spacing: Theme.sp3

        Text {
            Layout.fillWidth: true
            text: (i18n.language, i18n.t(steps.step === 0 ? "welcome_heading"
                                       : steps.step === 1 ? "welcome_theme_title"
                                       : steps.step === 2 ? "welcome_view_title"
                                       : steps.step === 3 ? "welcome_nav_title"
                                       : "welcome_detail_title"))
            color: steps.uiPalette.t1
            font.pixelSize: 22; font.weight: Font.Bold; font.family: steps.uiPalette.fontSans
            wrapMode: Text.WordWrap
        }
        Text {
            Layout.fillWidth: true
            text: (i18n.language, i18n.t(steps.step === 0 ? "welcome_blurb2"
                                       : steps.step === 1 ? "welcome_theme_note"
                                       : steps.step === 2 ? "welcome_view_note"
                                       : steps.step === 3 ? "welcome_nav_note"
                                       : "welcome_detail_note"))
            color: steps.uiPalette.t2
            font.pixelSize: 13; font.family: steps.uiPalette.fontSans
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
        color: steps.uiPalette.field
        border.color: steps.uiPalette.hair; border.width: 1

        ColumnLayout {
            id: langCol
            anchors.fill: parent
            anchors.margins: Theme.sp3
            spacing: 10

            Text {
                text: (i18n.language, i18n.t("welcome_lang_title"))
                color: steps.uiPalette.accent; font.pixelSize: 10; font.weight: Font.Bold
                font.letterSpacing: 1.2; font.family: steps.uiPalette.fontSans
            }
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 14
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 5
                    Text {
                        text: (i18n.language, i18n.t("set_language2"))
                        color: steps.uiPalette.t3; font.pixelSize: 11; font.family: steps.uiPalette.fontSans
                    }
                    TSelect {
                        Layout.fillWidth: true
                        uiPalette: steps.uiPalette
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
                        color: steps.uiPalette.t3; font.pixelSize: 11; font.family: steps.uiPalette.fontSans
                    }
                    TSelect {
                        Layout.fillWidth: true
                        uiPalette: steps.uiPalette
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
                    Text {
                        Layout.fillWidth: true
                        text: (i18n.language, i18n.t("set_content_language_note"))
                        color: steps.uiPalette.t4
                        font.pixelSize: 11
                        font.family: steps.uiPalette.fontSans
                        wrapMode: Text.WordWrap
                        lineHeight: 1.35
                    }
                }
            }
        }
    }

    // ---- step 1: theme ----
    WelcomeThemePicker {
        visible: steps.step === 1
        Layout.fillWidth: true; Layout.topMargin: Theme.sp1
        uiPalette: steps.uiPalette
    }

    // ---- step 2: how the library reads ----
    ColumnLayout {
        visible: steps.step === 2
        Layout.fillWidth: true; Layout.topMargin: Theme.sp1
        spacing: Theme.sp3

        Text {
            text: (i18n.language, i18n.t("welcome_view_q"))
            color: steps.uiPalette.accent; font.pixelSize: 10; font.weight: Font.Bold
            font.letterSpacing: 1.2; font.family: steps.uiPalette.fontSans
        }
        RowLayout {
            Layout.fillWidth: true
            spacing: 14
            LayoutChoiceTile {
                uiPalette: steps.uiPalette
                asking: "view"
                classic: false
                navLeft: steps.navLeft
                detailBottom: steps.detailBottom
                selected: !steps.classicView
                label: (i18n.language, i18n.t("welcome_view_grid"))
                onPicked: if (typeof settings !== "undefined") settings.set("classicMode", false)
            }
            LayoutChoiceTile {
                uiPalette: steps.uiPalette
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

    // ---- step 3: navigation ----
    ColumnLayout {
        visible: steps.step === 3
        Layout.fillWidth: true; Layout.topMargin: Theme.sp1
        spacing: Theme.sp3

        RowLayout {
            Layout.fillWidth: true
            spacing: 14
            LayoutChoiceTile {
                uiPalette: steps.uiPalette
                asking: "nav"
                navLeft: false
                detailBottom: steps.detailBottom
                selected: !steps.navLeft
                label: (i18n.language, i18n.t("welcome_nav_top"))
                onPicked: if (typeof settings !== "undefined") settings.set("layoutClassic", false)
            }
            LayoutChoiceTile {
                uiPalette: steps.uiPalette
                asking: "nav"
                navLeft: true
                detailBottom: steps.detailBottom
                selected: steps.navLeft
                label: (i18n.language, i18n.t("welcome_nav_left"))
                onPicked: if (typeof settings !== "undefined") settings.set("layoutClassic", true)
            }
            Item { Layout.fillWidth: true }
        }
    }

    // ---- step 4: detail panel ----
    ColumnLayout {
        visible: steps.step === 4
        Layout.fillWidth: true; Layout.topMargin: Theme.sp1
        spacing: Theme.sp3

        RowLayout {
            Layout.fillWidth: true
            spacing: 14
            LayoutChoiceTile {
                uiPalette: steps.uiPalette
                asking: "detail"
                navLeft: steps.navLeft
                detailBottom: false
                selected: !steps.detailBottom
                label: (i18n.language, i18n.t("welcome_detail_right"))
                onPicked: if (typeof settings !== "undefined") settings.set("detailBottom", false)
            }
            LayoutChoiceTile {
                uiPalette: steps.uiPalette
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
            radius: 8; color: steps.uiPalette.field
            IconImg { anchors.centerIn: parent; src: "qrc:/icons/discover.svg"; tint: steps.uiPalette.accentText; s: 15 }
        }
        Text {
            Layout.fillWidth: true; Layout.alignment: Qt.AlignVCenter
            text: (i18n.language, i18n.t("welcome_tour_hint"))
            color: steps.uiPalette.t3; font.pixelSize: 12; font.family: steps.uiPalette.fontSans
            wrapMode: Text.WordWrap; lineHeight: 1.35
        }
    }

    }

    OnboardingPreviewPane {
        id: previewPane
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.alignment: Qt.AlignTop
        classic: steps.classicView
        navLeft: steps.navLeft
        detailBottom: steps.detailBottom
        step: steps.step
        narrow: steps.narrow
        uiPalette: steps.uiPalette
    }
}
