// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Desktop toast overlay — a frameless, click-through-elsewhere window pinned to
// the screen's bottom-right (like a native notification), hosting the app's
// custom toast cards. Sized to the card stack so it only covers the toasts.
import QtQuick
import QtQuick.Layouts
import QtQuick.Effects
import "theme"

Window {
    id: overlay
    flags: Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint | Qt.Tool | Qt.WindowDoesNotAcceptFocus
    color: "transparent"
    transientParent: null

    readonly property int cardW: 380
    readonly property int edge: 16
    width: cardW
    height: Math.max(1, stack.implicitHeight)
    visible: toastModel.count > 0

    // bottom-right of the available screen area (excludes taskbar/dock)
    x: Screen.virtualX + Screen.desktopAvailableWidth - width - edge
    y: Screen.virtualY + Screen.desktopAvailableHeight - height - edge

    // a card's action button was clicked (actionId set via show())
    signal actionTriggered(string actionId)

    // level: 0 info, 1 warning, 2 critical/error, 3 success
    function show(title, body, level, actionId, actionText) {
        var kind = level === 3 ? "success" : level === 2 ? "error"
                 : level === 1 ? "warning" : "info"
        toastModel.insert(0, { mTitle: title || "", mBody: body || "", mKind: kind,
                               mActionId: actionId || "", mActionText: actionText || "" })
    }

    ListModel { id: toastModel }

    Column {
        id: stack
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        spacing: 10

        Repeater {
            model: toastModel
            delegate: Rectangle {
                id: card
                width: overlay.cardW
                height: mActionId !== "" ? 104 : 86
                radius: 10
                color: Theme.panel
                border.width: 0
                opacity: 0

                readonly property color eyebrowColor:
                    mKind === "success" ? Theme.up :
                    mKind === "warning" ? Theme.accent :
                    mKind === "error"   ? Theme.accentDark : Theme.t1

                // severity = a soft accent strip on the left edge, not a hard
                // full-card border (which read as a random red box).
                Rectangle {
                    visible: mKind === "success" || mKind === "warning" || mKind === "error"
                    x: 1; y: 10; width: 3; height: card.height - 20; radius: 2
                    color: card.eyebrowColor
                }

                layer.enabled: true
                layer.effect: MultiEffect {
                    shadowEnabled: true; shadowColor: "#80000000"
                    shadowBlur: 0.6; shadowVerticalOffset: 2
                }

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
                    z: 2   // the action button must win clicks over the card-wide dismiss area

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
                        font.pixelSize: 11; font.weight: Font.DemiBold; font.family: Theme.fontSans
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
                    Text {
                        visible: mActionId !== ""
                        text: mActionText
                        color: actMa.containsMouse ? Theme.t1 : Theme.accentText
                        font.pixelSize: 10; font.weight: Font.DemiBold; font.family: Theme.fontSans
                        topPadding: 2
                        MouseArea {
                            id: actMa
                            anchors.fill: parent
                            anchors.margins: -4
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onEntered: dismissTimer.stop()
                            onExited: dismissTimer.restart()
                            onClicked: { overlay.actionTriggered(mActionId); card.dismiss() }
                        }
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
