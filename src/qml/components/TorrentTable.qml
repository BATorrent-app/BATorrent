import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import "../theme"

Rectangle {
    id: tableRoot
    color: Theme.bg

    property var model: null
    property int selectedIndex: -1
    property var selectedRows: []
    property int anchorRow: -1

    signal torrentSelected(int sourceRow)
    signal torrentDoubleClicked(int sourceRow)
    signal contextRequested(int sourceRow, real x, real y)
    signal selectionRowsChanged(var rows)

    function selectAll() {
        if (!model) return
        var n = model.rowCount ? model.rowCount() : (model.length || 0)
        var rows = []
        for (var i = 0; i < n; ++i) rows.push(i)
        tableRoot.selectedRows = rows
        tableRoot.selectedIndex = n > 0 ? n - 1 : -1
        tableRoot.anchorRow = 0
        tableRoot.selectionRowsChanged(rows)
    }

    function _selectRow(row, modifiers) {
        var rows = tableRoot.selectedRows.slice()
        var ctrl = (modifiers & Qt.ControlModifier) || (modifiers & Qt.MetaModifier)
        var shift = (modifiers & Qt.ShiftModifier)

        if (shift && tableRoot.anchorRow >= 0) {
            rows = []
            var from = Math.min(tableRoot.anchorRow, row)
            var to   = Math.max(tableRoot.anchorRow, row)
            for (var i = from; i <= to; ++i) rows.push(i)
        } else if (ctrl) {
            var idx = rows.indexOf(row)
            if (idx >= 0) rows.splice(idx, 1)
            else rows.push(row)
            tableRoot.anchorRow = row
        } else {
            rows = [row]
            tableRoot.anchorRow = row
        }
        tableRoot.selectedRows = rows
        tableRoot.selectedIndex = row
        tableRoot.selectionRowsChanged(rows)
    }

    readonly property int colSize: 80
    readonly property int colProgress: 90
    readonly property int colDownSpeed: 90
    readonly property int colUpSpeed: 90
    readonly property int colState: 90
    readonly property int colCategory: 100
    readonly property int colPeers: 70
    readonly property int paddingX: 12
    readonly property int colSpacing: 0

    readonly property int fixedColsTotal: colSize + colProgress + colDownSpeed + colUpSpeed + colState + colCategory + colPeers
    readonly property int colName: Math.max(180, tableRoot.width - 2 * paddingX - fixedColsTotal)

    function colorForStateKey(k) {
        if (k === "downloading") return Theme.stateDownloading
        if (k === "seeding")     return Theme.stateSeeding
        if (k === "finished")    return Theme.stateSeeding
        if (k === "completed")   return Theme.stateCompleted
        if (k === "error")       return Theme.stateError
        return Theme.statePaused
    }

    property string sortColumn: ""
    property bool sortAscending: true

    function toggleSort(col) {
        if (tableRoot.sortColumn === col) {
            tableRoot.sortAscending = !tableRoot.sortAscending
        } else {
            tableRoot.sortColumn = col
            tableRoot.sortAscending = true
        }
        if (typeof torrentFilter !== "undefined")
            torrentFilter.setSortColumn(col, tableRoot.sortAscending)
    }

    component HeaderCell: Item {
        id: hc
        height: parent.height
        property string col: ""
        property string title: ""
        property int extraLeftPadding: 0

        readonly property bool isActive: tableRoot.sortColumn === hc.col

        Row {
            anchors.left: parent.left
            anchors.leftMargin: hc.extraLeftPadding
            anchors.verticalCenter: parent.verticalCenter
            spacing: 4

            Label {
                anchors.verticalCenter: parent.verticalCenter
                text: hc.title
                color: hc.isActive ? Theme.text : (hcMouse.containsMouse ? Theme.text : Theme.muted)
                font.pixelSize: 11
                font.weight: Font.Black
                font.letterSpacing: 1.2
                Behavior on color { ColorAnimation { duration: 100 } }
            }
            Label {
                anchors.verticalCenter: parent.verticalCenter
                text: tableRoot.sortAscending ? "▲" : "▼"
                color: Theme.accent
                font.pixelSize: 8
                visible: hc.isActive
            }
        }

        MouseArea {
            id: hcMouse
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: tableRoot.toggleSort(hc.col)
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 36
            color: Theme.panel

            Row {
                anchors.fill: parent
                anchors.leftMargin: tableRoot.paddingX
                anchors.rightMargin: tableRoot.paddingX

                HeaderCell { width: tableRoot.colName;      col: "name";     title: qsTr("NAME");     extraLeftPadding: 26 }
                HeaderCell { width: tableRoot.colSize;      col: "size";     title: qsTr("SIZE") }
                HeaderCell { width: tableRoot.colProgress;  col: "progress"; title: qsTr("PROGRESS") }
                HeaderCell { width: tableRoot.colDownSpeed; col: "down";     title: qsTr("DOWN") }
                HeaderCell { width: tableRoot.colUpSpeed;   col: "up";       title: qsTr("UP") }
                HeaderCell { width: tableRoot.colState;     col: "state";    title: qsTr("STATE") }
                HeaderCell { width: tableRoot.colCategory;  col: "category"; title: qsTr("CATEGORY") }
                HeaderCell { width: tableRoot.colPeers;     col: "peers";    title: qsTr("PEERS") }
            }
        }

        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: Theme.border }

        Item {
            id: listArea
            Layout.fillWidth: true
            Layout.fillHeight: true

            property int hoveredRow: -1
            readonly property int rowHeight: 36

            ListView {
                id: list
                anchors.fill: parent
                clip: true
                model: tableRoot.model
                boundsBehavior: Flickable.StopAtBounds

            delegate: Item {
                id: row
                width: list.width
                height: 36

                required property int index
                required property var model

                readonly property bool isSelected: tableRoot.selectedRows.indexOf(row.index) >= 0
                readonly property bool isAlt: (row.index % 2) === 1
                readonly property string sk: row.model.stateKey || "paused"
                readonly property color stateColor: tableRoot.colorForStateKey(sk)
                readonly property bool isMuted: sk === "paused" || sk === "queued"
                readonly property color textColor: isMuted ? Theme.muted : Theme.text
                readonly property bool isActive: sk === "downloading" || sk === "seeding"

                Rectangle {
                    anchors.fill: parent
                    color: {
                        if (row.isSelected) return Theme.accentTint
                        if (listArea.hoveredRow === row.index) return Qt.rgba(Theme.surface.r, Theme.surface.g, Theme.surface.b, 0.6)
                        if (row.isAlt) return Qt.rgba(Theme.surface.r, Theme.surface.g, Theme.surface.b, 0.22)
                        return "transparent"
                    }
                    Behavior on color { ColorAnimation { duration: 80 } }
                }

                Row {
                    anchors.fill: parent
                    anchors.leftMargin: tableRoot.paddingX
                    anchors.rightMargin: tableRoot.paddingX

                    // Name + status dot
                    Item {
                        width: tableRoot.colName
                        height: parent.height

                        Item {
                            id: dotIcon
                            anchors.left: parent.left
                            anchors.verticalCenter: parent.verticalCenter
                            width: 18; height: 18

                            Rectangle {
                                anchors.centerIn: parent
                                width: 12; height: 12; radius: 6
                                color: row.stateColor
                                opacity: 0.30
                                visible: row.isActive
                            }
                            Rectangle {
                                anchors.centerIn: parent
                                width: 6; height: 6; radius: 3
                                color: row.stateColor
                            }
                        }

                        Label {
                            anchors.left: dotIcon.right
                            anchors.leftMargin: 8
                            anchors.right: parent.right
                            anchors.verticalCenter: parent.verticalCenter
                            text: row.model.torrentName || ""
                            color: row.textColor
                            font.pixelSize: Theme.fontBody
                            elide: Text.ElideRight
                        }
                    }

                    Label {
                        width: tableRoot.colSize
                        height: parent.height
                        text: row.model.size || ""
                        color: row.textColor
                        font.pixelSize: Theme.fontCaption
                        verticalAlignment: Text.AlignVCenter
                    }

                    // Progress bar — classic torrent style (filled bar + centered %)
                    Item {
                        width: tableRoot.colProgress
                        height: parent.height

                        Rectangle {
                            id: track
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.left: parent.left
                            anchors.leftMargin: 0
                            anchors.right: parent.right
                            anchors.rightMargin: 8
                            height: 18
                            radius: 4
                            color: Theme.surfaceAlt
                            clip: true

                            readonly property real prog: row.model.progress || 0

                            Rectangle {
                                anchors.top: parent.top
                                anchors.bottom: parent.bottom
                                anchors.left: parent.left
                                width: Math.max(track.prog > 0.001 ? 2 : 0, parent.width * track.prog)
                                color: row.stateColor
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
                        color: (row.model.downRate || 0) > 0 ? Theme.text : Theme.dim
                        font.pixelSize: Theme.fontCaption
                        verticalAlignment: Text.AlignVCenter
                    }

                    Label {
                        width: tableRoot.colUpSpeed
                        height: parent.height
                        text: row.model.upSpeed || ""
                        color: (row.model.upRate || 0) > 0 ? Theme.text : Theme.dim
                        font.pixelSize: Theme.fontCaption
                        verticalAlignment: Text.AlignVCenter
                    }

                    Label {
                        width: tableRoot.colState
                        height: parent.height
                        text: row.model.stateString || ""
                        color: row.stateColor
                        font.pixelSize: Theme.fontCaption
                        font.weight: Font.DemiBold
                        verticalAlignment: Text.AlignVCenter
                        elide: Text.ElideRight
                    }

                    Label {
                        width: tableRoot.colCategory
                        height: parent.height
                        text: row.model.category || ""
                        color: row.textColor
                        font.pixelSize: Theme.fontCaption
                        verticalAlignment: Text.AlignVCenter
                        elide: Text.ElideRight
                    }

                    Label {
                        width: tableRoot.colPeers
                        height: parent.height
                        text: row.model.numPeers !== undefined ? row.model.numPeers : ""
                        color: row.textColor
                        font.pixelSize: Theme.fontCaption
                        verticalAlignment: Text.AlignVCenter
                    }
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

            add: Transition {
                ParallelAnimation {
                    OpacityAnimator { from: 0; to: 1; duration: 220; easing.type: Easing.OutCubic }
                    NumberAnimation { property: "x"; from: -10; to: 0; duration: 220; easing.type: Easing.OutCubic }
                }
            }

            remove: Transition {
                ParallelAnimation {
                    OpacityAnimator { from: 1; to: 0; duration: 180; easing.type: Easing.InCubic }
                    NumberAnimation { property: "x"; to: 14; duration: 180; easing.type: Easing.InCubic }
                }
            }

            removeDisplaced: Transition {
                SequentialAnimation {
                    PauseAnimation { duration: 130 }
                    NumberAnimation { properties: "y"; duration: 180; easing.type: Easing.OutQuad }
                }
            }

            displaced: Transition {
                NumberAnimation { properties: "y"; duration: 240; easing.type: Easing.OutCubic }
            }

            move: Transition {
                NumberAnimation { properties: "y"; duration: 240; easing.type: Easing.OutCubic }
            }

            moveDisplaced: Transition {
                NumberAnimation { properties: "y"; duration: 240; easing.type: Easing.OutCubic }
            }
            }

            MouseArea {
                id: marqueeMouse
                anchors.fill: parent
                acceptedButtons: Qt.LeftButton | Qt.RightButton
                hoverEnabled: true
                z: 10

                property bool dragging: false
                property real startX: 0
                property real startY: 0
                property int startRow: -1

                function rowAt(my) {
                    if (!tableRoot.model || list.count <= 0) return -1
                    var contentY = list.contentY + my
                    var idx = Math.floor(contentY / listArea.rowHeight)
                    if (idx < 0 || idx >= list.count) return -1
                    return idx
                }

                onPositionChanged: function(mouse) {
                    listArea.hoveredRow = rowAt(mouse.y)
                    if (pressed && !dragging) {
                        if (Math.abs(mouse.x - startX) > 5 || Math.abs(mouse.y - startY) > 5)
                            dragging = true
                    }
                    if (dragging) {
                        var x1 = Math.min(startX, mouse.x)
                        var y1 = Math.min(startY, mouse.y)
                        var x2 = Math.max(startX, mouse.x)
                        var y2 = Math.max(startY, mouse.y)
                        marqueeRect.x = x1
                        marqueeRect.y = y1
                        marqueeRect.width = x2 - x1
                        marqueeRect.height = y2 - y1
                    }
                }

                onExited: listArea.hoveredRow = -1

                onPressed: function(mouse) {
                    startX = mouse.x
                    startY = mouse.y
                    startRow = rowAt(mouse.y)
                    dragging = false
                }

                onReleased: function(mouse) {
                    if (dragging) {
                        var y1Content = marqueeRect.y + list.contentY
                        var y2Content = y1Content + marqueeRect.height
                        var from = Math.max(0, Math.floor(y1Content / listArea.rowHeight))
                        var to = Math.min(list.count - 1, Math.floor(y2Content / listArea.rowHeight))
                        var rows = []
                        for (var i = from; i <= to; ++i) rows.push(i)
                        tableRoot.selectedRows = rows
                        tableRoot.selectedIndex = to
                        tableRoot.anchorRow = from
                        tableRoot.selectionRowsChanged(rows)
                        dragging = false
                        return
                    }
                    if (startRow < 0) return
                    if (mouse.button === Qt.RightButton) {
                        if (tableRoot.selectedRows.indexOf(startRow) < 0)
                            tableRoot._selectRow(startRow, Qt.NoModifier)
                        tableRoot.torrentSelected(startRow)
                        tableRoot.contextRequested(startRow, mouse.x, mouse.y)
                    } else {
                        tableRoot._selectRow(startRow, mouse.modifiers)
                        tableRoot.torrentSelected(startRow)
                    }
                }

                onDoubleClicked: function(mouse) {
                    var idx = rowAt(mouse.y)
                    if (idx >= 0) tableRoot.torrentDoubleClicked(idx)
                }
            }

            Rectangle {
                id: marqueeRect
                visible: marqueeMouse.dragging
                color: Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.18)
                border.color: Theme.accent
                border.width: 1
                radius: 2
                z: 11
            }
        }
    }
}
