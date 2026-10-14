// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

import QtQuick
import QWindowKit
import "theme"

// Takes the system title bar off a window (Windows) and hands its job to
// the window's own chrome: the main window's navigation bar, the player's
// top strip. Its empty stretch drags and double-click maximizes, the caption
// buttons keep snap layouts. `navBar` supplies minButton/maxButton/
// closeButton, the items a click must still reach (hitItems), and optionally
// a titleStrip narrower than itself. Loaded only in builds with QWindowKit
// (BAT_FRAMELESS); without it the window keeps its frame.
WindowAgent {
    id: agent
    property Window window: null
    property Item navBar: null
    property bool ready: false

    function attach() {
        if (ready || !window || !navBar) return ready
        if (!agent.setup(window)) return false
        agent.setWindowAttribute("dark-mode", Theme.isDark)
        agent.setTitleBar(navBar.titleStrip !== undefined && navBar.titleStrip ? navBar.titleStrip : navBar)
        agent.setSystemButton(WindowAgent.Minimize, navBar.minButton)
        agent.setSystemButton(WindowAgent.Maximize, navBar.maxButton)
        agent.setSystemButton(WindowAgent.Close, navBar.closeButton)
        var hits = navBar.hitItems || []
        for (var i = 0; i < hits.length; ++i)
            agent.setHitTestVisible(hits[i], true)
        ready = true
        return true
    }
    readonly property bool dark: Theme.isDark
    onDarkChanged: if (ready) agent.setWindowAttribute("dark-mode", dark)
}
