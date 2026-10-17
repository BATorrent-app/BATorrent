// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// The VPN page. First the answer to "am I protected?": one block in the
// state's colour (off grey, connecting amber, protected green, dropped red)
// with the interface the torrents leave through and what the kill switch is
// doing. Then the WireGuard profiles (one click connects, drop a .conf to
// import), what happens when the VPN drops, the interfaces by the names
// Windows gives them, and the advanced options folded away.
// Contexts: `vpn` (VpnManager, may be absent in Store builds), `settings`.
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import QtQuick.Dialogs
import "../theme"
import "../widgets"

ColumnLayout {
    id: page
    spacing: 28

    readonly property bool hasVpn: typeof vpn !== "undefined" && vpn !== null
    property var ifaces: []
    property string bound: ""
    property bool killSwitch: true
    property bool autoResume: true
    function refresh() {
        if (typeof settings === "undefined") return
        ifaces = settings.networkInterfaceList()
        bound = settings.get("outgoingInterface") || ""
        killSwitch = settings.getBool("killSwitchEnabled", true)
        autoResume = settings.getBool("autoResumeOnReconnect", true)
    }
    Component.onCompleted: refresh()
    Connections {
        target: typeof settings !== "undefined" ? settings : null
        function onChanged() { page.refresh() }
    }
    Timer { interval: 4000; repeat: true; running: page.visible; onTriggered: page.refresh() }

    function ifaceById(id) {
        for (var i = 0; i < ifaces.length; i++) if (ifaces[i].id === id) return ifaces[i]
        return null
    }
    readonly property var boundIface: bound.length > 0 ? ifaceById(bound) : null
    readonly property int wgState: hasVpn ? vpn.connState : 0
    // 0 off · 1 connecting · 2 protected · 3 dropped
    readonly property int st: wgState === 1 ? 1
                            : wgState === 3 ? 3
                            : wgState === 2 ? 2
                            : bound.length === 0 ? 0
                            : boundIface ? (boundIface.vpn ? 2 : 0)
                            : 3   // bound to an interface that is gone: the VPN dropped
    readonly property color stColor: st === 2 ? Theme.grn : st === 1 ? Theme.amber : st === 3 ? Theme.accentText : Theme.t2
    readonly property var activeProfile: {
        if (!hasVpn) return null
        for (var i = 0; i < vpn.profiles.length; ++i)
            if (vpn.profiles[i].id === vpn.activeProfileId) return vpn.profiles[i]
        return null
    }
    function profileLabel(p) { return p ? (p.endpoint && p.endpoint.length ? p.name + " · " + p.endpoint : p.name) : "" }
    function ifaceLabel(i) { return i ? (i.ip.length > 0 ? i.label + " · " + i.ip : i.label) : "" }

    component Card: Rectangle {
        Layout.fillWidth: true
        radius: Theme.rad(14)
        color: Theme.panel
        border.color: Theme.hairSoft
        border.width: 1
        clip: true
    }
    component SectionHead: ColumnLayout {
        property string title
        property string sub
        Layout.fillWidth: true
        spacing: 2
        Text { text: parent.title; color: Theme.t1; font.pixelSize: 18; font.weight: Font.DemiBold; font.family: Theme.fontSans }
        Text { visible: parent.sub.length > 0; Layout.fillWidth: true; text: parent.sub; color: Theme.t3; font.pixelSize: 14; font.family: Theme.fontSans; wrapMode: Text.WordWrap }
    }

    // ---- the answer ----
    Rectangle {
        Layout.fillWidth: true
        radius: Theme.rad(14)
        color: Theme.panel
        border.width: 1
        border.color: page.st === 2 ? Qt.rgba(Theme.grn.r, Theme.grn.g, Theme.grn.b, 0.25)
                    : page.st === 1 ? Qt.rgba(Theme.amber.r, Theme.amber.g, Theme.amber.b, 0.3)
                    : page.st === 3 ? Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.35) : Theme.hair
        implicitHeight: stCol.implicitHeight
        Behavior on border.color { ColorAnimation { duration: Theme.durBase } }
        ColumnLayout {
            id: stCol
            anchors.left: parent.left; anchors.right: parent.right
            spacing: 0
            RowLayout {
                Layout.fillWidth: true
                Layout.margins: 24
                Layout.bottomMargin: 20
                spacing: 20
                Rectangle {
                    Layout.preferredWidth: 52; Layout.preferredHeight: 52; radius: Theme.rad(26)
                    color: Qt.rgba(page.stColor.r, page.stColor.g, page.stColor.b, page.st === 0 ? 0.06 : 0.12)
                    IconImg { anchors.centerIn: parent; src: "qrc:/icons/set-vpn.svg"; s: 26; tint: page.stColor }
                    Rectangle {
                        anchors.fill: parent; radius: Theme.rad(26)
                        color: "transparent"; border.color: Theme.amber; border.width: 2
                        visible: page.st === 1
                        SequentialAnimation on opacity {
                            running: page.st === 1; loops: Animation.Infinite
                            NumberAnimation { from: 0.8; to: 0.0; duration: 900; easing.type: Theme.easeIn }
                        }
                    }
                }
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 4
                    Text {
                        text: (i18n.language, i18n.t(page.st === 2 ? "vpn_hero_protected" : page.st === 1 ? "vpn_state_connecting"
                                                   : page.st === 3 ? "vpn_dropped" : "vpn_off_title"))
                        color: page.stColor
                        font.pixelSize: 24; font.weight: Font.Bold; font.family: Theme.fontSans
                    }
                    Text {
                        Layout.fillWidth: true
                        text: (i18n.language, page.st === 2 ? (page.activeProfile && page.wgState === 2
                                                               ? page.profileLabel(page.activeProfile)
                                                                 + (vpn.forwardedPort > 0 ? " · " + i18n.t("vpn_port_open").arg(vpn.forwardedPort) : "")
                                                             : i18n.t("vpn_via").arg(page.ifaceLabel(page.boundIface)))
                              : page.st === 1 ? i18n.t("vpn_bringing_up").arg(page.profileLabel(page.activeProfile))
                              : page.st === 3 ? (page.hasVpn && vpn.lastError.length > 0 ? vpn.lastError : i18n.t("vpn_dropped_sub"))
                                                + (page.hasVpn && vpn.retryIn > 0 ? " " + i18n.t("vpn_retry_in").arg(vpn.retryIn) : "")
                              : i18n.t("vpn_off_sub"))
                        color: Theme.t2; font.pixelSize: 14; font.family: Theme.fontSans; font.features: Theme.tnum
                        wrapMode: Text.WordWrap
                    }
                }
                BtnFlat {
                    readonly property bool canConnect: page.hasVpn && vpn.profiles.length > 0
                    visible: page.st === 1 || page.wgState === 2 || ((page.st === 0 || page.st === 3) && canConnect)
                    lg: true
                    primary: page.st === 0 || page.st === 3
                    text: (i18n.language, page.st === 1 ? i18n.t("btn_cancel")
                          : page.wgState === 2 ? i18n.t("vpn_disconnect")
                          : page.st === 3 ? i18n.t("vpn_try_again")
                          : i18n.t("vpn_connect_named").arg((page.activeProfile || vpn.profiles[0]).name))
                    onClicked: {
                        if (page.st === 1 || page.wgState === 2) { vpn.disconnectVpn(); return }
                        vpn.connectProfile(vpn.activeProfileId !== "" ? vpn.activeProfileId : vpn.profiles[0].id)
                    }
                }
            }
            Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: Theme.hairSoft }
            RowLayout {
                Layout.fillWidth: true
                spacing: 0
                Repeater {
                    model: [
                        { k: "vpn_fact_torrents",
                          v: page.st === 2 ? i18n.t("vpn_fact_through_vpn") : page.st === 3 ? i18n.t("vpn_fact_blocked") : i18n.t("vpn_fact_real_ip"),
                          ink: page.st === 0 ? Theme.accentText : Theme.t1 },
                        { k: "vpn_fact_iface",
                          // the app's own tunnel, while it is up, before any adapter chosen below
                          v: page.wgState === 2 && vpn.connectedInterface.length > 0 ? vpn.connectedInterface
                             : page.boundIface ? page.ifaceLabel(page.boundIface)
                             : (page.hasVpn && vpn.connectedInterface.length > 0 ? vpn.connectedInterface
                             : (page.bound.length > 0 ? i18n.t("vpn_fact_iface_gone").arg(page.bound) : i18n.t("settings_iface_any"))),
                          ink: page.st === 3 ? Theme.accentText : Theme.t1 },
                        { k: "vpn_fact_killswitch",
                          v: !page.killSwitch ? i18n.t("vpn_ks_off") : page.st === 0 ? i18n.t("vpn_ks_idle")
                             : page.st === 3 ? i18n.t("vpn_ks_paused") : i18n.t("vpn_ks_on"),
                          ink: page.killSwitch && page.st === 2 ? Theme.grn : (page.st === 0 || !page.killSwitch ? Theme.t4 : Theme.t1) }
                    ]
                    Item {
                        required property var modelData
                        required property int index
                        Layout.fillWidth: true
                        Layout.preferredWidth: 1
                        implicitHeight: factCol.implicitHeight + 28
                        Rectangle { visible: index > 0; width: 1; height: parent.height; color: Theme.hairSoft }
                        ColumnLayout {
                            id: factCol
                            anchors.left: parent.left; anchors.right: parent.right
                            anchors.leftMargin: 24; anchors.rightMargin: 16
                            anchors.verticalCenter: parent.verticalCenter
                            spacing: 3
                            Text { text: (i18n.language, i18n.t(modelData.k)); color: Theme.t4; font.pixelSize: 11; font.weight: Font.DemiBold; font.letterSpacing: 0.9; font.capitalization: Font.AllUppercase; font.family: Theme.fontSans }
                            Text { Layout.fillWidth: true; text: (i18n.language, modelData.v); color: modelData.ink; font.pixelSize: 14; font.family: Theme.fontSans; font.features: Theme.tnum; elide: Text.ElideRight }
                        }
                    }
                }
            }
        }
    }

    // the in-app tunnel is not real yet: say so where it matters
    Rectangle {
        visible: page.hasVpn && !vpn.tunnelReal && vpn.profiles.length > 0
        Layout.fillWidth: true
        radius: Theme.rad(10)
        color: Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.08)
        border.color: Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.25); border.width: 1
        implicitHeight: warnRow.implicitHeight + 24
        RowLayout {
            id: warnRow
            anchors.left: parent.left; anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter
            anchors.leftMargin: 16; anchors.rightMargin: 16
            spacing: 10
            IconImg { Layout.alignment: Qt.AlignTop; Layout.topMargin: 2; src: "qrc:/icons/triangle-alert.svg"; tint: Theme.accentText; s: 16 }
            Text { Layout.fillWidth: true; text: (i18n.language, i18n.t("vpn_stub_warning")); color: Theme.tSoft; font.pixelSize: 14; font.family: Theme.fontSans; wrapMode: Text.WordWrap; lineHeight: 1.4 }
        }
    }

    // ---- profiles ----
    ColumnLayout {
        visible: page.hasVpn
        Layout.fillWidth: true
        spacing: 12
        RowLayout {
            Layout.fillWidth: true
            spacing: 12
            SectionHead { title: (i18n.language, i18n.t("vpn_profiles")); sub: (i18n.language, i18n.t("vpn_profiles_sub")) }
            BtnFlat { Layout.alignment: Qt.AlignBottom; icon: "qrc:/icons/plus.svg"; text: (i18n.language, i18n.t("vpn_import_wg")); onClicked: vpnFileDlg.open() }
        }
        Card {
            implicitHeight: profCol.implicitHeight
            DropArea {
                id: confDrop
                anchors.fill: parent
                function accepts(d) {
                    if (!d.hasUrls) return false
                    for (var i = 0; i < d.urls.length; ++i)
                        if (d.urls[i].toString().toLowerCase().endsWith(".conf")) return true
                    return false
                }
                onEntered: function(drag) { drag.accepted = accepts(drag) }
                onDropped: function(drop) {
                    for (var i = 0; i < drop.urls.length; ++i) {
                        var u = drop.urls[i].toString()
                        if (u.toLowerCase().endsWith(".conf")) vpn.importFromFile(u, "")
                    }
                    drop.accept()
                }
            }
            ColumnLayout {
                id: profCol
                anchors.left: parent.left; anchors.right: parent.right
                spacing: 0
                Repeater {
                    model: page.hasVpn ? vpn.profiles : []
                    delegate: Rectangle {
                        id: profRow
                        required property var modelData
                        required property int index
                        property bool editing: false
                        readonly property bool active: vpn.activeProfileId === modelData.id && (vpn.connState === 1 || vpn.connState === 2 || vpn.connState === 3)
                        Layout.fillWidth: true
                        implicitHeight: 60
                        color: active ? Theme.hover : (rowMa.containsMouse ? Qt.rgba(1, 1, 1, 0.03) : "transparent")
                        Rectangle { visible: index > 0; width: parent.width; height: 1; color: Theme.hairSoft }
                        function commitRename() { vpn.renameProfile(modelData.id, nameEdit.text); profRow.editing = false }
                        MouseArea { id: rowMa; anchors.fill: parent; hoverEnabled: true; acceptedButtons: Qt.NoButton }
                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 20; anchors.rightMargin: 20
                            spacing: 14
                            Rectangle { Layout.preferredWidth: 2; Layout.preferredHeight: 28; radius: Theme.rad(1); color: profRow.active ? Theme.t1 : "transparent" }
                            Rectangle {
                                Layout.preferredWidth: 34; Layout.preferredHeight: 24; radius: Theme.rad(4)
                                color: Theme.hover2
                                Text { anchors.centerIn: parent; text: (profRow.modelData.cc || "··").toUpperCase(); color: Theme.tSoft; font.pixelSize: 11; font.weight: Font.Bold; font.family: Theme.fontSans }
                            }
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 2
                                Text { visible: !profRow.editing; Layout.fillWidth: true; text: profRow.modelData.name; color: Theme.t1; font.pixelSize: 15; font.weight: Font.Medium; font.family: Theme.fontSans; elide: Text.ElideRight }
                                TextField {
                                    id: nameEdit
                                    visible: profRow.editing
                                    Layout.fillWidth: true
                                    text: profRow.modelData.name
                                    color: Theme.t1; font.pixelSize: 14; font.family: Theme.fontSans
                                    padding: 2; selectByMouse: true
                                    background: Rectangle { color: "transparent"; border.color: Theme.accent; border.width: 1; radius: Theme.rad(4) }
                                    onAccepted: profRow.commitRename()
                                    onActiveFocusChanged: if (!activeFocus && profRow.editing) profRow.commitRename()
                                    Keys.onEscapePressed: { text = profRow.modelData.name; profRow.editing = false }
                                }
                                Text { Layout.fillWidth: true; text: (profRow.modelData.endpoint || "") + " · WireGuard"; color: Theme.t4; font.pixelSize: 13; font.family: Theme.fontSans; elide: Text.ElideRight }
                            }
                            Text {
                                text: (i18n.language, i18n.t("vpn_rename"))
                                color: renMa.containsMouse ? Theme.t1 : Theme.t4; font.pixelSize: 13; font.family: Theme.fontSans
                                MouseArea { id: renMa; anchors.fill: parent; anchors.margins: -4; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                                    onClicked: { profRow.editing = true; nameEdit.forceActiveFocus(); nameEdit.selectAll() } }
                            }
                            Text {
                                text: (i18n.language, i18n.t("vpn_remove"))
                                color: rmMa.containsMouse ? Theme.accentText : Theme.t4; font.pixelSize: 13; font.family: Theme.fontSans
                                MouseArea { id: rmMa; anchors.fill: parent; anchors.margins: -4; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                                    onClicked: vpn.removeProfile(profRow.modelData.id) }
                            }
                            Text {
                                Layout.preferredWidth: 96
                                horizontalAlignment: Text.AlignRight
                                text: (i18n.language, !profRow.active ? i18n.t("vpn_connect")
                                      : vpn.connState === 3 ? i18n.t("vpn_state_failed")
                                      : vpn.connState === 1 ? i18n.t("vpn_state_connecting") : i18n.t("vpn_disconnect"))
                                color: profRow.active && vpn.connState === 3 ? Theme.accentText
                                     : profRow.active && vpn.connState === 1 ? Theme.amber
                                     : (goMa.containsMouse ? Theme.t1 : Theme.tSoft)
                                font.pixelSize: 13; font.weight: Font.DemiBold; font.family: Theme.fontSans
                                MouseArea { id: goMa; anchors.fill: parent; anchors.margins: -6; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                                    onClicked: profRow.active && vpn.connState !== 3 ? vpn.disconnectVpn() : vpn.connectProfile(profRow.modelData.id) }
                            }
                        }
                    }
                }
                Rectangle {
                    Layout.fillWidth: true
                    Layout.margins: 12
                    implicitHeight: 52
                    radius: Theme.rad(10)
                    color: confDrop.containsDrag ? Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.10) : "transparent"
                    border.color: confDrop.containsDrag ? Theme.accent : Theme.hair
                    border.width: 1
                    Text { anchors.centerIn: parent; text: (i18n.language, i18n.t("vpn_drop_conf_here")); color: Theme.t4; font.pixelSize: 14; font.family: Theme.fontSans }
                }
            }
        }
    }

    // ---- protection ----
    ColumnLayout {
        Layout.fillWidth: true
        spacing: 12
        SectionHead { title: (i18n.language, i18n.t("vpn_protection")); sub: (i18n.language, i18n.t("vpn_protection_sub")) }
        Card {
            implicitHeight: protCol.implicitHeight
            ColumnLayout {
                id: protCol
                anchors.left: parent.left; anchors.right: parent.right
                spacing: 0
                Repeater {
                    model: [
                        { key: "killSwitchEnabled", l: "vpn_fact_killswitch", d: "set_killswitch_note", on: page.killSwitch },
                        { key: "autoResumeOnReconnect", l: "settings_auto_resume", d: "set_autoresume_note", on: page.autoResume }
                    ]
                    Item {
                        required property var modelData
                        required property int index
                        Layout.fillWidth: true
                        implicitHeight: tCol.implicitHeight + 32
                        Rectangle { visible: index > 0; width: parent.width; height: 1; color: Theme.hairSoft }
                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 20; anchors.rightMargin: 20
                            spacing: 24
                            ColumnLayout {
                                id: tCol
                                Layout.fillWidth: true
                                spacing: 3
                                Text { text: (i18n.language, i18n.t(modelData.l)); color: Theme.t1; font.pixelSize: 15; font.weight: Font.Medium; font.family: Theme.fontSans }
                                Text { Layout.fillWidth: true; text: (i18n.language, i18n.t(modelData.d)); color: Theme.t3; font.pixelSize: 14; font.family: Theme.fontSans; wrapMode: Text.WordWrap; lineHeight: 1.35 }
                            }
                            TToggle { big: true; on: modelData.on; onToggled: function (v) { settings.set(modelData.key, v) } }
                        }
                    }
                }
                Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: Theme.hairSoft }
                RowLayout {
                    Layout.fillWidth: true
                    Layout.margins: 20
                    Layout.topMargin: 16; Layout.bottomMargin: 16
                    spacing: 24
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 3
                        Text { text: (i18n.language, i18n.t("set_iface2")); color: Theme.t1; font.pixelSize: 15; font.weight: Font.Medium; font.family: Theme.fontSans }
                        Text { Layout.fillWidth: true; text: (i18n.language, i18n.t("vpn_iface_only")); color: Theme.t3; font.pixelSize: 14; font.family: Theme.fontSans; wrapMode: Text.WordWrap }
                    }
                    TSelect {
                        implicitWidth: 280
                        implicitHeight: 40
                        font.pixelSize: 14
                        model: [(i18n.language, i18n.t("settings_iface_any"))].concat(page.ifaces.map(function (i) { return page.ifaceLabel(i) }))
                        currentIndex: {
                            if (page.bound.length === 0) return 0
                            for (var i = 0; i < page.ifaces.length; i++) if (page.ifaces[i].id === page.bound) return i + 1
                            return 0
                        }
                        onActivated: function (i) { settings.set("outgoingInterface", i === 0 ? "" : page.ifaces[i - 1].id) }
                    }
                }
            }
        }
        // the interfaces, by the names Windows shows; the technical id on hover
        Card {
            implicitHeight: ifCol.implicitHeight + 20
            ColumnLayout {
                id: ifCol
                anchors.left: parent.left; anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: 10
                anchors.leftMargin: 12; anchors.rightMargin: 12
                spacing: 0
                Text { Layout.leftMargin: 8; Layout.topMargin: 2; Layout.bottomMargin: 6; text: (i18n.language, i18n.t("vpn_ifaces_available")); color: Theme.t4; font.pixelSize: 11; font.weight: Font.DemiBold; font.letterSpacing: 0.9; font.capitalization: Font.AllUppercase; font.family: Theme.fontSans }
                Repeater {
                    model: page.ifaces
                    Rectangle {
                        required property var modelData
                        Layout.fillWidth: true
                        implicitHeight: 34
                        radius: Theme.rad(7)
                        color: modelData.id === page.bound ? Theme.hover2 : "transparent"
                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 8; anchors.rightMargin: 8
                            spacing: 10
                            Rectangle { Layout.preferredWidth: 7; Layout.preferredHeight: 7; radius: Theme.rad(3.5); color: modelData.vpn ? Theme.grn : Theme.warn }
                            Text { Layout.fillWidth: true; text: page.ifaceLabel(modelData); color: Theme.t1; font.pixelSize: 14; font.family: Theme.fontSans; elide: Text.ElideRight }
                            Text { text: (i18n.language, modelData.vpn ? "VPN" : i18n.t("vpn_iface_not_protecting")); color: modelData.vpn ? Theme.t4 : Theme.warn; font.pixelSize: 12; font.family: Theme.fontSans }
                        }
                        MouseArea { id: ifMa; anchors.fill: parent; hoverEnabled: true; acceptedButtons: Qt.NoButton }
                        ToolTip.visible: ifMa.containsMouse
                        ToolTip.text: modelData.id
                        ToolTip.delay: 500
                    }
                }
            }
        }
    }

    // ---- advanced, folded ----
    Card {
        id: adv
        property bool open: false
        implicitHeight: advCol.implicitHeight
        ColumnLayout {
            id: advCol
            anchors.left: parent.left; anchors.right: parent.right
            spacing: 0
            Item {
                Layout.fillWidth: true
                implicitHeight: 48
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 20; anchors.rightMargin: 20
                    spacing: 10
                    Text { Layout.fillWidth: true; text: (i18n.language, i18n.t("vpn_advanced")); color: Theme.t1; font.pixelSize: 15; font.weight: Font.Medium; font.family: Theme.fontSans }
                    Text { text: (i18n.language, i18n.t("vpn_advanced_hint")); color: Theme.t4; font.pixelSize: 13; font.family: Theme.fontSans }
                    IconImg { src: "qrc:/icons/chevron.svg"; s: 11; tint: Theme.t3; rotation: adv.open ? 0 : -90
                        Behavior on rotation { NumberAnimation { duration: Theme.durFast } } }
                }
                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: adv.open = !adv.open }
            }
            Item {
                visible: adv.open
                Layout.fillWidth: true
                implicitHeight: torCol.implicitHeight + 32
                Rectangle { width: parent.width; height: 1; color: Theme.hairSoft }
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 20; anchors.rightMargin: 20
                    spacing: 24
                    ColumnLayout {
                        id: torCol
                        Layout.fillWidth: true
                        spacing: 3
                        Text { text: (i18n.language, i18n.t("settings_use_tor")); color: Theme.t1; font.pixelSize: 15; font.weight: Font.Medium; font.family: Theme.fontSans }
                        Text { Layout.fillWidth: true; text: (i18n.language, i18n.t("set_use_tor_note")); color: Theme.t3; font.pixelSize: 14; font.family: Theme.fontSans; wrapMode: Text.WordWrap }
                    }
                    TToggle { big: true; on: typeof settings !== "undefined" && settings.getBool("useTor"); onToggled: function (v) { settings.set("useTor", v) } }
                }
            }
        }
    }

    FileDialog {
        id: vpnFileDlg
        nameFilters: ["WireGuard config (*.conf)", "All files (*)"]
        onAccepted: if (page.hasVpn) vpn.importFromFile(selectedFile.toString(), "")
    }
}
