// Inspect a .torrent file before adding. session.previewTorrent() (extended).
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import "theme"
import "widgets"

BatDialog {
    id: dlg
    title: (i18n.language, i18n.t("inspector_title2"))
    cardW: 600
    cardH: 560
    okText: (i18n.language, i18n.t("release_notes_close"))
    showCancel: false

    readonly property var sess: typeof session !== "undefined" ? session : null
    property string filePath: ""
    property var info: ({})
    property int tab: 0
    property string copyLabel: (i18n.language, i18n.t("inspector_copy_hash"))

    function load(path) {
        filePath = path
        copyLabel = (i18n.language, i18n.t("inspector_copy_hash"))
        tab = 0
        info = sess ? sess.previewTorrent(path) : ({})
        open()
    }

    // ----- error state -----
    ColumnLayout {
        visible: dlg.info.ok === false
        Layout.fillWidth: true
        spacing: Theme.sp3
        Text {
            Layout.fillWidth: true
            text: (i18n.language, i18n.t("inspector_read_fail")) + (dlg.info.error || "")
            color: Theme.accentText
            font.pointSize: 12
            font.family: Theme.fontSans
            wrapMode: Text.WordWrap
        }
    }

    // ----- content state -----
    ColumnLayout {
        visible: dlg.info.ok === true
        Layout.fillWidth: true
        spacing: Theme.sp3

        Text {
            Layout.fillWidth: true
            text: dlg.info.name || ""
            color: Theme.t1
            font.pointSize: 16; font.weight: Font.Bold; font.letterSpacing: -0.2
            font.family: Theme.fontSans
            wrapMode: Text.WordWrap
        }

        // metadata KV grid
        GridLayout {
            Layout.fillWidth: true
            columns: 2
            columnSpacing: Theme.sp4
            rowSpacing: 6

            component MK: Text { color: Theme.t3; font.pointSize: 11.5; font.weight: Font.DemiBold; font.family: Theme.fontSans }
            component MV: Text { Layout.fillWidth: true; color: Theme.t1; font.pointSize: 12; font.family: Theme.fontSans; wrapMode: Text.WrapAnywhere }

            MK { text: (i18n.language, i18n.t("add_torrent_col_size")) }   MV { text: dlg.info.totalSize || "—" }
            MK { text: (i18n.language, i18n.t("detail_files")) }  MV { text: dlg.info.fileCount !== undefined ? String(dlg.info.fileCount) : "—" }
            MK { text: (i18n.language, i18n.t("detail_pieces")) }   MV { text: dlg.info.pieces || "—" }
            MK { text: (i18n.language, i18n.t("detail_kv_hash")) }
            TextEdit {
                Layout.fillWidth: true
                text: dlg.info.infoHash || ""
                readOnly: true
                selectByMouse: true
                color: Theme.t1
                font.pointSize: 11
                font.family: Theme.fontMono
                wrapMode: TextEdit.WrapAnywhere
            }
            MK { visible: dlg.info.creator !== undefined; text: (i18n.language, i18n.t("inspector_creator2")) }
            MV { visible: dlg.info.creator !== undefined; text: dlg.info.creator || "" }
            MK { visible: dlg.info.created !== undefined; text: (i18n.language, i18n.t("inspector_created")) }
            MV { visible: dlg.info.created !== undefined; text: dlg.info.created || "" }
            MK { visible: dlg.info.comment !== undefined; text: (i18n.language, i18n.t("inspector_comment")) }
            MV { visible: dlg.info.comment !== undefined; text: dlg.info.comment || "" }
            MK { visible: dlg.info.isPrivate === true; text: (i18n.language, i18n.t("inspector_private")) }
            MV { visible: dlg.info.isPrivate === true; text: (i18n.language, i18n.t("inspector_yes")) }
        }

        // tabs: Files / Trackers
        RowLayout {
            Layout.fillWidth: true
            Layout.topMargin: Theme.sp2
            spacing: Theme.sp4
            component Tab: Text {
                property int idx: 0
                color: dlg.tab === idx ? Theme.t1 : Theme.t3
                font.pointSize: 12.5
                font.weight: dlg.tab === idx ? Font.DemiBold : Font.Normal
                font.family: Theme.fontSans
                MouseArea { anchors.fill: parent; anchors.margins: -6; cursorShape: Qt.PointingHandCursor; onClicked: dlg.tab = parent.idx }
            }
            Tab { idx: 0; text: (i18n.language, i18n.t("detail_files")) }
            Tab { idx: 1; text: (i18n.language, i18n.t("set_grp_trackers")) }
            Item { Layout.fillWidth: true }
        }
        Rectangle { Layout.fillWidth: true; height: 1; color: Theme.hairSoft }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 180
            radius: 8
            color: Theme.field
            border.color: Theme.hair
            border.width: 1

            ListView {
                id: filesView
                visible: dlg.tab === 0
                anchors.fill: parent
                anchors.margins: 6
                clip: true
                model: dlg.info.files || []
                boundsBehavior: Flickable.StopAtBounds
                delegate: RowLayout {
                    required property var modelData
                    width: ListView.view.width
                    height: 26
                    spacing: Theme.sp3
                    Text { Layout.fillWidth: true; leftPadding: 6; text: modelData.path; color: Theme.t2; font.pointSize: 11; font.family: Theme.fontMono; elide: Text.ElideMiddle }
                    Text { rightPadding: 6; text: modelData.size; color: Theme.t4; font.pointSize: 11; font.family: Theme.fontMono }
                }
            }
            ListView {
                id: trackersView
                visible: dlg.tab === 1
                anchors.fill: parent
                anchors.margins: 6
                clip: true
                model: dlg.info.trackers || []
                boundsBehavior: Flickable.StopAtBounds
                delegate: Text {
                    required property var modelData
                    width: ListView.view.width
                    height: 26
                    leftPadding: 6
                    verticalAlignment: Text.AlignVCenter
                    text: modelData
                    color: Theme.t2
                    font.pointSize: 11
                    font.family: Theme.fontMono
                    elide: Text.ElideRight
                }
            }
            Text {
                anchors.centerIn: parent
                visible: dlg.tab === 1 && (!dlg.info.trackers || dlg.info.trackers.length === 0)
                text: (i18n.language, i18n.t("inspector_no_trackers2"))
                color: Theme.t4
                font.pointSize: 11.5
                font.family: Theme.fontSans
            }
        }

        // footer actions
        RowLayout {
            Layout.fillWidth: true
            Layout.topMargin: Theme.sp2
            spacing: Theme.sp2
            BtnFlat {
                text: dlg.copyLabel
                onClicked: { if (dlg.sess) { dlg.sess.copyToClipboard(dlg.info.infoHash || ""); dlg.copyLabel = (i18n.language, i18n.t("inspector_copied")) } }
            }
            Item { Layout.fillWidth: true }
            BtnFlat {
                primary: true
                text: (i18n.language, i18n.t("inspector_add_now"))
                onClicked: { if (dlg.sess) dlg.sess.addTorrentFile(dlg.filePath); dlg.close() }
            }
        }
    }
}
