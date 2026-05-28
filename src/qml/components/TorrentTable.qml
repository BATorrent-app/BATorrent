import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import "../theme"

Rectangle {
    id: tableRoot
    color: Theme.bg

    property var model: null
    property int selectedIndex: -1

    signal torrentSelected(int sourceRow)
    signal torrentDoubleClicked(int sourceRow)
    signal contextRequested(int sourceRow, real x, real y)

    readonly property int colSize: 80
    readonly property int colProgress: 160
    readonly property int colDownSpeed: 90
    readonly property int colUpSpeed: 90
    readonly property int colState: 110
    readonly property int paddingX: 16
    readonly property int colSpacing: 10

    readonly property int fixedColsTotal: colSize + colProgress + colDownSpeed + colUpSpeed + colState + colSpacing * 5
    readonly property int colName: Math.min(360, Math.max(160, tableRoot.width - 2 * paddingX - fixedColsTotal))

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 30
            color: Theme.panel

            Row {
                anchors.fill: parent
                anchors.leftMargin: tableRoot.paddingX
                anchors.rightMargin: tableRoot.paddingX
                spacing: tableRoot.colSpacing

                Label { width: tableRoot.colName;      height: parent.height; text: qsTr("Name");     color: Theme.dim; font.pixelSize: Theme.fontCaption; font.weight: Font.Black; font.letterSpacing: 0.6; verticalAlignment: Text.AlignVCenter }
                Label { width: tableRoot.colSize;      height: parent.height; text: qsTr("Size");     color: Theme.dim; font.pixelSize: Theme.fontCaption; font.weight: Font.Black; font.letterSpacing: 0.6; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignRight }
                Label { width: tableRoot.colProgress;  height: parent.height; text: qsTr("Progress"); color: Theme.dim; font.pixelSize: Theme.fontCaption; font.weight: Font.Black; font.letterSpacing: 0.6; verticalAlignment: Text.AlignVCenter }
                Label { width: tableRoot.colDownSpeed; height: parent.height; text: qsTr("Down");     color: Theme.dim; font.pixelSize: Theme.fontCaption; font.weight: Font.Black; font.letterSpacing: 0.6; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignRight }
                Label { width: tableRoot.colUpSpeed;   height: parent.height; text: qsTr("Up");       color: Theme.dim; font.pixelSize: Theme.fontCaption; font.weight: Font.Black; font.letterSpacing: 0.6; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignRight }
                Label { width: tableRoot.colState;     height: parent.height; text: qsTr("State");    color: Theme.dim; font.pixelSize: Theme.fontCaption; font.weight: Font.Black; font.letterSpacing: 0.6; verticalAlignment: Text.AlignVCenter }
            }
        }

        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: Theme.border }

        ListView {
            id: list
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: tableRoot.model
            boundsBehavior: Flickable.StopAtBounds

            delegate: Item {
                id: row
                width: list.width
                height: 36

                required property int index
                required property var model

                readonly property bool isSelected: tableRoot.selectedIndex === row.index
                readonly property bool isAlt: (row.index % 2) === 1

                Rectangle {
                    anchors.fill: parent
                    color: {
                        if (row.isSelected) return Theme.accentTint
                        if (rowMouse.containsMouse) return Qt.rgba(Theme.surface.r, Theme.surface.g, Theme.surface.b, 0.6)
                        if (row.isAlt) return Qt.rgba(Theme.surface.r, Theme.surface.g, Theme.surface.b, 0.18)
                        return "transparent"
                    }
                    Behavior on color { ColorAnimation { duration: 80 } }
                }

                Rectangle {
                    anchors.left: parent.left
                    width: 3
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    color: row.isSelected ? Theme.accent : "transparent"
                }

                Row {
                    anchors.fill: parent
                    anchors.leftMargin: tableRoot.paddingX
                    anchors.rightMargin: tableRoot.paddingX
                    spacing: tableRoot.colSpacing

                    Label {
                        width: tableRoot.colName
                        height: parent.height
                        text: row.model.metaTitle && row.model.metaTitle.length > 0 ? row.model.metaTitle : (row.model.torrentName || "")
                        color: Theme.text
                        font.pixelSize: Theme.fontBody
                        font.weight: Font.DemiBold
                        verticalAlignment: Text.AlignVCenter
                        elide: Text.ElideRight
                    }

                    Label {
                        width: tableRoot.colSize
                        height: parent.height
                        text: row.model.size || ""
                        color: Theme.muted
                        font.pixelSize: Theme.fontCaption
                        verticalAlignment: Text.AlignVCenter
                        horizontalAlignment: Text.AlignRight
                    }

                    Item {
                        width: tableRoot.colProgress
                        height: parent.height

                        Rectangle {
                            id: track
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.left: parent.left
                            anchors.right: parent.right
                            height: 18
                            radius: 4
                            color: Theme.surfaceAlt
                            clip: true

                            readonly property real prog: row.model.progress || 0
                            readonly property color fillColor: {
                                var k = row.model.stateKey
                                if (k === "completed") return Theme.stateCompleted
                                if (prog >= 1.0)       return Theme.stateSeeding
                                if (k === "seeding")   return Theme.stateSeeding
                                if (k === "paused")    return Theme.statePaused
                                return Theme.stateDownloading
                            }

                            Rectangle {
                                anchors.top: parent.top
                                anchors.bottom: parent.bottom
                                anchors.left: parent.left
                                width: Math.max(track.prog > 0.001 ? 2 : 0, parent.width * track.prog)
                                color: track.fillColor
                                Behavior on width { NumberAnimation { duration: 400; easing.type: Easing.OutCubic } }
                            }

                            Label {
                                anchors.centerIn: parent
                                text: (track.prog * 100).toFixed(1) + "%"
                                color: {
                                    var fillEdge = track.width * track.prog
                                    var textCenter = track.width / 2
                                    return (textCenter < fillEdge - 4) ? "#ffffff" : Theme.text
                                }
                                font.pixelSize: 11
                                font.weight: Font.DemiBold
                            }
                        }
                    }

                    Label {
                        width: tableRoot.colDownSpeed
                        height: parent.height
                        text: row.model.downSpeed || ""
                        color: Theme.text
                        font.pixelSize: Theme.fontCaption
                        font.family: "Menlo"
                        verticalAlignment: Text.AlignVCenter
                        horizontalAlignment: Text.AlignRight
                    }

                    Label {
                        width: tableRoot.colUpSpeed
                        height: parent.height
                        text: row.model.upSpeed || ""
                        color: Theme.text
                        font.pixelSize: Theme.fontCaption
                        font.family: "Menlo"
                        verticalAlignment: Text.AlignVCenter
                        horizontalAlignment: Text.AlignRight
                    }

                    Row {
                        width: tableRoot.colState
                        height: parent.height
                        spacing: 6

                        Rectangle {
                            width: 8; height: 8; radius: 4
                            anchors.verticalCenter: parent.verticalCenter
                            color: {
                                var k = row.model.stateKey
                                if (k === "downloading") return Theme.stateDownloading
                                if (k === "seeding")     return Theme.stateSeeding
                                if (k === "completed")   return Theme.stateCompleted
                                return Theme.statePaused
                            }
                        }
                        Label {
                            text: row.model.stateString || ""
                            color: Theme.text
                            font.pixelSize: Theme.fontCaption
                            anchors.verticalCenter: parent.verticalCenter
                            elide: Text.ElideRight
                            width: tableRoot.colState - 14
                        }
                    }
                }

                MouseArea {
                    id: rowMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    acceptedButtons: Qt.LeftButton | Qt.RightButton
                    onClicked: function(mouse) {
                        tableRoot.selectedIndex = row.index
                        tableRoot.torrentSelected(row.index)
                        if (mouse.button === Qt.RightButton)
                            tableRoot.contextRequested(row.index, mouse.x, mouse.y)
                    }
                    onDoubleClicked: tableRoot.torrentDoubleClicked(row.index)
                }
            }

            ScrollBar.vertical: ScrollBar {
                policy: ScrollBar.AsNeeded
                contentItem: Rectangle {
                    implicitWidth: 5
                    radius: 2.5
                    color: Theme.border
                    opacity: parent.active ? 0.7 : 0.3
                    Behavior on opacity { OpacityAnimator { duration: 200 } }
                }
            }
        }
    }
}
