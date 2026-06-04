// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// In-app toast stack — QML port of the legacy gui/toast.cpp widget.
// Cards slide up from the bottom-right of the content area, auto-dismiss after
// 5s (paused on hover), fade in/out, and are clickable. Kind tints the eyebrow.
import QtQuick
import QtQuick.Layouts
import QtQuick.Effects
import "../theme"

Item {
    id: host
    anchors.fill: parent
    z: 9000

    // level: 0 info, 1 warning, 2 critical/error, 3 success
    function show(title, body, level) {
        var kind = level === 3 ? "success" : level === 2 ? "error"
                 : level === 1 ? "warning" : "info"
        toastModel.insert(0, { mTitle: title || "", mBody: body || "", mKind: kind })
    }

    ListModel { id: toastModel }

    Column {
        id: stack
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.rightMargin: 16
        anchors.bottomMargin: 16
        spacing: 10

        Repeater {
            model: toastModel
            delegate: Rectangle {
                id: card
                width: 380
                height: 86
                radius: 10
                color: Theme.panel
                border.width: (mKind === "warning" || mKind === "error") ? 1 : 0
                border.color: eyebrowColor
                opacity: 0

                readonly property color eyebrowColor:
                    mKind === "success" ? Theme.up :
                    mKind === "warning" ? Theme.accent :
                    mKind === "error"   ? Theme.accentDark : Theme.t1

                // logo
                Image {
                    id: tlogo
                    x: 16; anchors.verticalCenter: parent.verticalCenter
                    width: 44; height: 44
                    source: "qrc:/images/logo.svg"
                    sourceSize: Qt.size(88, 88)
                    fillMode: Image.PreserveAspectFit
                    layer.enabled: Theme.isLight
                    layer.effect: MultiEffect { colorization: 1.0; colorizationColor: Theme.t1 }
                }

                Column {
                    anchors.left: tlogo.right; anchors.leftMargin: 14
                    anchors.right: parent.right; anchors.rightMargin: 16
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 4

                    RowLayout {
                        width: parent.width
                        Text {
                            text: "BATORRENT"
                            color: card.eyebrowColor
                            font.pixelSize: 8; font.weight: Font.Bold
                            font.letterSpacing: 1.2; font.family: Theme.fontSans
                            Layout.fillWidth: true
                        }
                        Text {
                            text: (i18n.language, i18n.t("toast_now"))
                            color: Theme.t4; font.pixelSize: 8; font.family: Theme.fontSans
                        }
                    }
                    Text {
                        width: parent.width
                        text: mTitle
                        color: Theme.t1
                        font.pixelSize: 10; font.weight: Font.DemiBold; font.family: Theme.fontSans
                        elide: Text.ElideRight
                    }
                    Text {
                        width: parent.width
                        text: mBody
                        color: Theme.t3
                        font.pixelSize: 9; font.family: Theme.fontMono
                        elide: Text.ElideRight
                        visible: mBody.length > 0
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onEntered: dismissTimer.stop()
                    onExited: dismissTimer.restart()
                    onClicked: card.dismiss()
                }

                Behavior on opacity { NumberAnimation { duration: 220; easing.type: Easing.OutCubic } }
                Component.onCompleted: opacity = 1

                function dismiss() {
                    dismissTimer.stop()
                    opacity = 0
                    removeTimer.start()
                }
                Timer {
                    id: dismissTimer
                    interval: 5000; running: true; repeat: false
                    onTriggered: card.dismiss()
                }
                Timer {
                    id: removeTimer
                    interval: 240; repeat: false
                    onTriggered: { if (index >= 0 && index < toastModel.count) toastModel.remove(index) }
                }
            }
        }
    }
}
