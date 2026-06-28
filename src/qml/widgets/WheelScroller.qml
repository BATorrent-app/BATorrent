// Drop inside a Flickable to give mouse-wheel notches a usable, momentum-smooth
// step. A bare Flickable scrolls only a few px per notch on Windows — you had to
// spin the wheel many times. We reuse the Flickable's own flick() physics so the
// motion stays smooth (Mateus values that), and only touch real mouse wheels:
// touchpads keep their native pixel-precise scrolling.
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
        h.flick.flick(0, ev.angleDelta.y * h.factor)
        ev.accepted = true
    }
}
