// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

import QtQuick
import QWindowKit
import "theme"

// Takes the system title bar off the main window (Windows) and hands its
// job to the navigation bar: the bar's empty stretch drags and double-click
// maximizes, the caption buttons keep snap layouts. Loaded only in builds
// with QWindowKit (BAT_FRAMELESS); without it the window keeps its frame.
WindowAgent {
    id: agent
    property Window window: null
    property Item navBar: null
    property bool ready: false

    function attach() {
        if (ready || !window || !navBar) return ready
        if (!agent.setup(window)) return false
        agent.setWindowAttribute("dark-mode", Theme.isDark)
        agent.setTitleBar(navBar)
        agent.setSystemButton(WindowAgent.Minimize, navBar.minButton)
        agent.setSystemButton(WindowAgent.Maximize, navBar.maxButton)
        agent.setSystemButton(WindowAgent.Close, navBar.closeButton)
        for (var i = 0; i < navBar.hitItems.length; ++i)
            agent.setHitTestVisible(navBar.hitItems[i], true)
        ready = true
        return true
    }
    readonly property bool dark: Theme.isDark
    onDarkChanged: if (ready) agent.setWindowAttribute("dark-mode", dark)
}
