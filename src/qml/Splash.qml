// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Startup splash v2.0 — "Stroke": the bat's outline is drawn (stroke-dashoffset),
// then the whole logo fills uniformly while the white border fades out fast, with
// a soft red glow + wordmark. Ported 1:1 from the approved HTML prototype.
import QtQuick
import QtQuick.Shapes
import QtQuick.Effects
import "theme"

Item {
    id: root
    anchors.fill: parent
    z: 10000
    focus: true

    signal finished()

    property real dur: 1.6                 // seconds to draw the outline
    readonly property int pathCount: 9
    readonly property real stagger: 55     // ms between paths starting to draw
    readonly property real allDrawnMs: dur * 1000 + (pathCount - 1) * stagger

    property real clock: 0                 // ms since start
    property bool morphing: false

    // after the outline is done: border fades (.18s), fill rises (.35s)
    readonly property real strokeAlpha: clock < allDrawnMs ? 1 : Math.max(0, 1 - (clock - allDrawnMs) / 180)
    readonly property real fillAlpha:   clock < allDrawnMs ? 0 : Math.min(1, (clock - allDrawnMs) / 350)
    readonly property color strokeCol:  Qt.rgba(1, 1, 1, strokeAlpha)
    readonly property real wordFade:    clock < allDrawnMs ? 0 : (clock - allDrawnMs)

    readonly property real strokeW: 6      // viewBox units (~1.4px on screen)
    // measured arc length of each path (viewBox units); dash = length so the
    // contour draws over the full duration instead of snapping done early.
    readonly property var lengths: [3168.2, 1601.5, 1613.1, 859.3, 24.2, 23.5, 16.2, 20.3, 15.9]

    function dashUnit(i) { return lengths[i] / strokeW }   // dash length in pen-width units

    function drawOffset(i) {
        var local = (clock - i * stagger) / (dur * 1000)
        if (local <= 0) return dashUnit(i)
        if (local >= 1) return 0
        var e = local * local * (3 - 2 * local)   // smoothstep ≈ ease cubic-bezier
        return dashUnit(i) * (1 - e)
    }

    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#120c10" }
            GradientStop { position: 0.55; color: "#0a0709" }
            GradientStop { position: 1.0; color: "#050406" }
        }
    }

    Column {
        anchors.centerIn: parent
        spacing: 14

        Item {
            id: wrap
            width: 230; height: 230
            anchors.horizontalCenter: parent.horizontalCenter
            layer.enabled: true
            layer.effect: MultiEffect {
                shadowEnabled: true
                shadowColor: Qt.rgba(0.898, 0.2, 0.169, 0.42 * root.fillAlpha)
                shadowBlur: 0.9
                shadowVerticalOffset: 0
                shadowHorizontalOffset: 0
            }

            Shape {
                id: bat
                width: 1024; height: 1024
                // CurveRenderer has its own analytic anti-aliasing; the default
                // GeometryRenderer relies on MSAA, which the Windows swapchain
                // runs at 1x, so the bat outline came out jagged there.
                preferredRendererType: Shape.CurveRenderer
                transform: Scale { xScale: wrap.width / 1024; yScale: wrap.height / 1024 }

                ShapePath {
                    fillColor: Qt.rgba(0.913, 0.913, 0.910, root.fillAlpha)
                    strokeColor: root.strokeCol; strokeWidth: root.strokeW
                    capStyle: ShapePath.RoundCap; joinStyle: ShapePath.RoundJoin
                    strokeStyle: ShapePath.DashLine; dashPattern: [root.dashUnit(0), root.dashUnit(0)]; dashOffset: root.drawOffset(0)
                    PathSvg { path: "M538.537 172.298C664.779 188.363 759.287 293.88 742.198 423.784C734.863 479.539 707.18 525.394 668.451 565.101C650.374 583.787 629.569 604.252 613.322 624.2C574.423 671.424 545.56 726.083 528.497 784.838C521.123 810.471 516.378 836.708 514.066 863.255C513.616 868.431 512.363 877.957 512.498 882.893L511.498 882.977C511.627 876.689 510.771 868.185 510.168 861.839C507.609 835.434 502.677 809.312 495.431 783.792C479.504 727.629 452.421 675.252 415.801 629.788C385.628 592.402 347.846 560.946 320.014 521.882C300.016 493.815 287.288 458.143 282.392 424.21C274.164 367.065 289.165 309.006 324.044 262.999C362.966 211.053 418.716 181.898 482.267 172.802L481.603 173.925C461.232 180.122 448.23 184.205 429.764 194.352C309.464 260.459 292.068 430.182 421.943 498.427C439.331 507.564 451.987 512.123 470.779 517.383C457.444 486.212 450.765 456.914 454.173 422.612C454.617 418.143 456.442 407.396 457.908 403.247C460.184 421.457 463.685 434.427 475.872 448.056C507.285 483.188 556.318 459.786 563.655 417.429C564.712 411.326 564.974 406.556 565.171 400.511C572.759 428.935 571.532 460.603 563.225 488.761C560.243 498.873 556.389 508.149 552.36 517.883C615.516 501.141 670.624 461.1 687.723 395.01C699.394 349.898 690.019 301.639 665.831 262.163C636.392 214.115 592.182 186.363 538.242 172.686L538.537 172.298Z" }
                }
                ShapePath {
                    fillColor: Qt.rgba(0.913, 0.913, 0.910, root.fillAlpha)
                    strokeColor: root.strokeCol; strokeWidth: root.strokeW
                    capStyle: ShapePath.RoundCap; joinStyle: ShapePath.RoundJoin
                    strokeStyle: ShapePath.DashLine; dashPattern: [root.dashUnit(1), root.dashUnit(1)]; dashOffset: root.drawOffset(1)
                    PathSvg { path: "M534.008 137.411C648.749 143.524 756.122 195.847 831.643 282.447C893.601 351.834 933.363 438.184 945.804 530.371C953.541 586.098 951.051 624.449 940.261 678.953C938.082 638.28 921.564 603.747 891.269 576.48C860.592 549.066 820.214 535.078 779.153 537.64C763.925 538.481 746.282 541.63 731.301 544.334C741.532 527.187 750.581 512.126 758.617 493.701C780.31 443.799 784.814 388.106 771.423 335.366C754.467 268.442 708.694 211.261 649.676 176.269C619.513 158.283 586.193 146.22 551.507 140.729C548.971 140.316 537.277 138.311 535.296 138.521L534.008 137.411Z" }
                }
                ShapePath {
                    fillColor: Qt.rgba(0.913, 0.913, 0.910, root.fillAlpha)
                    strokeColor: root.strokeCol; strokeWidth: root.strokeW
                    capStyle: ShapePath.RoundCap; joinStyle: ShapePath.RoundJoin
                    strokeStyle: ShapePath.DashLine; dashPattern: [root.dashUnit(2), root.dashUnit(2)]; dashOffset: root.drawOffset(2)
                    PathSvg { path: "M82.9909 682.051C78.2625 660.041 75.0019 626.663 74.4772 603.623C72.0046 477.587 120.087 355.806 207.994 265.454C269.472 201.792 349.451 159.128 436.565 143.524C452.471 140.661 473.891 137.869 490.114 137.493L489.501 138.553C485.347 138.269 469.916 140.988 465.142 141.934C422.892 149.917 383.041 167.501 348.665 193.33C293.641 234.87 257.179 293.616 247.752 362.231C240.624 415.946 251.007 470.528 277.361 517.874C282.571 527.37 287.836 536.391 294.4 545.028C273.07 541.15 254.256 537.224 232.504 537.504C190.645 537.659 150.632 554.756 121.587 584.899C93.9304 613.43 82.393 642.632 82.9909 682.051Z" }
                }
                ShapePath {
                    fillColor: Qt.rgba(0.906, 0.129, 0.204, root.fillAlpha)   // red wing
                    strokeColor: root.strokeCol; strokeWidth: root.strokeW
                    capStyle: ShapePath.RoundCap; joinStyle: ShapePath.RoundJoin
                    strokeStyle: ShapePath.DashLine; dashPattern: [root.dashUnit(3), root.dashUnit(3)]; dashOffset: root.drawOffset(3)
                    PathSvg { path: "M773.437 568.224C776.099 568.239 778.762 568.296 781.422 568.394C820.72 569.914 859.997 587.145 886.575 616.213C907.716 639.336 921.992 671.567 924.975 702.732C926.28 716.369 925.819 728.73 925.218 742.334C922.542 726.099 919.874 713.612 913.579 698.268C897.264 658.508 865.646 631.714 826.624 615.746C806.49 607.891 785.041 603.954 763.429 604.148C723.603 604.615 685.029 618.138 653.63 642.643C639.783 653.35 630.708 662.582 618.592 674.951C631.201 655.204 637.335 643.058 654.416 625.385C689.089 589.512 722.723 569.75 773.437 568.224Z" }
                }
                ShapePath {
                    fillColor: Qt.rgba(0.224, 0.227, 0.227, root.fillAlpha)
                    strokeColor: root.strokeCol; strokeWidth: root.strokeW
                    capStyle: ShapePath.RoundCap; joinStyle: ShapePath.RoundJoin
                    strokeStyle: ShapePath.DashLine; dashPattern: [root.dashUnit(4), root.dashUnit(4)]; dashOffset: root.drawOffset(4)
                    PathSvg { path: "M535.296 138.521C532.826 138.666 525.787 137.997 523.443 137.336C526.675 136.816 530.787 136.885 534.008 137.411L535.296 138.521Z" }
                }
                ShapePath {
                    fillColor: Qt.rgba(0.435, 0.439, 0.455, root.fillAlpha)
                    strokeColor: root.strokeCol; strokeWidth: root.strokeW
                    capStyle: ShapePath.RoundCap; joinStyle: ShapePath.RoundJoin
                    strokeStyle: ShapePath.DashLine; dashPattern: [root.dashUnit(5), root.dashUnit(5)]; dashOffset: root.drawOffset(5)
                    PathSvg { path: "M482.267 172.802C485.103 172.106 490.091 171.415 492.908 171.849C489.248 172.536 485.257 173.37 481.603 173.925L482.267 172.802Z" }
                }
                ShapePath {
                    fillColor: Qt.rgba(0.224, 0.227, 0.227, root.fillAlpha)
                    strokeColor: root.strokeCol; strokeWidth: root.strokeW
                    capStyle: ShapePath.RoundCap; joinStyle: ShapePath.RoundJoin
                    strokeStyle: ShapePath.DashLine; dashPattern: [root.dashUnit(6), root.dashUnit(6)]; dashOffset: root.drawOffset(6)
                    PathSvg { path: "M490.114 137.493C492.654 137.196 494.612 136.779 497.115 137.251L496.898 137.709C494.472 138.267 491.991 138.55 489.501 138.553L490.114 137.493Z" }
                }
                ShapePath {
                    fillColor: Qt.rgba(0.224, 0.227, 0.227, root.fillAlpha)
                    strokeColor: root.strokeCol; strokeWidth: root.strokeW
                    capStyle: ShapePath.RoundCap; joinStyle: ShapePath.RoundJoin
                    strokeStyle: ShapePath.DashLine; dashPattern: [root.dashUnit(7), root.dashUnit(7)]; dashOffset: root.drawOffset(7)
                    PathSvg { path: "M528.55 171.837C531.497 171.486 535.904 171.006 538.537 172.298L538.242 172.686C534.499 172.754 532.254 172.37 528.55 171.837Z" }
                }
                ShapePath {
                    fillColor: Qt.rgba(0.435, 0.439, 0.455, root.fillAlpha)
                    strokeColor: root.strokeCol; strokeWidth: root.strokeW
                    capStyle: ShapePath.RoundCap; joinStyle: ShapePath.RoundJoin
                    strokeStyle: ShapePath.DashLine; dashPattern: [root.dashUnit(8), root.dashUnit(8)]; dashOffset: root.drawOffset(8)
                    PathSvg { path: "M511.498 882.977L512.498 882.893C512.536 884.867 512.734 888.649 511.95 890.318C511.241 888.155 511.476 885.331 511.498 882.977Z" }
                }
            }
        }

        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            opacity: Math.min(1, root.wordFade / 500)
            transform: Translate { y: (1 - Math.min(1, root.wordFade / 500)) * 8 }
            Text { text: "BAT"; color: "#e5332b"; font.pixelSize: 30; font.weight: Font.Bold; font.family: Theme.fontSans }
            Text { text: "orrent"; color: "#f3f3f4"; font.pixelSize: 30; font.weight: Font.Bold; font.family: Theme.fontSans }
        }
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: (typeof i18n !== "undefined") ? (i18n.language, i18n.t("splash_tagline")) : "Cliente BitTorrent leve"
            color: "#9a959c"; font.pixelSize: 13; font.weight: Font.Medium; font.family: Theme.fontSans
            opacity: Math.min(1, Math.max(0, root.wordFade - 120) / 500)
        }
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            visible: text.length > 0
            text: (typeof themeBridge !== "undefined" && themeBridge.appVersion) ? ("v" + themeBridge.appVersion) : ""
            color: "#5a565c"; font.pixelSize: 11; font.weight: Font.DemiBold
            font.letterSpacing: 0.5; font.family: Theme.fontMono
            opacity: Math.min(1, Math.max(0, root.wordFade - 240) / 500)
        }
    }

    Timer {
        id: clockTimer
        interval: 16; repeat: true; running: true
        onTriggered: {
            root.clock += 16
            if (root.clock > root.allDrawnMs + 1500) { clockTimer.stop(); root.exit(false) }
        }
    }

    function exit(quick) {
        if (root.morphing) return
        root.morphing = true
        clockTimer.stop()
        fadeOut.duration = quick ? 260 : 900
        fadeOut.start()
    }
    NumberAnimation {
        id: fadeOut
        target: root; property: "opacity"; to: 0; duration: 900
        easing.type: Easing.InOutQuad
        onFinished: root.finished()
    }

    MouseArea { anchors.fill: parent; onClicked: root.exit(true) }
    Keys.onPressed: function(e) { root.exit(true); e.accepted = true }
}
