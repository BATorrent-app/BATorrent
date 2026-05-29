// Source: BATorrent Home.html + batorrent-home.css (+ batorrent-home.js model)
import QtQuick
import QtQuick.Effects
import QtQuick.Layouts
import QtQuick.Shapes
import "theme"

Window {
    id: win
    visible: true
    width: 1360
    height: 884
    minimumWidth: 1000
    minimumHeight: 640
    color: Theme.bg
    title: "BATorrent"

    property int selected: 2
    property bool gridView: true

    // ----- ground-truth model (batorrent-home.js) -----
    ListModel {
        id: torrents
        ListElement { file: "Hollow.Knight-GOG.bin"; title: "Hollow Knight"; cat: "Jogos"; poster: "qrc:/images/hollow.webp"; state: "dl"; label: "Baixando"; size: "5.20 GB"; pct: 42; down: "2.4 MB/s"; up: "180 KB/s"; peers: "24" }
        ListElement { file: "007.First.Light-RUNE.bin"; title: "007 First Light"; cat: "Jogos"; poster: "qrc:/images/007.jpg"; state: "pa"; label: "Pausado"; size: "7.49 GB"; pct: 12; down: "—"; up: "—"; peers: "0" }
        ListElement { file: "Forza.Horizon.6-CODEX.bin"; title: "Forza Horizon 6"; cat: "Jogos"; poster: "qrc:/images/forza.png"; state: "dl"; label: "Baixando"; size: "92.4 GB"; pct: 67; down: "6.1 MB/s"; up: "412 KB/s"; peers: "38" }
    }

    // ----- state→color helpers (fl-* / bg-* / st-*) -----
    function fillFor(s) {
        if (s === "se" || s === "cp") return Theme.amber
        if (s === "pa") return Theme.pausedFill
        return Theme.accent
    }
    function textFor(s) {
        if (s === "se" || s === "cp") return Theme.up
        if (s === "pa") return Theme.t3
        return Theme.accentText
    }
    function dotFor(s) {
        if (s === "se" || s === "cp") return Theme.amber
        if (s === "pa") return Theme.t4
        return Theme.accent
    }

    // ----- reusable tinted icon -----
    component IconImg: Item {
        id: ico
        property string src
        property color tint: Theme.t3
        property int s: 18
        implicitWidth: s
        implicitHeight: s
        Image {
            id: imgSrc
            anchors.fill: parent
            source: ico.src
            sourceSize: Qt.size(ico.s * 2, ico.s * 2)
            fillMode: Image.PreserveAspectFit
            visible: false
            layer.enabled: true
        }
        MultiEffect {
            source: imgSrc
            anchors.fill: imgSrc
            colorization: 1.0
            colorizationColor: ico.tint
        }
    }

    // ----- reusable toolbar button (.tbtn) -----
    component TBtn: Rectangle {
        id: tb
        property string label
        property string icon
        property bool disabled: false
        Layout.preferredWidth: 52
        Layout.preferredHeight: 54
        color: !disabled && tbMa.containsMouse ? Theme.hover : "transparent"
        radius: 8
        opacity: disabled ? 0.35 : 1.0

        Column {
            anchors.centerIn: parent
            spacing: 3
            IconImg {
                anchors.horizontalCenter: parent.horizontalCenter
                src: tb.icon
                tint: !tb.disabled && tbMa.containsMouse ? Theme.t1 : Theme.t3
                s: 18
            }
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: tb.label
                color: !tb.disabled && tbMa.containsMouse ? Theme.t1 : Theme.t3
                font.pointSize: 10.5
                font.family: Theme.fontSans
                font.weight: Font.Medium
            }
        }
        MouseArea {
            id: tbMa
            anchors.fill: parent
            hoverEnabled: !tb.disabled
            cursorShape: tb.disabled ? Qt.ArrowCursor : Qt.PointingHandCursor
        }
    }

    // ----- vertical divider between toolbar groups (.tgrp + .tgrp) -----
    component TGrpDiv: Rectangle {
        Layout.preferredWidth: 1
        Layout.preferredHeight: 26
        Layout.leftMargin: 8
        Layout.rightMargin: 8
        Layout.alignment: Qt.AlignVCenter
        color: Theme.hair
    }

    // ----- pill (filter) -----
    component Pill: Rectangle {
        id: pi
        property string label
        property string count
        property bool on: false
        radius: 8
        height: 30
        implicitWidth: pillRow.implicitWidth + 26
        color: on ? Theme.accentTint : (piMa.containsMouse ? Theme.hover : "transparent")

        Row {
            id: pillRow
            anchors.centerIn: parent
            spacing: 7
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: pi.label
                color: pi.on ? Theme.accentText : (piMa.containsMouse ? Theme.t2 : Theme.t3)
                font.pointSize: 12
                font.family: Theme.fontSans
                font.weight: Font.Medium
            }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: pi.count
                color: pi.on ? Theme.accentText : Theme.t4
                font.pointSize: 11
                font.family: Theme.fontMono
            }
        }
        MouseArea {
            id: piMa
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // ================== TITLEBAR ==================
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 34
            color: Theme.elev

            Row {
                anchors.left: parent.left
                anchors.leftMargin: Theme.sp4
                anchors.verticalCenter: parent.verticalCenter
                spacing: Theme.sp2
                Rectangle { width: 12; height: 12; radius: 6; color: "#ff5f57"; anchors.verticalCenter: parent.verticalCenter }
                Rectangle { width: 12; height: 12; radius: 6; color: "#febc2e"; anchors.verticalCenter: parent.verticalCenter }
                Rectangle { width: 12; height: 12; radius: 6; color: "#28c840"; anchors.verticalCenter: parent.verticalCenter }
            }
            Text {
                anchors.centerIn: parent
                text: "BATorrent"
                color: Theme.t2
                font.pointSize: 12.5
                font.weight: Font.DemiBold
                font.family: Theme.fontSans
            }
            Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hairSoft }
        }

        // ================== TOOLBAR ==================
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 66
            color: Theme.elev
            Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hair }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Theme.sp4
                anchors.rightMargin: Theme.sp4
                spacing: Theme.sp2

                // .brand (logo 30, padding-left 4)
                Image {
                    Layout.preferredWidth: 30
                    Layout.preferredHeight: 30
                    Layout.leftMargin: 4
                    Layout.alignment: Qt.AlignVCenter
                    source: "qrc:/images/logo.svg"
                    sourceSize: Qt.size(60, 60)
                    fillMode: Image.PreserveAspectFit
                }
                // .brand-div (1×26 hair, margin 0 4)
                Rectangle {
                    Layout.preferredWidth: 1
                    Layout.preferredHeight: 26
                    Layout.leftMargin: 4
                    Layout.rightMargin: 4
                    Layout.alignment: Qt.AlignVCenter
                    color: Theme.hair
                }

                // G1: Abrir, Magnet
                TBtn { label: "Abrir";   icon: "qrc:/icons/open.svg" }
                TBtn { label: "Magnet";  icon: "qrc:/icons/magnet.svg" }
                TGrpDiv {}
                // G2: Pausar, Retomar, Parar
                TBtn { label: "Pausar";  icon: "qrc:/icons/pause.svg" }
                TBtn { label: "Retomar"; icon: "qrc:/icons/play.svg" }
                TBtn { label: "Parar";   icon: "qrc:/icons/stop.svg" }
                TGrpDiv {}
                // G3: Remover
                TBtn { label: "Remover"; icon: "qrc:/icons/trash.svg" }
                TGrpDiv {}
                // G4: Buscar, RSS
                TBtn { label: "Buscar";  icon: "qrc:/icons/search.svg" }
                TBtn { label: "RSS";     icon: "qrc:/icons/rss.svg" }
                TGrpDiv {}
                // G5: Config.
                TBtn { label: "Config."; icon: "qrc:/icons/settings.svg" }

                // .tb-spacer
                Item { Layout.fillWidth: true }

                // .spd-mod (border hair, radius 9, 2 cols)
                Rectangle {
                    Layout.preferredHeight: 44
                    Layout.alignment: Qt.AlignVCenter
                    implicitWidth: spdRow.width + 0
                    color: "transparent"
                    border.color: Theme.hair
                    border.width: 1
                    radius: 9

                    Row {
                        id: spdRow
                        anchors.centerIn: parent
                        spacing: 0

                        // .c Download
                        Item {
                            width: dlCol.implicitWidth + 28
                            height: 44
                            Column {
                                id: dlCol
                                anchors.centerIn: parent
                                spacing: 3
                                Text {
                                    text: "DOWNLOAD"
                                    color: Theme.t4
                                    font.pointSize: 9
                                    font.weight: Font.Bold
                                    font.letterSpacing: 1
                                    font.family: Theme.fontSans
                                }
                                Row {
                                    spacing: 5
                                    IconImg {
                                        anchors.verticalCenter: parent.verticalCenter
                                        src: "qrc:/icons/download.svg"
                                        tint: Theme.accentText
                                        s: 12
                                    }
                                    Text {
                                        anchors.verticalCenter: parent.verticalCenter
                                        text: "8.5 MB/s"
                                        color: Theme.accentText
                                        font.pointSize: 13
                                        font.weight: Font.Bold
                                        font.family: Theme.fontMono
                                    }
                                }
                            }
                        }
                        // divider .c + .c
                        Rectangle { width: 1; height: 44; color: Theme.hair }
                        // .c Upload
                        Item {
                            width: upCol.implicitWidth + 28
                            height: 44
                            Column {
                                id: upCol
                                anchors.centerIn: parent
                                spacing: 3
                                Text {
                                    text: "UPLOAD"
                                    color: Theme.t4
                                    font.pointSize: 9
                                    font.weight: Font.Bold
                                    font.letterSpacing: 1
                                    font.family: Theme.fontSans
                                }
                                Row {
                                    spacing: 5
                                    IconImg {
                                        anchors.verticalCenter: parent.verticalCenter
                                        src: "qrc:/icons/upload.svg"
                                        tint: Theme.up
                                        s: 12
                                    }
                                    Text {
                                        anchors.verticalCenter: parent.verticalCenter
                                        text: "1.7 MB/s"
                                        color: Theme.up
                                        font.pointSize: 13
                                        font.family: Theme.fontMono
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        // ================== SUBBAR ==================
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 54
            color: "transparent"
            Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hair }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Theme.sp4
                anchors.rightMargin: Theme.sp4
                spacing: Theme.sp3

                // .search (240×34, padding 0 11, gap 8, bg panel)
                Rectangle {
                    Layout.preferredWidth: 240
                    Layout.preferredHeight: 34
                    Layout.alignment: Qt.AlignVCenter
                    color: Theme.panel
                    border.color: Theme.hair
                    border.width: 1
                    radius: 8

                    Row {
                        anchors.left: parent.left
                        anchors.leftMargin: 11
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 8
                        IconImg {
                            anchors.verticalCenter: parent.verticalCenter
                            src: "qrc:/icons/search.svg"
                            tint: Theme.t4
                            s: 14
                        }
                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: "Buscar torrents"
                            color: Theme.t4
                            font.pointSize: 12.5
                            font.family: Theme.fontSans
                        }
                    }
                }

                // .seg (toggle Grade/Lista) — padding 2, bg panel
                Rectangle {
                    Layout.alignment: Qt.AlignVCenter
                    Layout.preferredHeight: 32
                    implicitWidth: segRow.implicitWidth + 4
                    color: Theme.panel
                    border.color: Theme.hair
                    border.width: 1
                    radius: 8

                    Row {
                        id: segRow
                        anchors.centerIn: parent
                        spacing: 2

                        Rectangle {
                            implicitWidth: segGr.implicitWidth + 22
                            height: 28
                            radius: 6
                            color: win.gridView ? Qt.rgba(1,1,1,0.08) : "transparent"
                            Row {
                                id: segGr
                                anchors.centerIn: parent
                                spacing: 6
                                IconImg {
                                    anchors.verticalCenter: parent.verticalCenter
                                    src: "qrc:/icons/grid.svg"
                                    tint: win.gridView ? Theme.t1 : Theme.t3
                                    s: 14
                                }
                                Text {
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: "Grade"
                                    color: win.gridView ? Theme.t1 : Theme.t3
                                    font.pointSize: 11.5
                                    font.weight: Font.Medium
                                    font.family: Theme.fontSans
                                }
                            }
                            MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: win.gridView = true }
                        }
                        Rectangle {
                            implicitWidth: segLi.implicitWidth + 22
                            height: 28
                            radius: 6
                            color: !win.gridView ? Qt.rgba(1,1,1,0.08) : "transparent"
                            Row {
                                id: segLi
                                anchors.centerIn: parent
                                spacing: 6
                                IconImg {
                                    anchors.verticalCenter: parent.verticalCenter
                                    src: "qrc:/icons/list.svg"
                                    tint: !win.gridView ? Theme.t1 : Theme.t3
                                    s: 14
                                }
                                Text {
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: "Lista"
                                    color: !win.gridView ? Theme.t1 : Theme.t3
                                    font.pointSize: 11.5
                                    font.weight: Font.Medium
                                    font.family: Theme.fontSans
                                }
                            }
                            MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: win.gridView = false }
                        }
                    }
                }

                // .pills (gap 4) — 6 pills: Todos(3,on), Ativos(2), Baixando(2), Semeando(0), Pausado(1), Concluído(0)
                Row {
                    Layout.alignment: Qt.AlignVCenter
                    spacing: Theme.sp1
                    Pill { label: "Todos";     count: "3"; on: true }
                    Pill { label: "Ativos";    count: "2" }
                    Pill { label: "Baixando";  count: "2" }
                    Pill { label: "Semeando";  count: "0" }
                    Pill { label: "Pausado";   count: "1" }
                    Pill { label: "Concluído"; count: "0" }
                }

                Item { Layout.fillWidth: true }

                // .cat (Todas as categorias + chevron)
                Rectangle {
                    Layout.alignment: Qt.AlignVCenter
                    Layout.preferredHeight: 34
                    implicitWidth: catRow.implicitWidth + 24
                    color: "transparent"
                    border.color: Theme.hair
                    border.width: 1
                    radius: 8

                    Row {
                        id: catRow
                        anchors.centerIn: parent
                        spacing: 8
                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: "Todas as categorias"
                            color: Theme.t2
                            font.pointSize: 12
                            font.family: Theme.fontSans
                        }
                        IconImg {
                            anchors.verticalCenter: parent.verticalCenter
                            src: "qrc:/icons/chevron.svg"
                            tint: Theme.t4
                            s: 13
                        }
                    }
                    MouseArea { anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor }
                }

                // .donate (♥ Doar)
                Rectangle {
                    Layout.alignment: Qt.AlignVCenter
                    Layout.preferredHeight: 34
                    implicitWidth: donRow.implicitWidth + 26
                    color: donMa.containsMouse ? Qt.rgba(229/255, 51/255, 43/255, 0.10) : "transparent"
                    border.color: Qt.rgba(229/255, 51/255, 43/255, 0.4)
                    border.width: 1
                    radius: 8

                    Row {
                        id: donRow
                        anchors.centerIn: parent
                        spacing: 7
                        IconImg {
                            anchors.verticalCenter: parent.verticalCenter
                            src: "qrc:/icons/heart.svg"
                            tint: Theme.accentText
                            s: 14
                        }
                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: "Doar"
                            color: Theme.accentText
                            font.pointSize: 12
                            font.weight: Font.DemiBold
                            font.family: Theme.fontSans
                        }
                    }
                    MouseArea {
                        id: donMa
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: Qt.openUrlExternally("https://github.com/sponsors/Mateuscruz19")
                    }
                }
            }
        }

        // ================== CONTENT (grid OR list) ==================
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            // anime art (eyes top-right; spider bottom-right)
            Image {
                id: animeArt
                visible: Theme.hasAnime
                source: Theme.hasAnime ? Theme.animeSource : ""
                fillMode: Image.PreserveAspectFit
                width: Theme.animeBottom ? 560 : 460
                opacity: win.gridView ? 0.9 : 0.6
                anchors.right: parent.right
                anchors.rightMargin: -10
                anchors.top: Theme.animeBottom ? undefined : parent.top
                anchors.bottom: Theme.animeBottom ? parent.bottom : undefined
                anchors.bottomMargin: Theme.animeBottom ? -80 : 0
                z: 0
            }

            // ----- GRID -----
            GridView {
                id: grid
                visible: win.gridView
                anchors.fill: parent
                topMargin: Theme.sp5
                bottomMargin: Theme.sp5
                leftMargin: Theme.sp4
                rightMargin: Theme.sp4
                cellWidth: 178 + Theme.sp4
                cellHeight: 286
                clip: true
                model: torrents
                interactive: true
                z: 1

                delegate: Item {
                    width: 178
                    height: 286

                    // .poster wrapper (aspect 3:4 ≈ 178:237)
                    Item {
                        id: posterWrap
                        width: 178
                        height: 237

                        // Content rendered to layer, masked by rounded rect
                        Rectangle {
                            id: posterBg
                            anchors.fill: parent
                            color: "#161618"
                            visible: false
                            layer.enabled: true

                            Image {
                                anchors.fill: parent
                                source: model.poster
                                fillMode: Image.PreserveAspectCrop
                                asynchronous: true
                            }
                            // .pscrim — gradient bottom 42% transparent→rgba(0,0,0,0.5)
                            Rectangle {
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.bottom: parent.bottom
                                height: parent.height * 0.42
                                gradient: Gradient {
                                    GradientStop { position: 0.0; color: "transparent" }
                                    GradientStop { position: 1.0; color: Qt.rgba(0, 0, 0, 0.5) }
                                }
                            }
                            // .pbar (z2 in CSS — top of poster content layer)
                            Rectangle {
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.bottom: parent.bottom
                                height: 3
                                color: Qt.rgba(0, 0, 0, 0.5)
                                Rectangle {
                                    height: parent.height
                                    width: parent.width * model.pct / 100
                                    color: win.fillFor(model.state)
                                }
                            }
                        }
                        Rectangle {
                            id: posterMask
                            anchors.fill: parent
                            radius: 10
                            color: "white"
                            visible: false
                            layer.enabled: true
                        }
                        MultiEffect {
                            source: posterBg
                            anchors.fill: parent
                            maskEnabled: true
                            maskSource: posterMask
                        }
                        // border overlay (radius 10, hair / accent when sel)
                        Rectangle {
                            anchors.fill: parent
                            radius: 10
                            color: "transparent"
                            border.color: win.selected === index ? Theme.accent : (tileMa.containsMouse ? Qt.rgba(1,1,1,0.2) : Theme.hair)
                            border.width: win.selected === index ? 2 : 1
                        }
                        MouseArea {
                            id: tileMa
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: win.selected = index
                        }
                    }

                    // .tmeta (padding-top 12: .st dot+label · .sz)
                    RowLayout {
                        anchors.top: posterWrap.bottom
                        anchors.topMargin: 12
                        anchors.left: posterWrap.left
                        anchors.right: posterWrap.right
                        spacing: 0

                        Row {
                            spacing: 6
                            Rectangle {
                                width: 6
                                height: 6
                                radius: 3
                                anchors.verticalCenter: parent.verticalCenter
                                color: win.dotFor(model.state)
                            }
                            Text {
                                anchors.verticalCenter: parent.verticalCenter
                                text: model.label
                                color: win.textFor(model.state)
                                font.pointSize: 11.5
                                font.family: Theme.fontSans
                            }
                        }
                        Item { Layout.fillWidth: true }
                        Text {
                            text: model.size
                            color: Theme.t4
                            font.pointSize: 11.5
                            font.family: Theme.fontMono
                        }
                    }
                }
            }

            // ----- LIST -----
            ListView {
                id: list
                visible: !win.gridView
                anchors.fill: parent
                clip: true
                model: torrents
                interactive: true
                z: 1

                header: Rectangle {
                    width: ListView.view.width
                    height: 36
                    color: "transparent"
                    Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hair }

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: Theme.sp4
                        anchors.rightMargin: Theme.sp4
                        spacing: Theme.sp4

                        Text { text: "NOME"; Layout.fillWidth: true; color: Theme.t4; font.pointSize: 10.5; font.weight: Font.DemiBold; font.letterSpacing: 0.6; font.family: Theme.fontSans }
                        Text { text: "TAMANHO"; Layout.preferredWidth: 78; horizontalAlignment: Text.AlignRight; color: Theme.t4; font.pointSize: 10.5; font.weight: Font.DemiBold; font.letterSpacing: 0.6; font.family: Theme.fontSans }
                        Text { text: "PROGRESSO"; Layout.preferredWidth: 130; color: Theme.t4; font.pointSize: 10.5; font.weight: Font.DemiBold; font.letterSpacing: 0.6; font.family: Theme.fontSans }
                        Text { text: "DOWN"; Layout.preferredWidth: 78; horizontalAlignment: Text.AlignRight; color: Theme.t4; font.pointSize: 10.5; font.weight: Font.DemiBold; font.letterSpacing: 0.6; font.family: Theme.fontSans }
                        Text { text: "UP"; Layout.preferredWidth: 78; horizontalAlignment: Text.AlignRight; color: Theme.t4; font.pointSize: 10.5; font.weight: Font.DemiBold; font.letterSpacing: 0.6; font.family: Theme.fontSans }
                        Text { text: "ESTADO"; Layout.preferredWidth: 110; color: Theme.t4; font.pointSize: 10.5; font.weight: Font.DemiBold; font.letterSpacing: 0.6; font.family: Theme.fontSans }
                        Text { text: "CATEGORIA"; Layout.preferredWidth: 90; color: Theme.t4; font.pointSize: 10.5; font.weight: Font.DemiBold; font.letterSpacing: 0.6; font.family: Theme.fontSans }
                        Text { text: "PEERS"; Layout.preferredWidth: 56; horizontalAlignment: Text.AlignRight; color: Theme.t4; font.pointSize: 10.5; font.weight: Font.DemiBold; font.letterSpacing: 0.6; font.family: Theme.fontSans }
                    }
                }

                delegate: Rectangle {
                    width: ListView.view.width
                    height: 48
                    color: win.selected === index ? Theme.sel : (rowMa.containsMouse ? Theme.hover : "transparent")

                    // .sel inset 2px barra esquerda
                    Rectangle {
                        visible: win.selected === index
                        anchors.left: parent.left
                        anchors.top: parent.top
                        anchors.bottom: parent.bottom
                        width: 2
                        color: Theme.accent
                    }
                    // border-bottom hairSoft
                    Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hairSoft }

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: Theme.sp4
                        anchors.rightMargin: Theme.sp4
                        spacing: Theme.sp4

                        // .name: dot 7×7 + nome
                        Row {
                            Layout.fillWidth: true
                            spacing: Theme.sp3
                            Rectangle {
                                anchors.verticalCenter: parent.verticalCenter
                                width: 7; height: 7; radius: 3.5
                                color: win.dotFor(model.state)
                            }
                            Text {
                                anchors.verticalCenter: parent.verticalCenter
                                text: model.file
                                color: Theme.t1
                                font.pointSize: 13
                                font.family: Theme.fontSans
                                elide: Text.ElideRight
                                width: parent.parent.width - 30
                            }
                        }
                        Text {
                            text: model.size
                            Layout.preferredWidth: 78
                            horizontalAlignment: Text.AlignRight
                            color: Theme.t2
                            font.pointSize: 12
                            font.family: Theme.fontMono
                        }
                        // .prog (130) → .pbar2 height 18 black track + fill state + centered % white
                        Item {
                            Layout.preferredWidth: 130
                            Layout.preferredHeight: 18
                            Rectangle {
                                anchors.fill: parent
                                radius: 4
                                color: "#000"
                                border.color: Theme.hair
                                border.width: 1
                                clip: true
                                Rectangle {
                                    anchors.top: parent.top
                                    anchors.bottom: parent.bottom
                                    anchors.left: parent.left
                                    width: parent.width * model.pct / 100
                                    color: win.fillFor(model.state)
                                }
                                Text {
                                    anchors.centerIn: parent
                                    text: model.pct + "%"
                                    color: "#fff"
                                    font.pointSize: 10.5
                                    font.weight: Font.DemiBold
                                    font.family: Theme.fontMono
                                }
                            }
                        }
                        Text {
                            text: model.down
                            Layout.preferredWidth: 78
                            horizontalAlignment: Text.AlignRight
                            color: model.down === "—" ? Theme.t4 : Theme.accentText
                            font.pointSize: 12
                            font.family: Theme.fontMono
                        }
                        Text {
                            text: model.up
                            Layout.preferredWidth: 78
                            horizontalAlignment: Text.AlignRight
                            color: model.up === "—" ? Theme.t4 : Theme.up
                            font.pointSize: 12
                            font.family: Theme.fontMono
                        }
                        Text {
                            text: model.label
                            Layout.preferredWidth: 110
                            color: win.textFor(model.state)
                            font.pointSize: 12
                            font.weight: Theme.hasAnime ? Font.DemiBold : Font.Medium
                            font.family: Theme.fontSans
                        }
                        Text {
                            text: model.cat
                            Layout.preferredWidth: 90
                            color: Theme.t3
                            font.pointSize: 12
                            font.family: Theme.fontSans
                        }
                        Text {
                            text: model.peers
                            Layout.preferredWidth: 56
                            horizontalAlignment: Text.AlignRight
                            color: model.peers === "0" ? Theme.t4 : Theme.t2
                            font.pointSize: 12
                            font.family: Theme.fontMono
                        }
                    }
                    MouseArea {
                        id: rowMa
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: win.selected = index
                    }
                }
            }
        }

        // ================== GRAPH ==================
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 64
            color: Theme.bg
            Rectangle { anchors.top: parent.top; width: parent.width; height: 1; color: Theme.hair }

            Text {
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.topMargin: 6
                anchors.leftMargin: Theme.sp4
                text: "1 KB/s"
                color: Theme.t4
                font.pointSize: 10
                font.family: Theme.fontSans
            }
            Row {
                anchors.top: parent.top
                anchors.right: parent.right
                anchors.topMargin: 6
                anchors.rightMargin: Theme.sp4
                spacing: Theme.sp4
                Row {
                    spacing: 6
                    Rectangle { width: 6; height: 6; radius: 3; color: Theme.accent; anchors.verticalCenter: parent.verticalCenter }
                    Text { text: "8.5 MB/s"; color: Theme.t3; font.pointSize: 11; font.family: Theme.fontMono; anchors.verticalCenter: parent.verticalCenter }
                }
                Row {
                    spacing: 6
                    Rectangle { width: 6; height: 6; radius: 3; color: Theme.amber; anchors.verticalCenter: parent.verticalCenter }
                    Text { text: "1.7 MB/s"; color: Theme.t3; font.pointSize: 11; font.family: Theme.fontMono; anchors.verticalCenter: parent.verticalCenter }
                }
            }

            // 2 smooth curves: download accent + upload amber
            Shape {
                anchors.fill: parent
                anchors.topMargin: 24
                anchors.bottomMargin: 4
                antialiasing: true
                ShapePath {
                    strokeColor: Theme.accent
                    strokeWidth: 1.5
                    fillColor: "transparent"
                    startX: 0
                    startY: 24
                    PathCubic {
                        x: parent.width * 0.5
                        y: 6
                        control1X: parent.width * 0.18
                        control1Y: 24
                        control2X: parent.width * 0.32
                        control2Y: 4
                    }
                    PathCubic {
                        x: parent.width
                        y: 18
                        control1X: parent.width * 0.68
                        control1Y: 8
                        control2X: parent.width * 0.82
                        control2Y: 22
                    }
                }
                ShapePath {
                    strokeColor: Theme.amber
                    strokeWidth: 1.5
                    fillColor: "transparent"
                    startX: 0
                    startY: 30
                    PathCubic {
                        x: parent.width * 0.5
                        y: 22
                        control1X: parent.width * 0.18
                        control1Y: 30
                        control2X: parent.width * 0.32
                        control2Y: 18
                    }
                    PathCubic {
                        x: parent.width
                        y: 26
                        control1X: parent.width * 0.68
                        control1Y: 24
                        control2X: parent.width * 0.82
                        control2Y: 28
                    }
                }
            }
        }

        // ================== DETAIL ==================
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 270
            color: Theme.panel
            Rectangle { anchors.top: parent.top; width: parent.width; height: 1; color: Theme.hair }

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                // .dtabs (42px, 5 tabs)
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 42
                    color: "transparent"
                    Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hair }

                    Row {
                        anchors.fill: parent
                        anchors.leftMargin: Theme.sp5
                        spacing: Theme.sp5

                        Repeater {
                            model: [
                                { label: "Geral",    on: true  },
                                { label: "Peers",    on: false },
                                { label: "Arquivos", on: false },
                                { label: "Trackers", on: false },
                                { label: "Pedaços",  on: false }
                            ]
                            delegate: Item {
                                width: tabText.implicitWidth
                                height: 42

                                Text {
                                    id: tabText
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: modelData.label
                                    color: modelData.on ? Theme.t1 : Theme.t3
                                    font.pointSize: 12.5
                                    font.weight: modelData.on ? Font.DemiBold : Font.Medium
                                    font.family: Theme.fontSans
                                }
                                Rectangle {
                                    visible: modelData.on
                                    anchors.left: parent.left
                                    anchors.right: parent.right
                                    anchors.bottom: parent.bottom
                                    height: 2
                                    color: Theme.accent
                                }
                            }
                        }
                    }
                }

                // .dbody — cover 104×146 + dmain 460 + dcols 3×
                RowLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.margins: Theme.sp5
                    spacing: Theme.sp6

                    // .dcover (radius 8 — mask via MultiEffect)
                    Item {
                        Layout.preferredWidth: 104
                        Layout.preferredHeight: 146
                        Layout.alignment: Qt.AlignTop

                        Rectangle {
                            id: dcoverContent
                            anchors.fill: parent
                            color: "#161618"
                            visible: false
                            layer.enabled: true
                            Image {
                                anchors.fill: parent
                                source: torrents.get(win.selected).poster
                                fillMode: Image.PreserveAspectCrop
                                asynchronous: true
                            }
                        }
                        Rectangle {
                            id: dcoverMask
                            anchors.fill: parent
                            radius: 8
                            color: "white"
                            visible: false
                            layer.enabled: true
                        }
                        MultiEffect {
                            source: dcoverContent
                            anchors.fill: parent
                            maskEnabled: true
                            maskSource: dcoverMask
                        }
                        Rectangle {
                            anchors.fill: parent
                            radius: 8
                            color: "transparent"
                            border.color: Theme.hair
                            border.width: 1
                        }
                    }

                    // .dmain max-width 460
                    ColumnLayout {
                        Layout.preferredWidth: 460
                        Layout.maximumWidth: 460
                        Layout.alignment: Qt.AlignTop
                        spacing: 6

                        Text {
                            text: torrents.get(win.selected).title
                            color: Theme.t1
                            font.pointSize: 17
                            font.weight: Font.DemiBold
                            font.letterSpacing: -0.2
                            font.family: Theme.fontSans
                        }
                        Text {
                            text: "2026 · Racing, Simulator, Sport · 8.6/10"
                            color: Theme.t3
                            font.pointSize: 11.5
                            font.family: Theme.fontSans
                        }
                        Text {
                            Layout.topMargin: 6
                            Layout.fillWidth: true
                            wrapMode: Text.WordWrap
                            color: Theme.t2
                            font.pointSize: 12
                            font.family: Theme.fontSans
                            lineHeight: 1.55
                            text: "Explore paisagens deslumbrantes ao volante de centenas de carros e torne-se uma lenda no Horizon Festival."
                        }
                    }

                    Item { Layout.fillWidth: true }

                    // .dcols (3 colunas KV)
                    RowLayout {
                        Layout.alignment: Qt.AlignTop
                        spacing: Theme.sp6

                        // INFO
                        ColumnLayout {
                            Layout.preferredWidth: 168
                            Layout.alignment: Qt.AlignTop
                            spacing: 0

                            Text {
                                text: "INFO"
                                color: Theme.t4
                                font.pointSize: 10
                                font.weight: Font.Bold
                                font.letterSpacing: 0.8
                                font.family: Theme.fontSans
                                Layout.bottomMargin: Theme.sp3
                            }
                            Repeater {
                                model: [
                                    { k: "Nome",    v: "Forza.Horizon.6-CODEX.bin" },
                                    { k: "Tamanho", v: "92.4 GB" },
                                    { k: "Hash",    v: "657c6a58…42e7" },
                                    { k: "Estado",  v: "Baixando" }
                                ]
                                delegate: RowLayout {
                                    Layout.fillWidth: true
                                    Text { text: modelData.k; color: Theme.t3; font.pointSize: 11.5; font.family: Theme.fontSans }
                                    Item { Layout.fillWidth: true }
                                    Text { text: modelData.v; color: Theme.t1; font.pointSize: 12; font.family: Theme.fontSans; elide: Text.ElideMiddle; Layout.maximumWidth: 110; horizontalAlignment: Text.AlignRight }
                                }
                            }
                        }

                        // TRANSFERÊNCIA
                        ColumnLayout {
                            Layout.preferredWidth: 168
                            Layout.alignment: Qt.AlignTop
                            spacing: 0

                            Text {
                                text: "TRANSFERÊNCIA"
                                color: Theme.t4
                                font.pointSize: 10
                                font.weight: Font.Bold
                                font.letterSpacing: 0.8
                                font.family: Theme.fontSans
                                Layout.bottomMargin: Theme.sp3
                            }
                            Repeater {
                                model: [
                                    { k: "Baixado",  v: "62.1 GB (67%)" },
                                    { k: "Download", v: "6.1 MB/s" },
                                    { k: "Upload",   v: "412 KB/s" },
                                    { k: "ETA",      v: "14 min" }
                                ]
                                delegate: RowLayout {
                                    Layout.fillWidth: true
                                    Text { text: modelData.k; color: Theme.t3; font.pointSize: 11.5; font.family: Theme.fontSans }
                                    Item { Layout.fillWidth: true }
                                    Text { text: modelData.v; color: Theme.t1; font.pointSize: 12; font.family: Theme.fontSans }
                                }
                            }
                        }

                        // PEERS
                        ColumnLayout {
                            Layout.preferredWidth: 168
                            Layout.alignment: Qt.AlignTop
                            spacing: 0

                            Text {
                                text: "PEERS"
                                color: Theme.t4
                                font.pointSize: 10
                                font.weight: Font.Bold
                                font.letterSpacing: 0.8
                                font.family: Theme.fontSans
                                Layout.bottomMargin: Theme.sp3
                            }
                            Repeater {
                                model: [
                                    { k: "Seeds", v: "12" },
                                    { k: "Peers", v: "38" },
                                    { k: "Ratio", v: "0.18" }
                                ]
                                delegate: RowLayout {
                                    Layout.fillWidth: true
                                    Text { text: modelData.k; color: Theme.t3; font.pointSize: 11.5; font.family: Theme.fontSans }
                                    Item { Layout.fillWidth: true }
                                    Text { text: modelData.v; color: Theme.t1; font.pointSize: 12; font.family: Theme.fontSans }
                                }
                            }
                        }
                    }
                }
            }
        }

        // ================== STATUSBAR ==================
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 30
            color: Theme.elev
            Rectangle { anchors.top: parent.top; width: parent.width; height: 1; color: Theme.hair }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Theme.sp4
                anchors.rightMargin: Theme.sp4
                spacing: Theme.sp2

                Text {
                    text: "3 torrents · 2 ativos"
                    color: Theme.t4
                    font.pointSize: 11.5
                    font.family: Theme.fontSans
                }
                Item { Layout.fillWidth: true }
                Rectangle { width: 6; height: 6; radius: 3; color: Theme.accent; anchors.verticalCenter: parent.verticalCenter }
                Text { text: "8.5 MB/s"; color: Theme.t3; font.pointSize: 11.5; font.family: Theme.fontMono }
                Rectangle { width: 6; height: 6; radius: 3; color: Theme.amber; anchors.verticalCenter: parent.verticalCenter }
                Text { text: "1.7 MB/s"; color: Theme.t3; font.pointSize: 11.5; font.family: Theme.fontMono }
                Text {
                    text: "·  Total 31,04 GB ↓ · 2,09 GB ↑ · Ratio 0.07"
                    color: Theme.t4
                    font.pointSize: 11.5
                    font.family: Theme.fontSans
                }
            }
        }
    }
}
