import QtQuick
import QtQuick.Layouts
import "."

Window {
    visible: true; width: 760; height: 560; color: Theme.bg; title: "RSS"
    readonly property string mono: "Menlo, 'SF Mono', monospace"
    ListModel { id: items
        ListElement { nm: "[SubsPlease] Frieren - 28 (1080p)"; meta: "1.3 GB · 2h"; auto: true }
        ListElement { nm: "[SubsPlease] Frieren - 27 (1080p)"; meta: "1.3 GB · 1s"; auto: true }
        ListElement { nm: "[Erai-raws] Frieren - 27 (720p)"; meta: "680 MB · 1s"; auto: false }
        ListElement { nm: "[SubsPlease] Frieren - 26 (1080p)"; meta: "1.3 GB · 1sem"; auto: true }
    }
    ColumnLayout { anchors.fill: parent; spacing: 0
        Rectangle { Layout.fillWidth: true; height: 36; color: Theme.elev; Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hairSoft }
            Text { anchors.centerIn: parent; text: "RSS"; color: Theme.t2; font.pixelSize: 12.5; font.bold: true } }
        RowLayout { Layout.fillWidth: true; Layout.fillHeight: true; spacing: 0
            Rectangle { Layout.preferredWidth: 232; Layout.fillHeight: true; color: "transparent"; Rectangle { anchors.right: parent.right; width: 1; height: parent.height; color: Theme.hair }
                ColumnLayout { anchors.fill: parent; anchors.margins: 12; spacing: 1
                    Repeater { model: [["Nyaa · Anime 1080p","12",true],["EZTV · Séries","31",false],["Linux ISOs","4",false]]
                        delegate: Rectangle { Layout.fillWidth: true; height: 38; radius: 7; color: index===0 ? Theme.sel : "transparent"
                            RowLayout { anchors.fill: parent; anchors.leftMargin: 11; anchors.rightMargin: 11; spacing: 10
                                Rectangle { width: 7; height: 7; radius: 3.5; color: modelData[2] ? Theme.up : Theme.t4 }
                                Text { text: modelData[0]; color: index===0 ? Theme.t1 : Theme.t2; font.pixelSize: 12.5; Layout.fillWidth: true; elide: Text.ElideRight }
                                Text { text: modelData[1]; color: Theme.t4; font.pixelSize: 11; font.family: mono } } } }
                }
            }
            ColumnLayout { Layout.fillWidth: true; Layout.fillHeight: true; spacing: 0
                Rectangle { Layout.fillWidth: true; height: 44; color: Theme.panel; Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hairSoft }
                    RowLayout { anchors.fill: parent; anchors.leftMargin: 24; anchors.rightMargin: 24; spacing: 9
                        Text { text: "⚙"; color: Theme.accentText; font.pixelSize: 14 }
                        Text { text: "Auto-baixar itens que contenham "; color: Theme.t2; font.pixelSize: 11.5 }
                        Text { text: "1080p"; color: Theme.t1; font.pixelSize: 11.5; font.bold: true }
                        Item { Layout.fillWidth: true }
                        Rectangle { width: 38; height: 21; radius: 11; color: Theme.accent; Rectangle { width: 15; height: 15; radius: 8; color: "#fff"; x: 21; anchors.verticalCenter: parent.verticalCenter } } } }
                ListView { Layout.fillWidth: true; Layout.fillHeight: true; model: items; interactive: false; clip: true
                    delegate: Rectangle { width: ListView.view.width; height: 46; color: "transparent"; Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hairSoft }
                        RowLayout { anchors.fill: parent; anchors.leftMargin: 24; anchors.rightMargin: 24; spacing: 12
                            Text { text: model.nm; Layout.fillWidth: true; color: Theme.t1; font.pixelSize: 12.5; elide: Text.ElideRight }
                            Rectangle { visible: model.auto; radius: 999; color: Qt.rgba(0.25,0.73,0.31,0.12); border.color: Qt.rgba(0.25,0.73,0.31,0.3); border.width: 1; implicitWidth: 40; implicitHeight: 18; Text { anchors.centerIn: parent; text: "Auto"; color: Theme.up; font.pixelSize: 9.5; font.bold: true } }
                            Text { text: model.meta; color: Theme.t4; font.pixelSize: 11; font.family: mono } } }
                }
            }
        }
    }
}
