// Files pane for the detail panel. Data: session.selectedFiles
// rows: { path, size, progress, priority }
import QtQuick
import QtQuick.Layouts
import "../theme"

ColumnLayout {
    id: pane
    property var files: []
    spacing: 0

    function priName(p) {
        switch (p) { case 0: return (i18n.language, i18n.t("priority_skip")); case 1: return (i18n.language, i18n.t("priority_low")); case 7: return (i18n.language, i18n.t("priority_high")) }
        return (i18n.language, i18n.t("priority_normal"))
    }

    Rectangle {
        Layout.fillWidth: true
        Layout.preferredHeight: 30
        color: "transparent"
        Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hair }
        RowLayout {
            anchors.fill: parent; anchors.leftMargin: Theme.sp5; anchors.rightMargin: Theme.sp5
            Text { text: (i18n.language, i18n.t("search_col_name2")); Layout.fillWidth: true; color: Theme.t4; font.pointSize: 10; font.weight: Font.DemiBold; font.letterSpacing: 0.6; font.family: Theme.fontSans }
            Text { text: (i18n.language, i18n.t("detailfiles_progress")); Layout.preferredWidth: 128; color: Theme.t4; font.pointSize: 10; font.weight: Font.DemiBold; font.letterSpacing: 0.6; font.family: Theme.fontSans }
            Text { text: (i18n.language, i18n.t("search_col_size2")); Layout.preferredWidth: 74; horizontalAlignment: Text.AlignRight; color: Theme.t4; font.pointSize: 10; font.weight: Font.DemiBold; font.letterSpacing: 0.6; font.family: Theme.fontSans }
            Text { text: (i18n.language, i18n.t("detailfiles_priority")); Layout.preferredWidth: 70; color: Theme.t4; font.pointSize: 10; font.weight: Font.DemiBold; font.letterSpacing: 0.6; font.family: Theme.fontSans }
        }
    }
    Text {
        visible: pane.files.length === 0
        Layout.alignment: Qt.AlignHCenter; Layout.topMargin: 18
        text: (i18n.language, i18n.t("detailfiles_empty")); color: Theme.t4; font.pointSize: 11; font.family: Theme.fontSans
    }
    ListView {
        Layout.fillWidth: true; Layout.fillHeight: true; clip: true; model: pane.files
        visible: pane.files.length > 0
        boundsBehavior: Flickable.StopAtBounds
        delegate: Rectangle {
            width: ListView.view.width; height: 34; color: "transparent"
            Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hairSoft }
            RowLayout {
                anchors.fill: parent; anchors.leftMargin: Theme.sp5; anchors.rightMargin: Theme.sp5; spacing: 12
                Text { text: modelData.path; Layout.fillWidth: true; color: Theme.t1; font.pointSize: 12; font.family: Theme.fontSans; elide: Text.ElideMiddle }
                RowLayout {
                    Layout.preferredWidth: 128; spacing: 9
                    Rectangle {
                        Layout.fillWidth: true; height: 5; radius: 3; color: Theme.track; clip: true
                        Rectangle { anchors.left: parent.left; height: parent.height; width: parent.width * (modelData.progress || 0); radius: 3; color: Theme.accent }
                    }
                    Text { text: Math.round((modelData.progress || 0) * 100) + "%"; Layout.preferredWidth: 34; horizontalAlignment: Text.AlignRight; color: Theme.t2; font.pointSize: 11; font.family: Theme.fontMono }
                }
                Text { text: modelData.size; Layout.preferredWidth: 74; horizontalAlignment: Text.AlignRight; color: Theme.t3; font.pointSize: 11; font.family: Theme.fontMono }
                Text {
                    text: pane.priName(modelData.priority); Layout.preferredWidth: 70
                    color: modelData.priority === 0 ? Theme.t4 : modelData.priority === 7 ? Theme.accentText : Theme.t2
                    font.pointSize: 11; font.family: Theme.fontSans
                }
            }
        }
    }
}
