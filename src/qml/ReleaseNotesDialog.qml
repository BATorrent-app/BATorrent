// Source: BATorrent Release Notes.html + bat-dialog.css + <style> inline
import QtQuick
import QtQuick.Layouts
import "theme"
import "widgets"

BatDialog {
    id: dlg
    title: (i18n.language, i18n.t("release_notes_title2"))
    cardW: 540
    cardH: 560
    okText: (i18n.language, i18n.t("release_notes_close"))
    showCancel: false
    footHint: (i18n.language, i18n.t("rn_changelog_src"))

    // .nhead
    RowLayout {
        Layout.fillWidth: true
        Layout.bottomMargin: Theme.sp1
        spacing: 14

        // .badge 44×44 ★
        Rectangle {
            Layout.preferredWidth: 44
            Layout.preferredHeight: 44
            Layout.alignment: Qt.AlignTop
            radius: 10
            color: Theme.panel
            border.color: Theme.hair
            border.width: 1
            Text { anchors.centerIn: parent; text: "★"; color: Theme.accentText; font.pointSize: 22 }
        }
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 3
            RowLayout {
                spacing: 8
                Eyebrow { text: (i18n.language, i18n.t("release_notes_eyebrow")); red: true }
                TChip { text: "v2.6.1" }
            }
            Text {
                text: (i18n.language, i18n.t("release_notes_check"))
                color: Theme.t1
                font.pointSize: 19
                font.weight: Font.DemiBold
                font.letterSpacing: -0.3
                font.family: Theme.fontSans
            }
            Text {
                text: "28 mai 2026 · estável"
                color: Theme.t4
                font.pointSize: 10.5
                font.family: Theme.fontMono
            }
        }
    }
    Rectangle { Layout.fillWidth: true; height: 1; color: Theme.hairSoft }

    // changelog groups
    component ClGroup: ColumnLayout {
        id: clg
        property string tag
        property color tagColor: Theme.t4
        property var items: []
        Layout.fillWidth: true
        spacing: 8
        RowLayout {
            spacing: 7
            Rectangle { Layout.alignment: Qt.AlignVCenter; width: 7; height: 7; radius: 3.5; color: clg.tagColor }
            Text {
                text: clg.tag
                color: clg.tagColor
                font.pointSize: 9
                font.weight: Font.Bold
                font.letterSpacing: 1.4
                font.family: Theme.fontSans
                font.capitalization: Font.AllUppercase
            }
        }
        Repeater {
            model: clg.items
            delegate: RowLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 2
                spacing: 8
                Text { text: "•"; color: Theme.t4; font.pointSize: 13; Layout.alignment: Qt.AlignTop }
                Text {
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    textFormat: Text.StyledText
                    text: modelData
                    color: Theme.t2
                    font.pointSize: 11.5
                    font.family: Theme.fontSans
                    lineHeight: 1.45
                }
            }
        }
    }

    ClGroup {
        tag: (i18n.language, i18n.t("rn_critical_fix"))
        tagColor: Theme.accentText
        items: [
            "Corrigido o <b><font color='" + Theme.t1 + "'>auto-updater</font></b> quebrado desde a v2.5.0.",
            (i18n.language, i18n.t("rn_net_timeout")),
            (i18n.language, i18n.t("rn_manual_update"))
        ]
    }

    Rectangle { Layout.fillWidth: true; height: 1; color: Theme.hairSoft }
    Text {
        text: "DA v2.6.0"
        color: Theme.t4
        font.pointSize: 10
        font.weight: Font.Bold
        font.letterSpacing: 0.8
        font.family: Theme.fontSans
    }

    ClGroup {
        tag: (i18n.language, i18n.t("rn_search_plugins"))
        items: [ (i18n.language, i18n.t("rn_search_plugins_body")) ]
    }
    ClGroup {
        tag: (i18n.language, i18n.t("rn_i18n_cats"))
        items: [
            (i18n.language, i18n.t("rn_i18n_body")),
            (i18n.language, i18n.t("rn_cats_body"))
        ]
    }
}
