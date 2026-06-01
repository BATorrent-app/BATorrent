// Source: BATorrent Magnet.html + bat-dialog.css
import QtQuick
import QtQuick.Layouts
import "theme"
import "widgets"

BatDialog {
    id: dlg
    title: (i18n.language, i18n.t("magnet_title"))
    cardW: 480
    cardH: 470
    footHint: (i18n.language, i18n.t("magnet_multi_hint"))
    okText: (i18n.language, i18n.t("add_torrent_add_btn"))

    property alias magnetText: magnetArea.text
    onOpenedChanged: if (opened) magnetArea.text = ""

    // ----- col 1: eyebrow + title -----
    ColumnLayout {
        Layout.fillWidth: true
        spacing: Theme.sp1
        Eyebrow { text: (i18n.language, i18n.t("magnet_add")); red: true }
        Text {
            text: (i18n.language, i18n.t("magnet_link"))
            color: Theme.t1
            font.pointSize: 19
            font.weight: Font.DemiBold
            font.letterSpacing: -0.3
            font.family: Theme.fontSans
        }
    }

    // ----- col 2: label + textarea -----
    ColumnLayout {
        Layout.fillWidth: true
        spacing: 7
        Text {
            text: (i18n.language, i18n.t("magnet_paste"))
            color: Theme.t3
            font.pointSize: 11
            font.weight: Font.DemiBold
            font.family: Theme.fontSans
        }
        TArea {
            id: magnetArea
            Layout.fillWidth: true
            Layout.preferredHeight: 88
            placeholder: "magnet:?xt=urn:btih:..."
        }
    }

    // ----- col 3: meta-note card -----
    Rectangle {
        Layout.fillWidth: true
        Layout.preferredHeight: 60
        radius: 9
        color: Theme.panel
        border.color: Theme.hair
        border.width: 1

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 12
            anchors.rightMargin: 12
            anchors.topMargin: 10
            anchors.bottomMargin: 10
            spacing: 10

            // info icon (i circular) — desenhado inline já que não temos info.svg
            Rectangle {
                Layout.alignment: Qt.AlignTop
                width: 15; height: 15
                radius: 7.5
                color: "transparent"
                border.color: Theme.t3
                border.width: 1
                Text {
                    anchors.centerIn: parent
                    text: "i"
                    color: Theme.t3
                    font.pointSize: 9
                    font.weight: Font.Bold
                    font.family: Theme.fontSans
                }
            }
            Text {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                text: (i18n.language, i18n.t("magnet_meta_hint"))
                color: Theme.t3
                font.pointSize: 11
                font.family: Theme.fontSans
                lineHeight: 1.4
            }
        }
    }

    // ----- col 4: salvar em + path -----
    ColumnLayout {
        Layout.fillWidth: true
        spacing: 7
        Text {
            text: (i18n.language, i18n.t("detail_kv_save_to"))
            color: Theme.t3
            font.pointSize: 11
            font.weight: Font.DemiBold
            font.family: Theme.fontSans
        }
        PathFld {
            Layout.fillWidth: true
            text: "/Users/voce/Downloads"
        }
    }

    // ----- col 5: togrow Iniciar imediatamente -----
    RowLayout {
        Layout.fillWidth: true
        spacing: 12
        Text {
            text: (i18n.language, i18n.t("addt_start_now"))
            color: Theme.t2
            font.pointSize: 12
            font.family: Theme.fontSans
        }
        Item { Layout.fillWidth: true }
        TToggle { on: true }
    }
}
