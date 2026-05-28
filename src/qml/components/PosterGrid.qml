import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import "../theme"

GridView {
    id: grid

    cellWidth: 196
    cellHeight: 310
    topMargin: 8
    bottomMargin: 12
    leftMargin: 16
    rightMargin: 16
    clip: true
    boundsBehavior: Flickable.StopAtBounds

    signal torrentSelected(int sourceRow)
    signal torrentDoubleClicked(int sourceRow)

    property int selectedIndex: -1

    function openContextMenu(x, y) {
        contextMenu.x = x
        contextMenu.y = y
        contextMenu.open()
    }

    delegate: PosterCard {
        id: del
        width: grid.cellWidth
        height: grid.cellHeight

        required property int index
        required property var model

        torrentName: model.torrentName
        stateKey: model.stateKey
        progress: model.progress
        posterPath: model.posterPath
        metaTitle: model.metaTitle
        stateString: model.stateString
        size: model.size

        isSelected: grid.selectedIndex === del.index

        TapHandler {
            onTapped: {
                grid.selectedIndex = del.index
                grid.torrentSelected(del.index)
            }
            onDoubleTapped: grid.torrentDoubleClicked(del.index)
        }

        onContextRequested: function(pos) {
            grid.selectedIndex = del.index
            grid.torrentSelected(del.index)
            var global = del.mapToItem(grid, pos.x, pos.y)
            contextMenu.x = global.x
            contextMenu.y = global.y
            contextMenu.open()
        }
    }

    Menu {
        id: contextMenu
        modal: false
        padding: 4
        implicitWidth: 220

        background: Rectangle {
            color: Theme.panel
            border.color: Theme.border
            border.width: 1
            radius: Theme.radiusSm
        }

        delegate: MenuItem {
            id: mi
            implicitHeight: 28
            padding: 0

            contentItem: RowLayout {
                spacing: 8
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 10

                Label {
                    Layout.preferredWidth: 14
                    text: mi.action && mi.action.checkable && mi.action.checked ? "✓" : ""
                    color: Theme.accent
                    font.pixelSize: Theme.fontCaption
                    horizontalAlignment: Text.AlignHCenter
                }
                Label {
                    Layout.fillWidth: true
                    text: mi.text
                    color: mi.enabled ? Theme.text : Theme.dim
                    font.pixelSize: Theme.fontCaption
                    elide: Text.ElideRight
                }
            }

            background: Rectangle {
                color: mi.highlighted ? Theme.accentTint : "transparent"
                radius: 4
            }
        }

        Action {
            icon.source: "qrc:/icons/play.svg"
            text: qsTr("Resume")
            onTriggered: if (typeof session !== "undefined") session.resumeSelected()
        }
        Action {
            icon.source: "qrc:/icons/pause.svg"
            text: qsTr("Pause")
            onTriggered: if (typeof session !== "undefined") session.pauseSelected()
        }

        MenuSeparator { contentItem: Rectangle { implicitHeight: 1; color: Theme.border } }

        Action {
            text: qsTr("Force start")
            checkable: true
            checked: typeof session !== "undefined" && session.selectedForceStart
            onTriggered: if (typeof session !== "undefined") session.setSelectedForceStart(checked)
        }
        Action {
            text: qsTr("Super seeding")
            enabled: typeof session !== "undefined" && session.selectedAtFullProgress
            checkable: true
            checked: typeof session !== "undefined" && session.selectedSuperSeeding
            onTriggered: if (typeof session !== "undefined") session.setSelectedSuperSeeding(checked)
        }

        MenuSeparator { contentItem: Rectangle { implicitHeight: 1; color: Theme.border } }

        Action {
            text: qsTr("Mark completed")
            enabled: typeof session !== "undefined" && session.selectedAtFullProgress && !session.selectedCompleted
            onTriggered: if (typeof session !== "undefined") session.markSelectedCompleted()
        }
        Action {
            text: qsTr("Unmark completed")
            enabled: typeof session !== "undefined" && session.selectedCompleted
            onTriggered: if (typeof session !== "undefined") session.unmarkSelectedCompleted()
        }

        MenuSeparator { contentItem: Rectangle { implicitHeight: 1; color: Theme.border } }

        Action {
            text: qsTr("Open folder")
            onTriggered: if (typeof session !== "undefined") session.openSaveFolder()
        }
        Action {
            text: qsTr("Copy magnet")
            onTriggered: if (typeof session !== "undefined") session.copyMagnetLink()
        }
        Action {
            text: qsTr("Copy hash")
            onTriggered: if (typeof session !== "undefined") session.copyInfoHash()
        }

        MenuSeparator { contentItem: Rectangle { implicitHeight: 1; color: Theme.border } }

        Action {
            text: qsTr("Move queue up")
            onTriggered: if (typeof session !== "undefined") session.queueUpSelected()
        }
        Action {
            text: qsTr("Move queue down")
            onTriggered: if (typeof session !== "undefined") session.queueDownSelected()
        }
        Action {
            text: qsTr("Force recheck")
            onTriggered: if (typeof session !== "undefined") session.forceRecheckSelected()
        }
        Action {
            text: qsTr("Force reannounce")
            onTriggered: if (typeof session !== "undefined") session.forceReannounceSelected()
        }

        MenuSeparator { contentItem: Rectangle { implicitHeight: 1; color: Theme.border } }

        Action {
            icon.source: "qrc:/icons/trash.svg"
            text: qsTr("Remove")
            onTriggered: if (typeof session !== "undefined") session.removeSelected()
        }
        Action {
            text: qsTr("Remove + delete files")
            onTriggered: if (typeof session !== "undefined") session.removeSelectedWithFiles()
        }
    }

    ScrollBar.vertical: ScrollBar {
        policy: ScrollBar.AsNeeded
        contentItem: Rectangle {
            implicitWidth: 5
            radius: 2.5
            color: Theme.border
            opacity: parent.active ? 0.7 : 0.3
            Behavior on opacity {
                OpacityAnimator { duration: 200 }
            }
        }
    }

    add: Transition {
        OpacityAnimator { from: 0; to: 1; duration: 250; easing.type: Easing.OutCubic }
        ScaleAnimator { from: 0.9; to: 1; duration: 250; easing.type: Easing.OutCubic }
    }

    remove: Transition {
        OpacityAnimator { from: 1; to: 0; duration: 200; easing.type: Easing.InCubic }
        ScaleAnimator { from: 1; to: 0.85; duration: 200; easing.type: Easing.InCubic }
    }

    displaced: Transition {
        NumberAnimation { properties: "x,y"; duration: 200; easing.type: Easing.OutQuad }
    }

    removeDisplaced: Transition {
        SequentialAnimation {
            PauseAnimation { duration: 150 }
            NumberAnimation { properties: "x,y"; duration: 180; easing.type: Easing.OutQuad }
        }
    }
}
