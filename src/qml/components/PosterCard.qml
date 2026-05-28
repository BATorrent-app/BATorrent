import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Effects
import "../theme"

Item {
    id: card

    property string torrentName: ""
    property string posterPath: ""
    property string metaTitle: ""
    property string stateKey: "downloading"
    property real progress: 0
    property string stateString: ""
    property string size: ""
    property bool isSelected: false

    readonly property bool isHovered: hoverHandler.hovered

    activeFocusOnTab: true
    Accessible.role: Accessible.Button
    Accessible.name: metaTitle || torrentName

    signal contextRequested(point pos)

    HoverHandler { id: hoverHandler }

    TapHandler {
        acceptedButtons: Qt.RightButton
        onTapped: function(eventPoint, button) {
            card.contextRequested(eventPoint.position)
        }
    }

    Item {
        id: cardWrapper
        anchors.centerIn: parent
        width: 180
        height: 300

        scale: card.isHovered ? 1.035 : 1.0
        Behavior on scale {
            NumberAnimation { duration: 180; easing.type: Easing.OutCubic }
        }

        Rectangle {
            id: cardBg
            anchors.fill: parent
            radius: Theme.radiusLg
            color: Theme.panel

            Item {
                id: posterArea
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                height: 230

                Rectangle {
                    id: placeholder
                    anchors.fill: parent
                    color: Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.12)
                    topLeftRadius: Theme.radiusLg
                    topRightRadius: Theme.radiusLg
                    visible: posterImage.status !== Image.Ready

                    Image {
                        anchors.centerIn: parent
                        source: "qrc:/images/logo.svg"
                        width: 56
                        height: 56
                        sourceSize: Qt.size(112, 112)
                        opacity: 0.35
                    }
                }

                Image {
                    id: posterImage
                    anchors.fill: parent
                    source: card.posterPath ? "file://" + card.posterPath : ""
                    fillMode: Image.PreserveAspectCrop
                    asynchronous: true
                    sourceSize.width: 342
                    sourceSize.height: 513
                    visible: false
                    layer.enabled: true
                }

                Rectangle {
                    id: posterMask
                    anchors.fill: parent
                    color: "white"
                    topLeftRadius: Theme.radiusLg
                    topRightRadius: Theme.radiusLg
                    visible: false
                    layer.enabled: true
                }

                MultiEffect {
                    source: posterImage
                    anchors.fill: posterImage
                    maskEnabled: true
                    maskSource: posterMask
                    visible: posterImage.status === Image.Ready
                    opacity: posterImage.status === Image.Ready ? 1 : 0

                    Behavior on opacity {
                        OpacityAnimator { duration: 350; easing.type: Easing.OutCubic }
                    }
                }
            }

            Rectangle {
                id: progressTrack
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: posterArea.bottom
                height: 3
                color: Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.12)

                Rectangle {
                    height: parent.height
                    width: parent.width * card.progress
                    color: {
                        if (card.stateKey === "downloading") return Theme.stateDownloading
                        if (card.stateKey === "seeding") return Theme.stateSeeding
                        if (card.stateKey === "completed") return Theme.stateCompleted
                        return Theme.statePaused
                    }
                    Behavior on width {
                        NumberAnimation { duration: 500; easing.type: Easing.OutCubic }
                    }
                }
            }

            ColumnLayout {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: progressTrack.bottom
                anchors.bottom: parent.bottom
                anchors.margins: 10
                anchors.topMargin: Theme.spacingSm
                spacing: 2

                Label {
                    Layout.fillWidth: true
                    text: card.metaTitle || card.torrentName
                    color: Theme.text
                    font.pixelSize: Theme.fontBody
                    font.weight: Font.DemiBold
                    elide: Text.ElideRight
                    maximumLineCount: 1
                }

                Label {
                    Layout.fillWidth: true
                    text: card.stateString
                    color: Theme.muted
                    font.pixelSize: Theme.fontCaption
                    elide: Text.ElideRight
                    maximumLineCount: 1
                }

                Item { Layout.fillHeight: true }

                Label {
                    Layout.fillWidth: true
                    text: card.size
                    color: Theme.dim
                    font.pixelSize: Theme.fontCaption
                    visible: card.size.length > 0
                }
            }

            Rectangle {
                id: borderOverlay
                anchors.fill: parent
                radius: Theme.radiusLg
                color: "transparent"
                border.color: card.isSelected || card.activeFocus ? Theme.accent : "transparent"
                border.width: card.isSelected || card.activeFocus ? 2 : 0
                Behavior on border.color { ColorAnimation { duration: 150 } }
            }

            layer.enabled: true
        }

        MultiEffect {
            source: cardBg
            anchors.fill: cardBg
            shadowEnabled: true
            shadowBlur: card.isHovered ? 0.5 : 0.2
            shadowVerticalOffset: card.isHovered ? 10 : 4
            shadowHorizontalOffset: 0
            shadowColor: Qt.rgba(0, 0, 0, card.isHovered ? 0.35 : 0.12)
            shadowScale: 1.0

            Behavior on shadowBlur { NumberAnimation { duration: 180 } }
            Behavior on shadowVerticalOffset { NumberAnimation { duration: 180 } }
        }
    }
}
