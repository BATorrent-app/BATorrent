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

    Shortcut {
        sequence: StandardKey.Open
        onActivated: openDialog.open()
    }
    Shortcut {
        sequence: "Ctrl+M"
        onActivated: { magnetField.text = ""; magnetDialog.open() }
    }
    Shortcut {
        sequence: "Space"
        onActivated: if (typeof session !== "undefined") session.toggleSelectedPause()
    }
    Shortcut {
        sequence: StandardKey.Delete
        onActivated: if (typeof session !== "undefined") session.removeSelected()
    }
    Shortcut {
        sequence: StandardKey.Paste
        context: Qt.ApplicationShortcut
        onActivated: if (typeof session !== "undefined") session.smartPaste()
    }
    Shortcut {
        sequence: StandardKey.SelectAll
        onActivated: {
            if (root.posterMode) posterGrid.selectAll()
            else torrentTable.selectAll()
        }
    }
    Shortcut {
        sequences: ["Ctrl+Up", "Ctrl+Left"]
        onActivated: if (typeof session !== "undefined") session.queueUpSelected()
    }
    Shortcut {
        sequences: ["Ctrl+Down", "Ctrl+Right"]
        onActivated: if (typeof session !== "undefined") session.queueDownSelected()
    }

    Connections {
        target: typeof session !== "undefined" ? session : null
        function onQueueRefreshNeeded() {
            if (typeof torrentFilter === "undefined") return
            var sourceRows = session.selectedRows()
            var proxyRows = []
            for (var i = 0; i < sourceRows.length; ++i) {
                var pr = torrentFilter.mapFromSource(sourceRows[i])
                if (pr >= 0) proxyRows.push(pr)
            }
            torrentTable.selectedRows = proxyRows
            torrentTable.selectedIndex = proxyRows.length > 0
                ? proxyRows[proxyRows.length - 1] : -1
            torrentTable.anchorRow = proxyRows.length > 0 ? proxyRows[0] : -1

            posterGrid.selectedIndex = proxyRows.length > 0 ? proxyRows[0] : -1
        }
    }

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

    DropArea {
        id: dropZone
        anchors.fill: parent
        z: 100

        function hasTorrentUrl(drag) {
            if (!drag.hasUrls) return false
            for (var i = 0; i < drag.urls.length; ++i) {
                var u = drag.urls[i].toString()
                if (u.toLowerCase().endsWith(".torrent")) return true
            }
            return false
        }

        function hasMagnetText(drag) {
            if (!drag.hasText) return false
            return drag.text.indexOf("magnet:") === 0
        }

        onEntered: function(drag) {
            if (hasTorrentUrl(drag) || hasMagnetText(drag)) drag.accept()
            else drag.accepted = false
        }

        onDropped: function(drop) {
            if (typeof session === "undefined") return
            if (drop.hasUrls) {
                for (var i = 0; i < drop.urls.length; ++i) {
                    var u = drop.urls[i].toString()
                    if (u.toLowerCase().endsWith(".torrent"))
                        session.addTorrentFile(u)
                }
                drop.accept()
            } else if (drop.hasText && drop.text.indexOf("magnet:") === 0) {
                session.addMagnetUri(drop.text)
                drop.accept()
            }
        }
    }

    Rectangle {
        id: dropOverlay
        anchors.fill: parent
        z: 99
        color: Qt.rgba(0, 0, 0, 0.65)
        visible: opacity > 0.01
        opacity: dropZone.containsDrag ? 1 : 0
        Behavior on opacity { OpacityAnimator { duration: 150; easing.type: Easing.OutCubic } }

        Rectangle {
            anchors.centerIn: parent
            width: 360
            height: 200
            radius: Theme.radiusLg
            color: Theme.panel
            border.color: Theme.accent
            border.width: 2

            scale: dropZone.containsDrag ? 1.0 : 0.95
            Behavior on scale { NumberAnimation { duration: 180; easing.type: Easing.OutBack } }

            ColumnLayout {
                anchors.centerIn: parent
                spacing: 12
                width: parent.width - 32

                Image {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.preferredWidth: 56
                    Layout.preferredHeight: 56
                    source: "qrc:/icons/magnet.svg"
                    sourceSize: Qt.size(112, 112)
                    opacity: 0.9
                }
                Label {
                    Layout.alignment: Qt.AlignHCenter
                    text: qsTr("Drop to add torrent")
                    color: Theme.text
                    font.pixelSize: Theme.fontHeading
                    font.weight: Font.Bold
                }
                Label {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.fillWidth: true
                    text: qsTr(".torrent files or magnet links")
                    color: Theme.muted
                    font.pixelSize: Theme.fontCaption
                    horizontalAlignment: Text.AlignHCenter
                }
            }
        }
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
            posterMode: root.posterMode

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
                    var global = torrentTable.mapToItem(viewSwitcher, x, y)
                    posterGrid.openContextMenu(global.x, global.y)
                }
                onSelectionRowsChanged: function(rows) {
                    if (typeof session === "undefined") return
                    var sourceRows = []
                    for (var i = 0; i < rows.length; ++i) {
                        var sr = (typeof torrentFilter !== "undefined")
                            ? torrentFilter.mapToSource(rows[i]) : rows[i]
                        if (sr >= 0) sourceRows.push(sr)
                    }
                    session.setSelectedRows(sourceRows)
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
