import QtQuick
import QtQuick.Controls.Basic
import "../theme"

Item {
    id: tab
    property var trackers: []

    readonly property int colTier: 50
    readonly property int colStatus: 130
    readonly property int paddingX: 8
    readonly property int colSpacing: 10

    readonly property int colUrl: Math.max(160,
        width - 2 * paddingX - colTier - colStatus - 2 * colSpacing)

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
                Label { width: tab.colUrl; text: qsTr("URL"); color: Theme.dim; font.pixelSize: 10; font.weight: Font.Black; verticalAlignment: Text.AlignVCenter; height: parent.height }
                Label { width: tab.colTier; text: qsTr("Tier"); color: Theme.dim; font.pixelSize: 10; font.weight: Font.Black; horizontalAlignment: Text.AlignRight; verticalAlignment: Text.AlignVCenter; height: parent.height }
                Label { width: tab.colStatus; text: qsTr("Status"); color: Theme.dim; font.pixelSize: 10; font.weight: Font.Black; verticalAlignment: Text.AlignVCenter; height: parent.height }
            }
        }

        Rectangle { width: parent.width; height: 1; color: Theme.border }

        Label {
            visible: tab.trackers.length === 0
            anchors.horizontalCenter: parent.horizontalCenter
            topPadding: 24
            text: qsTr("No trackers")
            color: Theme.dim
            font.pixelSize: Theme.fontCaption
        }

        ListView {
            width: parent.width
            height: parent.height - 25
            clip: true
            model: tab.trackers
            visible: tab.trackers.length > 0
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
                    Label { width: tab.colUrl; text: modelData.url; color: Theme.text; font.pixelSize: Theme.fontCaption; font.family: "Menlo"; verticalAlignment: Text.AlignVCenter; height: parent.height; elide: Text.ElideRight }
                    Label { width: tab.colTier; text: modelData.tier; color: Theme.muted; font.pixelSize: Theme.fontCaption; horizontalAlignment: Text.AlignRight; verticalAlignment: Text.AlignVCenter; height: parent.height }
                    Label { width: tab.colStatus; text: modelData.status; color: Theme.text; font.pixelSize: Theme.fontCaption; verticalAlignment: Text.AlignVCenter; height: parent.height; elide: Text.ElideRight }
                }
            }

            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
        }
    }
}
