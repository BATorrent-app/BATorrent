import QtQuick
import QtQuick.Controls.Basic
import "../theme"

Item {
    id: tab
    property var files: []

    readonly property int colSize: 90
    readonly property int colProgress: 130
    readonly property int colPriority: 80
    readonly property int paddingX: 8
    readonly property int colSpacing: 10

    readonly property int colName: Math.max(120,
        width - 2 * paddingX - colSize - colProgress - colPriority - 3 * colSpacing)

    function priorityName(p) {
        switch (p) {
        case 0: return qsTr("Skip")
        case 1: return qsTr("Low")
        case 4: return qsTr("Normal")
        case 7: return qsTr("High")
        }
        return String(p)
    }

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
                Label { width: tab.colName; text: qsTr("Name"); color: Theme.dim; font.pixelSize: 10; font.weight: Font.Black; verticalAlignment: Text.AlignVCenter; height: parent.height }
                Label { width: tab.colSize; text: qsTr("Size"); color: Theme.dim; font.pixelSize: 10; font.weight: Font.Black; horizontalAlignment: Text.AlignRight; verticalAlignment: Text.AlignVCenter; height: parent.height }
                Label { width: tab.colProgress; text: qsTr("Progress"); color: Theme.dim; font.pixelSize: 10; font.weight: Font.Black; verticalAlignment: Text.AlignVCenter; height: parent.height }
                Label { width: tab.colPriority; text: qsTr("Priority"); color: Theme.dim; font.pixelSize: 10; font.weight: Font.Black; verticalAlignment: Text.AlignVCenter; height: parent.height }
            }
        }

        Rectangle { width: parent.width; height: 1; color: Theme.border }

        Label {
            visible: tab.files.length === 0
            anchors.horizontalCenter: parent.horizontalCenter
            topPadding: 24
            text: qsTr("No files")
            color: Theme.dim
            font.pixelSize: Theme.fontCaption
        }

        ListView {
            width: parent.width
            height: parent.height - 25
            clip: true
            model: tab.files
            visible: tab.files.length > 0
            boundsBehavior: Flickable.StopAtBounds

            delegate: Rectangle {
                width: ListView.view.width
                height: 28
                color: (index % 2 === 1) ? Qt.rgba(Theme.surface.r, Theme.surface.g, Theme.surface.b, 0.18) : "transparent"

                Row {
                    anchors.fill: parent
                    anchors.leftMargin: tab.paddingX
                    anchors.rightMargin: tab.paddingX
                    spacing: tab.colSpacing

                    Label {
                        width: tab.colName; text: modelData.path; color: Theme.text
                        font.pixelSize: Theme.fontCaption
                        verticalAlignment: Text.AlignVCenter; height: parent.height; elide: Text.ElideMiddle
                    }
                    Label {
                        width: tab.colSize; text: modelData.size; color: Theme.muted
                        font.pixelSize: Theme.fontCaption
                        horizontalAlignment: Text.AlignRight
                        verticalAlignment: Text.AlignVCenter; height: parent.height
                    }
                    Item {
                        width: tab.colProgress; height: parent.height
                        Rectangle {
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.left: parent.left; anchors.right: parent.right
                            anchors.rightMargin: 8
                            height: 6; radius: 3
                            color: Qt.rgba(Theme.border.r, Theme.border.g, Theme.border.b, 0.4)
                            Rectangle {
                                height: parent.height
                                width: parent.width * (modelData.progress || 0)
                                radius: 3
                                color: Theme.stateDownloading
                            }
                        }
                    }
                    Label {
                        width: tab.colPriority
                        text: tab.priorityName(modelData.priority)
                        color: modelData.priority === 0 ? Theme.dim : Theme.text
                        font.pixelSize: Theme.fontCaption
                        verticalAlignment: Text.AlignVCenter; height: parent.height
                    }
                }
            }

            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
        }
    }
}
