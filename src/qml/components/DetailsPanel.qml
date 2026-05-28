import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import "../theme"

Rectangle {
    id: detailsPanel
    color: Theme.panel

    readonly property bool hasData: typeof session !== "undefined" && session.hasSelection
    readonly property bool hasMetadata: hasData
        && (session.selectedPoster.length > 0
            || session.selectedMetaInfo.length > 0
            || session.selectedDescription.length > 0)
    property int currentTab: 0

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 32

            Row {
                anchors.left: parent.left
                anchors.bottom: parent.bottom
                anchors.leftMargin: Theme.spacingMd
                spacing: 0

                Repeater {
                    model: [qsTr("General"), qsTr("Peers"), qsTr("Files"), qsTr("Trackers"), qsTr("Pieces")]

                    Rectangle {
                        width: tabLabel.implicitWidth + 28
                        height: 32
                        color: "transparent"

                        readonly property bool active: detailsPanel.currentTab === index

                        Label {
                            id: tabLabel
                            anchors.centerIn: parent
                            text: modelData
                            color: parent.active ? Theme.text : Theme.muted
                            font.pixelSize: Theme.fontCaption
                            font.weight: Font.DemiBold
                        }

                        Rectangle {
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.bottom: parent.bottom
                            height: 2
                            color: parent.active ? Theme.accent : "transparent"
                            Behavior on color { ColorAnimation { duration: 150 } }
                        }

                        TapHandler { onTapped: detailsPanel.currentTab = index }
                        HoverHandler { cursorShape: Qt.PointingHandCursor }
                    }
                }
            }
        }

        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: Theme.border }

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: detailsPanel.currentTab

            Item {
                RowLayout {
                    anchors.fill: parent
                    anchors.margins: Theme.spacingMd
                    spacing: Theme.spacingLg

                    RowLayout {
                        Layout.preferredWidth: 320
                        Layout.fillHeight: true
                        Layout.alignment: Qt.AlignTop
                        spacing: Theme.spacingSm
                        visible: detailsPanel.hasMetadata

                        Image {
                            Layout.preferredWidth: 108
                            Layout.preferredHeight: 162
                            Layout.alignment: Qt.AlignTop
                            source: detailsPanel.hasData && session.selectedPoster ? "file://" + session.selectedPoster : ""
                            fillMode: Image.PreserveAspectCrop
                            asynchronous: true
                            sourceSize.width: 216
                            sourceSize.height: 324
                            visible: status === Image.Ready
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            Layout.alignment: Qt.AlignTop
                            spacing: 3

                            Label {
                                Layout.fillWidth: true
                                text: detailsPanel.hasData ? (session.selectedMetaTitle || session.selectedName) : ""
                                color: Theme.text
                                font.pixelSize: Theme.fontBody
                                font.weight: Font.Bold
                                wrapMode: Text.WordWrap
                                maximumLineCount: 2
                                elide: Text.ElideRight
                            }
                            Label {
                                Layout.fillWidth: true
                                text: detailsPanel.hasData ? session.selectedMetaInfo : ""
                                color: Theme.dim
                                font.pixelSize: 9
                                wrapMode: Text.WordWrap
                                visible: text.length > 0
                            }
                            Label {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                text: detailsPanel.hasData ? (session.selectedDescription || "").substring(0, 300) : ""
                                color: Theme.muted
                                font.pixelSize: 9
                                wrapMode: Text.WordWrap
                                elide: Text.ElideRight
                                visible: text.length > 0
                            }
                        }
                    }

                    Rectangle { Layout.preferredWidth: 1; Layout.fillHeight: true; color: Theme.border; visible: detailsPanel.hasMetadata }

                    GridLayout {
                        Layout.alignment: Qt.AlignTop
                        Layout.preferredWidth: 220
                        columns: 2
                        columnSpacing: Theme.spacingMd
                        rowSpacing: Theme.spacingXs

                        Label { text: qsTr("INFO"); color: Theme.dim; font.pixelSize: Theme.fontCaption; font.weight: Font.Black; Layout.columnSpan: 2; Layout.bottomMargin: Theme.spacingXs }
                        Label { text: qsTr("Name"); color: Theme.muted; font.pixelSize: Theme.fontCaption }
                        Label { text: detailsPanel.hasData ? session.selectedName : "—"; color: Theme.text; font.pixelSize: Theme.fontCaption; elide: Text.ElideRight; Layout.fillWidth: true; Layout.maximumWidth: 160 }
                        Label { text: qsTr("Size"); color: Theme.muted; font.pixelSize: Theme.fontCaption }
                        Label { text: detailsPanel.hasData ? session.selectedSize : "—"; color: Theme.text; font.pixelSize: Theme.fontCaption }
                        Label { text: qsTr("Hash"); color: Theme.muted; font.pixelSize: Theme.fontCaption }
                        Label { text: detailsPanel.hasData ? session.selectedHash : "—"; color: Theme.text; font.pixelSize: Theme.fontCaption; font.family: "Menlo" }
                        Label { text: qsTr("State"); color: Theme.muted; font.pixelSize: Theme.fontCaption }
                        Label { text: detailsPanel.hasData ? session.selectedState : "—"; color: Theme.text; font.pixelSize: Theme.fontCaption }
                    }

                    Rectangle { Layout.preferredWidth: 1; Layout.fillHeight: true; color: Theme.border }

                    GridLayout {
                        Layout.alignment: Qt.AlignTop
                        Layout.preferredWidth: 180
                        columns: 2
                        columnSpacing: Theme.spacingMd
                        rowSpacing: Theme.spacingXs

                        Label { text: qsTr("TRANSFER"); color: Theme.dim; font.pixelSize: Theme.fontCaption; font.weight: Font.Black; Layout.columnSpan: 2; Layout.bottomMargin: Theme.spacingXs }
                        Label { text: qsTr("Down"); color: Theme.muted; font.pixelSize: Theme.fontCaption }
                        Label { text: detailsPanel.hasData ? session.selectedDownloaded : "—"; color: Theme.text; font.pixelSize: Theme.fontCaption }
                        Label { text: qsTr("Up"); color: Theme.muted; font.pixelSize: Theme.fontCaption }
                        Label { text: detailsPanel.hasData ? session.selectedUploaded : "—"; color: Theme.text; font.pixelSize: Theme.fontCaption }
                        Label { text: qsTr("Speed"); color: Theme.muted; font.pixelSize: Theme.fontCaption }
                        Label { text: detailsPanel.hasData ? session.selectedSpeed : "—"; color: Theme.text; font.pixelSize: Theme.fontCaption }
                        Label { text: qsTr("ETA"); color: Theme.muted; font.pixelSize: Theme.fontCaption }
                        Label { text: detailsPanel.hasData ? session.selectedEta : "—"; color: Theme.text; font.pixelSize: Theme.fontCaption }
                    }

                    Rectangle { Layout.preferredWidth: 1; Layout.fillHeight: true; color: Theme.border }

                    GridLayout {
                        Layout.alignment: Qt.AlignTop
                        Layout.preferredWidth: 140
                        columns: 2
                        columnSpacing: Theme.spacingMd
                        rowSpacing: Theme.spacingXs

                        Label { text: qsTr("PEERS"); color: Theme.dim; font.pixelSize: Theme.fontCaption; font.weight: Font.Black; Layout.columnSpan: 2; Layout.bottomMargin: Theme.spacingXs }
                        Label { text: qsTr("Seeds"); color: Theme.muted; font.pixelSize: Theme.fontCaption }
                        Label { text: detailsPanel.hasData ? session.selectedSeeds : "0"; color: Theme.text; font.pixelSize: Theme.fontCaption }
                        Label { text: qsTr("Peers"); color: Theme.muted; font.pixelSize: Theme.fontCaption }
                        Label { text: detailsPanel.hasData ? session.selectedPeers : "0"; color: Theme.text; font.pixelSize: Theme.fontCaption }
                        Label { text: qsTr("Ratio"); color: Theme.muted; font.pixelSize: Theme.fontCaption }
                        Label { text: detailsPanel.hasData ? session.selectedRatio : "0.00"; color: Theme.text; font.pixelSize: Theme.fontCaption }
                    }

                    Item { Layout.fillWidth: true; Layout.fillHeight: true }
                }
            }

            Item {
                PeersTab {
                    anchors.fill: parent
                    anchors.margins: Theme.spacingSm
                    peers: detailsPanel.hasData ? session.selectedPeerList : []
                }
            }
            Item {
                FilesTab {
                    anchors.fill: parent
                    anchors.margins: Theme.spacingSm
                    files: detailsPanel.hasData ? session.selectedFiles : []
                }
            }
            Item {
                TrackersTab {
                    anchors.fill: parent
                    anchors.margins: Theme.spacingSm
                    trackers: detailsPanel.hasData ? session.selectedTrackers : []
                }
            }
            Item {
                PiecesTab {
                    anchors.fill: parent
                    anchors.margins: Theme.spacingSm
                    pieces: detailsPanel.hasData ? session.selectedPieces : []
                }
            }
        }
    }
}
