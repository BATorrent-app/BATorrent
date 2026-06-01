// Source: BATorrent Remove Confirm.html + bat-dialog.css
import QtQuick
import QtQuick.Layouts
import "theme"
import "widgets"

BatDialog {
    id: dlg
    title: (i18n.language, i18n.t("remove_title2"))
    cardW: 440
    cardH: 360
    okText: (i18n.language, i18n.t("addon_remove"))

    // default OFF — "Remove" only un-lists the torrent; deleting the downloaded
    // files is an explicit opt-in (matches qBittorrent/Transmission convention).
    property bool deleteFiles: false

    // .top — warn icon + title + description
    RowLayout {
        Layout.fillWidth: true
        Layout.topMargin: Theme.sp2
        spacing: Theme.sp3

        // .warn-ic 40×40
        Rectangle {
            Layout.preferredWidth: 40
            Layout.preferredHeight: 40
            Layout.alignment: Qt.AlignTop
            radius: 10
            color: Qt.rgba(229/255, 51/255, 43/255, 0.12)
            border.color: Qt.rgba(229/255, 51/255, 43/255, 0.32)
            border.width: 1
            Text {
                anchors.centerIn: parent
                text: "⚠"
                color: Theme.accentText
                font.pointSize: 20
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 6
            Text {
                text: (i18n.language, i18n.t("remove_confirm_q"))
                color: Theme.t1
                font.pointSize: 16
                font.weight: Font.DemiBold
                font.family: Theme.fontSans
            }
            Text {
                Layout.fillWidth: true
                Layout.maximumWidth: 320
                wrapMode: Text.WordWrap
                color: Theme.t3
                font.pointSize: 12
                font.family: Theme.fontSans
                textFormat: Text.StyledText
                text: "Você está prestes a remover <font color='" + Theme.t2 + "'><b>Forza.Horizon.6-CODEX</b></font> da lista. Esta ação não pode ser desfeita."
            }
        }
    }

    // .card — .opt clickable
    Rectangle {
        Layout.fillWidth: true
        Layout.preferredHeight: 64
        radius: 11
        color: Theme.panel
        border.color: Theme.hair
        border.width: 1

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 14
            anchors.rightMargin: 14
            spacing: Theme.sp3

            TChk {
                id: chk
                Layout.alignment: Qt.AlignVCenter
                on: dlg.deleteFiles
                onToggled: function(v) { dlg.deleteFiles = v }
            }
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2
                Text {
                    text: (i18n.language, i18n.t("remove_also_files"))
                    color: Theme.t1
                    font.pointSize: 12.5
                    font.family: Theme.fontSans
                }
                Text {
                    text: (i18n.language, i18n.t("remove_delete_note"))
                    color: Theme.t4
                    font.pointSize: 10.5
                    font.family: Theme.fontSans
                }
            }
        }
        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onClicked: { dlg.deleteFiles = !dlg.deleteFiles; chk.on = dlg.deleteFiles }
        }
    }
}
