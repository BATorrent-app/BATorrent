// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// What hangs behind the library: the art drawn for the theme or the user's
// own picture, in the top-right corner and fading into the page, then the
// theme's paper or film grain over it. It sits under the cards and never
// under the details panel, so text there keeps its contrast.
import QtQuick
import QtQuick.Window
import "../theme"

Item {
    id: root
    // the list puts state and peer columns right over the art: it steps back there
    property bool quiet: false

    readonly property var art: Theme.art
    readonly property bool userPic: Theme.userArtSource !== ""

    Item {
        id: slot
        visible: (root.art || root.userPic) && img.status === Image.Ready
        // the theme art takes its share of the width and the whole height;
        // a picture of the user's is an accent in the corner, as the anime art was
        readonly property real share: root.art ? root.art.width : 0.46
        width: root.userPic ? Math.min(460, parent.width * share) : parent.width * share
                + (root.art && root.art.bleed ? root.art.bleed : 0)
        height: root.userPic ? (img.implicitWidth > 0 ? img.implicitHeight * (width / img.implicitWidth) : 0)
                : parent.height * (root.art && root.art.tall ? root.art.tall : 1)
        anchors.right: parent.right
        anchors.rightMargin: root.art && root.art.bleed ? -root.art.bleed : 0
        y: root.art && root.art.bleed ? -root.art.bleed : 0
        opacity: (root.art ? root.art.opacity : 1) * (root.quiet ? 0.25 : 1)
        // one layer, so the opacity applies to the picture and its fades as a
        // group (per item, the fades would only be partly there and leave an edge)
        layer.enabled: visible
        Behavior on opacity { NumberAnimation { duration: Theme.durSlow; easing.type: Theme.easeOut } }

        Image {
            id: img
            anchors.fill: parent
            source: root.userPic ? Theme.userArtSource : Theme.artSource
            asynchronous: true
            fillMode: root.userPic || (root.art && root.art.fit === "contain")
                      ? Image.PreserveAspectFit : Image.PreserveAspectCrop
            horizontalAlignment: Image.AlignRight
            verticalAlignment: Image.AlignTop
            sourceSize.width: Math.ceil(width * Screen.devicePixelRatio)
            sourceSize.height: root.userPic ? undefined : Math.ceil(height * Screen.devicePixelRatio)
            smooth: true
            mipmap: true
        }
        // fade into the page on the left and at the bottom (the art that has
        // its own edges, the Haeckel plate and the cherry branch, does not)
        readonly property bool fades: root.userPic || (root.art && root.art.fade)
        Rectangle {
            visible: slot.fades
            anchors.fill: parent
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0.0; color: Theme.bg }
                GradientStop { position: root.userPic ? 0.45 : 0.55; color: "transparent" }
            }
        }
        Rectangle {
            visible: slot.fades
            anchors.fill: parent
            gradient: Gradient {
                GradientStop { position: root.userPic ? 0.65 : 0.5; color: "transparent" }
                GradientStop { position: 1.0; color: Theme.bg }
            }
        }
    }

    // paper (Light, Sakura) or film grain (Cinema), in every background mode
    Image {
        anchors.fill: parent
        visible: Theme.texture !== ""
        source: Theme.texture !== "" ? "qrc:/images/themes/paper-grain.png" : ""
        fillMode: Image.Tile
        opacity: Theme.texture === "grain" ? 0.5 : 0.7
        smooth: false
    }
}
