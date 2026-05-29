import QtQuick
import QtQuick.Layouts
import QtQuick.Shapes
import "."

Window {
    id: win
    visible: true
    width: 1360; height: 884
    color: Theme.bg
    title: "BATorrent"

    // ---------- data ----------
    ListModel {
        id: torrents
        ListElement { file: "Hollow.Knight-GOG.bin"; title: "Hollow Knight"; cat: "Jogos"; poster: "images/hollow.webp"; state: "dl"; label: "Baixando"; size: "5.20 GB"; pct: 42; down: "2.4 MB/s"; up: "180 KB/s"; peers: "24" }
        ListElement { file: "007.First.Light-RUNE.bin"; title: "007 First Light"; cat: "Jogos"; poster: "images/007.jpg"; state: "pa"; label: "Pausado"; size: "7.49 GB"; pct: 12; down: "—"; up: "—"; peers: "0" }
        ListElement { file: "Forza.Horizon.6-CODEX.bin"; title: "Forza Horizon 6"; cat: "Jogos"; poster: "images/forza.png"; state: "dl"; label: "Baixando"; size: "92.4 GB"; pct: 67; down: "6.1 MB/s"; up: "412 KB/s"; peers: "38" }
    }
    property int selected: 2
    property bool gridView: true

    function fillColor(s) {
        return s === "se" || s === "cp" ? Theme.up
             : s === "pa" ? Qt.rgba(0.5,0.5,0.55,1) : Theme.accent
    }
    function stateColor(s) {
        return s === "se" || s === "cp" ? Theme.up
             : s === "pa" ? Theme.t3 : Theme.accentText
    }

    readonly property string mono: "Menlo, 'SF Mono', monospace"
    readonly property string sans: "-apple-system, 'SF Pro Text', 'Segoe UI', sans-serif"

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // ===================== toolbar =====================
        Rectangle {
            Layout.fillWidth: true; height: 66; color: Theme.elev
            Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hair }
            RowLayout {
                anchors.fill: parent; anchors.leftMargin: 16; anchors.rightMargin: 16; spacing: 8
                // brand badge
                Rectangle {
                    width: 38; height: 38; radius: name === "darkstar" || true ? 9 : 9
                    color: Theme.accent
                    Image { anchors.centerIn: parent; width: 22; height: 22; source: "images/logo.svg"; fillMode: Image.PreserveAspectFit }
                }
                Rectangle { width: 1; height: 26; color: Theme.hair; Layout.leftMargin: 6; Layout.rightMargin: 6 }
                Repeater {
                    model: [["Abrir","📂"],["Magnet","🧲"],["Pausar","⏸"],["Retomar","▶"],["Parar","⏹"],["Remover","🗑"],["Buscar","🔍"],["RSS","📡"],["Config.","⚙"]]
                    delegate: ColumnLayout {
                        spacing: 3
                        Text { Layout.alignment: Qt.AlignHCenter; text: modelData[1]; color: Theme.t3; font.pixelSize: 16 }
                        Text { Layout.alignment: Qt.AlignHCenter; text: modelData[0]; color: Theme.t3; font.pixelSize: 10.5; font.family: win.sans }
                        Layout.rightMargin: 6
                    }
                }
                Item { Layout.fillWidth: true }
                // speed module
                Rectangle {
                    radius: 9; color: "transparent"; border.color: Theme.hair; border.width: 1
                    implicitWidth: row.width + 28; implicitHeight: 44
                    RowLayout {
                        id: row; anchors.centerIn: parent; spacing: 22
                        ColumnLayout { spacing: 2
                            Text { text: "DOWNLOAD"; color: Theme.t4; font.pixelSize: 9; font.bold: true; font.letterSpacing: 1 }
                            Text { text: "↓ 8.5 MB/s"; color: Theme.accentText; font.pixelSize: 13; font.family: win.mono; font.bold: true } }
                        ColumnLayout { spacing: 2
                            Text { text: "UPLOAD"; color: Theme.t4; font.pixelSize: 9; font.bold: true; font.letterSpacing: 1 }
                            Text { text: "↑ 1.7 MB/s"; color: Theme.up; font.pixelSize: 13; font.family: win.mono } }
                    }
                }
            }
        }

        // ===================== subbar =====================
        Rectangle {
            Layout.fillWidth: true; height: 54; color: "transparent"
            Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hair }
            RowLayout {
                anchors.fill: parent; anchors.leftMargin: 16; anchors.rightMargin: 16; spacing: 12
                Rectangle {
                    width: 240; height: 34; radius: 8; color: Theme.field; border.color: Theme.hair; border.width: 1
                    Text { anchors.left: parent.left; anchors.leftMargin: 12; anchors.verticalCenter: parent.verticalCenter; text: "Buscar torrents"; color: Theme.t4; font.pixelSize: 12.5; font.family: win.sans }
                }
                // view toggle
                Rectangle {
                    radius: 8; color: Theme.field; border.color: Theme.hair; border.width: 1
                    implicitWidth: seg.width + 4; height: 32
                    RowLayout { id: seg; anchors.centerIn: parent; spacing: 2
                        Repeater {
                            model: [["Grade",true],["Lista",false]]
                            delegate: Rectangle {
                                radius: 6; height: 28; implicitWidth: lbl.width + 22
                                color: (win.gridView === modelData[1]) ? Theme.sel : "transparent"
                                Text { id: lbl; anchors.centerIn: parent; text: modelData[0]; font.pixelSize: 11.5; font.family: win.sans
                                    color: (win.gridView === modelData[1]) ? Theme.t1 : Theme.t3 }
                                MouseArea { anchors.fill: parent; onClicked: win.gridView = modelData[1] }
                            }
                        }
                    }
                }
                // pills
                RowLayout { spacing: 4
                    Repeater {
                        model: [["Todos","3",true],["Baixando","2",false],["Semeando","0",false],["Pausado","1",false],["Concluído","0",false]]
                        delegate: Rectangle {
                            radius: 8; height: 30; implicitWidth: pl.width + 26
                            color: modelData[2] ? Theme.accent : "transparent"
                            RowLayout { id: pl; anchors.centerIn: parent; spacing: 7
                                Text { text: modelData[0]; font.pixelSize: 12; font.family: win.sans
                                    font.bold: modelData[2]; color: modelData[2] ? "#fff" : Theme.t3 }
                                Text { text: modelData[1]; font.pixelSize: 11; font.family: win.mono
                                    color: modelData[2] ? Qt.rgba(1,1,1,0.8) : Theme.t4 }
                            }
                        }
                    }
                }
                Item { Layout.fillWidth: true }
                // theme switcher (demo control)
                Rectangle {
                    radius: 8; height: 34; implicitWidth: tsw.width + 26; color: Theme.field; border.color: Theme.hair; border.width: 1
                    RowLayout { id: tsw; anchors.centerIn: parent; spacing: 8
                        Text { text: "Tema: " + Theme.name; color: Theme.t2; font.pixelSize: 12; font.family: win.sans }
                    }
                    MouseArea { anchors.fill: parent; onClicked: Theme.cycle() }
                }
                Rectangle {
                    radius: 8; height: 34; implicitWidth: an.width + 24; color: Theme.anime ? Theme.sel : Theme.field
                    border.color: Theme.anime ? Theme.accent : Theme.hair; border.width: 1
                    Text { id: an; anchors.centerIn: parent; text: "Anime"; color: Theme.anime ? Theme.accentText : Theme.t3; font.pixelSize: 12; font.family: win.sans }
                    MouseArea { anchors.fill: parent; onClicked: Theme.anime = !Theme.anime }
                }
            }
        }

        // ===================== content =====================
        Item {
            Layout.fillWidth: true; Layout.fillHeight: true; clip: true

            // anime accent art
            Image {
                visible: Theme.hasAnime
                source: Theme.hasAnime ? Theme.animeSource : ""
                fillMode: Image.PreserveAspectFit
                width: Theme.animeBottom ? 560 : 460
                opacity: win.gridView ? 0.9 : 0.6
                anchors.right: parent.right; anchors.rightMargin: -10
                anchors.top: Theme.animeBottom ? undefined : parent.top
                anchors.bottom: Theme.animeBottom ? parent.bottom : undefined
                anchors.bottomMargin: -80
            }

            // ---- GRID ----
            GridView {
                visible: win.gridView
                anchors.fill: parent; anchors.margins: 24
                cellWidth: 196; cellHeight: 300; interactive: false
                model: torrents
                delegate: Item {
                    width: 178; height: 286
                    Rectangle {
                        id: poster; width: 178; height: 237; radius: 10; clip: true
                        color: "#161618"; border.width: win.selected === index ? 2 : 1
                        border.color: win.selected === index ? Theme.accent : Theme.hair
                        Image { anchors.fill: parent; source: model.poster; fillMode: Image.PreserveAspectCrop }
                        Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 3; color: Qt.rgba(0,0,0,0.5)
                            Rectangle { height: parent.height; width: parent.width * model.pct/100; color: win.fillColor(model.state) } }
                        MouseArea { anchors.fill: parent; onClicked: win.selected = index }
                    }
                    RowLayout {
                        anchors.top: poster.bottom; anchors.topMargin: 12; width: 178
                        RowLayout { spacing: 6
                            Rectangle { width: 6; height: 6; radius: 3; color: win.fillColor(model.state) }
                            Text { text: model.label; color: win.stateColor(model.state); font.pixelSize: 11.5; font.family: win.sans } }
                        Item { Layout.fillWidth: true }
                        Text { text: model.size; color: Theme.t4; font.pixelSize: 11.5; font.family: win.mono }
                    }
                }
            }

            // ---- LIST ----
            ListView {
                visible: !win.gridView
                anchors.fill: parent; interactive: false
                model: torrents
                header: Rectangle {
                    width: ListView.view.width; height: 36; color: "transparent"
                    Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hair }
                    RowLayout { anchors.fill: parent; anchors.leftMargin: 16; anchors.rightMargin: 16; spacing: 16
                        Text { text: "NOME"; Layout.fillWidth: true; color: Theme.t4; font.pixelSize: 10.5; font.bold: true; font.letterSpacing: 0.6 }
                        Text { text: "TAMANHO"; Layout.preferredWidth: 78; horizontalAlignment: Text.AlignRight; color: Theme.t4; font.pixelSize: 10.5; font.bold: true }
                        Text { text: "PROGRESSO"; Layout.preferredWidth: 130; color: Theme.t4; font.pixelSize: 10.5; font.bold: true }
                        Text { text: "ESTADO"; Layout.preferredWidth: 100; color: Theme.t4; font.pixelSize: 10.5; font.bold: true }
                        Text { text: "PEERS"; Layout.preferredWidth: 56; horizontalAlignment: Text.AlignRight; color: Theme.t4; font.pixelSize: 10.5; font.bold: true }
                    }
                }
                delegate: Rectangle {
                    width: ListView.view.width; height: 48
                    color: win.selected === index ? Theme.sel : "transparent"
                    Rectangle { visible: win.selected === index; width: 2; height: parent.height; color: Theme.accent }
                    Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hairSoft }
                    RowLayout { anchors.fill: parent; anchors.leftMargin: 16; anchors.rightMargin: 16; spacing: 16
                        RowLayout { Layout.fillWidth: true; spacing: 12
                            Rectangle { width: 7; height: 7; radius: 3.5; color: win.fillColor(model.state) }
                            Text { text: model.file; color: Theme.t1; font.pixelSize: 13; font.family: win.sans; elide: Text.ElideRight; Layout.fillWidth: true } }
                        Text { text: model.size; Layout.preferredWidth: 78; horizontalAlignment: Text.AlignRight; color: Theme.t2; font.pixelSize: 12; font.family: win.mono }
                        // progress bar (black track, accent fill, white %)
                        Rectangle { Layout.preferredWidth: 130; height: 18; radius: 4; color: "#000"; border.color: Theme.hair; border.width: 1; clip: true
                            Rectangle { height: parent.height; width: parent.width * model.pct/100; color: win.fillColor(model.state) }
                            Text { anchors.centerIn: parent; text: model.pct + "%"; color: "#fff"; font.pixelSize: 10.5; font.bold: true; font.family: win.mono } }
                        Text { text: model.label; Layout.preferredWidth: 100; color: win.stateColor(model.state); font.pixelSize: 12; font.family: win.sans; font.bold: Theme.hasAnime }
                        Text { text: model.peers; Layout.preferredWidth: 56; horizontalAlignment: Text.AlignRight; color: Theme.t2; font.pixelSize: 12; font.family: win.mono }
                    }
                    MouseArea { anchors.fill: parent; onClicked: win.selected = index }
                }
            }
        }

        // ===================== graph =====================
        Rectangle {
            Layout.fillWidth: true; height: 64; color: Theme.bg
            Rectangle { anchors.top: parent.top; width: parent.width; height: 1; color: Theme.hair }
            Shape {
                anchors.fill: parent
                ShapePath {
                    strokeColor: Theme.accent; strokeWidth: 1.5; fillColor: "transparent"
                    startX: 0; startY: 44
                    PathCurve { x: win.width*0.5; y: 30 } PathCurve { x: win.width; y: 36 }
                }
                ShapePath {
                    strokeColor: Theme.up; strokeWidth: 1.5; fillColor: "transparent"
                    startX: 0; startY: 52
                    PathCurve { x: win.width*0.5; y: 46 } PathCurve { x: win.width; y: 50 }
                }
            }
        }

        // ===================== detail =====================
        Rectangle {
            Layout.fillWidth: true; height: 250; color: Theme.panel
            Rectangle { anchors.top: parent.top; width: parent.width; height: 1; color: Theme.hair }
            RowLayout {
                anchors.fill: parent; anchors.margins: 24; spacing: 32
                Rectangle { width: 104; height: 146; radius: 8; color: "#161618"; clip: true; border.color: Theme.hair; border.width: 1
                    Image { anchors.fill: parent; source: torrents.get(win.selected).poster; fillMode: Image.PreserveAspectCrop } }
                ColumnLayout { Layout.preferredWidth: 460; spacing: 6
                    Text { text: torrents.get(win.selected).title; color: Theme.t1; font.pixelSize: 17; font.bold: true; font.family: win.sans }
                    Text { text: "2026 · Racing, Simulator · 8.6/10"; color: Theme.t3; font.pixelSize: 11.5; font.family: win.sans }
                    Text { Layout.topMargin: 6; Layout.preferredWidth: 460; wrapMode: Text.WordWrap; color: Theme.t2; font.pixelSize: 12; font.family: win.sans
                        text: "Explore paisagens deslumbrantes ao volante de centenas de carros e torne-se uma lenda no Horizon Festival." }
                }
                Item { Layout.fillWidth: true }
                GridLayout { columns: 2; columnSpacing: 32; rowSpacing: 8
                    Repeater {
                        model: [["Download","6.1 MB/s"],["Upload","412 KB/s"],["Peers","38"],["Seeds","12"],["Ratio","0.18"],["ETA","14 min"]]
                        delegate: ColumnLayout { spacing: 3
                            Text { text: modelData[0]; color: Theme.t4; font.pixelSize: 10; font.bold: true; font.letterSpacing: 0.5 }
                            Text { text: modelData[1]; color: Theme.t1; font.pixelSize: 13; font.family: win.mono } }
                    }
                }
            }
        }

        // ===================== statusbar =====================
        Rectangle {
            Layout.fillWidth: true; height: 30; color: Theme.elev
            Rectangle { anchors.top: parent.top; width: parent.width; height: 1; color: Theme.hair }
            RowLayout { anchors.fill: parent; anchors.leftMargin: 16; anchors.rightMargin: 16; spacing: 8
                Text { text: "3 torrents · 2 ativos"; color: Theme.t4; font.pixelSize: 11.5; font.family: win.sans }
                Item { Layout.fillWidth: true }
                Rectangle { width: 6; height: 6; radius: 3; color: Theme.accent }
                Text { text: "8.5 MB/s"; color: Theme.t3; font.pixelSize: 11.5; font.family: win.mono }
                Rectangle { width: 6; height: 6; radius: 3; color: Theme.up }
                Text { text: "1.7 MB/s"; color: Theme.t3; font.pixelSize: 11.5; font.family: win.mono }
            }
        }
    }
}
