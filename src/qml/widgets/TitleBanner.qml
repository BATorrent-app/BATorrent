// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// A wide banner for one title: its backdrop, its lettering on the dark side,
// a line of facts and a way in. The billboard rotates these; a search leads
// with one for its best match.
import QtQuick
import QtQuick.Layouts
import "../theme"

Item {
    id: banner
    property var item: null
    property string kicker
    property color kickerColor: Theme.stageT2
    property bool showRating: true
    property var extraFacts: []         // after year and kind: a studio, say
    property string body
    property string status              // a green-dot line: what can be got
    property string primaryLabel
    property string secondaryLabel
    property bool centered: false       // text block centred instead of resting on the bottom
    property real textWidth: 500
    property real logoWidth: 380
    property real logoHeight: 120
    readonly property alias textBlock: textCol
    readonly property alias textShift: shift
    readonly property alias art: img
    signal primaryClicked()
    signal secondaryClicked()

    function typeLabel(t) {
        return t === "movie" ? i18n.t("search_type_movie")
             : t === "series" ? i18n.t("search_type_series")
             : t === "game" ? i18n.t("search_type_game") : ""
    }

    Image {
        id: img
        anchors.fill: parent
        source: banner.item
            ? ((banner.item.backdrop || "").length > 0 ? banner.item.backdrop : (banner.item.poster || ""))
            : ""
        fillMode: Image.PreserveAspectCrop
        verticalAlignment: Image.AlignTop
        asynchronous: true
        cache: true
        opacity: status === Image.Ready ? 1 : 0
        Behavior on opacity { NumberAnimation { duration: Theme.durSlower } }
    }
    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            orientation: Gradient.Horizontal
            GradientStop { position: 0.0;  color: Theme.stageWash(0.92) }
            GradientStop { position: 0.38; color: Theme.stageWash(0.58) }
            GradientStop { position: 0.72; color: Theme.stageWash(0) }
        }
    }
    Rectangle {
        anchors.fill: parent
        visible: !banner.centered
        gradient: Gradient {
            GradientStop { position: 0.6; color: Theme.stageWash(0) }
            GradientStop { position: 1.0; color: Theme.stageWash(0.7) }
        }
    }

    ColumnLayout {
        id: textCol
        transform: Translate { id: shift }
        anchors.left: parent.left
        anchors.leftMargin: banner.centered ? 44 : 48
        anchors.bottom: banner.centered ? undefined : parent.bottom
        anchors.bottomMargin: 44
        anchors.verticalCenter: banner.centered ? parent.verticalCenter : undefined
        width: Math.min(banner.textWidth, parent.width - 96)
        spacing: banner.centered ? 14 : 16

        Text {
            visible: banner.kicker.length > 0
            text: banner.kicker
            color: banner.kickerColor
            font.pixelSize: 11; font.weight: Font.DemiBold
            font.letterSpacing: 1.2; font.family: Theme.fontSans
        }
        TitleMark {
            Layout.preferredWidth: maxWidth
            Layout.preferredHeight: maxHeight
            maxWidth: banner.logoWidth
            maxHeight: banner.logoHeight
            fontSize: 40
            tmdbId: banner.item ? (banner.item.tmdbId || 0) : 0
            type: banner.item ? (banner.item.type || "") : ""
            title: banner.item ? (banner.item.title || banner.item.name || "") : ""
        }
        Row {
            spacing: 14
            Text {
                visible: banner.showRating && banner.item && (banner.item.rating || 0) > 0
                text: banner.item ? "★ " + (banner.item.rating || 0).toFixed(1) : ""
                color: Theme.star
                font.pixelSize: 14; font.weight: Font.DemiBold
                font.family: Theme.fontSans; font.features: Theme.tnum
            }
            Repeater {
                model: banner.item
                       ? [banner.item.year || "", (i18n.language, banner.typeLabel(banner.item.type || ""))]
                         .concat(banner.extraFacts).filter(function (f) { return f.length > 0 })
                       : []
                Text {
                    required property var modelData
                    text: modelData
                    color: Theme.stageInk
                    font.pixelSize: 14; font.family: Theme.fontSans; font.features: Theme.tnum
                }
            }
        }
        Text {
            Layout.fillWidth: true
            visible: banner.body.length > 0
            text: banner.body
            color: Theme.stageInk
            font.pixelSize: 15
            font.family: Theme.fontSans
            lineHeight: 1.55
            wrapMode: Text.WordWrap
            maximumLineCount: 3
            elide: Text.ElideRight
        }
        Row {
            visible: banner.status.length > 0
            spacing: 8
            Rectangle {
                anchors.verticalCenter: parent.verticalCenter
                width: 6; height: 6; radius: 3
                color: Theme.grn
            }
            Text {
                text: banner.status
                color: Theme.stageMuted
                font.pixelSize: 13; font.family: Theme.fontSans; font.features: Theme.tnum
            }
        }
        Row {
            Layout.topMargin: 4
            spacing: 10
            component Cta: Rectangle {
                id: cta
                property alias label: ctaTxt.text
                property bool light: true
                signal clicked()
                visible: label.length > 0
                width: ctaTxt.implicitWidth + 40
                height: 40
                radius: 4
                color: light ? (ctaMa.containsMouse ? "#ffffff" : Theme.stageT1)
                             : (ctaMa.containsMouse ? Qt.rgba(1, 1, 1, 0.16) : Qt.rgba(1, 1, 1, 0.1))
                Behavior on color { ColorAnimation { duration: Theme.durFast } }
                scale: ctaMa.pressed ? Theme.pressScale : 1
                Text {
                    id: ctaTxt
                    anchors.centerIn: parent
                    color: cta.light ? Theme.stageBg : Theme.stageT1
                    font.pixelSize: 14
                    font.weight: cta.light ? Font.DemiBold : Font.Medium
                    font.family: Theme.fontSans
                }
                MouseArea {
                    id: ctaMa
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: cta.clicked()
                }
            }
            Cta {
                label: banner.primaryLabel
                onClicked: banner.primaryClicked()
            }
            Cta {
                light: false
                label: banner.secondaryLabel
                onClicked: banner.secondaryClicked()
            }
        }
    }
}
