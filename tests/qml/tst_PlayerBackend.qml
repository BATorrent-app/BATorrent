// SPDX-License-Identifier: MIT
// PlayerBackend: the isolated-decoder switch hands the player the right
// decoder and keeps the other one idle, with source/volume/mute carried over.

import QtQuick
import QtTest
import "qrc:/src/qml/player"

Item {
    id: root
    width: 200
    height: 200

    Component {
        id: backendComp
        PlayerBackend {}
    }

    TestCase {
        name: "PlayerBackend"
        when: windowShown

        function test_stockByDefault() {
            var b = createTemporaryObject(backendComp, root, { source: "http://127.0.0.1:1/stream/x/0" })
            verify(!!b)
            verify(!b.isolated)
            verify(String(b.player).indexOf("IsolatedMediaPlayer") < 0)
            compare(String(b.player.source), "http://127.0.0.1:1/stream/x/0")
        }

        function test_isolatedTakesTheSource() {
            var b = createTemporaryObject(backendComp, root, { isolated: true, volume: 0.4 })
            verify(String(b.player).indexOf("IsolatedMediaPlayer") >= 0)
            compare(b.player.volume, 0.4)
            compare(b.player.mediaStatus, 0)
            compare(b.player.playbackState, 0)
            verify(!b.player.muted)
        }

        function test_silentThumbnailsStayMuted() {
            var b = createTemporaryObject(backendComp, root, { isolated: true, audible: false })
            verify(b.player.muted)
        }
    }
}
