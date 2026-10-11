// SPDX-License-Identifier: MIT
// Where an episode stands is one fact: finishing it marks it watched and
// clears the position; stopping in the middle saves the position and clears
// watched, so no screen can say "Watched" while another offers to resume.

import QtQuick
import QtTest
import "qrc:/src/qml/player"

Item {
    id: root
    width: 400
    height: 300

    QtObject {
        id: settings
        property var store: ({})
        function set(k, v) { var s = store; s[k] = v; store = s }
        function get(k) { return store[k] }
    }
    QtObject {
        id: mockPlayer
        property real duration: 3000000
        property real position: 0
        property bool seekable: true
        property int playbackState: 0
    }

    Component { id: resumeComp; PlayerResume { mediaPlayer: mockPlayer; infoHash: "h"; fileIndex: 1 } }

    TestCase {
        name: "PlayerResume"
        when: windowShown

        function init() { settings.store = ({}) }

        function test_finishingMarksWatchedAndClearsThePosition() {
            var r = createTemporaryObject(resumeComp, root)
            mockPlayer.position = mockPlayer.duration - 5000
            r.save()
            compare(settings.get("resume_h_1_watched"), true)
            compare(settings.get("resume_h_1"), 0)
        }

        function test_stoppingMidwayAfterFinishingIsNoLongerWatched() {
            var r = createTemporaryObject(resumeComp, root)
            mockPlayer.position = mockPlayer.duration - 5000
            r.save()
            mockPlayer.position = 450000
            r.save()
            compare(settings.get("resume_h_1"), 450000)
            compare(settings.get("resume_h_1_watched"), false)
        }

        function test_theFirstSecondsAreNotAPositionWorthKeeping() {
            var r = createTemporaryObject(resumeComp, root)
            mockPlayer.position = 3000
            r.save()
            compare(settings.get("resume_h_1"), undefined)
            compare(settings.get("resume_h_1_watched"), undefined)
        }
    }
}
