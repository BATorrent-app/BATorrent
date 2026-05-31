// Source: bat-dialog.css  (tokens em Theme.qml)
// Overlay reutilizável: backdrop full-fill + card centralizado (titlebar + body slot + footer).
// Instanciado dentro da Main (não é Window) para o backdrop cobrir o app inteiro (inset:0).
import QtQuick
import QtQuick.Effects
import QtQuick.Layouts
import "theme"
import "widgets"

Item {
    id: dlg
    anchors.fill: parent
    z: 200
    // keep the item alive briefly after close so the exit animation can play
    visible: opened || anim > 0
    property bool opened: false
    // 0..1 drives the entrance/exit (backdrop fade + card scale+fade)
    property real anim: 0
    Behavior on anim { NumberAnimation { duration: 150; easing.type: Easing.OutCubic } }
    onOpenedChanged: anim = opened ? 1 : 0

    function open()  { opened = true }
    function close() { opened = false }

    property alias title: ttl.text
    property int cardW: 480
    property int cardH: 460
    property string footHint: ""
    property string okText: "OK"
    property string cancelText: "Cancelar"
    property bool showFooter: true
    property bool showCancel: true
    property bool showOk: true
    default property alias bodyContent: bodyHost.data

    signal accepted()
    signal rejected()

    // backdrop (rgba(0,0,0,0.5) dark / rgba(20,20,28,0.32) light)
    Rectangle {
        anchors.fill: parent
        opacity: dlg.anim
        color: Theme.isDark ? Qt.rgba(0, 0, 0, 0.5)
                            : Qt.rgba(20/255, 20/255, 28/255, 0.32)
        MouseArea { anchors.fill: parent; onClicked: { dlg.rejected(); dlg.close() } }
    }

    // shadow behind card
    MultiEffect {
        source: card
        anchors.fill: card
        shadowEnabled: true
        shadowBlur: 1.0
        shadowVerticalOffset: 40
        shadowColor: Qt.rgba(0, 0, 0, 0.6)
    }

    // .dlg card
    Rectangle {
        id: card
        anchors.centerIn: parent
        width: dlg.cardW
        height: dlg.cardH
        opacity: dlg.anim
        scale: 0.97 + 0.03 * dlg.anim
        radius: 13
        color: Theme.bg
        border.color: Theme.isDark ? Qt.rgba(1, 1, 1, 0.09) : Qt.rgba(0, 0, 0, 0.14)
        border.width: 1
        clip: true
        // swallow backdrop clicks landing on the card
        MouseArea { anchors.fill: parent }

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            // .tb titlebar 36
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 36
                color: Theme.elev

                Text {
                    id: ttl
                    anchors.centerIn: parent
                    color: Theme.t2
                    font.pointSize: 12.5
                    font.weight: Font.DemiBold
                    font.family: Theme.fontSans
                }
                // .x-close
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
                Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hairSoft }
            }

            // .body (scrollable, padding 24)
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

            // .foot 56
            Rectangle {
                visible: dlg.showFooter
                Layout.fillWidth: true
                Layout.preferredHeight: 56
                color: Theme.elev
                Rectangle { anchors.top: parent.top; width: parent.width; height: 1; color: Theme.hair }
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: Theme.sp5
                    anchors.rightMargin: 20
                    Text {
                        text: dlg.footHint
                        color: Theme.t4
                        font.pointSize: 10.5
                        font.family: Theme.fontSans
                    }
                    Item { Layout.fillWidth: true }
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
}
