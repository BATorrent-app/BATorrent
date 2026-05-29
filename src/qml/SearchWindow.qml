// Source: BATorrent Search.html (bat-dialog.css + <style> inline)
import QtQuick
import QtQuick.Layouts
import "theme"
import "widgets"

Window {
    id: win
    width: 780
    height: 560
    minimumWidth: 620
    minimumHeight: 420
    color: Theme.bg
    title: "Buscar torrents"

    ListModel {
        id: results
        ListElement { name: "Forza.Horizon.6-CODEX"; prov: "The Pirate Bay"; size: "92.4 GB"; se: "412"; le: "88"; age: "3d" }
        ListElement { name: "Forza Horizon 6 [FitGirl Repack] selective"; prov: "The Pirate Bay"; size: "58.1 GB"; se: "389"; le: "120"; age: "3d" }
        ListElement { name: "Forza.Horizon.6.MULTi15-ElAmigos"; prov: "1337x"; size: "71.9 GB"; se: "154"; le: "63"; age: "2d" }
        ListElement { name: "Forza Horizon 6 Deluxe Edition"; prov: "RuTracker"; size: "96.0 GB"; se: "77"; le: "41"; age: "5d" }
        ListElement { name: "Forza.Horizon.6.Update.v1.2-CODEX"; prov: "The Pirate Bay"; size: "2.1 GB"; se: "203"; le: "12"; age: "1d" }
        ListElement { name: "Forza Horizon 6 OST (FLAC)"; prov: "1337x"; size: "480 MB"; se: "34"; le: "5"; age: "6d" }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // ----- titlebar (.tb) -----
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 36
            color: Theme.elev
            Text {
                anchors.centerIn: parent
                text: "Buscar torrents"
                color: Theme.t2
                font.pointSize: 12.5
                font.weight: Font.DemiBold
                font.family: Theme.fontSans
            }
            Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hairSoft }
        }

        // ----- .sbar (padding s4 s5, gap s3) -----
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 36 + 2 * Theme.sp4
            color: "transparent"
            Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hair }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Theme.sp5
                anchors.rightMargin: Theme.sp5
                anchors.topMargin: Theme.sp4
                anchors.bottomMargin: Theme.sp4
                spacing: Theme.sp3

                TFld {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 36
                    icon: "qrc:/icons/search.svg"
                    text: "forza horizon 6"
                }
                // .prov
                Rectangle {
                    Layout.preferredHeight: 36
                    implicitWidth: provRow.implicitWidth + 24
                    radius: 8
                    color: "transparent"
                    border.color: Theme.hair
                    border.width: 1
                    Row {
                        id: provRow
                        anchors.centerIn: parent
                        spacing: 8
                        Text { anchors.verticalCenter: parent.verticalCenter; text: "Todos os provedores"; color: Theme.t2; font.pointSize: 12; font.family: Theme.fontSans }
                        IconImg { anchors.verticalCenter: parent.verticalCenter; src: "qrc:/icons/chevron.svg"; tint: Theme.t4; s: 13 }
                    }
                    MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor }
                }
            }
        }

        // ----- .rhd header -----
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 34
            color: "transparent"
            Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hair }
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Theme.sp5
                anchors.rightMargin: Theme.sp5
                spacing: Theme.sp4
                Text { text: "NOME"; Layout.fillWidth: true; color: Theme.t4; font.pointSize: 10; font.weight: Font.DemiBold; font.letterSpacing: 0.6; font.family: Theme.fontSans }
                Text { text: "PROVEDOR"; Layout.preferredWidth: 120; color: Theme.t4; font.pointSize: 10; font.weight: Font.DemiBold; font.letterSpacing: 0.6; font.family: Theme.fontSans }
                Text { text: "TAMANHO"; Layout.preferredWidth: 78; horizontalAlignment: Text.AlignRight; color: Theme.t4; font.pointSize: 10; font.weight: Font.DemiBold; font.letterSpacing: 0.6; font.family: Theme.fontSans }
                Text { text: "SEEDS"; Layout.preferredWidth: 56; horizontalAlignment: Text.AlignRight; color: Theme.t4; font.pointSize: 10; font.weight: Font.DemiBold; font.letterSpacing: 0.6; font.family: Theme.fontSans }
                Text { text: "LEECH"; Layout.preferredWidth: 56; horizontalAlignment: Text.AlignRight; color: Theme.t4; font.pointSize: 10; font.weight: Font.DemiBold; font.letterSpacing: 0.6; font.family: Theme.fontSans }
                Text { text: "IDADE"; Layout.preferredWidth: 60; horizontalAlignment: Text.AlignRight; color: Theme.t4; font.pointSize: 10; font.weight: Font.DemiBold; font.letterSpacing: 0.6; font.family: Theme.fontSans }
                Item { Layout.preferredWidth: 36 }
            }
        }

        // ----- .results -----
        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: results
            boundsBehavior: Flickable.StopAtBounds

            delegate: Rectangle {
                width: ListView.view.width
                height: 46
                color: resMa.containsMouse ? Theme.hover : "transparent"
                Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hairSoft }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: Theme.sp5
                    anchors.rightMargin: Theme.sp5
                    spacing: Theme.sp4

                    Text {
                        Layout.fillWidth: true
                        text: model.name
                        color: Theme.t1
                        font.pointSize: 12.5
                        font.family: Theme.fontSans
                        elide: Text.ElideRight
                    }
                    // .prv chip
                    Item {
                        Layout.preferredWidth: 120
                        TChip { anchors.verticalCenter: parent.verticalCenter; text: model.prov }
                    }
                    Text { text: model.size; Layout.preferredWidth: 78; horizontalAlignment: Text.AlignRight; color: Theme.t2; font.pointSize: 12; font.family: Theme.fontMono }
                    Text { text: model.se; Layout.preferredWidth: 56; horizontalAlignment: Text.AlignRight; color: Theme.grn; font.pointSize: 12; font.family: Theme.fontMono }
                    Text { text: model.le; Layout.preferredWidth: 56; horizontalAlignment: Text.AlignRight; color: Theme.t3; font.pointSize: 12; font.family: Theme.fontMono }
                    Text { text: model.age; Layout.preferredWidth: 60; horizontalAlignment: Text.AlignRight; color: Theme.t4; font.pointSize: 11.5; font.family: Theme.fontMono }
                    // .add button
                    Item {
                        Layout.preferredWidth: 36
                        Rectangle {
                            anchors.right: parent.right
                            anchors.verticalCenter: parent.verticalCenter
                            width: 28; height: 28; radius: 7
                            color: "transparent"
                            border.color: addMa.containsMouse ? Theme.accent : Theme.hair
                            border.width: 1
                            Text {
                                anchors.centerIn: parent
                                text: "+"
                                color: addMa.containsMouse ? Theme.accentText : Theme.t3
                                font.pointSize: 15
                            }
                            MouseArea { id: addMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor }
                        }
                    }
                }
                MouseArea { id: resMa; anchors.fill: parent; hoverEnabled: true; acceptedButtons: Qt.NoButton }
            }
        }

        // ----- .foot -----
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 56
            color: Theme.elev
            Rectangle { anchors.top: parent.top; width: parent.width; height: 1; color: Theme.hair }
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Theme.sp5
                anchors.rightMargin: 20
                Text { text: "42 resultados · 3 provedores"; color: Theme.t4; font.pointSize: 10.5; font.family: Theme.fontSans }
                Item { Layout.fillWidth: true }
                BtnFlat { text: "Fechar"; onClicked: win.close() }
            }
        }
    }
}
