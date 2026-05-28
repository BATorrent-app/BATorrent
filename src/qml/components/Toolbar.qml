import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import "../theme"

Rectangle {
    id: toolbar
    color: Theme.panel

    signal openClicked()
    signal magnetClicked()
    signal pauseClicked()
    signal resumeClicked()
    signal stopClicked()
    signal removeClicked()
    signal searchClicked()
    signal rssClicked()
    signal settingsClicked()

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Theme.spacingLg
        anchors.rightMargin: Theme.spacingLg
        spacing: 2

        Image {
            source: "qrc:/images/logo.svg"
            Layout.preferredWidth: 26
            Layout.preferredHeight: 26
            Layout.rightMargin: 10
            Layout.alignment: Qt.AlignVCenter
            sourceSize: Qt.size(26, 26)
        }

        Column {
            spacing: 0
            Layout.rightMargin: 6
            Layout.alignment: Qt.AlignVCenter

            Label {
                text: "BATorrent"
                color: Theme.text
                font.pixelSize: Theme.fontBody
                font.weight: Font.Bold
            }
            Label {
                text: "V2.6.1"
                color: Theme.dim
                font.pixelSize: 7
                font.weight: Font.Black
                font.letterSpacing: 1.0
            }
        }

        Repeater {
            model: [
                {icon: "qrc:/icons/open.svg", label: qsTr("Open"), action: "open"},
                {icon: "qrc:/icons/magnet.svg", label: qsTr("Magnet"), action: "magnet"},
                {icon: "qrc:/icons/pause.svg", label: qsTr("Pause"), action: "pause"},
                {icon: "qrc:/icons/play.svg", label: qsTr("Resume"), action: "resume"},
                {icon: "qrc:/icons/stop.svg", label: qsTr("Stop"), action: "stop"},
                {icon: "qrc:/icons/trash.svg", label: qsTr("Remove"), action: "remove"},
                {icon: "qrc:/icons/search.svg", label: qsTr("Search"), action: "search"},
                {icon: "qrc:/icons/rss.svg", label: qsTr("RSS"), action: "rss"},
                {icon: "qrc:/icons/settings.svg", label: qsTr("Settings"), action: "settings"}
            ]

            AbstractButton {
                id: tbBtn
                Layout.preferredWidth: 64
                Layout.fillHeight: true
                padding: 0

                onClicked: {
                    switch (modelData.action) {
                    case "open":     toolbar.openClicked(); break
                    case "magnet":   toolbar.magnetClicked(); break
                    case "pause":    toolbar.pauseClicked(); break
                    case "resume":   toolbar.resumeClicked(); break
                    case "stop":     toolbar.stopClicked(); break
                    case "remove":   toolbar.removeClicked(); break
                    case "search":   toolbar.searchClicked(); break
                    case "rss":      toolbar.rssClicked(); break
                    case "settings": toolbar.settingsClicked(); break
                    }
                }

                contentItem: Item {
                    Column {
                        anchors.centerIn: parent
                        spacing: 3

                        Image {
                            source: modelData.icon
                            anchors.horizontalCenter: parent.horizontalCenter
                            width: 18
                            height: 18
                            sourceSize: Qt.size(18, 18)
                        }

                        Label {
                            text: modelData.label
                            anchors.horizontalCenter: parent.horizontalCenter
                            color: tbBtn.hovered ? Theme.text : Theme.muted
                            font.pixelSize: Theme.fontCaption
                            font.weight: Font.Medium
                        }
                    }
                }

                background: Rectangle {
                    radius: Theme.radiusSm
                    color: tbBtn.hovered ? Theme.accentTint : "transparent"
                    anchors.fill: parent
                    anchors.topMargin: 6
                    anchors.bottomMargin: 6
                }
            }
        }

        Item { Layout.fillWidth: true }

        Rectangle {
            Layout.preferredHeight: 44
            Layout.preferredWidth: speedRow.implicitWidth + 28
            radius: Theme.radiusMd
            color: Theme.surface
            border.color: Theme.border
            border.width: 1

            RowLayout {
                id: speedRow
                anchors.centerIn: parent
                spacing: 14

                ColumnLayout {
                    spacing: 0
                    Label {
                        text: qsTr("DOWNLOAD")
                        color: Theme.dim
                        font.pixelSize: 8
                        font.weight: Font.Black
                        font.letterSpacing: 0.8
                    }
                    Label {
                        textFormat: Text.StyledText
                        text: "<font color='#dc2626'>↓</font> "
                            + (typeof session !== "undefined" ? session.totalDownSpeed : "0 KB/s")
                        color: Theme.text
                        font.pixelSize: Theme.fontBody
                        font.weight: Font.Bold
                    }
                }

                Rectangle {
                    Layout.preferredWidth: 1
                    Layout.preferredHeight: 28
                    color: Theme.border
                }

                ColumnLayout {
                    spacing: 0
                    Label {
                        text: qsTr("UPLOAD")
                        color: Theme.dim
                        font.pixelSize: 8
                        font.weight: Font.Black
                        font.letterSpacing: 0.8
                    }
                    Label {
                        textFormat: Text.StyledText
                        text: "<font color='#fbbf24'>↑</font> "
                            + (typeof session !== "undefined" ? session.totalUpSpeed : "0 KB/s")
                        color: Theme.text
                        font.pixelSize: Theme.fontBody
                        font.weight: Font.Bold
                    }
                }
            }
        }
    }
}
