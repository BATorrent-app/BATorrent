// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Keyboard shortcuts for the player window. Host passes pw + panel refs so
// Escape can dismiss options / subs / PiP / fullscreen before closing.
import QtQuick
import QtQuick.Window

Item {
    id: root

    property var pw
    // Named differently from host ids — same-name bindings self-shadow to undefined.
    property var optionsPanel
    property var subsPanel
    property bool extSubsActive: false

    Shortcut { sequence: "["; enabled: root.extSubsActive; onActivated: if (pw) pw.bumpSubOffset(-500) }
    Shortcut { sequence: "]"; enabled: root.extSubsActive; onActivated: if (pw) pw.bumpSubOffset(500) }
    Shortcut { sequence: "Space";  onActivated: if (pw) pw.togglePlay() }
    Shortcut { sequence: "P";      onActivated: if (pw) pw.togglePip() }
    Shortcut { sequence: "Right";  onActivated: if (pw) pw.seekBy(10000) }
    Shortcut { sequence: "Left";   onActivated: if (pw) pw.seekBy(-10000) }
    Shortcut { sequence: "Up";     onActivated: if (pw) pw.bumpVolume(0.05) }
    Shortcut { sequence: "Down";   onActivated: if (pw) pw.bumpVolume(-0.05) }
    Shortcut { sequence: "F";      onActivated: if (pw) pw.toggleFullscreen() }
    Shortcut { sequence: "M";      onActivated: if (pw) pw.muted = !pw.muted }
    Shortcut {
        sequence: "Escape"
        onActivated: {
            if (optionsPanel && optionsPanel.open) { optionsPanel.closePanel(); return }
            if (subsPanel && subsPanel.open) { subsPanel.closePanel(); return }
            if (!pw) return
            if (pw.pipMode) { pw.togglePip(); return }
            if (pw.visibility === Window.FullScreen) pw.visibility = Window.Windowed
            else pw.close()
        }
    }
}
