// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Network: the verdict first, in words, with what it means and why; the
// fixes as buttons; then the parts it is made of. Reads the same answer as
// the status bar's port line (session.portStatus), so the two agree.
import QtQuick
import QtQuick.Layouts
import "../theme"
import "../widgets"

Window {
    id: win
    Shortcut { sequences: [StandardKey.Cancel]; onActivated: win.close() }
    width: 600
    height: 560
    minimumWidth: 520
    minimumHeight: 480
    color: Theme.elev
    flags: Theme.unifiedChrome ? (Qt.Window | Qt.ExpandedClientAreaHint | Qt.NoTitleBarBackgroundHint) : Qt.Window
    title: (i18n.language, i18n.t("net_title"))

    readonly property bool frameless: typeof framelessWindow !== "undefined" && framelessWindow === true
    Loader {
        id: framelessLoader
        active: win.frameless
        source: win.frameless ? "../AppFramelessAgent.qml" : ""
        onLoaded: { item.window = win; item.navBar = titleBar }
    }
    function prepareFrame() { if (framelessLoader.item) framelessLoader.item.attach() }

    readonly property var sess: typeof session !== "undefined" ? session : null
    property var diag: ({})
    property real checkedAt: 0
    property int tick: 0
    property bool retrying: false
    function run() {
        if (!sess) return
        diag = sess.diagnostics()
        checkedAt = Date.now()
    }
    onVisibleChanged: if (visible) run()
    Timer { interval: 1000; repeat: true; running: win.visible; onTriggered: win.tick++ }
    // after asking the router again, look a few seconds later
    Timer { id: retryTimer; interval: 6000; onTriggered: { win.retrying = false; win.run() } }

    readonly property bool listening: diag.port > 0
    readonly property bool reachable: diag.status === 1
    readonly property bool vpnOn: typeof vpn !== "undefined" && vpn !== null && vpn.connectedInterface !== undefined && vpn.connectedInterface.length > 0
    // 0 fine, 1 reachable only one way, 2 not listening
    readonly property int verdict: !listening ? 2 : reachable ? 0 : 1

    function since(ms) {
        win.tick
        var s = Math.max(0, Math.floor((Date.now() - ms) / 1000))
        return s < 60 ? i18n.t("net_checked_s").arg(s) : i18n.t("net_checked_m").arg(Math.floor(s / 60))
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        WindowTitleBar {
            id: titleBar
            Layout.fillWidth: true
            Layout.preferredHeight: implicitHeight
            appWindow: win
            ownsTitleBar: win.frameless
            title: win.title
        }

        Flickable {
            id: flick
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            contentWidth: width
            contentHeight: body.implicitHeight + 40
            boundsBehavior: Flickable.StopAtBounds

            ColumnLayout {
                id: body
                x: 22; y: 22
                width: flick.width - 44
                spacing: 16

                // ---- the verdict ----
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 14
                    Rectangle {
                        Layout.alignment: Qt.AlignTop
                        width: 34; height: 34; radius: 17
                        color: win.verdict === 0 ? Qt.rgba(Theme.grn.r, Theme.grn.g, Theme.grn.b, 0.16)
                             : Qt.rgba(Theme.warn.r, Theme.warn.g, Theme.warn.b, 0.16)
                        Text {
                            anchors.centerIn: parent
                            text: win.verdict === 0 ? "✓" : "!"
                            color: win.verdict === 0 ? Theme.grn : Theme.warn
                            font.pixelSize: 17; font.weight: Font.Bold; font.family: Theme.fontSans
                        }
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 6
                        Text {
                            Layout.fillWidth: true
                            text: (i18n.language, i18n.t(["net_v_ok", "net_v_blocked", "net_v_closed"][win.verdict]))
                            color: Theme.t1; font.pixelSize: 19; font.weight: Font.DemiBold; font.family: Theme.fontSans
                            wrapMode: Text.WordWrap
                        }
                        Text {
                            Layout.fillWidth: true
                            text: (i18n.language, i18n.t(["net_v_ok_d", "net_v_blocked_d", "net_v_closed_d"][win.verdict]).arg(win.diag.port || 0))
                            color: Theme.t2; font.pixelSize: 14; font.family: Theme.fontSans
                            wrapMode: Text.WordWrap; lineHeight: 1.4
                        }
                        Row {
                            Layout.topMargin: 6
                            visible: win.verdict !== 0
                            spacing: 8
                            BtnFlat {
                                primary: true
                                visible: win.verdict === 1
                                opacity: win.retrying ? 0.6 : 1
                                text: (i18n.language, i18n.t(win.retrying ? "net_retrying" : "net_try_upnp"))
                                onClicked: {
                                    if (win.retrying || !win.sess) return
                                    win.retrying = true
                                    win.sess.retryPortMapping()
                                    retryTimer.restart()
                                }
                            }
                            BtnFlat {
                                text: (i18n.language, i18n.t("net_by_hand")) + " ↗"
                                onClicked: Qt.openUrlExternally("https://portforward.com/")
                            }
                        }
                    }
                }

                // ---- the parts ----
                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: parts.implicitHeight
                    radius: Theme.radCard(10)
                    color: "transparent"
                    border.color: Theme.hair; border.width: 1
                    Column {
                        id: parts
                        width: parent.width
                        Repeater {
                            model: [
                                { k: "net_listen", ok: win.listening ? 0 : 2,
                                  d: win.listening ? "net_listen_ok" : "net_listen_no",
                                  v: win.listening ? i18n.t("net_val_port").arg(win.diag.port) : "—" },
                                { k: "net_reach", ok: win.reachable ? 0 : 1,
                                  d: win.reachable ? (win.diag.incoming ? "net_reach_in" : "net_reach_map") : "net_reach_no",
                                  v: win.reachable ? i18n.t("net_val_open") : i18n.t("net_val_blocked") },
                                { k: "net_dht", ok: !win.diag.dhtOn ? 2 : win.diag.dhtNodes > 0 ? 0 : 1,
                                  d: !win.diag.dhtOn ? "net_dht_off" : win.diag.dhtNodes > 0 ? "net_dht_ok" : "net_dht_warm",
                                  v: win.diag.dhtOn ? i18n.t("net_val_nodes").arg(win.diag.dhtNodes || 0) : i18n.t("net_val_off") },
                                { k: "net_peers", ok: win.diag.peers > 0 ? 0 : 3,
                                  d: win.diag.peers > 0 ? (win.diag.incoming ? "net_peers_both" : "net_peers_out") : "net_peers_none",
                                  v: String(win.diag.peers || 0) },
                                { k: "net_vpn", ok: win.vpnOn ? 0 : 3,
                                  d: win.vpnOn ? "net_vpn_on" : "net_vpn_off",
                                  v: win.vpnOn ? vpn.connectedInterface : i18n.t("net_val_off") }
                            ]
                            delegate: Item {
                                required property var modelData
                                required property int index
                                width: parts.width
                                height: 56
                                Rectangle { visible: index > 0; width: parent.width; height: 1; color: Theme.hairSoft }
                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: 14; anchors.rightMargin: 14
                                    spacing: 12
                                    Rectangle {
                                        width: 7; height: 7; radius: 3.5
                                        color: modelData.ok === 0 ? Theme.grn : modelData.ok === 1 ? Theme.warn
                                             : modelData.ok === 2 ? Theme.down : Theme.t4
                                    }
                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        spacing: 2
                                        Text { text: (i18n.language, i18n.t(modelData.k)); color: Theme.t1; font.pixelSize: 14; font.weight: Font.DemiBold; font.family: Theme.fontSans }
                                        Text { Layout.fillWidth: true; text: (i18n.language, i18n.t(modelData.d)); color: Theme.t3; font.pixelSize: 12; font.family: Theme.fontSans; elide: Text.ElideRight }
                                    }
                                    Text {
                                        text: (i18n.language, modelData.v)
                                        color: modelData.ok === 1 ? Theme.warn : modelData.ok === 2 ? Theme.downText : Theme.t2
                                        font.pixelSize: 12; font.family: Theme.fontMono
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 56
            Rectangle { width: parent.width; height: 1; color: Theme.hairSoft }
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 22; anchors.rightMargin: 14
                Text {
                    Layout.fillWidth: true
                    text: (i18n.language, win.checkedAt > 0 ? win.since(win.checkedAt) : "")
                    color: Theme.t3; font.pixelSize: 13; font.family: Theme.fontSans; font.features: Theme.tnum
                }
                BtnFlat { text: (i18n.language, i18n.t("net_run_again")); onClicked: win.run() }
            }
        }
    }
}
