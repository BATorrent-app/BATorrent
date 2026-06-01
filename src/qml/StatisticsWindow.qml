// Session + all-time statistics. Reads session.statistics() (QmlSessionBridge).
import QtQuick
import QtQuick.Layouts
import "theme"
import "widgets"

Window {
    id: win
    width: 560
    height: 480
    minimumWidth: 480
    minimumHeight: 420
    color: Theme.bg
    title: (i18n.language, i18n.t("stats_title"))

    readonly property var sess: typeof session !== "undefined" ? session : null
    property var stats: ({})
    onVisibleChanged: if (visible && sess) stats = sess.statistics()

    component KV: RowLayout {
        property string k: ""
        property string v: ""
        Layout.fillWidth: true
        spacing: 12
        Text { Layout.preferredWidth: 110; text: k; color: Theme.t3; font.pointSize: 11.5; font.weight: Font.DemiBold; font.family: Theme.fontSans }
        Text { text: v; color: Theme.t1; font.pointSize: 12; font.family: Theme.fontMono }
        Item { Layout.fillWidth: true }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 36
            color: Theme.elev
            Text { anchors.centerIn: parent; text: (i18n.language, i18n.t("stats_title")); color: Theme.t2; font.pointSize: 12.5; font.weight: Font.DemiBold; font.family: Theme.fontSans }
            Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hairSoft }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: Theme.sp6
            spacing: Theme.sp2

            Eyebrow { text: (i18n.language, i18n.t("stats_overview")); red: true }
            Text {
                text: (i18n.language, i18n.t("stats_title2"))
                color: Theme.t1
                font.pointSize: 18; font.weight: Font.Bold; font.letterSpacing: -0.3
                font.family: Theme.fontSans
                Layout.bottomMargin: Theme.sp4
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: Theme.sp6

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignTop
                    spacing: Theme.sp1
                    Text { text: (i18n.language, i18n.t("stats_alltime2")); color: Theme.t4; font.pointSize: 9.5; font.weight: Font.Bold; font.letterSpacing: 1.2; font.family: Theme.fontSans; Layout.bottomMargin: Theme.sp1 }
                    KV { k: (i18n.language, i18n.t("detail_kv_downloaded")); v: win.stats.totalDownloaded || "—" }
                    KV { k: (i18n.language, i18n.t("detail_kv_uploaded")); v: win.stats.totalUploaded || "—" }
                    KV { k: (i18n.language, i18n.t("stats_ratio")); v: win.stats.totalRatio || "—" }
                    KV { k: (i18n.language, i18n.t("search_source_torrents")); v: win.stats.torrentsAdded !== undefined ? String(win.stats.torrentsAdded) : "—" }
                    KV { k: (i18n.language, i18n.t("stats_uptime2")); v: win.stats.uptime || "—" }
                }
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignTop
                    spacing: Theme.sp1
                    Text { text: (i18n.language, i18n.t("stats_session2")); color: Theme.t4; font.pointSize: 9.5; font.weight: Font.Bold; font.letterSpacing: 1.2; font.family: Theme.fontSans; Layout.bottomMargin: Theme.sp1 }
                    KV { k: (i18n.language, i18n.t("detail_kv_downloaded")); v: win.stats.sessionDownloaded || "—" }
                    KV { k: (i18n.language, i18n.t("detail_kv_uploaded")); v: win.stats.sessionUploaded || "—" }
                    KV { k: (i18n.language, i18n.t("stats_ratio")); v: win.stats.sessionRatio || "—" }
                    KV { k: (i18n.language, i18n.t("stats_uptime2")); v: win.stats.uptime || "—" }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 56
            color: Theme.elev
            Rectangle { anchors.top: parent.top; width: parent.width; height: 1; color: Theme.hair }
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Theme.sp5
                anchors.rightMargin: 20
                Item { Layout.fillWidth: true }
                BtnFlat { primary: true; text: (i18n.language, i18n.t("release_notes_close")); onClicked: win.close() }
            }
        }
    }
}
