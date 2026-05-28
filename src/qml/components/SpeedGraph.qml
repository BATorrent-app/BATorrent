import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Shapes
import "../theme"

Item {
    id: graph

    implicitWidth: 300
    implicitHeight: 120

    readonly property var dl: typeof session !== "undefined" ? session.downloadHistory : []
    readonly property var ul: typeof session !== "undefined" ? session.uploadHistory : []
    readonly property int maxBytes: typeof session !== "undefined" ? session.historyMaxBytes : 1024
    readonly property int scaledMax: Math.max(1024, Math.round(graph.maxBytes * 1.2))
    readonly property int slots: 60

    readonly property color dlColor: Theme.stateDownloading
    readonly property color ulColor: "#fbbf24"
    readonly property bool hasTraffic: {
        for (var i = 0; i < dl.length; ++i) if (dl[i] > 0) return true
        for (var j = 0; j < ul.length; ++j) if (ul[j] > 0) return true
        return false
    }

    Rectangle {
        anchors.fill: parent
        color: Theme.surface
        radius: Theme.radiusSm
    }

    function _xAt(i, len) {
        var step = graph.width / (graph.slots - 1)
        var off = (graph.slots - len) * step
        return off + i * step
    }
    function _yAt(v) {
        return graph.height - (v / graph.scaledMax) * (graph.height - 14)
    }
    function buildPath(arr) {
        if (!arr || arr.length === 0) return ""
        var n = arr.length
        var bottom = graph.height
        var firstX = _xAt(0, n)
        var lastX  = _xAt(n - 1, n)
        var s = "M " + firstX.toFixed(1) + "," + bottom.toFixed(1)
        s += " L " + firstX.toFixed(1) + "," + _yAt(arr[0]).toFixed(1)
        for (var i = 1; i < n; ++i) {
            var px = _xAt(i - 1, n), py = _yAt(arr[i - 1])
            var x  = _xAt(i, n),     y  = _yAt(arr[i])
            var cx = (px + x) / 2
            s += " C " + cx.toFixed(1) + "," + py.toFixed(1) + " "
                      + cx.toFixed(1) + "," + y.toFixed(1) + " "
                      + x.toFixed(1) + "," + y.toFixed(1)
        }
        s += " L " + lastX.toFixed(1) + "," + bottom.toFixed(1) + " Z"
        return s
    }
    function formatScale(b) {
        if (b >= 1024 * 1024) return (b / (1024 * 1024)).toFixed(1) + " MB/s"
        return Math.round(b / 1024) + " KB/s"
    }

    readonly property string ulPath: buildPath(graph.ul)
    readonly property string dlPath: buildPath(graph.dl)

    Repeater {
        model: 3
        Rectangle {
            x: 0
            y: graph.height * (index + 1) / 4
            width: graph.width
            height: 1
            color: Theme.border
            opacity: 0.45
        }
    }

    Label {
        anchors.centerIn: parent
        text: qsTr("No traffic yet")
        color: Theme.dim
        font.pixelSize: Theme.fontCaption
        visible: !graph.hasTraffic
    }

    Shape {
        anchors.fill: parent
        antialiasing: true
        layer.enabled: true
        layer.samples: 4

        ShapePath {
            strokeWidth: 1.5
            strokeColor: graph.ulColor
            fillGradient: LinearGradient {
                x1: 0; y1: 0
                x2: 0; y2: graph.height
                GradientStop { position: 0.0; color: Qt.rgba(graph.ulColor.r, graph.ulColor.g, graph.ulColor.b, 0.18) }
                GradientStop { position: 1.0; color: Qt.rgba(graph.ulColor.r, graph.ulColor.g, graph.ulColor.b, 0.02) }
            }
            PathSvg { path: graph.ulPath }
        }

        ShapePath {
            strokeWidth: 1.5
            strokeColor: graph.dlColor
            fillGradient: LinearGradient {
                x1: 0; y1: 0
                x2: 0; y2: graph.height
                GradientStop { position: 0.0; color: Qt.rgba(graph.dlColor.r, graph.dlColor.g, graph.dlColor.b, 0.22) }
                GradientStop { position: 1.0; color: Qt.rgba(graph.dlColor.r, graph.dlColor.g, graph.dlColor.b, 0.02) }
            }
            PathSvg { path: graph.dlPath }
        }
    }

    Text {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.margins: 4
        text: graph.formatScale(graph.scaledMax)
        color: Theme.muted
        font.pixelSize: 9
    }

    Row {
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: 6
        spacing: 14

        Row {
            spacing: 5
            Rectangle { anchors.verticalCenter: parent.verticalCenter; width: 8; height: 8; radius: 4; color: graph.dlColor }
            Label {
                anchors.verticalCenter: parent.verticalCenter
                text: typeof session !== "undefined" ? "↓ " + session.totalDownSpeed : "↓ 0"
                color: Theme.text
                font.pixelSize: Theme.fontCaption
                font.weight: Font.DemiBold
            }
        }
        Row {
            spacing: 5
            Rectangle { anchors.verticalCenter: parent.verticalCenter; width: 8; height: 8; radius: 4; color: graph.ulColor }
            Label {
                anchors.verticalCenter: parent.verticalCenter
                text: typeof session !== "undefined" ? "↑ " + session.totalUpSpeed : "↑ 0"
                color: Theme.text
                font.pixelSize: Theme.fontCaption
                font.weight: Font.DemiBold
            }
        }
    }
}
