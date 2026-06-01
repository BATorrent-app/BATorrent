// Peers pane for the detail panel. Data: session.selectedPeerList
// rows: { ip, port, client, downSpeed, upSpeed, progress }
import QtQuick
import QtQuick.Layouts
import "../theme"

ColumnLayout {
    id: pane
    property var peers: []
    spacing: 0

    Rectangle {
        Layout.fillWidth: true
        Layout.preferredHeight: 30
        color: "transparent"
        Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hair }
        RowLayout {
            anchors.fill: parent; anchors.leftMargin: Theme.sp5; anchors.rightMargin: Theme.sp5
            Text { text: (i18n.language, i18n.t("detailpeers_country")); Layout.preferredWidth: 40; color: Theme.t4; font.pointSize: 10; font.weight: Font.DemiBold; font.letterSpacing: 0.6; font.family: Theme.fontSans }
            Text { text: (i18n.language, i18n.t("detailpeers_ip")); Layout.fillWidth: true; color: Theme.t4; font.pointSize: 10; font.weight: Font.DemiBold; font.letterSpacing: 0.6; font.family: Theme.fontSans }
            Text { text: (i18n.language, i18n.t("detailpeers_client")); Layout.preferredWidth: 150; color: Theme.t4; font.pointSize: 10; font.weight: Font.DemiBold; font.letterSpacing: 0.6; font.family: Theme.fontSans }
            Text { text: (i18n.language, i18n.t("detailpeers_progr")); Layout.preferredWidth: 60; horizontalAlignment: Text.AlignRight; color: Theme.t4; font.pointSize: 10; font.weight: Font.DemiBold; font.letterSpacing: 0.6; font.family: Theme.fontSans }
            Text { text: (i18n.language, i18n.t("detailpeers_down")); Layout.preferredWidth: 90; horizontalAlignment: Text.AlignRight; color: Theme.t4; font.pointSize: 10; font.weight: Font.DemiBold; font.letterSpacing: 0.6; font.family: Theme.fontSans }
            Text { text: (i18n.language, i18n.t("detailpeers_up")); Layout.preferredWidth: 90; horizontalAlignment: Text.AlignRight; color: Theme.t4; font.pointSize: 10; font.weight: Font.DemiBold; font.letterSpacing: 0.6; font.family: Theme.fontSans }
        }
    }
    Text {
        visible: pane.peers.length === 0
        Layout.alignment: Qt.AlignHCenter; Layout.topMargin: 18
        text: (i18n.language, i18n.t("detailpeers_empty")); color: Theme.t4; font.pointSize: 11; font.family: Theme.fontSans
    }
    ListView {
        Layout.fillWidth: true; Layout.fillHeight: true; clip: true; model: pane.peers
        visible: pane.peers.length > 0
        boundsBehavior: Flickable.StopAtBounds
        delegate: Rectangle {
            width: ListView.view.width; height: 32; color: "transparent"
            Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hairSoft }
            RowLayout {
                anchors.fill: parent; anchors.leftMargin: Theme.sp5; anchors.rightMargin: Theme.sp5
                Text { text: modelData.flag || ""; Layout.preferredWidth: 40; font.pointSize: 13; font.family: Theme.fontSans }
                Text { text: modelData.ip; Layout.fillWidth: true; color: Theme.t1; font.pointSize: 12; font.family: Theme.fontMono; elide: Text.ElideRight }
                Text { text: modelData.client; Layout.preferredWidth: 150; color: Theme.t2; font.pointSize: 12; font.family: Theme.fontSans; elide: Text.ElideRight }
                Text { text: Math.round((modelData.progress || 0) * 100) + "%"; Layout.preferredWidth: 60; horizontalAlignment: Text.AlignRight; color: Theme.t2; font.pointSize: 12; font.family: Theme.fontMono }
                Text { text: modelData.downSpeed; Layout.preferredWidth: 90; horizontalAlignment: Text.AlignRight; color: Theme.accentText; font.pointSize: 12; font.family: Theme.fontMono }
                Text { text: modelData.upSpeed; Layout.preferredWidth: 90; horizontalAlignment: Text.AlignRight; color: Theme.up; font.pointSize: 12; font.family: Theme.fontMono }
            }
        }
    }
}
