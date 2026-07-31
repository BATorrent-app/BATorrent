// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

import QtQuick
import "windows"

TrayPopupWindow {
    required property var host

    onShowApp:      { host.show(); host.raise(); host.requestActivate() }
    onOpenTorrent:  { host.show(); host.raise(); host.requestActivate(); host.openFileDlg.open() }
    onOpenMagnet:   { host.show(); host.raise(); host.requestActivate(); host.magnetDlg.open() }
    onPauseAll:     if (typeof session !== "undefined") session.pauseAll()
    onResumeAll:    if (typeof session !== "undefined") session.resumeAll()
    onOpenSettings: { host.show(); host.raise(); host.requestActivate(); host.currentPage = 3 }
    onQuitApp:      Qt.quit()
}
