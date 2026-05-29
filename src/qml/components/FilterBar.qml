import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import "../theme"

Rectangle {
    id: filterBar
    color: Theme.bg

    property string activeFilter: "all"
    property bool posterMode: true

    signal filterChanged(string filterState)
    signal searchEdited(string text)
    signal viewToggleClicked()

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Theme.spacingSm
        anchors.rightMargin: Theme.spacingSm
        spacing: Theme.spacingSm

        TextField {
            id: searchField
            Layout.preferredWidth: 260
            Layout.preferredHeight: 32
            placeholderText: qsTr("Search torrents...")
            color: Theme.text
            placeholderTextColor: Theme.dim
            font.pixelSize: Theme.fontBody
            onTextChanged: filterBar.searchEdited(text)

            background: Rectangle {
                radius: Theme.radiusSm
                color: Theme.surface
                border.color: parent.activeFocus ? Theme.accent : Theme.border
                border.width: 1
            }
        }

        AbstractButton {
            id: viewToggle
            Layout.preferredHeight: 32
            Layout.preferredWidth: viewRow.implicitWidth + 20
            padding: 0
            activeFocusOnTab: true
            onClicked: filterBar.viewToggleClicked()

            contentItem: Item {
                Row {
                    id: viewRow
                    anchors.centerIn: parent
                    spacing: 6

                    Image {
                        anchors.verticalCenter: parent.verticalCenter
                        source: filterBar.posterMode ? "qrc:/icons/grid.svg" : "qrc:/icons/list.svg"
                        width: 16
                        height: 16
                        sourceSize: Qt.size(32, 32)
                        fillMode: Image.PreserveAspectFit
                        smooth: true
                    }
                    Label {
                        anchors.verticalCenter: parent.verticalCenter
                        text: filterBar.posterMode ? qsTr("Grid view") : qsTr("List view")
                        color: viewToggle.hovered ? Theme.text : Theme.muted
                        font.pixelSize: Theme.fontCaption
                        font.weight: Font.DemiBold
                    }
                }
            }

            background: Rectangle {
                radius: Theme.radiusSm
                color: Theme.surface
                border.color: viewToggle.hovered ? Theme.accent : Theme.border
                border.width: 1
            }
        }

        Item { Layout.preferredWidth: Theme.spacingSm }

        Repeater {
            model: [
                {label: qsTr("All"),         filterState: "all"},
                {label: qsTr("Active"),      filterState: "active"},
                {label: qsTr("Downloading"), filterState: "downloading"},
                {label: qsTr("Seeding"),     filterState: "seeding"},
                {label: qsTr("Paused"),      filterState: "paused"},
                {label: qsTr("Completed"),   filterState: "completed"}
            ]

            AbstractButton {
                id: pill
                Layout.preferredHeight: 28
                checkable: true
                checked: filterBar.activeFilter === modelData.filterState
                activeFocusOnTab: true

                readonly property int liveCount: {
                    if (typeof session === "undefined") return 0
                    switch (modelData.filterState) {
                    case "all":         return session.torrentCount
                    case "active":      return session.activeCount
                    case "downloading": return session.downloadingCount
                    case "seeding":     return session.seedingCount
                    case "paused":      return session.pausedCount
                    case "completed":   return session.completedCount
                    }
                    return 0
                }

                onClicked: {
                    filterBar.activeFilter = modelData.filterState
                    filterBar.filterChanged(modelData.filterState)
                }

                contentItem: Label {
                    text: modelData.label + "  " + pill.liveCount
                    color: pill.checked ? Theme.text : Theme.muted
                    font.pixelSize: Theme.fontCaption
                    font.weight: Font.DemiBold
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    leftPadding: 14
                    rightPadding: 14
                }

                background: Rectangle {
                    radius: 14
                    color: pill.checked ? Theme.accentTint : "transparent"
                    border.color: pill.checked ? Theme.accent : Theme.border
                    border.width: 1

                    Behavior on color { ColorAnimation { duration: 150 } }
                    Behavior on border.color { ColorAnimation { duration: 150 } }
                }
            }
        }

        ComboBox {
            id: categoryCombo
            Layout.preferredWidth: 160
            Layout.preferredHeight: 32
            Layout.leftMargin: Theme.spacingSm
            model: [qsTr("All Categories"), qsTr("Movies"), qsTr("Games"), qsTr("Software"), qsTr("Music")]
            font.pixelSize: Theme.fontCaption

            contentItem: Label {
                text: categoryCombo.displayText
                color: Theme.text
                font.pixelSize: Theme.fontCaption
                verticalAlignment: Text.AlignVCenter
                leftPadding: 10
                rightPadding: 24
                elide: Text.ElideRight
            }

            background: Rectangle {
                radius: Theme.radiusSm
                color: Theme.surface
                border.color: categoryCombo.activeFocus || categoryCombo.hovered ? Theme.accent : Theme.border
                border.width: 1
            }

            indicator: Canvas {
                x: categoryCombo.width - width - 8
                y: (categoryCombo.height - height) / 2
                width: 10
                height: 6
                contextType: "2d"
                onPaint: {
                    var ctx = getContext("2d")
                    ctx.reset()
                    ctx.fillStyle = Theme.muted
                    ctx.beginPath()
                    ctx.moveTo(0, 0)
                    ctx.lineTo(width, 0)
                    ctx.lineTo(width / 2, height)
                    ctx.closePath()
                    ctx.fill()
                }
            }

            popup: Popup {
                y: categoryCombo.height + 4
                width: categoryCombo.width
                padding: 4

                background: Rectangle {
                    radius: Theme.radiusSm
                    color: Theme.surface
                    border.color: Theme.border
                    border.width: 1
                }

                contentItem: ListView {
                    implicitHeight: contentHeight
                    model: categoryCombo.popup.visible ? categoryCombo.delegateModel : null
                    currentIndex: categoryCombo.highlightedIndex
                    clip: true
                }
            }

            delegate: ItemDelegate {
                width: categoryCombo.width
                height: 28

                contentItem: Label {
                    text: modelData
                    color: Theme.text
                    font.pixelSize: Theme.fontCaption
                    verticalAlignment: Text.AlignVCenter
                    leftPadding: 8
                }

                background: Rectangle {
                    color: hovered ? Theme.accentTint : "transparent"
                    radius: 4
                }
            }
        }

        Item { Layout.fillWidth: true }

        AbstractButton {
            id: donateBtn
            Layout.preferredWidth: donateLabel.implicitWidth + 22
            Layout.preferredHeight: 26
            Layout.leftMargin: Theme.spacingLg
            Layout.rightMargin: Theme.spacingSm
            activeFocusOnTab: true

            contentItem: Row {
                anchors.centerIn: parent
                spacing: 5

                Label {
                    anchors.verticalCenter: parent.verticalCenter
                    text: "♥"
                    color: Theme.accent
                    font.pixelSize: 12
                }
                Label {
                    id: donateLabel
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("Donate")
                    color: donateBtn.hovered ? Theme.text : Theme.muted
                    font.pixelSize: Theme.fontCaption
                    font.weight: Font.DemiBold
                }
            }

            background: Rectangle {
                radius: 13
                color: donateBtn.hovered ? Theme.accentTint : "transparent"
                border.color: donateBtn.hovered ? Theme.accent : Theme.border
                border.width: 1
                Behavior on color { ColorAnimation { duration: 120 } }
                Behavior on border.color { ColorAnimation { duration: 120 } }
            }

            onClicked: Qt.openUrlExternally("https://github.com/sponsors/Mateuscruz19")
        }
    }
}
