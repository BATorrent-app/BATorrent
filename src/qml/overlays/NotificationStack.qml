// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Notices inside the window: bottom right, above the status bar, three at
// most and the rest counted toward the Overview. One signal per notice, a dot
// in the title: green when something is ready, orange when it needs the
// person. Never red; border, text and icon stay neutral. A ready notice goes
// after 6 seconds (held while the pointer is on it); one that needs the
// person stays until they act or close it. What leaves is still in the
// Overview, which reads the same state.
//
// While a game or a video fills the screen (and "Quiet in full screen" is
// on) notices wait, whether they were going to this window or to the
// system's, and come back as one summary (summaryReady).
import QtQuick
import QtQuick.Layouts
import "../theme"
import "../widgets"

Item {
    id: stack
    anchors.fill: parent

    // the status bar's height, so the stack sits above it
    property int bottomInset: 30
    // action: { id, hash?, path? }, as given in the notice's actions
    signal actionTriggered(var action)
    // the held notices, once the full screen is gone: title, body, count
    signal summaryReady(string title, string body)

    ListModel { id: notes }
    property int uid: 0
    readonly property int maxShown: 3
    readonly property int hidden: Math.max(0, notes.count - maxShown)

    function settingOn(key, def) { return typeof settings === "undefined" ? def : settings.getBool(key, def) }
    // the Notices group in Settings: one switch per kind
    function wanted(category) {
        return category === "ready" ? settingOn("notifyReady", true)
             : category === "needs" ? settingOn("notifyNeeds", true)
             : category === "rss" ? settingOn("notifyRss", true)
             : category === "version" ? settingOn("notifyNewVersion", true) : true
    }

    // n: { tone: "ready" | "needs" | "", category: "ready" | "needs" | "rss" | "version" | "",
    //      title, body, poster, tag, actions: [{ id, label, hash?, path? }] (the first one is primary) }
    function push(n) {
        if (!wanted(n.category || "")) return
        if (hold(n)) return
        var a = n.actions || []
        notes.append({
            uid: ++uid,
            tone: n.tone || "",
            title: n.title || "",
            body: n.body || "",
            poster: n.poster || "",
            tag: n.tag || "",
            // a ListModel keeps plain values: the actions travel as JSON
            a1: a.length > 0 ? JSON.stringify(a[0]) : "", a1Label: a.length > 0 ? a[0].label : "",
            a2: a.length > 1 ? JSON.stringify(a[1]) : "", a2Label: a.length > 1 ? a[1].label : "",
            sticky: n.tone === "needs"
        })
    }
    // Kept for its quiet: true when the notice waits for the full screen to end.
    function hold(n) {
        if (!quiet) return false
        held.push(n)
        return true
    }
    // the old call: level 3 ready, 1 and 2 need the person, 0 neither
    function show(title, body, level, actionId, actionText) {
        push({ tone: level === 3 ? "ready" : (level === 1 || level === 2) ? "needs" : "",
               category: level === 3 ? "ready" : (level === 1 || level === 2) ? "needs" : "",
               title: title, body: body,
               actions: actionId ? [{ id: actionId, label: actionText }] : [] })
    }
    function remove(id) {
        for (var i = 0; i < notes.count; ++i)
            if (notes.get(i).uid === id) { notes.remove(i); return }
    }
    function trigger(json, id) {
        remove(id)
        if (json) actionTriggered(JSON.parse(json))
    }

    // ---- quiet in full screen ----
    property var held: []
    property bool fullscreenNow: false
    readonly property bool quiet: fullscreenNow && settingOn("quietFullscreen", true)
    Timer {
        interval: 3000; repeat: true; running: true
        onTriggered: stack.fullscreenNow = typeof themeBridge !== "undefined" && themeBridge.exclusiveFullscreenInFront()
    }
    onQuietChanged: if (!quiet && held.length > 0) {
        var n = held.length, titles = held.map(function (h) { return h.title }).slice(0, 3)
        held = []
        summaryReady(i18n.t("notif_while_away").arg(n), titles.join(" · "))
    }

    Column {
        anchors.right: parent.right
        anchors.rightMargin: 16
        anchors.bottom: parent.bottom
        anchors.bottomMargin: stack.bottomInset + 16
        spacing: 10
        // newest at the bottom, nearest the eye
        Repeater {
            model: notes
            delegate: Notice {
                visible: index >= notes.count - stack.maxShown
                height: visible ? implicitHeight : 0
            }
        }
        Rectangle {
            visible: stack.hidden > 0
            x: 380 - width
            width: moreTxt.implicitWidth + 24; height: 30
            radius: Theme.rad(999)
            color: moreMa.containsMouse ? Theme.hover : Theme.elev
            border.color: Theme.hair; border.width: 1
            Text {
                id: moreTxt
                anchors.centerIn: parent
                text: (i18n.language, i18n.t("notif_more")).arg(stack.hidden)
                color: Theme.t2; font.pixelSize: 13; font.family: Theme.fontSans; font.features: Theme.tnum
            }
            MouseArea { id: moreMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: stack.actionTriggered({ id: "overview" }) }
        }
    }

    component Notice: Rectangle {
        id: card
        required property var model
        required property int index
        width: 380
        implicitHeight: body.implicitHeight + 28
        radius: Theme.rad(10)
        color: Theme.elev
        border.color: Theme.hair; border.width: 1
        // in from the right, once
        property real enter: 0
        opacity: enter
        transform: Translate { x: Theme.reduceMotion ? 0 : 24 * (1 - card.enter) }
        Component.onCompleted: enter = 1
        Behavior on enter { NumberAnimation { duration: Theme.durSlow; easing.type: Theme.easeOut } }

        HoverHandler { id: hover }
        Timer {
            interval: 6000
            running: !model.sticky && !hover.hovered && card.visible
            onTriggered: stack.remove(model.uid)
        }

        RowLayout {
            id: body
            x: 14; y: 14
            width: parent.width - 28
            spacing: 12
            Rectangle {
                Layout.alignment: Qt.AlignTop
                visible: model.poster !== "" || model.tag !== ""
                Layout.preferredWidth: 40; Layout.preferredHeight: 56
                radius: Theme.rad(4)
                color: Theme.hover
                Text { anchors.centerIn: parent; visible: model.poster === ""; text: model.tag; color: Theme.t3; font.pixelSize: 11; font.weight: Font.Bold; font.capitalization: Font.AllUppercase; font.family: Theme.fontSans }
                RoundedImage { anchors.fill: parent; visible: model.poster !== ""; radius: Theme.rad(4); sources: model.poster !== "" ? [model.poster] : []; sourceWidth: 80 }
            }
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 4
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    Rectangle {
                        visible: model.tone !== ""
                        width: 7; height: 7; radius: 3.5
                        color: model.tone === "ready" ? Theme.grn : Theme.warn
                    }
                    Text {
                        Layout.fillWidth: true
                        text: model.title
                        color: Theme.t1; font.pixelSize: 14; font.weight: Font.DemiBold; font.family: Theme.fontSans
                        elide: Text.ElideRight
                    }
                    CloseButton { size: 22; iconSize: 11; onClicked: stack.remove(model.uid) }
                }
                Text {
                    Layout.fillWidth: true
                    visible: model.body !== ""
                    text: model.body
                    color: Theme.t3; font.pixelSize: 13; font.family: Theme.fontSans; font.features: Theme.tnum
                    wrapMode: Text.WordWrap; maximumLineCount: 2; elide: Text.ElideRight
                }
                Row {
                    Layout.topMargin: 6
                    visible: model.a1 !== ""
                    spacing: 8
                    // the primary is light, never red: red is for downloading and errors
                    LightBtn {
                        height: 28
                        text: model.a1Label
                        onClicked: stack.trigger(model.a1, model.uid)
                    }
                    BtnFlat {
                        visible: model.a2 !== ""
                        sm: true
                        text: model.a2Label
                        onClicked: stack.trigger(model.a2, model.uid)
                    }
                }
            }
        }
    }
}
