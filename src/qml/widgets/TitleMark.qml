// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// A title's own lettering when TMDB has it, its name set in type when not.
// Asks for the logo itself, so any tile or banner can just name the title.
import QtQuick
import "../theme"

Item {
    id: mark
    property int tmdbId: 0
    property string type
    property string title
    property real maxWidth: 380
    property real maxHeight: 120
    property int fontSize: 15
    property string logo

    implicitWidth: img.visible ? img.paintedWidth : Math.min(maxWidth, name.implicitWidth)
    implicitHeight: img.visible ? img.paintedHeight : name.implicitHeight

    readonly property var disco: typeof discovery !== "undefined" ? discovery : null
    function ask() {
        logo = ""
        if (disco && tmdbId > 0) disco.fetchLogo(tmdbId, type)
    }
    onTmdbIdChanged: ask()
    onTypeChanged: ask()
    Component.onCompleted: ask()
    Connections {
        target: mark.disco
        ignoreUnknownSignals: true
        function onTitleLogoReady(id, type, url) {
            if (id === mark.tmdbId && type === mark.type) mark.logo = url
        }
    }

    Image {
        id: img
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        width: mark.maxWidth
        height: mark.maxHeight
        source: mark.logo
        fillMode: Image.PreserveAspectFit
        horizontalAlignment: Image.AlignLeft
        verticalAlignment: Image.AlignBottom
        asynchronous: true
        cache: true
        smooth: true
        visible: status === Image.Ready
        opacity: visible ? 1 : 0
        Behavior on opacity { NumberAnimation { duration: Theme.durSlow } }
    }
    Text {
        id: name
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        width: mark.maxWidth
        visible: !img.visible
        text: mark.title
        color: Theme.stageT1
        font.pixelSize: mark.fontSize
        font.weight: Font.Bold
        font.letterSpacing: -0.2
        font.family: Theme.fontSans
        lineHeight: 1.15
        wrapMode: Text.WordWrap
        maximumLineCount: 2
        elide: Text.ElideRight
    }
}
