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
    // Named differently from host ids: same-name bindings self-shadow to undefined.
    property var optionsPanel
    property var morePanel
    property var skipChip
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
    Shortcut { sequence: "Shift+Right"; onActivated: if (pw) pw.seekBy(60000) }
    Shortcut { sequence: "Shift+Left";  onActivated: if (pw) pw.seekBy(-60000) }
    Shortcut { sequence: "Home";   onActivated: if (pw) pw.seekFraction(0) }
    Shortcut { sequence: "N";      onActivated: if (pw && pw.hasNext) pw.playNext() }
    Shortcut { sequence: ",";      onActivated: if (pw) pw.frameStep(-1) }
    Shortcut { sequence: ".";      onActivated: if (pw) pw.frameStep(1) }
    Shortcut { sequence: "Z";      onActivated: if (pw) pw.cycleAspect() }
    Shortcut { sequence: "R";      onActivated: if (pw) pw.rotate() }
    Shortcut { sequence: "T";      onActivated: if (pw) pw.toggleOnTop() }
    Shortcut { sequence: "Ctrl+S"; onActivated: if (pw) pw.screenshot() }
    Shortcut { sequences: ["?", "H"]; onActivated: if (pw) pw.helpOpen = !pw.helpOpen }
    // 1..9 jump to 10..90%, 0 to the start: the YouTube habit
    Repeater {
        model: 10
        Item {
            required property int index
            Shortcut { sequence: String(index); onActivated: if (root.pw) root.pw.seekFraction(index / 10) }
        }
    }
    // S / A step through the tracks the same way the options panel picks
    // them, so the choice is remembered for the torrent like a click is
    Shortcut {
        sequence: "S"
        onActivated: {
            // over an opening or ending, S skips it (the chip says so)
            if (skipChip && skipChip.skipNow()) return
            if (!pw || !optionsPanel) return
            var n = pw.subtitleTrackCount(), cur = pw.activeSubtitle()
            if (n === 0) return
            if (cur + 1 >= n) optionsPanel.selectSubOff()
            else optionsPanel.selectSubTrack(cur + 1)
            pw.notice(pw.trackLabel("sub"))
        }
    }
    Shortcut {
        sequence: "A"
        onActivated: {
            if (!pw || !optionsPanel) return
            var n = pw.audioTrackCount()
            if (n < 2) return
            optionsPanel.selectAudioTrack((pw.activeAudio() + 1) % n)
            pw.notice(pw.trackLabel("audio"))
        }
    }
    Shortcut {
        sequence: "Escape"
        onActivated: {
            if (pw && pw.helpOpen) { pw.helpOpen = false; return }
            if (morePanel && morePanel.open) { morePanel.closePanel(); return }
            if (optionsPanel && optionsPanel.open) {
                if (optionsPanel.online) optionsPanel.online = false
                else optionsPanel.closePanel()
                return
            }
            if (!pw) return
            if (pw.pipMode) { pw.togglePip(); return }
            if (pw.visibility === Window.FullScreen) pw.visibility = Window.Windowed
            else pw.close()
        }
    }
}
