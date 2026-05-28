import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Dialogs
import QtQuick.Shapes
import "theme"
import "components"

ApplicationWindow {
    id: root
    visible: true
    width: 1280
    height: 800
    minimumWidth: 800
    minimumHeight: 600
    title: "BATorrent"
    color: Theme.bg

    property bool posterMode: true

    FileDialog {
        id: openDialog
        title: qsTr("Open torrent file")
        nameFilters: [qsTr("Torrent files (*.torrent)"), qsTr("All files (*)")]
        onAccepted: {
            if (typeof session !== "undefined")
                session.addTorrentFile(selectedFile.toString())
        }
    }

    Dialog {
        id: magnetDialog
        title: qsTr("Add magnet link")
        anchors.centerIn: parent
        width: 520
        modal: true
        standardButtons: Dialog.Ok | Dialog.Cancel

        background: Rectangle {
            radius: Theme.radiusMd
            color: Theme.panel
            border.color: Theme.border
            border.width: 1
        }

        contentItem: ColumnLayout {
            spacing: Theme.spacingSm

            Label {
                text: qsTr("Paste a magnet URI:")
                color: Theme.muted
                font.pixelSize: Theme.fontCaption
            }

            TextField {
                id: magnetField
                Layout.fillWidth: true
                placeholderText: "magnet:?xt=urn:btih:..."
                color: Theme.text
                placeholderTextColor: Theme.dim
                font.pixelSize: Theme.fontBody
                selectByMouse: true

                background: Rectangle {
                    radius: Theme.radiusSm
                    color: Theme.surface
                    border.color: parent.activeFocus ? Theme.accent : Theme.border
                    border.width: 1
                }
            }
        }

        onAccepted: {
            if (magnetField.text.length > 0 && typeof session !== "undefined")
                session.addMagnetUri(magnetField.text)
            magnetField.text = ""
        }
        onRejected: magnetField.text = ""
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Toolbar {
            Layout.fillWidth: true
            Layout.preferredHeight: 64

            onOpenClicked: openDialog.open()
            onMagnetClicked: { magnetField.text = ""; magnetDialog.open() }
            onPauseClicked: if (typeof session !== "undefined") {
                if (session.hasSelection) session.pauseSelected(); else session.pauseAll()
            }
            onResumeClicked: if (typeof session !== "undefined") {
                if (session.hasSelection) session.resumeSelected(); else session.resumeAll()
            }
            onStopClicked: if (typeof session !== "undefined" && session.hasSelection) session.pauseSelected()
            onRemoveClicked: if (typeof session !== "undefined") session.removeSelected()
        }

        FilterBar {
            Layout.fillWidth: true
            Layout.preferredHeight: 44

            onFilterChanged: function(state) {
                if (typeof torrentFilter !== "undefined") torrentFilter.setFilterState(state)
            }
            onSearchEdited: function(text) {
                if (typeof torrentFilter !== "undefined") torrentFilter.setSearchText(text)
            }
            onViewToggleClicked: root.posterMode = !root.posterMode
        }

        Item {
            id: viewSwitcher
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            PosterGrid {
                id: posterGrid
                anchors.fill: parent
                model: typeof torrentModel !== "undefined" ? torrentModel : null

                visible: opacity > 0.01
                opacity: root.posterMode ? 1 : 0
                scale: root.posterMode ? 1 : 0.96
                Behavior on opacity { OpacityAnimator { duration: 130; easing.type: Easing.InOutQuad } }
                Behavior on scale { NumberAnimation { duration: 130; easing.type: Easing.InOutQuad } }

                onTorrentSelected: function(row) {
                    if (typeof session !== "undefined") {
                        var sourceRow = (typeof torrentFilter !== "undefined")
                            ? torrentFilter.mapToSource(row) : row
                        session.setSelectedIndex(sourceRow)
                    }
                }
            }

            TorrentTable {
                id: torrentTable
                anchors.fill: parent
                model: typeof torrentModel !== "undefined" ? torrentModel : null

                visible: opacity > 0.01
                opacity: root.posterMode ? 0 : 1
                scale: root.posterMode ? 0.96 : 1
                Behavior on opacity { OpacityAnimator { duration: 130; easing.type: Easing.InOutQuad } }
                Behavior on scale { NumberAnimation { duration: 130; easing.type: Easing.InOutQuad } }

                onTorrentSelected: function(row) {
                    if (typeof session !== "undefined") {
                        var sourceRow = (typeof torrentFilter !== "undefined")
                            ? torrentFilter.mapToSource(row) : row
                        session.setSelectedIndex(sourceRow)
                    }
                }
                onContextRequested: function(row, x, y) {
                    if (typeof session !== "undefined") {
                        var sourceRow = (typeof torrentFilter !== "undefined")
                            ? torrentFilter.mapToSource(row) : row
                        session.setSelectedIndex(sourceRow)
                    }
                    var global = torrentTable.mapToItem(viewSwitcher, x, y)
                    posterGrid.openContextMenu(global.x, global.y)
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: Theme.border
        }

        SpeedGraph {
            Layout.fillWidth: true
            Layout.preferredHeight: 70
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: Theme.border
        }

        DetailsPanel {
            Layout.fillWidth: true
            Layout.preferredHeight: 220
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 28
            color: Theme.panel

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Theme.spacingMd
                anchors.rightMargin: Theme.spacingMd

                Label {
                    text: typeof session !== "undefined"
                        ? qsTr("%1 torrents · %2 active").arg(session.torrentCount).arg(session.activeCount)
                        : qsTr("0 torrents")
                    color: Theme.muted
                    font.pixelSize: Theme.fontCaption
                }

                Item { Layout.fillWidth: true }

                Label {
                    text: typeof session !== "undefined"
                        ? qsTr("↓ %1   ↑ %2   ·   Total: %3 down · %4 up   ·   Ratio %5")
                            .arg(session.totalDownSpeed).arg(session.totalUpSpeed)
                            .arg(session.totalDownloaded).arg(session.totalUploaded)
                            .arg(session.globalRatio)
                        : ""
                    color: Theme.muted
                    font.pixelSize: Theme.fontCaption
                }
            }
        }
    }
}
