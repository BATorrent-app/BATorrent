// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// The wait before a stream can play, inside the player it will play in: the
// title's still behind it, three steps (getting info, finding peers,
// buffering), a title that says what is left ("Starting in about 6 s"), one
// reason, one bar along the bottom that fills to what is needed to start (not
// the whole file), and what to do when it is slow or nobody shares it.
import QtQuick
import QtQuick.Effects
import QtQuick.Layouts
import "../theme"
import "../widgets"

Item {
    id: root
    property var pw
    // { hash, title, subtitle, art, release, season, episode, candidates: [{idx, quality, label, seeds}] }
    readonly property var w: pw && pw.waiting ? pw.waiting : null
    // { meta, have, need, down, peers, seeds, elapsed, noSource } from session.waitState
    readonly property var s: pw && pw.waitInfo ? pw.waitInfo : ({})
    visible: !!w
    property bool keepWaiting: false
    onWChanged: keepWaiting = false

    readonly property bool noSource: s.noSource === true
    readonly property bool meta: s.meta === true
    readonly property real have: s.have || 0
    readonly property real need: s.need || 0
    readonly property real down: s.down || 0
    readonly property int peers: s.peers || 0
    readonly property int seeds: s.seeds || 0
    readonly property int elapsed: s.elapsed || 0
    // 0 getting info, 1 finding peers, 2 buffering
    readonly property int stage: !meta ? 0 : (have <= 0 && down <= 0 ? 1 : 2)
    readonly property real eta: down > 0 ? Math.max(0, need - have) / down : Infinity
    readonly property bool slow: !noSource && stage === 2 && eta > 30
    readonly property bool late: !noSource && stage < 2 && elapsed > 30
    readonly property bool warnState: noSource || slow || late

    // a healthier release of the same thing, from the list it came from
    readonly property var alt: {
        if (!w || !(slow || late) || keepWaiting) return null
        var c = w.candidates || []
        var best = null
        for (var i = 0; i < c.length; i++) {
            var r = c[i]
            if (r.seeds < 30 || r.seeds <= seeds * 3) continue
            var same = r.quality === w.quality
            if (!best || (same && !best.same) || (same === best.same && r.seeds > best.seeds))
                best = { idx: r.idx, label: r.label, seeds: r.seeds, same: same }
        }
        return best
    }

    function mb(b) { return pw ? pw.fmtBytes(b) : "" }
    function clock(sec) { var m = Math.floor(sec / 60), x = sec % 60; return m + ":" + (x < 10 ? "0" : "") + x }
    function about(sec) {
        if (!isFinite(sec)) return ""
        return sec < 60 ? i18n.t("wait_about_s").arg(Math.max(1, Math.ceil(sec)))
                        : i18n.t("wait_about_min").arg(Math.max(1, Math.round(sec / 60)))
    }
    function playAlt() {
        if (!alt || typeof search === "undefined" || !search) return
        if (typeof session !== "undefined") session.cancelWatch(w.hash)
        if (w.episode > 0) search.addAndWatchEpisode(alt.idx, w.season, w.episode)
        else search.addAndWatch(alt.idx)
    }

    // ---- the still, blurred and dimmed ----
    Rectangle { anchors.fill: parent; color: "#000000" }
    Image {
        id: art
        anchors.fill: parent
        source: root.w ? (root.w.art || "") : ""
        fillMode: Image.PreserveAspectCrop
        asynchronous: true
        visible: false
    }
    MultiEffect {
        anchors.fill: parent
        source: art
        visible: art.status === Image.Ready
        blurEnabled: true
        blur: 0.55
        blurMax: 48
        opacity: 0.4
    }
    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: Qt.rgba(0, 0, 0, 0.35) }
            GradientStop { position: 1.0; color: Qt.rgba(0, 0, 0, 0.75) }
        }
    }
    // the wait takes clicks, so nothing under it reacts
    MouseArea { anchors.fill: parent }

    // ---- the middle: steps, what is left, why, what to do ----
    ColumnLayout {
        anchors.centerIn: parent
        width: Math.min(parent.width - 80, 560)
        spacing: 0

        Row {
            Layout.alignment: Qt.AlignHCenter
            spacing: 10
            Repeater {
                model: ["wait_step_info", "wait_step_peers", "wait_step_buffer"]
                Row {
                    id: step
                    required property var modelData
                    required property int index
                    readonly property bool done: index < root.stage
                    readonly property bool now: index === root.stage
                    spacing: 10
                    Rectangle {
                        visible: step.index > 0
                        anchors.verticalCenter: parent.verticalCenter
                        width: 22; height: 1; color: Qt.rgba(1, 1, 1, 0.25)
                    }
                    Row {
                        spacing: 7
                        Rectangle {
                            id: stepDot
                            anchors.verticalCenter: parent.verticalCenter
                            width: 7; height: 7; radius: 3.5
                            color: step.done ? Theme.grn : step.now ? (root.warnState ? Theme.warn : "#ffffff") : "transparent"
                            border.width: step.done || step.now ? 0 : 1
                            border.color: Qt.rgba(1, 1, 1, 0.45)
                            // the step under way breathes, so the wait reads as alive
                            SequentialAnimation on opacity {
                                running: step.now && root.visible && !root.noSource
                                loops: Animation.Infinite
                                onRunningChanged: if (!running) stepDot.opacity = 1
                                NumberAnimation { to: 0.3; duration: 700; easing.type: Easing.InOutSine }
                                NumberAnimation { to: 1; duration: 700; easing.type: Easing.InOutSine }
                            }
                        }
                        Text {
                            text: (i18n.language, i18n.t(step.modelData))
                            color: step.now ? "#ffffff" : Qt.rgba(1, 1, 1, step.done ? 0.7 : 0.45)
                            font.pixelSize: 13; font.weight: step.now ? Font.DemiBold : Font.Normal; font.family: Theme.fontSans
                        }
                    }
                }
            }
        }

        Text {
            Layout.fillWidth: true
            Layout.topMargin: 18
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            color: "#ffffff"
            font.pixelSize: 22; font.weight: Font.DemiBold; font.family: Theme.fontSans; font.features: Theme.tnum
            text: {
                i18n.language
                if (!root.w) return ""
                if (root.noSource) return i18n.t("wait_t_nosource")
                if (root.stage === 0) return i18n.t(root.w.episode > 0 ? "wait_t_info_ep" : "wait_t_info")
                if (root.stage === 1) return i18n.t("wait_t_peers")
                if (root.slow) return i18n.t("wait_t_slow").arg(root.about(root.eta))
                return isFinite(root.eta) ? i18n.t("wait_t_starting").arg(root.about(root.eta)) : i18n.t("wait_t_buffering")
            }
        }
        Text {
            Layout.fillWidth: true
            Layout.topMargin: 10
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            color: Qt.rgba(1, 1, 1, 0.85)
            font.pixelSize: 14; font.family: Theme.fontSans; font.features: Theme.tnum
            text: {
                i18n.language
                if (!root.w) return ""
                if (root.noSource) return i18n.t("wait_r_nosource").arg(root.elapsed)
                if (root.stage === 0) return i18n.t("wait_r_info")
                if (root.stage === 1) return root.peers > 0 ? i18n.t(root.peers === 1 ? "wait_r_peers_1" : "wait_r_peers").arg(root.peers)
                                                            : i18n.t("wait_r_nopeers")
                if (root.slow && root.alt) return i18n.t("wait_r_slow_alt").arg(root.seeds).arg(root.alt.label).arg(root.alt.seeds)
                if (root.slow) return i18n.t("wait_r_slow").arg(root.seeds).arg(root.mb(root.down) + "/s")
                return i18n.t("wait_r_buffering").arg(root.mb(root.have)).arg(root.mb(root.need))
            }
        }

        Row {
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: 20
            spacing: 8
            // the no-source way out
            WaitBtn {
                visible: root.noSource
                light: true
                text: (i18n.language, i18n.t("wait_choose_other"))
                onClicked: root.pw.cancelWait(false)
            }
            WaitBtn {
                visible: root.noSource
                text: (i18n.language, i18n.t("wait_back_find"))
                onClicked: root.pw.cancelWait(true)
            }
            // slow: the healthier release, or keep going
            WaitBtn {
                visible: !root.noSource && !!root.alt
                light: true
                text: root.alt ? (i18n.language, i18n.t("wait_play_alt")).arg(root.alt.label) : ""
                onClicked: root.playAlt()
            }
            WaitBtn {
                visible: !root.noSource && !!root.alt
                text: (i18n.language, i18n.t("wait_keep"))
                onClicked: root.keepWaiting = true
            }
            // early on: just the way back
            WaitBtn {
                visible: !root.noSource && !root.alt && root.stage < 2
                text: (i18n.language, i18n.t("btn_cancel"))
                onClicked: root.pw.cancelWait(false)
            }
        }
    }

    // ---- the one bar, and the line under it ----
    Item {
        anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom
        anchors.leftMargin: 24; anchors.rightMargin: 24; anchors.bottomMargin: 18
        height: 34
        Rectangle {
            id: track
            width: parent.width; height: 4; radius: 2
            color: Qt.rgba(1, 1, 1, 0.18)
            // no bytes yet: a light runs along the bar, so it is plainly working
            Item {
                anchors.fill: parent
                clip: true
                visible: root.stage < 2 && !root.noSource && root.visible
                Rectangle {
                    id: sweep
                    width: parent.width * 0.22; height: parent.height; radius: 2
                    gradient: Gradient {
                        orientation: Gradient.Horizontal
                        GradientStop { position: 0.0; color: Qt.rgba(1, 1, 1, 0) }
                        GradientStop { position: 0.5; color: Qt.rgba(1, 1, 1, 0.75) }
                        GradientStop { position: 1.0; color: Qt.rgba(1, 1, 1, 0) }
                    }
                    NumberAnimation on x {
                        running: sweep.parent.visible && !Theme.reduceMotion
                        loops: Animation.Infinite
                        from: -sweep.width; to: track.width
                        duration: 1400; easing.type: Easing.InOutQuad
                    }
                    // reduced motion: it fades in place instead
                    SequentialAnimation on opacity {
                        running: sweep.parent.visible && Theme.reduceMotion
                        loops: Animation.Infinite
                        NumberAnimation { to: 0.2; duration: 800 }
                        NumberAnimation { to: 1; duration: 800 }
                    }
                }
            }
            Rectangle {
                height: parent.height; radius: 2
                width: root.need > 0 && !root.noSource ? Math.max(height, parent.width * Math.min(1, root.have / root.need)) : 0
                visible: root.stage === 2
                color: root.slow ? Theme.warn : Theme.accent
                Behavior on width { NumberAnimation { duration: Theme.durSlow; easing.type: Theme.easeOut } }
            }
        }
        Text {
            anchors.left: parent.left; anchors.bottom: parent.bottom
            text: root.stage === 2 && !root.noSource ? root.mb(root.have).replace(/ \S+$/, "") + " / " + root.mb(root.need)
                                                    : root.clock(root.elapsed)
            color: "#ffffff"; font.pixelSize: 13; font.weight: Font.DemiBold; font.family: Theme.fontSans; font.features: Theme.tnum
        }
        Row {
            anchors.right: parent.right; anchors.bottom: parent.bottom
            spacing: 16
            Text {
                visible: root.down > 0
                text: "↓ " + root.mb(root.down) + "/s"
                color: root.slow ? Theme.warn : "#ffffff"
                font.pixelSize: 13; font.family: Theme.fontSans; font.features: Theme.tnum
            }
            Text {
                text: (i18n.language, root.slow ? i18n.t("find_rel_seeds").arg(root.seeds)
                                                : i18n.t(root.peers === 1 ? "wait_n_peers_1" : "wait_n_peers").arg(root.peers))
                color: root.noSource ? Theme.warn : "#ffffff"
                font.pixelSize: 13; font.family: Theme.fontSans; font.features: Theme.tnum
            }
            Text {
                visible: text.length > 0
                text: root.w ? (root.w.release || "") : ""
                color: Qt.rgba(1, 1, 1, 0.6)
                font.pixelSize: 13; font.family: Theme.fontSans; font.features: Theme.tnum
            }
        }
    }

    component WaitBtn: Rectangle {
        id: b
        property string text
        property bool light: false
        signal clicked()
        width: bTxt.implicitWidth + 28; height: 34
        radius: Theme.rad(6)
        color: light ? (bMa.containsMouse ? "#ffffff" : Qt.rgba(1, 1, 1, 0.92))
                     : (bMa.containsMouse ? Qt.rgba(1, 1, 1, 0.12) : Qt.rgba(0, 0, 0, 0.25))
        border.width: light ? 0 : 1
        border.color: Qt.rgba(1, 1, 1, 0.35)
        Text { id: bTxt; anchors.centerIn: parent; text: b.text; color: b.light ? "#0e0e10" : "#ffffff"; font.pixelSize: 14; font.weight: Font.DemiBold; font.family: Theme.fontSans }
        MouseArea { id: bMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: b.clicked() }
    }
}
