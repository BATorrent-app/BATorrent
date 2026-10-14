// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Tray popup shown on right-click of the system tray icon: what the app is
// doing (status, speeds, the downloads in flight) above the few things worth
// doing from the tray. Pause/resume everything is one button in the header,
// since only one of the two ever makes sense at a time.
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Effects
import QtQuick.Layouts
import "../theme"
import "../widgets"

Window {
    id: pop
    width: 320
    height: col.implicitHeight + 2
    flags: Qt.Popup | Qt.FramelessWindowHint | Qt.NoDropShadowWindowHint
    color: "transparent"
    visible: false

    // actions delegated to Main (keeps this file free of global ids)
    signal showApp()
    signal openTorrent()
    signal openMagnet()
    signal pauseAll()
    signal resumeAll()
    signal openSettings()
    signal quitApp()
    signal selectTorrent(string infoHash)

    readonly property var sess: typeof session !== "undefined" ? session : null

    // Anchor near the tray icon using its reported geometry (passed in). macOS
    // menu bar is at the top → drop below the icon; Windows/Linux tray is at the
    // bottom → rise above it. Falls back to a screen corner if geometry is empty.
    function popUpAt(iconRect) {
        var scr = pop.screen || Qt.application.screens[0]
        var g = scr ? Qt.rect(scr.virtualX, scr.virtualY, scr.width, scr.height) : Qt.rect(0,0,1920,1080)
        var margin = 6
        // SystemTrayIcon.geometry is unreliable on Windows (often empty or a bogus
        // rect that lands the popup off-screen: issue #20). Anchor to the tray
        // corner of the screen there; only trust the icon rect on macOS/Linux.
        var useIcon = iconRect && iconRect.width > 0 && Qt.platform.os !== "windows"
        if (useIcon) {
            // center under/over the icon
            pop.x = Math.round(iconRect.x + iconRect.width / 2 - pop.width / 2)
            if (Qt.platform.os === "osx")
                pop.y = Math.round(iconRect.y + iconRect.height + margin)
            else
                pop.y = Math.round(iconRect.y - pop.height - margin)
        } else {
            // anchor at the click itself: centered above the cursor (the
            // Windows taskbar sits at the bottom); beats any corner guess
            var c = typeof themeBridge !== "undefined" ? themeBridge.cursorPos() : null
            if (c) {
                // clamp against the monitor under the cursor, not pop.screen
                var scrs = Qt.application.screens
                for (var i = 0; i < scrs.length; ++i) {
                    var sc = scrs[i]
                    if (c.x >= sc.virtualX && c.x < sc.virtualX + sc.width
                            && c.y >= sc.virtualY && c.y < sc.virtualY + sc.height) {
                        g = Qt.rect(sc.virtualX, sc.virtualY, sc.width, sc.height)
                        break
                    }
                }
                pop.x = Math.round(c.x - pop.width / 2)
                pop.y = Math.round(c.y - pop.height - margin)
            } else {
                pop.x = g.x + g.width - pop.width - 8
                pop.y = (Qt.platform.os === "osx") ? g.y + 30 : g.y + g.height - pop.height - 48
            }
        }
        // clamp to the screen
        if (pop.x + pop.width > g.x + g.width - 4) pop.x = g.x + g.width - pop.width - 4
        if (pop.x < g.x + 4) pop.x = g.x + 4
        if (pop.y + pop.height > g.y + g.height - 4) pop.y = g.y + g.height - pop.height - 4
        if (pop.y < g.y + 4) pop.y = g.y + 4
        pop.show()
        pop.raise()
        pop.requestActivate()
    }

    onActiveChanged: if (!active) pop.hide()   // click-away closes

    readonly property var downloads: pop.sess ? pop.sess.activeDownloads : []
    readonly property bool busy: downloads.length > 0
    // seeding counts too: the header button has to be able to stop an upload
    readonly property bool running: pop.sess !== null && pop.sess.activeCount > 0
    function label(key) {
        var s = String(i18n.t(key)).toLowerCase()
        return s.charAt(0).toUpperCase() + s.slice(1)
    }
    readonly property bool moving: pop.sess && (pop.sess.totalDownSpeed.indexOf("0 ") !== 0
                                                || pop.sess.totalUpSpeed.indexOf("0 ") !== 0)

    Rectangle {
        anchors.fill: parent
        radius: 12
        color: Theme.panel
        border.color: Theme.hair
        border.width: 1
        clip: true

        ColumnLayout {
            id: col
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: 1
            spacing: 0

            // ── header: name, what it is doing, pause/resume everything ──
            RowLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 14; Layout.rightMargin: 14
                Layout.topMargin: 14; Layout.bottomMargin: 12
                spacing: 12
                Image {
                    source: "qrc:/images/logo.svg"
                    sourceSize: Qt.size(60, 60)
                    Layout.preferredWidth: 30; Layout.preferredHeight: 30
                    fillMode: Image.PreserveAspectFit
                    layer.enabled: Theme.isLight
                    layer.effect: MultiEffect { colorization: 1.0; colorizationColor: Theme.t1 }
                }
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2
                    Text { text: "BATorrent"; color: Theme.t1; font.pixelSize: 14; font.weight: Font.DemiBold; font.family: Theme.fontSans }
                    RowLayout {
                        spacing: 6
                        Rectangle {
                            width: 6; height: 6; radius: 3
                            color: pop.busy ? Theme.grn : Theme.t4
                        }
                        Text {
                            Layout.fillWidth: true
                            text: pop.busy
                                  ? (i18n.language, i18n.t("tray_status_active")).arg(pop.downloads.length).arg(pop.sess ? pop.sess.torrentCount : 0)
                                  : (i18n.language, i18n.t("tray_status_idle")).arg(pop.sess ? pop.sess.torrentCount : 0)
                            color: Theme.t3
                            font.pixelSize: 12
                            font.family: Theme.fontSans
                            font.features: Theme.tnum
                            elide: Text.ElideRight
                        }
                    }
                }
                // one button: pauses everything while something runs, resumes otherwise
                Rectangle {
                    Layout.preferredWidth: 32; Layout.preferredHeight: 32
                    radius: 8
                    color: tgMa.containsMouse ? Theme.hover : Theme.bg
                    border.color: Theme.hair; border.width: 1
                    IconImg {
                        anchors.centerIn: parent
                        src: pop.running ? "qrc:/icons/pause.svg" : "qrc:/icons/play.svg"
                        s: 13
                        tint: Theme.t1
                    }
                    MouseArea {
                        id: tgMa
                        anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                        onClicked: pop.running ? pop.pauseAll() : pop.resumeAll()
                    }
                    ToolTip.visible: tgMa.containsMouse
                    ToolTip.delay: 400
                    ToolTip.text: (i18n.language, i18n.t(pop.running ? "tray_pause_all" : "tray_resume_all"))
                }
            }

            // ── speeds ──
            Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: Theme.hairSoft }
            RowLayout {
                Layout.fillWidth: true
                spacing: 0
                Repeater {
                    model: [
                        { label: (i18n.language, pop.label("graph_download")), icon: "qrc:/icons/arrow-down.svg", tint: Theme.accent,
                          val: pop.sess ? pop.sess.totalDownSpeed : "0 B/s" },
                        { label: (i18n.language, pop.label("graph_upload")), icon: "qrc:/icons/arrow-up.svg", tint: Theme.t4,
                          val: pop.sess ? pop.sess.totalUpSpeed : "0 B/s" }
                    ]
                    delegate: Item {
                        required property var modelData
                        required property int index
                        Layout.fillWidth: true
                        Layout.preferredHeight: 60
                        Rectangle {
                            visible: index === 0
                            anchors.right: parent.right
                            width: 1; height: parent.height
                            color: Theme.hairSoft
                        }
                        ColumnLayout {
                            anchors.left: parent.left; anchors.leftMargin: 14
                            anchors.verticalCenter: parent.verticalCenter
                            spacing: 2
                            RowLayout {
                                spacing: 5
                                IconImg { src: modelData.icon; s: 11; tint: modelData.tint }
                                Text { text: modelData.label; color: Theme.t4; font.pixelSize: 11; font.family: Theme.fontSans }
                            }
                            Text {
                                text: modelData.val
                                color: pop.moving ? Theme.t1 : Theme.t4
                                font.pixelSize: 18
                                font.weight: Font.DemiBold
                                font.family: Theme.fontSans
                                font.features: Theme.tnum
                            }
                        }
                    }
                }
            }
            Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: Theme.hairSoft }

            // ── what is downloading ──
            ColumnLayout {
                visible: pop.busy
                Layout.fillWidth: true
                Layout.topMargin: 6; Layout.bottomMargin: 6
                spacing: 0
                Repeater {
                    model: pop.downloads.slice(0, 3)
                    delegate: Rectangle {
                        required property var modelData
                        Layout.fillWidth: true
                        Layout.preferredHeight: 46
                        color: dlMa.containsMouse ? Theme.hover : "transparent"
                        ColumnLayout {
                            anchors.left: parent.left; anchors.leftMargin: 14
                            anchors.right: parent.right; anchors.rightMargin: 14
                            anchors.verticalCenter: parent.verticalCenter
                            spacing: 6
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8
                                Text {
                                    Layout.fillWidth: true
                                    text: modelData.title || ""
                                    color: Theme.t1
                                    font.pixelSize: 12
                                    font.family: Theme.fontSans
                                    elide: Text.ElideRight
                                }
                                Text {
                                    text: Math.round((modelData.progress || 0) * 100) + "% · " + (modelData.downSpeed || "")
                                    color: Theme.t3
                                    font.pixelSize: 11
                                    font.family: Theme.fontSans
                                    font.features: Theme.tnum
                                }
                            }
                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 3
                                radius: 2
                                color: Theme.track
                                Rectangle {
                                    width: parent.width * Math.max(0, Math.min(1, modelData.progress || 0))
                                    height: parent.height; radius: 2
                                    color: Theme.accent
                                }
                            }
                        }
                        MouseArea {
                            id: dlMa
                            anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                            onClicked: { pop.selectTorrent(modelData.infoHash); pop.hide() }
                        }
                    }
                }
            }
            Rectangle { visible: pop.busy; Layout.fillWidth: true; Layout.preferredHeight: 1; color: Theme.hairSoft }

            // ── actions ──
            component Act_: Rectangle {
                id: r
                property string label
                property string icon
                property string shortcut: ""
                signal clicked()
                Layout.fillWidth: true
                Layout.preferredHeight: 34
                radius: 7
                color: rma.containsMouse ? Theme.hover : "transparent"
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 8; anchors.rightMargin: 8
                    spacing: 10
                    IconImg { src: r.icon; s: 15; tint: rma.containsMouse ? Theme.t1 : Theme.t3 }
                    Text { Layout.fillWidth: true; text: r.label; color: rma.containsMouse ? Theme.t1 : Theme.t2; font.pixelSize: 13; font.family: Theme.fontSans; elide: Text.ElideRight }
                    Text { visible: r.shortcut.length > 0; text: r.shortcut; color: Theme.t4; font.pixelSize: 11; font.family: Theme.fontSans; font.features: Theme.tnum }
                }
                MouseArea { id: rma; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: { r.clicked(); pop.hide() } }
            }
            ColumnLayout {
                Layout.fillWidth: true
                Layout.margins: 6
                spacing: 0
                Act_ { label: (i18n.language, i18n.t("tray_show"));         icon: "qrc:/icons/hub.svg";      shortcut: "Ctrl+1"; onClicked: pop.showApp() }
                Act_ { label: (i18n.language, i18n.t("tray_open_torrent")); icon: "qrc:/icons/download.svg"; shortcut: "Ctrl+O"; onClicked: pop.openTorrent() }
                Act_ { label: (i18n.language, i18n.t("tray_open_magnet"));  icon: "qrc:/icons/magnet.svg";   shortcut: "Ctrl+V"; onClicked: pop.openMagnet() }
            }

            // ── footer: preferences, quit ──
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 42
                color: Theme.bg
                bottomLeftRadius: 11; bottomRightRadius: 11
                Rectangle { anchors.top: parent.top; width: parent.width; height: 1; color: Theme.hairSoft }
                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 6
                    spacing: 4
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        radius: 7
                        color: prefMa.containsMouse ? Theme.hover : "transparent"
                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 8; anchors.rightMargin: 8
                            spacing: 8
                            IconImg { src: "qrc:/icons/settings.svg"; s: 14; tint: prefMa.containsMouse ? Theme.t1 : Theme.t3 }
                            Text { Layout.fillWidth: true; text: (i18n.language, i18n.t("tray_preferences")); color: prefMa.containsMouse ? Theme.t1 : Theme.t2; font.pixelSize: 12; font.family: Theme.fontSans; elide: Text.ElideRight }
                        }
                        MouseArea { id: prefMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: { pop.openSettings(); pop.hide() } }
                    }
                    Rectangle {
                        Layout.fillHeight: true
                        Layout.preferredWidth: quitRow.implicitWidth + 20
                        radius: 7
                        color: quitMa.containsMouse ? Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.14) : "transparent"
                        RowLayout {
                            id: quitRow
                            anchors.centerIn: parent
                            spacing: 8
                            Text { text: (i18n.language, i18n.t("tray_quit")); color: Theme.accentText; font.pixelSize: 12; font.family: Theme.fontSans }
                            Text { text: "Ctrl+Q"; color: Theme.t4; font.pixelSize: 11; font.family: Theme.fontSans }
                        }
                        MouseArea { id: quitMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: { pop.quitApp(); pop.hide() } }
                    }
                }
            }
        }
    }
}
