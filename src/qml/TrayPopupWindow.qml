// Rich tray popup shown on right-click of the system tray icon. Mirrors the
// legacy TrayPopup: header (logo + name + counts), DOWN/UP speed strip, and a
// list of menu actions. VPN / auto-shutdown rows from the legacy version are
// omitted — the QML session bridge doesn't expose those yet.
import QtQuick
import QtQuick.Effects
import QtQuick.Layouts
import "theme"
import "widgets"

Window {
    id: pop
    width: 280
    height: col.implicitHeight + 2
    flags: Qt.Popup | Qt.FramelessWindowHint | Qt.NoDropShadowWindowHint
    color: "transparent"
    visible: false

    // actions delegated to Main (keeps this file free of global ids)
    signal showApp()
    signal openTorrent()
    signal openMagnet()
    signal pauseAll()
    signal resumeAll()
    signal openSettings()
    signal quitApp()

    readonly property var sess: typeof session !== "undefined" ? session : null

    // Show anchored to a screen corner: bottom-right on Windows/Linux (taskbar
    // tray), top-right on macOS (menu bar). Clamped to the available geometry.
    function popUp() {
        var scr = Qt.application.screens[0]
        var g = scr ? Qt.rect(scr.virtualX, scr.virtualY, scr.width, scr.height) : Qt.rect(0,0,1920,1080)
        var margin = 8
        pop.x = g.x + g.width - pop.width - margin
        if (Qt.platform.os === "osx")
            pop.y = g.y + margin + 24
        else
            pop.y = g.y + g.height - pop.height - margin - 40
        pop.show()
        pop.raise()
        pop.requestActivate()
    }

    onActiveChanged: if (!active) pop.hide()   // click-away closes

    Rectangle {
        anchors.fill: parent
        radius: 10
        color: Theme.panel
        border.color: Theme.hair
        border.width: 1

        ColumnLayout {
            id: col
            anchors.fill: parent
            anchors.margins: 1
            spacing: 0

            // ── header ──
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 56
                color: Theme.bg
                topLeftRadius: 10; topRightRadius: 10
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 12; anchors.rightMargin: 12
                    spacing: 10
                    Image {
                        source: "qrc:/images/logo.svg"
                        sourceSize: Qt.size(56, 56)
                        Layout.preferredWidth: 28; Layout.preferredHeight: 28
                        fillMode: Image.PreserveAspectFit
                        layer.enabled: Theme.isLight
                        layer.effect: MultiEffect { colorization: 1.0; colorizationColor: Theme.t1 }
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 1
                        Text { text: "BATorrent"; color: Theme.t1; font.pointSize: 12.5; font.weight: Font.Bold; font.family: Theme.fontSans }
                        Text {
                            text: (pop.sess ? pop.sess.torrentCount : 0) + " torrents · "
                                + (pop.sess ? pop.sess.activeCount : 0) + " ativos"
                            color: Theme.t3; font.pointSize: 10; font.family: Theme.fontMono
                        }
                    }
                    Rectangle {
                        width: 8; height: 8; radius: 4
                        Layout.alignment: Qt.AlignTop
                        color: (pop.sess && pop.sess.activeCount > 0) ? Theme.amber : Theme.t4
                    }
                }
            }

            // ── speed strip ──
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 58
                color: Theme.field
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 12; anchors.rightMargin: 12
                    spacing: 16
                    Repeater {
                        model: [
                            { eb: "↓ DOWN", val: pop.sess ? pop.sess.totalDownSpeed : "0 B/s" },
                            { eb: "↑ UP",   val: pop.sess ? pop.sess.totalUpSpeed   : "0 B/s" }
                        ]
                        delegate: ColumnLayout {
                            required property var modelData
                            Layout.fillWidth: true
                            spacing: 2
                            Text { text: modelData.eb; color: Theme.t4; font.pointSize: 8; font.weight: Font.Bold; font.letterSpacing: 1.2; font.family: Theme.fontSans }
                            Text { text: modelData.val; color: Theme.t1; font.pointSize: 14; font.weight: Font.Bold; font.family: Theme.fontMono }
                        }
                    }
                }
            }

            // ── menu rows ──
            component Row_: Rectangle {
                id: r
                property string label
                property string shortcut: ""
                property bool danger: false
                signal clicked()
                Layout.fillWidth: true
                Layout.preferredHeight: 32
                color: rma.containsMouse ? Theme.hover : "transparent"
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 14; anchors.rightMargin: 12
                    Text { text: r.label; color: r.danger ? Theme.accentText : Theme.t1; font.pointSize: 11; font.family: Theme.fontSans; Layout.fillWidth: true }
                    Text { text: r.shortcut; visible: r.shortcut.length > 0; color: Theme.t4; font.pointSize: 9.5; font.family: Theme.fontMono }
                }
                MouseArea { id: rma; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: { r.clicked(); pop.hide() } }
            }
            component Div_: Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: Theme.hairSoft }

            Item { Layout.preferredHeight: 4; Layout.fillWidth: true }
            Row_ { label: qsTr("Mostrar");          shortcut: "Ctrl+1"; onClicked: pop.showApp() }
            Row_ { label: qsTr("Abrir Torrent…");   shortcut: "Ctrl+O"; onClicked: pop.openTorrent() }
            Row_ { label: qsTr("Abrir Link Magnet…"); shortcut: "Ctrl+V"; onClicked: pop.openMagnet() }
            Div_ {}
            Row_ { label: qsTr("Pausar Todos");     onClicked: pop.pauseAll() }
            Row_ { label: qsTr("Continuar Todos");  onClicked: pop.resumeAll() }
            Div_ {}
            Row_ { label: qsTr("Preferências…");    shortcut: "Ctrl+,"; onClicked: pop.openSettings() }
            Row_ { label: qsTr("Sair"); danger: true; shortcut: "Ctrl+Q"; onClicked: pop.quitApp() }
            Item { Layout.preferredHeight: 4; Layout.fillWidth: true }
        }
    }
}
