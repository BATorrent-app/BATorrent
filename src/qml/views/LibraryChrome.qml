// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

import QtQuick
import QtQuick.Layouts
import "../widgets"

// Downloads chrome under the toolbar: filter + library + detail + status.
// Takes an explicit host Window (for FilterBar/LibraryView contracts); does
// not look up sibling ids in Main.
ColumnLayout {
    id: root
    spacing: 0

    required property var host
    required property var controller

    signal addMagnetRequested()
    signal addLinkRequested()
    signal renameFileRequested(int idx, string current)
    signal findRequested()
    signal findTitleRequested(var item)

    // nothing in the list: no filters, no inspector, nothing to act on
    readonly property bool empty: typeof session !== "undefined" && session.torrentCount === 0

    property alias filterBar: filterBar
    property alias libraryView: libraryView

    FilterBar {
        id: filterBar
        visible: !root.empty
        win: root.host
        controller: root.controller
    }

    RowLayout {
        Layout.fillWidth: true
        Layout.fillHeight: true
        spacing: 0
        LibraryView {
            id: libraryView
            win: root.host
            controller: root.controller
            onAddMagnetRequested: root.addMagnetRequested()
            onAddLinkRequested: root.addLinkRequested()
            onFindRequested: root.findRequested()
            onFindTitleRequested: function (it) { root.findTitleRequested(it) }
        }
        DetailSidebar {
            id: detailSidebar
            win: root.host
            controller: root.controller
            visible: !root.empty
            showInspector: root.controller.gridView
            onRenameFileRequested: function(idx, current) { root.renameFileRequested(idx, current) }
        }
    }

    DetailPanel {
        win: root.host
        controller: root.controller
        visible: !root.controller.gridView && !root.empty
        onRenameFileRequested: function(idx, current) { root.renameFileRequested(idx, current) }
    }

    StatusBar { onPortClicked: root.host.showWin(root.host.diagWinLoader) }
}
