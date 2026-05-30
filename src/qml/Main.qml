// Source: BATorrent Home.html + batorrent-home.css (+ batorrent-home.js model)
import QtQuick
import QtQuick.Effects
import QtQuick.Layouts
import QtQuick.Shapes
import QtQuick.Dialogs
import Qt.labs.platform as Platform
import "theme"
import "widgets"

Window {
    id: win
    visible: true
    width: 1360
    height: 884
    minimumWidth: 1000
    minimumHeight: 640
    color: Theme.bg
    title: "BATorrent"

    property int selected: -1
    property bool gridView: true
    property string activeFilter: "all"

    // live model from C++ (QmlTorrentFilterProxy → QmlPosterModel). Roles:
    // torrentName, metaTitle, stateKey, progress(0..1), posterPath, stateString,
    // downSpeed, upSpeed, category, numPeers, downRate, upRate, size, infoHash.
    readonly property var model: typeof torrentModel !== "undefined" ? torrentModel : null
    readonly property bool hasSel: typeof session !== "undefined" && session.hasSelection

    // ----- state→color helpers (keyed by real stateKey) -----
    function fillFor(k) {
        if (k === "seeding" || k === "finished" || k === "completed") return Theme.amber
        if (k === "paused" || k === "queued") return Theme.pausedFill
        return Theme.accent
    }
    function textFor(k) {
        if (k === "seeding" || k === "finished" || k === "completed") return Theme.up
        if (k === "paused" || k === "queued") return Theme.t3
        return Theme.accentText
    }
    function dotFor(k) {
        if (k === "seeding" || k === "finished" || k === "completed") return Theme.amber
        if (k === "paused" || k === "queued") return Theme.t4
        return Theme.accent
    }
    function selectRow(proxyRow) {
        win.selected = proxyRow
        if (typeof session !== "undefined" && typeof torrentFilter !== "undefined")
            session.setSelectedIndex(torrentFilter.mapToSource(proxyRow))
    }
    function setFilter(f) {
        win.activeFilter = f
        if (typeof torrentFilter !== "undefined") torrentFilter.setFilterState(f)
    }

    // ================== NATIVE MENU BAR (ported from mainwindow.cpp) ==================
    Platform.MenuBar {
        Platform.Menu {
            title: qsTr("Arquivo")
            Platform.MenuItem { text: qsTr("Abrir torrent…"); shortcut: StandardKey.Open; onTriggered: openFileDlg.open() }
            Platform.MenuItem { text: qsTr("Adicionar magnet…"); shortcut: "Ctrl+M"; onTriggered: magnetDlg.open() }
            Platform.MenuItem { text: qsTr("Criar torrent…"); onTriggered: createDlg.open() }
            Platform.MenuSeparator {}
            Platform.MenuItem { text: qsTr("Sair"); shortcut: StandardKey.Quit; onTriggered: Qt.quit() }
        }
        Platform.Menu {
            title: qsTr("Torrent")
            Platform.MenuItem { text: qsTr("Pausar"); enabled: win.hasSel; onTriggered: session.pauseSelected() }
            Platform.MenuItem { text: qsTr("Retomar"); enabled: win.hasSel; onTriggered: session.resumeSelected() }
            Platform.MenuSeparator {}
            Platform.MenuItem { text: qsTr("Pausar todos"); onTriggered: if (typeof session !== "undefined") session.pauseAll() }
            Platform.MenuItem { text: qsTr("Retomar todos"); onTriggered: if (typeof session !== "undefined") session.resumeAll() }
            Platform.MenuSeparator {}
            Platform.MenuItem { text: qsTr("Remover…"); shortcut: StandardKey.Delete; enabled: win.hasSel; onTriggered: removeDlg.open() }
            Platform.MenuItem { text: qsTr("Remover e excluir arquivos"); enabled: win.hasSel; onTriggered: if (typeof session !== "undefined") session.removeSelectedWithFiles() }
        }
        Platform.Menu {
            title: qsTr("Configurações")
            Platform.MenuItem { text: qsTr("Preferências…"); shortcut: StandardKey.Preferences; onTriggered: settingsWin.show() }
            Platform.MenuItem { text: qsTr("Addons…"); onTriggered: addAddonDlg.open() }
            Platform.MenuItem { text: qsTr("RSS…"); onTriggered: rssWin.show() }
            Platform.MenuSeparator {}
            Platform.MenuItem { text: qsTr("Buscar torrents…"); onTriggered: searchWin.show() }
            Platform.MenuSeparator {}
            Platform.MenuItem { text: qsTr("Teste de velocidade"); onTriggered: Qt.openUrlExternally("https://fast.com") }
        }
        Platform.Menu {
            title: qsTr("Ajuda")
            Platform.MenuItem { text: qsTr("Boas-vindas"); onTriggered: welcomeDlg.open() }
            Platform.MenuItem { text: qsTr("Notas de versão"); onTriggered: releaseNotesDlg.open() }
            Platform.MenuSeparator {}
            Platform.MenuItem { text: qsTr("Doar"); onTriggered: Qt.openUrlExternally("https://github.com/sponsors/Mateuscruz19") }
            Platform.MenuItem { text: qsTr("Sobre o BATorrent"); onTriggered: aboutDlg.open() }
        }
    }

    // (IconImg vem de widgets/)

    // ----- reusable toolbar button (.tbtn) -----
    component TBtn: Rectangle {
        id: tb
        property string label
        property string icon
        property bool disabled: false
        signal clicked()
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
            onClicked: if (!tb.disabled) tb.clicked()
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
        property string state: "all"
        property bool on: win.activeFilter === state
        signal clicked()
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
            onClicked: pi.clicked()
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

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
                TBtn { label: "Abrir";   icon: "qrc:/icons/open.svg";  onClicked: openFileDlg.open() }
                TBtn { label: "Magnet";  icon: "qrc:/icons/magnet.svg"; onClicked: magnetDlg.open() }
                TGrpDiv {}
                // G2: Pausar, Retomar, Parar
                TBtn { label: "Pausar";  icon: "qrc:/icons/pause.svg"; disabled: !win.hasSel; onClicked: session.pauseSelected() }
                TBtn { label: "Retomar"; icon: "qrc:/icons/play.svg";  disabled: !win.hasSel; onClicked: session.resumeSelected() }
                TBtn { label: "Parar";   icon: "qrc:/icons/stop.svg";  disabled: !win.hasSel; onClicked: session.pauseSelected() }
                TGrpDiv {}
                // G3: Remover
                TBtn { label: "Remover"; icon: "qrc:/icons/trash.svg"; disabled: !win.hasSel; onClicked: removeDlg.open() }
                TGrpDiv {}
                // G4: Buscar, RSS
                TBtn { label: "Buscar";  icon: "qrc:/icons/search.svg"; onClicked: searchWin.show() }
                TBtn { label: "RSS";     icon: "qrc:/icons/rss.svg";    onClicked: rssWin.show() }
                TGrpDiv {}
                // G5: Config.
                TBtn { label: "Config."; icon: "qrc:/icons/settings.svg"; onClicked: settingsWin.show() }

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
                                        text: typeof session !== "undefined" ? session.totalDownSpeed : "0 KB/s"
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
                                        text: typeof session !== "undefined" ? session.totalUpSpeed : "0 KB/s"
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
                    border.color: searchInput.activeFocus ? Theme.accent : Theme.hair
                    border.width: 1
                    radius: 8

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 11
                        anchors.rightMargin: 11
                        spacing: 8
                        IconImg {
                            Layout.alignment: Qt.AlignVCenter
                            src: "qrc:/icons/search.svg"
                            tint: Theme.t4
                            s: 14
                        }
                        TextInput {
                            id: searchInput
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            verticalAlignment: TextInput.AlignVCenter
                            color: Theme.t1
                            font.pointSize: 12.5
                            font.family: Theme.fontSans
                            clip: true
                            onTextChanged: if (typeof torrentFilter !== "undefined") torrentFilter.setSearchText(text)
                            Text {
                                anchors.verticalCenter: parent.verticalCenter
                                text: "Buscar torrents"
                                color: Theme.t4
                                font: searchInput.font
                                visible: searchInput.text.length === 0 && !searchInput.activeFocus
                            }
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

                // .pills (gap 4) — 6 pills, counts from session, click sets filter
                Row {
                    Layout.alignment: Qt.AlignVCenter
                    spacing: Theme.sp1
                    Pill { label: "Todos";     state: "all";         count: typeof session !== "undefined" ? session.torrentCount : 0;     onClicked: win.setFilter("all") }
                    Pill { label: "Ativos";    state: "active";      count: typeof session !== "undefined" ? session.activeCount : 0;      onClicked: win.setFilter("active") }
                    Pill { label: "Baixando";  state: "downloading"; count: typeof session !== "undefined" ? session.downloadingCount : 0; onClicked: win.setFilter("downloading") }
                    Pill { label: "Semeando";  state: "seeding";     count: typeof session !== "undefined" ? session.seedingCount : 0;     onClicked: win.setFilter("seeding") }
                    Pill { label: "Pausado";   state: "paused";      count: typeof session !== "undefined" ? session.pausedCount : 0;      onClicked: win.setFilter("paused") }
                    Pill { label: "Concluído"; state: "completed";   count: typeof session !== "undefined" ? session.completedCount : 0;   onClicked: win.setFilter("completed") }
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

            readonly property bool empty: typeof session !== "undefined" && session.torrentCount === 0

            // empty state (no torrents)
            EmptyState {
                anchors.centerIn: parent
                visible: parent.empty
                onOpenClicked: addTorrentDlg.open()
                onMagnetClicked: magnetDlg.open()
            }

            // anime art (eyes top-right; spider bottom-right)
            Image {
                id: animeArt
                visible: Theme.hasAnime && !parent.empty
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
                visible: win.gridView && !parent.empty
                anchors.fill: parent
                topMargin: Theme.sp5
                bottomMargin: Theme.sp5
                leftMargin: Theme.sp4
                rightMargin: Theme.sp4
                cellWidth: 178 + Theme.sp4
                cellHeight: 286
                clip: true
                model: win.model
                interactive: true
                z: 1

                delegate: Item {
                    id: tile
                    width: 178
                    height: 286

                    required property int index
                    required property string torrentName
                    required property string metaTitle
                    required property string stateKey
                    required property real progress
                    required property string posterPath
                    required property string stateString
                    required property string category
                    required property string size

                    readonly property string posterUrl: posterPath && posterPath.length > 0 ? "file://" + posterPath : ""

                    // .poster wrapper (aspect 3:4 ≈ 178:237)
                    Item {
                        id: posterWrap
                        width: 178
                        height: 237

                        // fallback (no poster): tinted bg + watermark + category + title
                        Rectangle {
                            anchors.fill: parent
                            radius: 10
                            color: "#161618"
                            visible: tile.posterUrl === ""
                            Text {
                                anchors.right: parent.right
                                anchors.bottom: parent.bottom
                                anchors.rightMargin: -10
                                anchors.bottomMargin: -22
                                text: (tile.metaTitle || tile.torrentName).charAt(0).toUpperCase()
                                color: Qt.rgba(1, 1, 1, 0.05)
                                font.pointSize: 105
                                font.weight: Font.Bold
                                font.family: Theme.fontSans
                            }
                            Text {
                                anchors.left: parent.left; anchors.top: parent.top
                                anchors.leftMargin: 13; anchors.topMargin: 12
                                text: tile.category
                                color: Qt.rgba(1, 1, 1, 0.42)
                                font.pointSize: 9.5; font.weight: Font.Bold; font.letterSpacing: 1.3
                                font.capitalization: Font.AllUppercase
                                font.family: Theme.fontSans
                            }
                            Text {
                                anchors.left: parent.left; anchors.right: parent.right
                                anchors.bottom: parent.bottom
                                anchors.leftMargin: 13; anchors.rightMargin: 13; anchors.bottomMargin: 15
                                text: tile.metaTitle || tile.torrentName
                                color: "#f5f5f6"
                                font.pointSize: 18; font.weight: Font.Bold; font.letterSpacing: -0.3
                                font.family: Theme.fontSans
                                wrapMode: Text.WordWrap
                                maximumLineCount: 3
                                elide: Text.ElideRight
                            }
                        }

                        // poster image (masked rounded) — only when present
                        Rectangle {
                            id: posterBg
                            anchors.fill: parent
                            color: "#161618"
                            visible: false
                            layer.enabled: true
                            Image {
                                anchors.fill: parent
                                source: tile.posterUrl
                                fillMode: Image.PreserveAspectCrop
                                asynchronous: true
                            }
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
                            visible: tile.posterUrl !== ""
                        }
                        // .pbar progress (bottom, over everything)
                        Rectangle {
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.bottom: parent.bottom
                            height: 3
                            color: Qt.rgba(0, 0, 0, 0.5)
                            Rectangle {
                                height: parent.height
                                width: parent.width * tile.progress
                                color: win.fillFor(tile.stateKey)
                            }
                        }
                        // border overlay (radius 10, hair / accent when sel)
                        Rectangle {
                            anchors.fill: parent
                            radius: 10
                            color: "transparent"
                            border.color: win.selected === tile.index ? Theme.accent : (tileMa.containsMouse ? Qt.rgba(1,1,1,0.2) : Theme.hair)
                            border.width: win.selected === tile.index ? 2 : 1
                        }
                        MouseArea {
                            id: tileMa
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: win.selectRow(tile.index)
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
                                color: win.dotFor(tile.stateKey)
                            }
                            Text {
                                anchors.verticalCenter: parent.verticalCenter
                                text: tile.stateString
                                color: win.textFor(tile.stateKey)
                                font.pointSize: 11.5
                                font.family: Theme.fontSans
                            }
                        }
                        Item { Layout.fillWidth: true }
                        Text {
                            text: tile.size
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
                visible: !win.gridView && !parent.empty
                anchors.fill: parent
                clip: true
                model: win.model
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
                    id: lrow
                    width: ListView.view.width
                    height: 56

                    required property int index
                    required property string torrentName
                    required property string stateKey
                    required property real progress
                    required property string stateString
                    required property string size
                    required property string downSpeed
                    required property string upSpeed
                    required property int downRate
                    required property int upRate
                    required property string category
                    required property int numPeers
                    required property string posterPath

                    readonly property string posterUrl: posterPath && posterPath.length > 0 ? "file://" + posterPath : ""

                    color: win.selected === index ? Theme.sel : (rowMa.containsMouse ? Theme.hover : "transparent")

                    // .sel inset 2px barra esquerda
                    Rectangle {
                        visible: win.selected === lrow.index
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

                        // .name: poster thumb + nome
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: Theme.sp3
                            PosterThumb {
                                Layout.alignment: Qt.AlignVCenter
                                posterUrl: lrow.posterUrl
                                letter: lrow.torrentName.length > 0 ? lrow.torrentName.charAt(0).toUpperCase() : ""
                            }
                            Text {
                                Layout.fillWidth: true
                                Layout.alignment: Qt.AlignVCenter
                                text: lrow.torrentName
                                color: Theme.t1
                                font.pointSize: 13
                                font.family: Theme.fontSans
                                elide: Text.ElideRight
                            }
                        }
                        Text {
                            text: lrow.size
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
                                    width: parent.width * lrow.progress
                                    color: win.fillFor(lrow.stateKey)
                                }
                                Text {
                                    anchors.centerIn: parent
                                    text: Math.round(lrow.progress * 100) + "%"
                                    color: "#fff"
                                    font.pointSize: 10.5
                                    font.weight: Font.DemiBold
                                    font.family: Theme.fontMono
                                }
                            }
                        }
                        Text {
                            text: lrow.downRate > 0 ? lrow.downSpeed : "—"
                            Layout.preferredWidth: 78
                            horizontalAlignment: Text.AlignRight
                            color: lrow.downRate > 0 ? Theme.accentText : Theme.t4
                            font.pointSize: 12
                            font.family: Theme.fontMono
                        }
                        Text {
                            text: lrow.upRate > 0 ? lrow.upSpeed : "—"
                            Layout.preferredWidth: 78
                            horizontalAlignment: Text.AlignRight
                            color: lrow.upRate > 0 ? Theme.up : Theme.t4
                            font.pointSize: 12
                            font.family: Theme.fontMono
                        }
                        Text {
                            text: lrow.stateString
                            Layout.preferredWidth: 110
                            color: win.textFor(lrow.stateKey)
                            font.pointSize: 12
                            font.weight: Theme.hasAnime ? Font.DemiBold : Font.Medium
                            font.family: Theme.fontSans
                        }
                        Text {
                            text: lrow.category
                            Layout.preferredWidth: 90
                            color: Theme.t3
                            font.pointSize: 12
                            font.family: Theme.fontSans
                            elide: Text.ElideRight
                        }
                        Text {
                            text: lrow.numPeers
                            Layout.preferredWidth: 56
                            horizontalAlignment: Text.AlignRight
                            color: lrow.numPeers === 0 ? Theme.t4 : Theme.t2
                            font.pointSize: 12
                            font.family: Theme.fontMono
                        }
                    }
                    MouseArea {
                        id: rowMa
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: win.selectRow(lrow.index)
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
                id: graphShape
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
                        x: graphShape.width * 0.5
                        y: 6
                        control1X: graphShape.width * 0.18
                        control1Y: 24
                        control2X: graphShape.width * 0.32
                        control2Y: 4
                    }
                    PathCubic {
                        x: graphShape.width
                        y: 18
                        control1X: graphShape.width * 0.68
                        control1Y: 8
                        control2X: graphShape.width * 0.82
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
                        x: graphShape.width * 0.5
                        y: 22
                        control1X: graphShape.width * 0.18
                        control1Y: 30
                        control2X: graphShape.width * 0.32
                        control2Y: 18
                    }
                    PathCubic {
                        x: graphShape.width
                        y: 26
                        control1X: graphShape.width * 0.68
                        control1Y: 24
                        control2X: graphShape.width * 0.82
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
                                source: win.hasSel && session.selectedPoster.length > 0 ? "file://" + session.selectedPoster : ""
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
                            text: win.hasSel ? (session.selectedMetaTitle.length > 0 ? session.selectedMetaTitle : session.selectedName) : "Nenhum torrent selecionado"
                            color: Theme.t1
                            font.pointSize: 17
                            font.weight: Font.DemiBold
                            font.letterSpacing: -0.2
                            font.family: Theme.fontSans
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                        Text {
                            visible: win.hasSel && session.selectedMetaInfo.length > 0
                            text: win.hasSel ? session.selectedMetaInfo : ""
                            color: Theme.t3
                            font.pointSize: 11.5
                            font.family: Theme.fontSans
                        }
                        Text {
                            visible: win.hasSel && session.selectedDescription.length > 0
                            Layout.topMargin: 6
                            Layout.fillWidth: true
                            wrapMode: Text.WordWrap
                            maximumLineCount: 4
                            elide: Text.ElideRight
                            color: Theme.t2
                            font.pointSize: 12
                            font.family: Theme.fontSans
                            lineHeight: 1.55
                            text: win.hasSel ? session.selectedDescription : ""
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
                                    { k: "Nome",    v: win.hasSel ? session.selectedName : "—" },
                                    { k: "Tamanho", v: win.hasSel ? session.selectedSize : "—" },
                                    { k: "Hash",    v: win.hasSel ? session.selectedHash : "—" },
                                    { k: "Estado",  v: win.hasSel ? session.selectedState : "—" }
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
                                    { k: "Baixado",  v: win.hasSel ? session.selectedDownloaded : "—" },
                                    { k: "Download", v: win.hasSel ? session.selectedDownSpeed : "—" },
                                    { k: "Upload",   v: win.hasSel ? session.selectedUpSpeed : "—" },
                                    { k: "ETA",      v: win.hasSel ? session.selectedEta : "—" }
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
                                    { k: "Seeds", v: win.hasSel ? String(session.selectedSeeds) : "—" },
                                    { k: "Peers", v: win.hasSel ? String(session.selectedPeers) : "—" },
                                    { k: "Ratio", v: win.hasSel ? session.selectedRatio : "—" }
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
                    text: typeof session !== "undefined"
                          ? session.torrentCount + " torrents · " + session.activeCount + " ativos"
                          : "0 torrents"
                    color: Theme.t4
                    font.pointSize: 11.5
                    font.family: Theme.fontSans
                }
                Item { Layout.fillWidth: true }
                Rectangle { Layout.alignment: Qt.AlignVCenter; width: 6; height: 6; radius: 3; color: Theme.accent }
                Text { text: typeof session !== "undefined" ? session.totalDownSpeed : "0 KB/s"; color: Theme.t3; font.pointSize: 11.5; font.family: Theme.fontMono }
                Rectangle { Layout.alignment: Qt.AlignVCenter; width: 6; height: 6; radius: 3; color: Theme.amber }
                Text { text: typeof session !== "undefined" ? session.totalUpSpeed : "0 KB/s"; color: Theme.t3; font.pointSize: 11.5; font.family: Theme.fontMono }
                Text {
                    text: typeof session !== "undefined"
                          ? "·  Total " + session.totalDownloaded + " ↓ · " + session.totalUploaded + " ↑ · Ratio " + session.globalRatio
                          : ""
                    color: Theme.t4
                    font.pointSize: 11.5
                    font.family: Theme.fontSans
                }
            }
        }
    }

    // ================== NATIVE FILE PICKER (Abrir) ==================
    FileDialog {
        id: openFileDlg
        title: "Abrir torrent"
        nameFilters: ["Arquivos torrent (*.torrent)", "Todos os arquivos (*)"]
        onAccepted: if (typeof session !== "undefined") session.addTorrentFile(selectedFile.toString())
    }

    // ================== OVERLAY DIALOGS (in-app, backdrop covers all) ==================
    MagnetDialog {
        id: magnetDlg
        onAccepted: if (magnetText.length > 0 && typeof session !== "undefined") session.addMagnetUri(magnetText)
    }
    AddTorrentDialog  { id: addTorrentDlg }
    RemoveDialog {
        id: removeDlg
        onAccepted: if (typeof session !== "undefined") {
            if (deleteFiles) session.removeSelectedWithFiles(); else session.removeSelected()
        }
    }
    CreateTorrentDialog { id: createDlg }
    AddAddonDialog      { id: addAddonDlg }
    WelcomeDialog       { id: welcomeDlg }
    ReleaseNotesDialog  { id: releaseNotesDlg }
    AboutDialog         { id: aboutDlg }

    // ================== TOP-LEVEL WINDOWS ==================
    SearchWindow      { id: searchWin;   visible: false }
    RssWindow         { id: rssWin;      visible: false }
    SettingsWindow    { id: settingsWin; visible: false }
}
