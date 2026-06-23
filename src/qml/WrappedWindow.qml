// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// "Your Year in Torrents" — a dramatic, shareable yearly recap built from the
// local StatsHistory. Gothic-cinema look to match the app's identity.
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import "theme"
import "widgets"

Window {
    id: win
    width: 760; height: 860
    minimumWidth: 620; minimumHeight: 640
    title: "BATorrent — " + (i18n.language, i18n.t("wrapped_title"))
    color: "#0a0a0d"
    flags: Qt.Window

    property int year: 0
    property var data: ({})
    readonly property var months: data.months || []
    readonly property var cats: data.categories || []

    function openFor(y) {
        win.year = y
        win.data = (typeof session !== "undefined") ? session.wrapped(y) : ({})
        win.show(); win.raise(); win.requestActivate()
    }

    function fmtBytes(b) {
        b = Number(b) || 0
        if (b < 1024) return b + " B"
        var u = ["KB", "MB", "GB", "TB", "PB"], i = -1
        do { b /= 1024; i++ } while (b >= 1024 && i < u.length - 1)
        return (b >= 100 ? Math.round(b) : b.toFixed(1)) + " " + u[i]
    }
    readonly property var monthInitials: ["J","F","M","A","M","J","J","A","S","O","N","D"]
    readonly property real maxMonth: {
        var m = 1
        for (var i = 0; i < months.length; i++) m = Math.max(m, Number(months[i]) || 0)
        return m
    }

    // backdrop: dark with a blood-red glow up top
    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#1a0608" }
            GradientStop { position: 0.4; color: "#0a0a0d" }
            GradientStop { position: 1.0; color: "#08080a" }
        }
    }
    Rectangle {
        width: parent.width * 1.4; height: width
        x: -width * 0.2; y: -width * 0.62
        radius: width / 2
        opacity: 0.5
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#66dc2626" }
            GradientStop { position: 0.6; color: "#00dc2626" }
        }
    }

    Flickable {
        anchors.fill: parent
        contentHeight: col.implicitHeight + 64
        boundsBehavior: Flickable.StopAtBounds
        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

        ColumnLayout {
            id: col
            width: parent.width
            x: 0
            spacing: 26

            Item { Layout.preferredHeight: 14; Layout.fillWidth: true }

            // header
            ColumnLayout {
                Layout.alignment: Qt.AlignHCenter
                spacing: 6
                Image {
                    Layout.alignment: Qt.AlignHCenter
                    source: "qrc:/images/logo.svg"
                    sourceSize: Qt.size(96, 96)
                    Layout.preferredWidth: 48; Layout.preferredHeight: 48
                    fillMode: Image.PreserveAspectFit
                }
                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: (i18n.language, i18n.t("wrapped_title"))
                    color: "#f5f5f6"; font.pixelSize: 26; font.weight: Font.Bold; font.family: Theme.fontSans
                }
                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: win.year
                    color: "#dc2626"; font.pixelSize: 40; font.weight: Font.Black; font.family: Theme.fontMono; font.letterSpacing: 4
                }
            }

            // hero number — total downloaded
            ColumnLayout {
                Layout.alignment: Qt.AlignHCenter
                spacing: 2
                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: win.fmtBytes(win.data.down || 0)
                    color: "#ffffff"; font.pixelSize: 56; font.weight: Font.Black; font.family: Theme.fontSans
                }
                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: (i18n.language, i18n.t("wrapped_downloaded"))
                    color: "#b8b8be"; font.pixelSize: 13; font.weight: Font.DemiBold
                    font.capitalization: Font.AllUppercase; font.letterSpacing: 2; font.family: Theme.fontSans
                }
            }

            // stat trio
            RowLayout {
                Layout.alignment: Qt.AlignHCenter
                Layout.maximumWidth: 560
                spacing: 14
                Repeater {
                    model: [
                        { v: String(win.data.added || 0),     l: i18n.t("wrapped_added") },
                        { v: String(win.data.completed || 0), l: i18n.t("wrapped_completed") },
                        { v: String(win.data.activeDays || 0), l: i18n.t("wrapped_active_days") }
                    ]
                    delegate: Rectangle {
                        id: statCard
                        required property var modelData
                        Layout.fillWidth: true
                        Layout.preferredHeight: 92
                        radius: 14
                        color: "#14141a"
                        border.color: "#26171a"; border.width: 1
                        ColumnLayout {
                            anchors.centerIn: parent
                            spacing: 4
                            Text {
                                Layout.alignment: Qt.AlignHCenter
                                text: statCard.modelData.v
                                color: "#dc2626"; font.pixelSize: 30; font.weight: Font.Black; font.family: Theme.fontSans
                            }
                            Text {
                                Layout.alignment: Qt.AlignHCenter
                                text: statCard.modelData.l
                                color: "#a8a8b0"; font.pixelSize: 11; font.family: Theme.fontSans
                                horizontalAlignment: Text.AlignHCenter
                            }
                        }
                    }
                }
            }

            // month chart
            ColumnLayout {
                Layout.alignment: Qt.AlignHCenter
                Layout.maximumWidth: 560
                Layout.fillWidth: true
                Layout.leftMargin: 24; Layout.rightMargin: 24
                spacing: 10
                visible: (win.data.down || 0) > 0
                Text {
                    text: (i18n.language, i18n.t("wrapped_by_month"))
                    color: "#e8e8ec"; font.pixelSize: 13; font.weight: Font.Bold; font.family: Theme.fontSans
                }
                RowLayout {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 130
                    spacing: 6
                    Repeater {
                        model: 12
                        delegate: ColumnLayout {
                            required property int index
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            spacing: 4
                            Item { Layout.fillHeight: true; Layout.fillWidth: true
                                Rectangle {
                                    anchors.bottom: parent.bottom
                                    width: parent.width
                                    radius: 3
                                    height: Math.max(2, parent.height * ((Number(win.months[index]) || 0) / win.maxMonth))
                                    gradient: Gradient {
                                        GradientStop { position: 0.0; color: "#ef4444" }
                                        GradientStop { position: 1.0; color: "#7f1d1d" }
                                    }
                                    Behavior on height { NumberAnimation { duration: 500; easing.type: Easing.OutCubic } }
                                }
                            }
                            Text {
                                Layout.alignment: Qt.AlignHCenter
                                text: win.monthInitials[index]
                                color: "#70707a"; font.pixelSize: 9; font.family: Theme.fontMono
                            }
                        }
                    }
                }
            }

            // top categories
            ColumnLayout {
                Layout.alignment: Qt.AlignHCenter
                Layout.maximumWidth: 560
                Layout.fillWidth: true
                Layout.leftMargin: 24; Layout.rightMargin: 24
                spacing: 8
                visible: win.cats.length > 0
                Text {
                    text: (i18n.language, i18n.t("wrapped_top_categories"))
                    color: "#e8e8ec"; font.pixelSize: 13; font.weight: Font.Bold; font.family: Theme.fontSans
                }
                Repeater {
                    model: win.cats.slice(0, 5)
                    delegate: RowLayout {
                        required property var modelData
                        Layout.fillWidth: true
                        spacing: 10
                        Text {
                            text: modelData.name
                            color: "#d0d0d6"; font.pixelSize: 12; font.family: Theme.fontSans
                            Layout.preferredWidth: 110; elide: Text.ElideRight
                        }
                        Rectangle {
                            Layout.fillWidth: true
                            height: 8; radius: 4; color: "#1a1a20"
                            Rectangle {
                                height: parent.height; radius: 4
                                width: parent.width * (modelData.count / Math.max(1, win.cats[0].count))
                                gradient: Gradient {
                                    orientation: Gradient.Horizontal
                                    GradientStop { position: 0.0; color: "#7f1d1d" }
                                    GradientStop { position: 1.0; color: "#ef4444" }
                                }
                            }
                        }
                        Text {
                            text: modelData.count
                            color: "#a8a8b0"; font.pixelSize: 12; font.family: Theme.fontMono
                            Layout.preferredWidth: 34; horizontalAlignment: Text.AlignRight
                        }
                    }
                }
            }

            // busiest day
            Text {
                Layout.alignment: Qt.AlignHCenter
                visible: (win.data.busiestDown || 0) > 0
                text: i18n.t("wrapped_busiest").arg(win.data.busiestDay || "").arg(win.fmtBytes(win.data.busiestDown || 0))
                color: "#a8a8b0"; font.pixelSize: 12; font.family: Theme.fontSans
                horizontalAlignment: Text.AlignHCenter
            }

            // empty state
            Text {
                Layout.alignment: Qt.AlignHCenter
                visible: (win.data.down || 0) === 0 && (win.data.added || 0) === 0
                text: (i18n.language, i18n.t("wrapped_empty"))
                color: "#80808a"; font.pixelSize: 13; font.family: Theme.fontSans
                horizontalAlignment: Text.AlignHCenter
                Layout.maximumWidth: 420; wrapMode: Text.WordWrap
            }

            Text {
                Layout.alignment: Qt.AlignHCenter
                text: "Made with BATorrent  🦇"
                color: "#55555e"; font.pixelSize: 11; font.family: Theme.fontSans
            }
            Item { Layout.preferredHeight: 8; Layout.fillWidth: true }
        }
    }
}
