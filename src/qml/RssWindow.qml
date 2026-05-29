// Source: BATorrent RSS.html (bat-dialog.css + <style> inline)
import QtQuick
import QtQuick.Layouts
import "theme"
import "widgets"

Window {
    id: win
    width: 760
    height: 560
    minimumWidth: 620
    minimumHeight: 420
    color: Theme.bg
    title: "RSS"

    property int selectedFeed: 0

    ListModel {
        id: feeds
        ListElement { name: "Nyaa · Anime 1080p"; count: "12"; healthy: true }
        ListElement { name: "EZTV · Séries"; count: "31"; healthy: true }
        ListElement { name: "Linux ISOs"; count: "4"; healthy: false }
    }
    ListModel {
        id: items
        ListElement { nm: "[SubsPlease] Frieren - 28 (1080p)"; auto: true; meta: "1.3 GB · 2h" }
        ListElement { nm: "[SubsPlease] Frieren - 27 (1080p)"; auto: true; meta: "1.3 GB · 1s" }
        ListElement { nm: "[Erai-raws] Frieren - 27 (720p)"; auto: false; meta: "680 MB · 1s" }
        ListElement { nm: "[SubsPlease] Frieren - 26 (1080p)"; auto: true; meta: "1.3 GB · 1sem" }
        ListElement { nm: "[SubsPlease] Frieren - 25 (1080p)"; auto: true; meta: "1.3 GB · 2sem" }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // titlebar
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 36
            color: Theme.elev
            Text { anchors.centerIn: parent; text: "RSS"; color: Theme.t2; font.pointSize: 12.5; font.weight: Font.DemiBold; font.family: Theme.fontSans }
            Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hairSoft }
        }

        // .rhead
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 32 + 2 * Theme.sp4
            color: "transparent"
            Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hair }
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Theme.sp5
                anchors.rightMargin: Theme.sp5
                spacing: Theme.sp3
                Text { text: "Feeds RSS"; color: Theme.t1; font.pointSize: 16; font.weight: Font.DemiBold; font.family: Theme.fontSans }
                TChip { text: "3 feeds" }
                Item { Layout.fillWidth: true }
                BtnFlat { sm: true; text: "Editar regras" }
                BtnFlat { sm: true; primary: true; text: "＋ Adicionar feed" }
            }
        }

        // .rcols
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            // .feeds (232)
            Rectangle {
                Layout.preferredWidth: 232
                Layout.fillHeight: true
                color: "transparent"
                Rectangle { anchors.right: parent.right; width: 1; height: parent.height; color: Theme.hair }

                ListView {
                    anchors.fill: parent
                    anchors.topMargin: Theme.sp3
                    anchors.bottomMargin: Theme.sp3
                    anchors.leftMargin: Theme.sp2
                    anchors.rightMargin: Theme.sp2
                    clip: true
                    model: feeds
                    spacing: 1
                    boundsBehavior: Flickable.StopAtBounds

                    delegate: Rectangle {
                        width: ListView.view.width
                        height: 38
                        radius: 7
                        color: win.selectedFeed === index ? Theme.sel : (feedMa.containsMouse ? Theme.hover : "transparent")

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 11
                            anchors.rightMargin: 11
                            spacing: 10
                            Rectangle {
                                Layout.preferredWidth: 7; Layout.preferredHeight: 7; radius: 3.5
                                color: model.healthy ? Theme.grn : Theme.t4
                            }
                            Text {
                                Layout.fillWidth: true
                                text: model.name
                                color: win.selectedFeed === index ? Theme.t1 : (feedMa.containsMouse ? Theme.t1 : Theme.t2)
                                font.pointSize: 12.5
                                font.family: Theme.fontSans
                                elide: Text.ElideRight
                            }
                            Text { text: model.count; color: Theme.t4; font.pointSize: 11; font.family: Theme.fontMono }
                        }
                        MouseArea { id: feedMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: win.selectedFeed = index }
                    }
                }
            }

            // .items
            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 0

                // .rule banner
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 38
                    color: Theme.panel
                    Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hairSoft }
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: Theme.sp5
                        anchors.rightMargin: Theme.sp5
                        spacing: 9
                        IconImg { src: "qrc:/icons/rss.svg"; tint: Theme.accentText; s: 14 }
                        Text {
                            textFormat: Text.StyledText
                            text: "Auto-baixar itens que contenham <b><font color='" + Theme.t1 + "'>1080p</font></b>"
                            color: Theme.t2
                            font.pointSize: 11.5
                            font.family: Theme.fontSans
                        }
                        Item { Layout.fillWidth: true }
                        TToggle { on: true }
                    }
                }

                // .ilist
                ListView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: items
                    boundsBehavior: Flickable.StopAtBounds

                    delegate: Rectangle {
                        width: ListView.view.width
                        height: 46
                        color: itemMa.containsMouse ? Theme.hover : "transparent"
                        Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hairSoft }
                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: Theme.sp5
                            anchors.rightMargin: Theme.sp5
                            spacing: Theme.sp3
                            Text {
                                Layout.fillWidth: true
                                text: model.nm
                                color: Theme.t1
                                font.pointSize: 12.5
                                font.family: Theme.fontSans
                                elide: Text.ElideRight
                            }
                            // badge-auto
                            Rectangle {
                                visible: model.auto
                                implicitWidth: badgeLbl.implicitWidth + 14
                                implicitHeight: 18
                                radius: 999
                                color: Qt.rgba(63/255, 185/255, 80/255, 0.12)
                                border.color: Qt.rgba(63/255, 185/255, 80/255, 0.3)
                                border.width: 1
                                Text { id: badgeLbl; anchors.centerIn: parent; text: "Auto"; color: Theme.grn; font.pointSize: 9.5; font.weight: Font.DemiBold; font.family: Theme.fontSans }
                            }
                            Text { text: model.meta; color: Theme.t4; font.pointSize: 11; font.family: Theme.fontMono }
                            // .dl
                            Rectangle {
                                Layout.preferredWidth: 28; Layout.preferredHeight: 28
                                radius: 7
                                color: "transparent"
                                border.color: dlMa.containsMouse ? Theme.accent : Theme.hair
                                border.width: 1
                                IconImg { anchors.centerIn: parent; src: "qrc:/icons/download.svg"; tint: dlMa.containsMouse ? Theme.accentText : Theme.t3; s: 14 }
                                MouseArea { id: dlMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor }
                            }
                        }
                        MouseArea { id: itemMa; anchors.fill: parent; hoverEnabled: true; acceptedButtons: Qt.NoButton }
                    }
                }
            }
        }
    }
}
