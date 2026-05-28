import QtQuick
import QtQuick.Controls.Basic
import "../theme"

Item {
    id: tab
    property var peers: []

    readonly property int colIp: 140
    readonly property int colPort: 60
    readonly property int colClient: -1
    readonly property int colDown: 90
    readonly property int colUp: 90
    readonly property int colProgress: 80
    readonly property int paddingX: 8
    readonly property int colSpacing: 10

    readonly property int clientWidth: Math.max(80,
        width - 2 * paddingX - colIp - colPort - colDown - colUp - colProgress - 5 * colSpacing)

    Column {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            width: parent.width
            height: 24
            color: "transparent"
            Row {
                anchors.fill: parent
                anchors.leftMargin: tab.paddingX
                anchors.rightMargin: tab.paddingX
                spacing: tab.colSpacing
                Label { width: tab.colIp; text: qsTr("IP"); color: Theme.dim; font.pixelSize: 10; font.weight: Font.Black; verticalAlignment: Text.AlignVCenter; height: parent.height }
                Label { width: tab.colPort; text: qsTr("Port"); color: Theme.dim; font.pixelSize: 10; font.weight: Font.Black; verticalAlignment: Text.AlignVCenter; height: parent.height }
                Label { width: tab.clientWidth; text: qsTr("Client"); color: Theme.dim; font.pixelSize: 10; font.weight: Font.Black; verticalAlignment: Text.AlignVCenter; height: parent.height }
                Label { width: tab.colDown; text: qsTr("Down"); color: Theme.dim; font.pixelSize: 10; font.weight: Font.Black; horizontalAlignment: Text.AlignRight; verticalAlignment: Text.AlignVCenter; height: parent.height }
                Label { width: tab.colUp; text: qsTr("Up"); color: Theme.dim; font.pixelSize: 10; font.weight: Font.Black; horizontalAlignment: Text.AlignRight; verticalAlignment: Text.AlignVCenter; height: parent.height }
                Label { width: tab.colProgress; text: qsTr("Progress"); color: Theme.dim; font.pixelSize: 10; font.weight: Font.Black; horizontalAlignment: Text.AlignRight; verticalAlignment: Text.AlignVCenter; height: parent.height }
            }
        }

        Rectangle { width: parent.width; height: 1; color: Theme.border }

        Label {
            visible: tab.peers.length === 0
            anchors.horizontalCenter: parent.horizontalCenter
            topPadding: 24
            text: qsTr("No peers connected")
            color: Theme.dim
            font.pixelSize: Theme.fontCaption
        }

        ListView {
            width: parent.width
            height: parent.height - 25
            clip: true
            model: tab.peers
            visible: tab.peers.length > 0
            boundsBehavior: Flickable.StopAtBounds

            delegate: Rectangle {
                width: ListView.view.width
                height: 26
                color: (index % 2 === 1) ? Qt.rgba(Theme.surface.r, Theme.surface.g, Theme.surface.b, 0.18) : "transparent"

                Row {
                    anchors.fill: parent
                    anchors.leftMargin: tab.paddingX
                    anchors.rightMargin: tab.paddingX
                    spacing: tab.colSpacing
                    Label { width: tab.colIp; text: modelData.ip; color: Theme.text; font.pixelSize: Theme.fontCaption; font.family: "Menlo"; verticalAlignment: Text.AlignVCenter; height: parent.height; elide: Text.ElideRight }
                    Label { width: tab.colPort; text: modelData.port; color: Theme.muted; font.pixelSize: Theme.fontCaption; verticalAlignment: Text.AlignVCenter; height: parent.height }
                    Label { width: tab.clientWidth; text: modelData.client; color: Theme.text; font.pixelSize: Theme.fontCaption; verticalAlignment: Text.AlignVCenter; height: parent.height; elide: Text.ElideRight }
                    Label { width: tab.colDown; text: modelData.downSpeed; color: Theme.text; font.pixelSize: Theme.fontCaption; font.family: "Menlo"; horizontalAlignment: Text.AlignRight; verticalAlignment: Text.AlignVCenter; height: parent.height }
                    Label { width: tab.colUp; text: modelData.upSpeed; color: Theme.text; font.pixelSize: Theme.fontCaption; font.family: "Menlo"; horizontalAlignment: Text.AlignRight; verticalAlignment: Text.AlignVCenter; height: parent.height }
                    Label { width: tab.colProgress; text: ((modelData.progress || 0) * 100).toFixed(0) + "%"; color: Theme.muted; font.pixelSize: Theme.fontCaption; horizontalAlignment: Text.AlignRight; verticalAlignment: Text.AlignVCenter; height: parent.height }
                }
            }

            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
        }
    }
}
