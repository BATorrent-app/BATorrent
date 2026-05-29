// Source: BATorrent Detail Tabs.html + bat-detail.css (+ inline script data)
import QtQuick
import QtQuick.Layouts
import "theme"
import "widgets"

Window {
    id: win
    width: 940
    height: 420
    minimumWidth: 700
    minimumHeight: 360
    color: Theme.panel
    title: "Detalhes"

    property int tab: 1   // 0 Geral · 1 Peers (on por padrão) · 2 Arquivos · 3 Trackers · 4 Pedaços

    // ---- data ----
    ListModel {
        id: peers
        ListElement { ip: "192.168.1.42"; cli: "qBittorrent 4.6"; fl: "🇧🇷 D U"; pct: 98; dn: "1.2 MB/s"; up: "84 KB/s"; health: "grn" }
        ListElement { ip: "10.0.4.18"; cli: "Transmission 4.0"; fl: "🇺🇸 D"; pct: 72; dn: "880 KB/s"; up: "—"; health: "grn" }
        ListElement { ip: "89.34.12.7"; cli: "libtorrent 2.0"; fl: "🇩🇪 D U I"; pct: 54; dn: "640 KB/s"; up: "12 KB/s"; health: "amb" }
        ListElement { ip: "201.17.88.3"; cli: "BATorrent 2.6"; fl: "🇧🇷 U"; pct: 100; dn: "—"; up: "220 KB/s"; health: "grn" }
        ListElement { ip: "77.91.5.220"; cli: "Deluge 2.1"; fl: "🇳🇱 D"; pct: 31; dn: "410 KB/s"; up: "—"; health: "amb" }
        ListElement { ip: "45.12.9.88"; cli: "µTorrent 3.5"; fl: "🇫🇷 D E"; pct: 18; dn: "180 KB/s"; up: "—"; health: "red" }
        ListElement { ip: "123.45.6.78"; cli: "Vuze 5.7"; fl: "🇯🇵 D"; pct: 44; dn: "240 KB/s"; up: "8 KB/s"; health: "amb" }
    }
    ListModel {
        id: dfiles
        ListElement { nm: "Forza.Horizon.6-CODEX.bin"; pct: 100; sz: "78.0 GB"; pri: "Alta"; priClass: "high"; on: true }
        ListElement { nm: "data1.pak"; pct: 82; sz: "8.2 GB"; pri: "Normal"; priClass: ""; on: true }
        ListElement { nm: "data2.pak"; pct: 40; sz: "5.9 GB"; pri: "Normal"; priClass: ""; on: true }
        ListElement { nm: "setup.exe"; pct: 100; sz: "18 MB"; pri: "Alta"; priClass: "high"; on: true }
        ListElement { nm: "soundtrack.flac"; pct: 0; sz: "492 MB"; pri: "Pular"; priClass: "skip"; on: false }
        ListElement { nm: "readme.txt"; pct: 100; sz: "4 KB"; pri: "Baixa"; priClass: ""; on: true }
    }
    ListModel {
        id: trackers
        ListElement { url: "udp://tracker.opentrackr.org:1337"; st: "ok"; stTxt: "Funcionando"; sd: "12"; pr: "38"; le: "9"; nx: "28 min" }
        ListElement { url: "udp://tracker.openbittorrent.com:6969"; st: "ok"; stTxt: "Funcionando"; sd: "8"; pr: "22"; le: "4"; nx: "28 min" }
        ListElement { url: "https://tracker.example.org/announce"; st: "work"; stTxt: "Atualizando…"; sd: "—"; pr: "—"; le: "—"; nx: "agora" }
        ListElement { url: "udp://tracker.dead.example:80"; st: "err"; stTxt: "Tempo esgotado"; sd: "—"; pr: "—"; le: "—"; nx: "2 min" }
        ListElement { url: "** [DHT] **"; st: "ok"; stTxt: "Funcionando"; sd: "—"; pr: "14"; le: "—"; nx: "—" }
        ListElement { url: "** [PEX] **"; st: "ok"; stTxt: "Funcionando"; sd: "—"; pr: "6"; le: "—"; nx: "—" }
    }

    function healthColor(h) {
        return h === "grn" ? Theme.grn : h === "amb" ? Theme.amber : Theme.accent
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // ===== .dtabs (44px) =====
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 44
            color: "transparent"
            Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hair }

            Row {
                anchors.fill: parent
                anchors.leftMargin: Theme.sp5
                spacing: Theme.sp5

                Repeater {
                    model: [
                        { label: "Geral",    ct: "" },
                        { label: "Peers",    ct: "38" },
                        { label: "Arquivos", ct: "6" },
                        { label: "Trackers", ct: "4" },
                        { label: "Pedaços",  ct: "" }
                    ]
                    delegate: Item {
                        height: 44
                        width: tabRow.implicitWidth
                        readonly property bool on: win.tab === index

                        Row {
                            id: tabRow
                            anchors.verticalCenter: parent.verticalCenter
                            spacing: 7
                            Text {
                                anchors.verticalCenter: parent.verticalCenter
                                text: modelData.label
                                color: parent.parent.on ? Theme.t1 : (tabMa.containsMouse ? Theme.t2 : Theme.t3)
                                font.pointSize: 12.5
                                font.weight: Font.Medium
                                font.family: Theme.fontSans
                            }
                            Text {
                                visible: modelData.ct !== ""
                                anchors.verticalCenter: parent.verticalCenter
                                text: modelData.ct
                                color: Theme.t4
                                font.pointSize: 11
                                font.family: Theme.fontMono
                            }
                        }
                        Rectangle {
                            visible: parent.on
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.bottom: parent.bottom
                            height: 2
                            color: Theme.accent
                        }
                        MouseArea { id: tabMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: win.tab = index }
                    }
                }
            }
        }

        // ===== .dview (stacked panes) =====
        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: win.tab

            // --- 0: Geral ---
            RowLayout {
                Layout.margins: Theme.sp5
                spacing: Theme.sp6
                Rectangle { Layout.preferredWidth: 96; Layout.preferredHeight: 134; Layout.alignment: Qt.AlignTop; radius: 8; color: "#161618"; border.color: Theme.hair; border.width: 1 }
                ColumnLayout {
                    Layout.preferredWidth: 360
                    Layout.alignment: Qt.AlignTop
                    spacing: 6
                    Text { text: "Forza Horizon 6"; color: Theme.t1; font.pointSize: 16; font.weight: Font.DemiBold; font.family: Theme.fontSans }
                    Text { text: "2026 · Racing, Simulator, Sport · 8.6/10"; color: Theme.t3; font.pointSize: 11.5; font.family: Theme.fontSans }
                    Text {
                        Layout.fillWidth: true; Layout.topMargin: 6
                        wrapMode: Text.WordWrap
                        text: "Explore paisagens deslumbrantes ao volante de centenas de carros e torne-se uma lenda no Horizon Festival."
                        color: Theme.t2; font.pointSize: 12; font.family: Theme.fontSans; lineHeight: 1.5
                    }
                }
                Item { Layout.fillWidth: true }
                RowLayout {
                    Layout.alignment: Qt.AlignTop
                    spacing: Theme.sp6
                    DetailKVCol {
                        head: "TRANSFERÊNCIA"
                        rows: [["Baixado","62.1 GB"],["Download","6.1 MB/s"],["Upload","412 KB/s"],["ETA","14 min"]]
                    }
                    DetailKVCol {
                        head: "PEERS"
                        rows: [["Seeds","12"],["Peers","38"],["Ratio","0.18"]]
                    }
                }
            }

            // --- 1: Peers ---
            ColumnLayout {
                spacing: 0
                // header .thd
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 32
                    color: Theme.panel
                    Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hair }
                    RowLayout {
                        anchors.fill: parent; anchors.leftMargin: Theme.sp5; anchors.rightMargin: Theme.sp5
                        Text { text: "ENDEREÇO IP"; Layout.fillWidth: true; color: Theme.t4; font.pointSize: 10; font.weight: Font.DemiBold; font.letterSpacing: 0.6; font.family: Theme.fontSans }
                        Text { text: "CLIENTE"; Layout.preferredWidth: 150; color: Theme.t4; font.pointSize: 10; font.weight: Font.DemiBold; font.letterSpacing: 0.6; font.family: Theme.fontSans }
                        Text { text: "FLAGS"; Layout.preferredWidth: 90; color: Theme.t4; font.pointSize: 10; font.weight: Font.DemiBold; font.letterSpacing: 0.6; font.family: Theme.fontSans }
                        Text { text: "PROGRESSO"; Layout.preferredWidth: 64; horizontalAlignment: Text.AlignRight; color: Theme.t4; font.pointSize: 10; font.weight: Font.DemiBold; font.letterSpacing: 0.6; font.family: Theme.fontSans }
                        Text { text: "DOWN"; Layout.preferredWidth: 96; horizontalAlignment: Text.AlignRight; color: Theme.t4; font.pointSize: 10; font.weight: Font.DemiBold; font.letterSpacing: 0.6; font.family: Theme.fontSans }
                        Text { text: "UP"; Layout.preferredWidth: 96; horizontalAlignment: Text.AlignRight; color: Theme.t4; font.pointSize: 10; font.weight: Font.DemiBold; font.letterSpacing: 0.6; font.family: Theme.fontSans }
                    }
                }
                ListView {
                    Layout.fillWidth: true; Layout.fillHeight: true; clip: true; model: peers
                    boundsBehavior: Flickable.StopAtBounds
                    delegate: Rectangle {
                        width: ListView.view.width; height: 38; color: prMa.containsMouse ? Theme.hover : "transparent"
                        Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hairSoft }
                        RowLayout {
                            anchors.fill: parent; anchors.leftMargin: Theme.sp5; anchors.rightMargin: Theme.sp5
                            Text { text: model.ip; Layout.fillWidth: true; color: Theme.t1; font.pointSize: 12; font.family: Theme.fontMono }
                            Text { text: model.cli; Layout.preferredWidth: 150; color: Theme.t2; font.pointSize: 12; font.family: Theme.fontSans }
                            Text { text: model.fl; Layout.preferredWidth: 90; color: Theme.t4; font.pointSize: 11; font.family: Theme.fontMono }
                            Row {
                                Layout.preferredWidth: 64
                                layoutDirection: Qt.RightToLeft
                                spacing: 5
                                Text { anchors.verticalCenter: parent.verticalCenter; text: model.pct + "%"; color: Theme.t2; font.pointSize: 12; font.family: Theme.fontMono }
                                Rectangle { anchors.verticalCenter: parent.verticalCenter; width: 6; height: 6; radius: 3; color: win.healthColor(model.health) }
                            }
                            Text { text: model.dn; Layout.preferredWidth: 96; horizontalAlignment: Text.AlignRight; color: model.dn === "—" ? Theme.t4 : Theme.accentText; font.pointSize: 12; font.family: Theme.fontMono }
                            Text { text: model.up; Layout.preferredWidth: 96; horizontalAlignment: Text.AlignRight; color: model.up === "—" ? Theme.t4 : Theme.up; font.pointSize: 12; font.family: Theme.fontMono }
                        }
                        MouseArea { id: prMa; anchors.fill: parent; hoverEnabled: true; acceptedButtons: Qt.NoButton }
                    }
                }
            }

            // --- 2: Arquivos ---
            ListView {
                clip: true; model: dfiles
                boundsBehavior: Flickable.StopAtBounds
                delegate: Rectangle {
                    width: ListView.view.width; height: 40; color: "transparent"
                    Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hairSoft }
                    RowLayout {
                        anchors.fill: parent; anchors.leftMargin: Theme.sp5; anchors.rightMargin: Theme.sp5; spacing: 12
                        TChk { Layout.alignment: Qt.AlignVCenter; on: model.on; onToggled: function(v){ dfiles.setProperty(index,"on",v) } }
                        IconImg { Layout.alignment: Qt.AlignVCenter; src: "qrc:/icons/open.svg"; tint: Theme.t4; s: 15 }
                        Text { Layout.fillWidth: true; text: model.nm; color: Theme.t1; font.pointSize: 12; font.family: Theme.fontSans; elide: Text.ElideRight }
                        // .fp progress 128
                        RowLayout {
                            Layout.preferredWidth: 128
                            spacing: 9
                            Rectangle {
                                Layout.fillWidth: true; height: 5; radius: 3; color: Theme.track; clip: true
                                Rectangle { anchors.left: parent.left; height: parent.height; width: parent.width * model.pct/100; color: Theme.accent }
                            }
                            Text { text: model.pct + "%"; Layout.preferredWidth: 34; horizontalAlignment: Text.AlignRight; color: Theme.t2; font.pointSize: 11; font.family: Theme.fontMono }
                        }
                        Text { text: model.sz; Layout.preferredWidth: 74; horizontalAlignment: Text.AlignRight; color: Theme.t3; font.pointSize: 11; font.family: Theme.fontMono }
                        // .pri pseudo-select
                        Rectangle {
                            Layout.preferredWidth: 84; height: 24; radius: 6
                            color: Theme.field
                            border.color: model.priClass === "high" ? Qt.rgba(229/255,51/255,43/255,0.3) : Theme.hair
                            border.width: 1
                            RowLayout {
                                anchors.fill: parent; anchors.leftMargin: 8; anchors.rightMargin: 8; spacing: 6
                                Text {
                                    Layout.fillWidth: true
                                    text: model.pri
                                    color: model.priClass === "high" ? Theme.accentText : model.priClass === "skip" ? Theme.t4 : Theme.t2
                                    font.pointSize: 11
                                    font.family: Theme.fontSans
                                }
                                IconImg { src: "qrc:/icons/chevron.svg"; tint: Theme.t4; s: 11 }
                            }
                        }
                    }
                }
            }

            // --- 3: Trackers ---
            ColumnLayout {
                spacing: 0
                Rectangle {
                    Layout.fillWidth: true; Layout.preferredHeight: 32; color: Theme.panel
                    Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hair }
                    RowLayout {
                        anchors.fill: parent; anchors.leftMargin: Theme.sp5; anchors.rightMargin: Theme.sp5
                        Text { text: "URL"; Layout.fillWidth: true; color: Theme.t4; font.pointSize: 10; font.weight: Font.DemiBold; font.letterSpacing: 0.6; font.family: Theme.fontSans }
                        Text { text: "STATUS"; Layout.preferredWidth: 120; color: Theme.t4; font.pointSize: 10; font.weight: Font.DemiBold; font.letterSpacing: 0.6; font.family: Theme.fontSans }
                        Text { text: "SEEDS"; Layout.preferredWidth: 70; horizontalAlignment: Text.AlignRight; color: Theme.t4; font.pointSize: 10; font.weight: Font.DemiBold; font.letterSpacing: 0.6; font.family: Theme.fontSans }
                        Text { text: "PEERS"; Layout.preferredWidth: 70; horizontalAlignment: Text.AlignRight; color: Theme.t4; font.pointSize: 10; font.weight: Font.DemiBold; font.letterSpacing: 0.6; font.family: Theme.fontSans }
                        Text { text: "LEECH"; Layout.preferredWidth: 70; horizontalAlignment: Text.AlignRight; color: Theme.t4; font.pointSize: 10; font.weight: Font.DemiBold; font.letterSpacing: 0.6; font.family: Theme.fontSans }
                        Text { text: "PRÓX."; Layout.preferredWidth: 96; horizontalAlignment: Text.AlignRight; color: Theme.t4; font.pointSize: 10; font.weight: Font.DemiBold; font.letterSpacing: 0.6; font.family: Theme.fontSans }
                    }
                }
                ListView {
                    Layout.fillWidth: true; Layout.fillHeight: true; clip: true; model: trackers
                    boundsBehavior: Flickable.StopAtBounds
                    delegate: Rectangle {
                        width: ListView.view.width; height: 38; color: tkMa.containsMouse ? Theme.hover : "transparent"
                        Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hairSoft }
                        RowLayout {
                            anchors.fill: parent; anchors.leftMargin: Theme.sp5; anchors.rightMargin: Theme.sp5
                            Text { text: model.url; Layout.fillWidth: true; color: Theme.t2; font.pointSize: 11.5; font.family: Theme.fontMono; elide: Text.ElideRight }
                            Row {
                                Layout.preferredWidth: 120
                                spacing: 7
                                Rectangle {
                                    anchors.verticalCenter: parent.verticalCenter
                                    width: 6; height: 6; radius: 3
                                    color: model.st === "ok" ? Theme.grn : model.st === "work" ? Theme.t4 : Theme.accent
                                }
                                Text {
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: model.stTxt
                                    color: model.st === "ok" ? Theme.grn : model.st === "work" ? Theme.t3 : Theme.accentText
                                    font.pointSize: 12
                                    font.family: Theme.fontSans
                                }
                            }
                            Text { text: model.sd; Layout.preferredWidth: 70; horizontalAlignment: Text.AlignRight; color: model.sd === "—" ? Theme.t4 : Theme.t2; font.pointSize: 12; font.family: Theme.fontMono }
                            Text { text: model.pr; Layout.preferredWidth: 70; horizontalAlignment: Text.AlignRight; color: model.pr === "—" ? Theme.t4 : Theme.t2; font.pointSize: 12; font.family: Theme.fontMono }
                            Text { text: model.le; Layout.preferredWidth: 70; horizontalAlignment: Text.AlignRight; color: model.le === "—" ? Theme.t4 : Theme.t2; font.pointSize: 12; font.family: Theme.fontMono }
                            Text { text: model.nx; Layout.preferredWidth: 96; horizontalAlignment: Text.AlignRight; color: Theme.t4; font.pointSize: 12; font.family: Theme.fontMono }
                        }
                        MouseArea { id: tkMa; anchors.fill: parent; hoverEnabled: true; acceptedButtons: Qt.NoButton }
                    }
                }
            }

            // --- 4: Pedaços ---
            ColumnLayout {
                Layout.margins: Theme.sp5
                spacing: 0
                GridLayout {
                    Layout.fillWidth: true
                    columns: 40
                    columnSpacing: 3
                    rowSpacing: 3
                    Repeater {
                        model: 560
                        delegate: Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: width
                            radius: 2
                            color: {
                                var done = Math.floor(560 * 0.62)
                                if (index < done) return Theme.accent
                                if (index < done + 6) return (index % 2) ? Theme.amber : Qt.rgba(229/255,51/255,43/255,0.4)
                                return Theme.field
                            }
                        }
                    }
                }
                // .plegend
                RowLayout {
                    Layout.topMargin: Theme.sp4
                    spacing: Theme.sp5
                    Repeater {
                        model: [
                            { c: Theme.accent, t: "Concluído" },
                            { c: Theme.amber, t: "Baixando" },
                            { c: Qt.rgba(229/255,51/255,43/255,0.4), t: "Parcial" },
                            { c: Theme.field, t: "Pendente" }
                        ]
                        delegate: Row {
                            spacing: 7
                            Rectangle { anchors.verticalCenter: parent.verticalCenter; width: 10; height: 10; radius: 2; color: modelData.c }
                            Text { anchors.verticalCenter: parent.verticalCenter; text: modelData.t; color: Theme.t3; font.pointSize: 11; font.family: Theme.fontSans }
                        }
                    }
                }
                // .pinfo
                RowLayout {
                    Layout.topMargin: Theme.sp4
                    Layout.fillWidth: true
                    spacing: Theme.sp5
                    Rectangle { Layout.fillWidth: false; Layout.preferredWidth: 1; Layout.preferredHeight: 1; color: "transparent" }
                    Repeater {
                        model: [
                            { k: "PEDAÇOS", v: "14.760" },
                            { k: "TAMANHO", v: "8 MiB" },
                            { k: "CONCLUÍDOS", v: "9.889 (67%)" },
                            { k: "DISPONIBILIDADE", v: "2.41" }
                        ]
                        delegate: ColumnLayout {
                            spacing: 4
                            Text { text: modelData.k; color: Theme.t4; font.pointSize: 10; font.letterSpacing: 0.5; font.family: Theme.fontSans }
                            Text { text: modelData.v; color: Theme.t1; font.pointSize: 13; font.family: Theme.fontMono }
                        }
                    }
                    Item { Layout.fillWidth: true }
                }
                Item { Layout.fillHeight: true }
            }
        }
    }

    // KV column component (Geral tab)
    component DetailKVCol: ColumnLayout {
        property string head
        property var rows: []
        Layout.alignment: Qt.AlignTop
        spacing: 0
        Text {
            text: head
            color: Theme.t4
            font.pointSize: 10
            font.weight: Font.Bold
            font.letterSpacing: 0.8
            font.family: Theme.fontSans
            Layout.bottomMargin: 12
        }
        Repeater {
            model: rows
            delegate: RowLayout {
                Layout.minimumWidth: 150
                spacing: Theme.sp4
                Text { text: modelData[0]; color: Theme.t3; font.pointSize: 11.5; font.family: Theme.fontSans }
                Item { Layout.fillWidth: true }
                Text {
                    text: modelData[1]
                    color: modelData[0] === "Download" ? Theme.accentText : modelData[0] === "Upload" ? Theme.up : Theme.t1
                    font.pointSize: 12
                    font.family: Theme.fontMono
                }
            }
        }
    }
}
