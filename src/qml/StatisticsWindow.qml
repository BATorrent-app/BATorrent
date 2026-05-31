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
    title: "Estatísticas"

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
            Text { anchors.centerIn: parent; text: "Estatísticas"; color: Theme.t2; font.pointSize: 12.5; font.weight: Font.DemiBold; font.family: Theme.fontSans }
            Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hairSoft }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: Theme.sp6
            spacing: Theme.sp2

            Eyebrow { text: "VISÃO GERAL"; red: true }
            Text {
                text: "Estatísticas de transferência"
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
                    Text { text: "TODO O PERÍODO"; color: Theme.t4; font.pointSize: 9.5; font.weight: Font.Bold; font.letterSpacing: 1.2; font.family: Theme.fontSans; Layout.bottomMargin: Theme.sp1 }
                    KV { k: "Baixado"; v: win.stats.totalDownloaded || "—" }
                    KV { k: "Enviado"; v: win.stats.totalUploaded || "—" }
                    KV { k: "Proporção"; v: win.stats.totalRatio || "—" }
                    KV { k: "Torrents"; v: win.stats.torrentsAdded !== undefined ? String(win.stats.torrentsAdded) : "—" }
                    KV { k: "Tempo ativo"; v: win.stats.uptime || "—" }
                }
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignTop
                    spacing: Theme.sp1
                    Text { text: "ESTA SESSÃO"; color: Theme.t4; font.pointSize: 9.5; font.weight: Font.Bold; font.letterSpacing: 1.2; font.family: Theme.fontSans; Layout.bottomMargin: Theme.sp1 }
                    KV { k: "Baixado"; v: win.stats.sessionDownloaded || "—" }
                    KV { k: "Enviado"; v: win.stats.sessionUploaded || "—" }
                    KV { k: "Proporção"; v: win.stats.sessionRatio || "—" }
                    KV { k: "Tempo ativo"; v: win.stats.uptime || "—" }
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
                BtnFlat { primary: true; text: "Fechar"; onClicked: win.close() }
            }
        }
    }
}
