// Source: bat-dialog.css  (tokens em Theme.qml)
// Chrome reutilizável: backdrop + card centralizado com titlebar + body slot + footer.
import QtQuick
import QtQuick.Effects
import QtQuick.Layouts
import "theme"
import "widgets"

Window {
    id: dlg

    // ------- API -------
    property alias title: ttl.text                 // .ttl text
    property int cardW: 480                        // .dlg width
    property int cardH: 460                        // .dlg height
    property string footHint: ""                   // .hint
    property string okText: "OK"                   // .acts > primary
    property string cancelText: "Cancelar"         // .acts > flat
    property bool showFooter: true
    property bool showCancel: true
    property bool showOk: true
    default property alias bodyContent: bodyHost.data

    signal accepted()
    signal rejected()

    // ------- Window setup -------
    visible: true
    width: cardW + 120
    height: cardH + 120
    color: "transparent"
    flags: Qt.FramelessWindowHint | Qt.Dialog
    modality: Qt.ApplicationModal

    // ------- backdrop (rgba(0,0,0,0.5) dark / rgba(20,20,28,0.32) light) -------
    Rectangle {
        anchors.fill: parent
        color: Theme.isDark ? Qt.rgba(0, 0, 0, 0.5)
                            : Qt.rgba(20/255, 20/255, 28/255, 0.32)
    }

    // ------- .dlg (card) -------
    Rectangle {
        id: card
        anchors.centerIn: parent
        width: dlg.cardW
        height: dlg.cardH
        radius: 13
        color: Theme.bg
        border.color: Theme.isDark ? Qt.rgba(1, 1, 1, 0.09)
                                   : Qt.rgba(0, 0, 0, 0.14)
        border.width: 1
        clip: true

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            // -------- .tb (titlebar, 36px) --------
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 36
                color: Theme.elev

                // .ttl (centralizado absoluto, 12.5 / 600 / t2)
                Text {
                    id: ttl
                    anchors.centerIn: parent
                    color: Theme.t2
                    font.pointSize: 12.5
                    font.weight: Font.DemiBold
                    font.family: Theme.fontSans
                }

                // .x-close (margin-left auto, 22×22, radius 6, color t4 → hover bg hover + t1)
                Rectangle {
                    anchors.right: parent.right
                    anchors.rightMargin: 7
                    anchors.verticalCenter: parent.verticalCenter
                    width: 22
                    height: 22
                    radius: 6
                    color: xMa.containsMouse ? Theme.hover : "transparent"

                    IconImg {
                        anchors.centerIn: parent
                        src: "qrc:/icons/close.svg"
                        tint: xMa.containsMouse ? Theme.t1 : Theme.t4
                        s: 13
                    }
                    MouseArea {
                        id: xMa
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: { dlg.rejected(); dlg.close() }
                    }
                }

                // border-bottom 1px hairSoft
                Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hairSoft }
            }

            // -------- .body (flex:1, overflow-y auto, padding 24) --------
            Flickable {
                id: bodyScroll
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                contentWidth: width
                contentHeight: bodyHost.implicitHeight + 2 * Theme.sp5
                boundsBehavior: Flickable.StopAtBounds

                ColumnLayout {
                    id: bodyHost
                    x: Theme.sp5
                    y: Theme.sp5
                    width: bodyScroll.width - 2 * Theme.sp5
                    spacing: Theme.sp4
                }
            }

            // -------- .foot (56px) --------
            Rectangle {
                visible: dlg.showFooter
                Layout.fillWidth: true
                Layout.preferredHeight: 56
                color: Theme.elev

                // border-top hair
                Rectangle { anchors.top: parent.top; width: parent.width; height: 1; color: Theme.hair }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: Theme.sp5
                    anchors.rightMargin: 20

                    // .hint
                    Text {
                        text: dlg.footHint
                        color: Theme.t4
                        font.pointSize: 10.5
                        font.family: Theme.fontSans
                    }

                    Item { Layout.fillWidth: true }

                    // .acts (Row spacing 8)
                    Row {
                        spacing: Theme.sp2

                        BtnFlat {
                            visible: dlg.showCancel
                            text: dlg.cancelText
                            onClicked: { dlg.rejected(); dlg.close() }
                        }
                        BtnFlat {
                            visible: dlg.showOk
                            primary: true
                            text: dlg.okText
                            onClicked: { dlg.accepted(); dlg.close() }
                        }
                    }
                }
            }
        }
    }

    // box-shadow do CSS: aproximação via MultiEffect
    MultiEffect {
        source: card
        anchors.fill: card
        z: -1
        shadowEnabled: true
        shadowBlur: 1.0
        shadowVerticalOffset: 50
        shadowHorizontalOffset: 0
        shadowColor: Qt.rgba(0, 0, 0, 0.85)
        shadowScale: 1.0
    }
}
