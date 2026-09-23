// Put this inside a Flickable so each mouse-wheel notch scrolls a useful amount.
// A bare Flickable moves only a few px per notch on Windows. It reuses the
// Flickable's flick() so the motion stays smooth, and only handles real mouse
// wheels; touchpads keep their native pixel scrolling.
import QtQuick

WheelHandler {
    id: h
    required property Flickable flick
    property real factor: 6.5          // velocity per wheel-notch unit (angleDelta/120 ≈ one notch)
    acceptedDevices: PointerDevice.Mouse

    onWheel: function(ev) {
        const max = Math.max(0, h.flick.contentHeight - h.flick.height)
        if (max <= 0) { ev.accepted = false; return }     // nothing to scroll → let it bubble
        // Horizontal-wheel / shift-scroll: leave to default.
        if (ev.angleDelta.y === 0) { ev.accepted = false; return }
        // flick() REPLACES the velocity, so spinning fast would crawl at one
        // notch's worth: stack onto the current velocity when same-direction.
        // (verticalVelocity and flick() use opposite sign conventions.)
        const add = ev.angleDelta.y * h.factor
        const cur = -h.flick.verticalVelocity
        h.flick.flick(0, (cur !== 0 && Math.sign(cur) === Math.sign(add)) ? cur + add : add)
        ev.accepted = true
    }
}
