// Recently-removed torrents with one-click restore. session.recentlyRemoved()/restoreRemoved().
import QtQuick
import QtQuick.Layouts
import "theme"
import "widgets"

Window {
    id: win
    width: 620
    height: 480
    minimumWidth: 520
    minimumHeight: 380
    color: Theme.bg
    title: "Removidos recentemente"

    readonly property var sess: typeof session !== "undefined" ? session : null
    property var entries: []
    function reload() { if (sess) entries = sess.recentlyRemoved() }
    onVisibleChanged: if (visible) reload()

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 36
            color: Theme.elev
            Text { anchors.centerIn: parent; text: "Removidos recentemente"; color: Theme.t2; font.pointSize: 12.5; font.weight: Font.DemiBold; font.family: Theme.fontSans }
            Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hairSoft }
        }

        Text {
            Layout.fillWidth: true
            Layout.leftMargin: Theme.sp5
            Layout.rightMargin: Theme.sp5
            Layout.topMargin: Theme.sp3
            text: "Restaure um torrent removido por engano. O histórico guarda os últimos 50."
            color: Theme.t4
            font.pointSize: 11
            font.family: Theme.fontSans
            wrapMode: Text.WordWrap
        }

        ListView {
            id: list
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: Theme.sp4
            clip: true
            model: win.entries
            boundsBehavior: Flickable.StopAtBounds
            currentIndex: -1

            delegate: Rectangle {
                required property var modelData
                required property int index
                width: ListView.view.width
                height: 52
                radius: 7
                color: list.currentIndex === index ? Theme.sel : (rowMa.containsMouse ? Theme.hover : "transparent")

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 12
                    anchors.rightMargin: 12
                    spacing: 12
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2
                        Text { Layout.fillWidth: true; text: modelData.name; color: Theme.t1; font.pointSize: 12.5; font.family: Theme.fontSans; elide: Text.ElideRight }
                        Text { text: modelData.size + " · " + modelData.when; color: Theme.t4; font.pointSize: 10.5; font.family: Theme.fontMono }
                    }
                    BtnFlat {
                        sm: true; text: "Restaurar"
                        onClicked: { if (win.sess && win.sess.restoreRemoved(modelData.hash)) win.reload() }
                    }
                }
                MouseArea { id: rowMa; anchors.fill: parent; hoverEnabled: true; acceptedButtons: Qt.LeftButton; onClicked: list.currentIndex = index }
            }
        }

        Text {
            visible: win.entries.length === 0
            Layout.alignment: Qt.AlignHCenter
            text: "Nenhum torrent removido recentemente."
            color: Theme.t4
            font.pointSize: 12
            font.family: Theme.fontSans
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 56
            color: Theme.elev
            Rectangle { anchors.top: parent.top; width: parent.width; height: 1; color: Theme.hair }
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Theme.sp5
                anchors.rightMargin: 20
                BtnFlat { text: "Limpar histórico"; onClicked: { if (win.sess) { win.sess.clearRemovedHistory(); win.reload() } } }
                Item { Layout.fillWidth: true }
                BtnFlat { primary: true; text: "Fechar"; onClicked: win.close() }
            }
        }
    }
}
