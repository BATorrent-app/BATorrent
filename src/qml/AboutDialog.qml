// Source: BATorrent About.html + bat-dialog.css + <style> inline
import QtQuick
import QtQuick.Layouts
import "theme"
import "widgets"

BatDialog {
    id: dlg
    title: (i18n.language, i18n.t("about_title"))
    cardW: 460
    cardH: 540
    okText: (i18n.language, i18n.t("action_donate"))
    showCancel: false
    footHint: "© 2026 · open-source"

    // .ahero
    ColumnLayout {
        Layout.fillWidth: true
        Layout.alignment: Qt.AlignHCenter
        spacing: 8

        Image {
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: 58
            Layout.preferredHeight: 58
            source: "qrc:/images/logo.svg"
            sourceSize: Qt.size(116, 116)
            fillMode: Image.PreserveAspectFit
        }
        // wordmark BAT + orrent
        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: 0
            Text { text: "BAT"; color: Theme.accent; font.pointSize: 22; font.weight: Font.Black; font.letterSpacing: 1; font.family: Theme.fontSans }
            Text { text: "orrent"; color: Theme.t1; font.pointSize: 22; font.weight: Font.Black; font.letterSpacing: 1; font.family: Theme.fontSans }
        }
        // .vrow
        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: 8
            TChip { text: "v2.6.1" }
            Text { text: (i18n.language, i18n.t("about_build_stable")); color: Theme.t4; font.pointSize: 10.5; font.family: Theme.fontMono }
        }
        Text {
            Layout.alignment: Qt.AlignHCenter
            text: (i18n.language, i18n.t("about_description"))
            color: Theme.t2
            font.pointSize: 11.5
            font.family: Theme.fontSans
        }
    }

    // .privacy card
    Rectangle {
        Layout.fillWidth: true
        Layout.topMargin: Theme.sp3
        Layout.bottomMargin: Theme.sp3
        radius: 12
        color: Theme.panel
        border.color: Theme.hair
        border.width: 1
        implicitHeight: privRow.implicitHeight + 26

        RowLayout {
            id: privRow
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            anchors.leftMargin: 13
            anchors.rightMargin: 13
            spacing: 11

            Rectangle {
                Layout.preferredWidth: 30
                Layout.preferredHeight: 30
                Layout.alignment: Qt.AlignTop
                radius: 8
                color: Theme.field
                Text { anchors.centerIn: parent; text: "🛡"; font.pointSize: 14 }
            }
            Text {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                text: (i18n.language, i18n.t("about_no_telemetry_body"))
                color: Theme.t3
                font.pointSize: 10.5
                font.family: Theme.fontSans
                lineHeight: 1.45
            }
        }
    }

    // .glabel BIBLIOTECAS
    Text {
        text: (i18n.language, i18n.t("about_libraries"))
        color: Theme.t4
        font.pointSize: 10
        font.weight: Font.Bold
        font.letterSpacing: 0.8
        font.family: Theme.fontSans
    }
    ColumnLayout {
        Layout.fillWidth: true
        spacing: 0
        Repeater {
            model: [
                { nm: "Qt", v: "6.10.2" },
                { nm: "libtorrent-rasterbar", v: "2.0.11" },
                { nm: "OpenSSL", v: "3.6.1" },
                { nm: "Boost", v: "1.86" }
            ]
            delegate: ColumnLayout {
                Layout.fillWidth: true
                spacing: 0
                RowLayout {
                    Layout.fillWidth: true
                    Layout.topMargin: 8
                    Layout.bottomMargin: 8
                    Text { text: modelData.nm; color: Theme.t1; font.pointSize: 12; font.family: Theme.fontSans }
                    Item { Layout.fillWidth: true }
                    Text { text: modelData.v; color: Theme.t4; font.pointSize: 11; font.family: Theme.fontMono }
                }
                Rectangle { visible: index < 3; Layout.fillWidth: true; height: 1; color: Theme.hairSoft }
            }
        }
    }

    // .license
    RowLayout {
        Layout.fillWidth: true
        Layout.topMargin: Theme.sp1
        Text { text: (i18n.language, i18n.t("about_license")); color: Theme.t3; font.pointSize: 12; font.family: Theme.fontSans }
        Item { Layout.fillWidth: true }
        Text { text: "MIT"; color: Theme.t2; font.pointSize: 12; font.family: Theme.fontMono }
    }
}
