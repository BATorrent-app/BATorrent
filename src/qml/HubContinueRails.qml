// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Continue Watching + Continue Playing hero rails at the top of Hub.
import QtQuick
import QtQuick.Layouts
import QtQuick.Effects
import "theme"
import "widgets"

RowLayout {
    id: root
    required property var page

    Layout.fillWidth: true
    Layout.leftMargin: Theme.sp5
    Layout.rightMargin: Theme.sp5
    visible: !page.empty
    spacing: 20

    component RailPlaceholder: Rectangle {
        property alias text: ph.text
        Layout.fillWidth: true
        Layout.preferredHeight: Math.round(page.railCardW * 1.5)
        radius: 10
        color: "transparent"
        border.color: Theme.hairSoft
        border.width: 1
        Text {
            id: ph
            anchors.centerIn: parent
            width: parent.width - 32
            color: Theme.t4
            font.pixelSize: 12
            font.family: Theme.fontSans
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
        }
    }

    // ---------- CONTINUE WATCHING ----------
    Rectangle {
        id: cwHero
        Layout.fillWidth: true
        Layout.preferredHeight: 224
        radius: 16
        color: Theme.elev
        border.color: Theme.hair
        border.width: 1
        clip: true
        readonly property var it: page.continueItems.length > 0 ? page.continueItems[0] : null

        Image {
            id: cwBg
            anchors.fill: parent
            visible: false
            asynchronous: true
            cache: true
            source: cwHero.it ? (cwHero.it.poster || "") : ""
            fillMode: Image.PreserveAspectCrop
        }
        MultiEffect {
            anchors.fill: parent
            source: cwBg
            blurEnabled: true
            blur: 1.0
            blurMax: 40
            brightness: -0.35
            saturation: -0.1
            opacity: cwHero.it ? 0.5 : 0
        }
        Rectangle {
            anchors.fill: parent
            opacity: cwHero.it ? 1 : 0
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0.0; color: "#f00e0e10" }
                GradientStop { position: 0.62; color: "#cc0e0e10" }
                GradientStop { position: 1.0; color: "#660e0e10" }
            }
        }
        Text {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.margins: 20
            visible: cwHero.it === null
            text: (i18n.language, i18n.t("hub_continue")).toUpperCase()
            color: Theme.accent
            font.pixelSize: 11
            font.weight: Font.Bold
            font.letterSpacing: 1.2
            font.family: Theme.fontSans
            z: 2
        }
        RailPlaceholder {
            anchors.centerIn: parent
            visible: cwHero.it === null
            text: (i18n.language, i18n.t("hub_watch_placeholder"))
        }

        Item {
            anchors.fill: parent
            anchors.margins: 24
            visible: cwHero.it !== null

            Item {
                id: cwPoster
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                height: parent.height
                width: height * 0.7
                Rectangle {
                    id: cwPC
                    anchors.fill: parent
                    color: "#161618"
                    visible: false
                    layer.enabled: true
                    Image {
                        anchors.fill: parent
                        source: cwHero.it ? (cwHero.it.poster || "") : ""
                        fillMode: Image.PreserveAspectCrop
                        asynchronous: true
                        cache: true
                    }
                }
                Rectangle {
                    id: cwPM
                    anchors.fill: parent
                    radius: 14
                    color: "white"
                    visible: false
                    layer.enabled: true
                }
                MultiEffect {
                    source: cwPC
                    anchors.fill: parent
                    maskEnabled: true
                    maskSource: cwPM
                }
                Rectangle {
                    anchors.fill: parent
                    radius: 14
                    color: "transparent"
                    border.color: "#33ffffff"
                    border.width: 1
                }
            }

            ColumnLayout {
                anchors.left: parent.left
                anchors.right: cwPoster.left
                anchors.rightMargin: 22
                anchors.verticalCenter: parent.verticalCenter
                spacing: 7
                Text {
                    text: (i18n.language, i18n.t("hub_continue")).toUpperCase()
                    color: Theme.accent
                    font.pixelSize: 11
                    font.weight: Font.Bold
                    font.letterSpacing: 1.2
                    font.family: Theme.fontSans
                }
                Text {
                    Layout.fillWidth: true
                    text: cwHero.it ? (cwHero.it.title || "") : ""
                    color: "#fff"
                    font.pixelSize: 27
                    font.weight: Font.Bold
                    font.family: Theme.fontSans
                    elide: Text.ElideRight
                    maximumLineCount: 2
                    wrapMode: Text.WordWrap
                }
                Text {
                    color: "#a8a8b0"
                    font.pixelSize: 12
                    font.weight: Font.DemiBold
                    font.family: Theme.fontSans
                    text: {
                        if (!cwHero.it) return ""
                        var parts = []
                        var ep = page.episodeLabel(cwHero.it)
                        if (ep.length > 0) parts.push(ep)
                        else if ((cwHero.it.year || "").length > 0) parts.push(cwHero.it.year)
                        parts.push(cwHero.it.isSeries ? i18n.t("hub_series") : i18n.t("hub_movie"))
                        return parts.join("  ·  ")
                    }
                }
                RowLayout {
                    Layout.fillWidth: true
                    Layout.topMargin: 5
                    spacing: 12
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 4
                        Layout.alignment: Qt.AlignVCenter
                        radius: 2
                        color: "#40ffffff"
                        Rectangle {
                            width: parent.width * Math.min(1, cwHero.it ? (cwHero.it.watchedPct || 0) : 0)
                            height: parent.height
                            radius: 2
                            color: Theme.accent
                        }
                    }
                    Text {
                        text: cwHero.it ? (page.fmtTime(cwHero.it.resumeMs) + " / " + page.fmtTime(cwHero.it.durMs)) : ""
                        color: "#c7c7cc"
                        font.pixelSize: 12
                        font.family: Theme.fontMono
                    }
                }
                RowLayout {
                    Layout.topMargin: 6
                    spacing: 14
                    BtnFlat {
                        primary: true
                        icon: "qrc:/icons/play.svg"
                        text: (i18n.language, i18n.t("hub_resume"))
                        onClicked: if (cwHero.it && page.api) page.api.playByHashFile(cwHero.it.infoHash, cwHero.it.fileIndex)
                    }
                    Text {
                        text: cwHero.it ? page.fmtLeft((cwHero.it.durMs || 0) - (cwHero.it.resumeMs || 0)) : ""
                        color: "#a8a8b0"
                        font.pixelSize: 12
                        font.family: Theme.fontSans
                    }
                }
            }
        }
    }

    // ---------- CONTINUE PLAYING ----------
    Rectangle {
        id: cpHero
        Layout.fillWidth: true
        Layout.preferredHeight: 224
        readonly property bool sug: page.continuePlaying.length === 0 && it !== null
        radius: 16
        color: Theme.elev
        border.color: Theme.hair
        border.width: 1
        clip: true
        readonly property var it: page.continuePlaying.length > 0 ? page.continuePlaying[0] : page.suggestedGame

        Image {
            id: cpBg
            anchors.fill: parent
            visible: false
            asynchronous: true
            cache: true
            source: cpHero.it ? (cpHero.it.poster || "") : ""
            fillMode: Image.PreserveAspectCrop
        }
        MultiEffect {
            anchors.fill: parent
            source: cpBg
            blurEnabled: true
            blur: 1.0
            blurMax: 44
            brightness: -0.4
            saturation: -0.1
            opacity: cpHero.it ? 0.5 : 0
        }
        Rectangle {
            anchors.fill: parent
            opacity: cpHero.it ? 1 : 0
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#e00e0e10" }
                GradientStop { position: 1.0; color: "#aa0e0e10" }
            }
        }
        Text {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.margins: 20
            text: (i18n.language, (cpHero.sug ? i18n.t("hub_ready_to_play") : i18n.t("hub_continue_playing"))).toUpperCase()
            color: Theme.accent
            font.pixelSize: 11
            font.weight: Font.Bold
            font.letterSpacing: 1.2
            font.family: Theme.fontSans
            z: 2
        }
        RailPlaceholder {
            anchors.centerIn: parent
            visible: cpHero.it === null
            text: (i18n.language, i18n.t("hub_play_placeholder"))
        }

        RowLayout {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.margins: 20
            spacing: 14
            visible: cpHero.it !== null
            Item {
                Layout.preferredWidth: 78
                Layout.preferredHeight: 104
                Layout.alignment: Qt.AlignBottom
                Rectangle {
                    id: cpPC
                    anchors.fill: parent
                    color: "#161618"
                    visible: false
                    layer.enabled: true
                    Image {
                        anchors.fill: parent
                        source: cpHero.it ? (cpHero.it.poster || "") : ""
                        fillMode: Image.PreserveAspectCrop
                        asynchronous: true
                        cache: true
                    }
                }
                Rectangle {
                    id: cpPM
                    anchors.fill: parent
                    radius: 12
                    color: "white"
                    visible: false
                    layer.enabled: true
                }
                MultiEffect {
                    source: cpPC
                    anchors.fill: parent
                    maskEnabled: true
                    maskSource: cpPM
                }
                Rectangle {
                    anchors.fill: parent
                    radius: 12
                    color: "transparent"
                    border.color: "#33ffffff"
                    border.width: 1
                }
            }
            ColumnLayout {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignBottom
                spacing: 6
                Text {
                    Layout.fillWidth: true
                    text: cpHero.it ? (cpHero.it.title || "") : ""
                    color: "#fff"
                    font.pixelSize: 19
                    font.weight: Font.Bold
                    font.family: Theme.fontSans
                    elide: Text.ElideRight
                    maximumLineCount: 2
                    wrapMode: Text.WordWrap
                }
                Text {
                    Layout.fillWidth: true
                    color: "#a8a8b0"
                    font.pixelSize: 12
                    font.family: Theme.fontSans
                    elide: Text.ElideRight
                    text: {
                        if (!cpHero.it) return ""
                        if (cpHero.sug) return i18n.t("hub_ready_to_play")
                        var parts = []
                        if ((cpHero.it.playSeconds || 0) > 0)
                            parts.push(page.fmtPlaytime(cpHero.it.playSeconds) + " " + i18n.t("hub_played"))
                        var ago = page.fmtAgo(cpHero.it.lastPlayed)
                        if (ago.length > 0) parts.push(ago)
                        return parts.join("  ·  ")
                    }
                }
                BtnFlat {
                    Layout.topMargin: 4
                    primary: true
                    icon: "qrc:/icons/play.svg"
                    text: (i18n.language, cpHero.sug ? i18n.t("hub_gs_play") : i18n.t("hub_resume"))
                    onClicked: if (cpHero.it) page.gamePrimary(cpHero.it)
                }
            }
        }
    }
}
