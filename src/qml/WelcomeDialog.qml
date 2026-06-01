// Source: BATorrent Welcome.html + bat-dialog.css + <style> inline
import QtQuick
import QtQuick.Layouts
import "theme"
import "widgets"

BatDialog {
    id: dlg
    title: (i18n.language, i18n.t("welcome_window_title"))
    cardW: 560
    cardH: 470
    okText: (i18n.language, i18n.t("welcome_start"))
    showCancel: false

    property bool dontShow: false

    // .hero (centralizado)
    ColumnLayout {
        Layout.fillWidth: true
        Layout.alignment: Qt.AlignHCenter
        spacing: 8

        Image {
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: 52
            Layout.preferredHeight: 52
            source: "qrc:/images/logo.svg"
            sourceSize: Qt.size(104, 104)
            fillMode: Image.PreserveAspectFit
        }
        Eyebrow { Layout.alignment: Qt.AlignHCenter; text: (i18n.language, i18n.t("welcome_eyebrow")); red: true }
        Text {
            Layout.alignment: Qt.AlignHCenter
            text: (i18n.language, i18n.t("welcome_heading"))
            color: Theme.t1
            font.pointSize: 25
            font.weight: Font.Black
            font.family: Theme.fontSans
        }
        Text {
            Layout.alignment: Qt.AlignHCenter
            Layout.maximumWidth: 400
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            text: (i18n.language, i18n.t("welcome_blurb2"))
            color: Theme.t2
            font.pointSize: 12.5
            font.family: Theme.fontSans
        }
    }

    // .cards grid 2×2
    GridLayout {
        Layout.fillWidth: true
        Layout.topMargin: 6
        columns: 2
        rowSpacing: 10
        columnSpacing: 10

        Repeater {
            model: [
                { icon: "qrc:/icons/open.svg",     t: (i18n.language, i18n.t("welcome_card_open_title")),  d: (i18n.language, i18n.t("welcome_card_open_desc")) },
                { icon: "qrc:/icons/magnet.svg",   t: (i18n.language, i18n.t("empty_paste_btn")),    d: (i18n.language, i18n.t("welcome_card_magnet_desc")) },
                { icon: "qrc:/icons/search.svg",   t: (i18n.language, i18n.t("empty_search_btn")),          d: (i18n.language, i18n.t("welcome_card_search_desc")) },
                { icon: "qrc:/icons/rss.svg",      t: (i18n.language, i18n.t("welcome_card_rss_title")),   d: (i18n.language, i18n.t("welcome_card_rss_desc")) }
            ]
            delegate: Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 64
                radius: 11
                color: Theme.panel
                border.color: cardMa.containsMouse ? Theme.accent : Theme.hair
                border.width: 1

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 13
                    anchors.rightMargin: 13
                    spacing: 11

                    Rectangle {
                        Layout.preferredWidth: 38
                        Layout.preferredHeight: 38
                        radius: 9
                        color: Theme.field
                        IconImg { anchors.centerIn: parent; src: modelData.icon; tint: Theme.accentText; s: 18 }
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2
                        Text { text: modelData.t; color: Theme.t1; font.pointSize: 12.5; font.weight: Font.DemiBold; font.family: Theme.fontSans }
                        Text { text: modelData.d; color: Theme.t4; font.pointSize: 10.5; font.family: Theme.fontSans }
                    }
                }
                MouseArea { id: cardMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor }
            }
        }
    }

    // footer checkbox handled via footHint slot — but checklist puts checkbox at footer-left.
    // BatDialog footer only has hint+buttons; embed the (i18n.language, i18n.t("welcome_dont_show2")) as last body row aligned bottom.
    RowLayout {
        Layout.fillWidth: true
        Layout.topMargin: 4
        spacing: 8
        TChk { id: dsChk; on: dlg.dontShow; onToggled: function(v) { dlg.dontShow = v } }
        Text { text: (i18n.language, i18n.t("welcome_dont_show")); color: Theme.t3; font.pointSize: 11.5; font.family: Theme.fontSans }
    }
}
