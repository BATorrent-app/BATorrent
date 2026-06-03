// WebUI phone pairing: one tap enables the WebUI with a generated password and
// LAN access, then shows the QR + URL + credentials. `pairing` + `settings` bridges.
import QtQuick
import QtQuick.Layouts
import "theme"
import "widgets"

BatDialog {
    id: dlg
    title: (i18n.language, i18n.t("pairing_title"))
    cardW: 460
    cardH: 620
    okText: (i18n.language, i18n.t("release_notes_close"))
    showCancel: false

    readonly property var api: typeof pairing !== "undefined" ? pairing : null
    readonly property var set: typeof settings !== "undefined" ? settings : null
    property string copyLabel: (i18n.language, i18n.t("pairing_copy"))
    property bool active: false
    property string pw: ""
    property var rows: []

    // Called on open (from the menu) and after enable/disable so the QR and
    // credentials reflect the current server state.
    function reload() {
        if (api) api.refresh()
        if (set) { active = set.pairingActive(); pw = set.webUiPassword() }
        rows = (api && active) ? api.qrRows() : []
    }
    Component.onCompleted: reload()

    ColumnLayout {
        Layout.fillWidth: true
        spacing: Theme.sp1
        Eyebrow { text: (i18n.language, i18n.t("pairing_eyebrow")); red: true }
        Text {
            text: (i18n.language, i18n.t("pairing_title"))
            color: Theme.t1
            font.pixelSize: 19; font.weight: Font.DemiBold; font.letterSpacing: -0.3
            font.family: Theme.fontSans
        }
        Text {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: (i18n.language, i18n.t(dlg.active ? "pairing_login_hint" : "pairing_setup_hint"))
            color: Theme.t3
            font.pixelSize: 12
            font.family: Theme.fontSans
        }
    }

    // ===== NOT PAIRED: one-tap enable =====
    BtnFlat {
        visible: !dlg.active
        Layout.alignment: Qt.AlignHCenter
        Layout.topMargin: Theme.sp4
        primary: true
        text: (i18n.language, i18n.t("pairing_enable"))
        onClicked: {
            if (!dlg.set) return
            dlg.pw = dlg.set.enablePairing()
            dlg.active = true
            if (dlg.api) dlg.api.refresh()
            dlg.rows = dlg.api ? dlg.api.qrRows() : []
        }
    }

    // ===== PAIRED: QR =====
    Rectangle {
        visible: dlg.active && dlg.api && dlg.api.available && dlg.rows.length > 0
        Layout.alignment: Qt.AlignHCenter
        width: 224; height: 224; radius: 8; color: "white"
        Column {
            anchors.centerIn: parent
            property int mods: dlg.rows.length
            property real cell: mods > 0 ? 192 / mods : 0
            Repeater {
                model: dlg.rows
                delegate: Row {
                    required property var modelData
                    Repeater {
                        model: modelData.length
                        delegate: Rectangle {
                            required property int index
                            width: parent.parent.cell
                            height: parent.parent.cell
                            color: modelData.charAt(index) === "1" ? "black" : "white"
                        }
                    }
                }
            }
        }
    }

    // URL box
    Rectangle {
        visible: dlg.active && dlg.api && dlg.api.available
        Layout.fillWidth: true
        implicitHeight: 46; radius: 8
        color: Theme.panel; border.color: Theme.hair; border.width: 1
        Text {
            anchors.centerIn: parent
            text: dlg.api ? dlg.api.url : ""
            color: Theme.t1
            font.pixelSize: 15; font.weight: Font.Bold
            font.family: Theme.fontMono
        }
    }

    // Credentials (user + generated password) to type on the phone
    Rectangle {
        visible: dlg.active
        Layout.fillWidth: true
        implicitHeight: 54; radius: 8
        color: Theme.panel; border.color: Theme.hair; border.width: 1
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: Theme.sp3; anchors.rightMargin: Theme.sp3
            spacing: Theme.sp3
            ColumnLayout {
                spacing: 0
                Text { text: (i18n.language, i18n.t("pairing_user")); color: Theme.t3; font.pixelSize: 10; font.family: Theme.fontSans }
                Text { text: dlg.set ? dlg.set.webUiUser() : "admin"; color: Theme.t1; font.pixelSize: 14; font.weight: Font.Bold; font.family: Theme.fontMono }
            }
            Rectangle { Layout.preferredWidth: 1; Layout.fillHeight: true; Layout.topMargin: 10; Layout.bottomMargin: 10; color: Theme.hair }
            ColumnLayout {
                spacing: 0
                Text { text: (i18n.language, i18n.t("pairing_password")); color: Theme.t3; font.pixelSize: 10; font.family: Theme.fontSans }
                Text { text: dlg.pw; color: Theme.t1; font.pixelSize: 14; font.weight: Font.Bold; font.family: Theme.fontMono }
            }
            Item { Layout.fillWidth: true }
        }
    }

    // status + actions
    RowLayout {
        visible: dlg.active
        Layout.fillWidth: true
        spacing: Theme.sp2
        Text {
            text: "● " + (i18n.language, i18n.t("pairing_status_lan"))
            color: Theme.grn
            font.pixelSize: 12; font.family: Theme.fontSans
        }
        Item { Layout.fillWidth: true }
        BtnFlat { text: dlg.copyLabel; onClicked: { if (dlg.api) { dlg.api.copyUrl(); dlg.copyLabel = (i18n.language, i18n.t("inspector_copied")) } } }
        BtnFlat { text: (i18n.language, i18n.t("pairing_disable")); onClicked: { if (dlg.set) dlg.set.disablePairing(); dlg.active = false } }
    }

    // no network interface found at all
    Text {
        visible: dlg.active && (!dlg.api || !dlg.api.available)
        Layout.fillWidth: true
        wrapMode: Text.WordWrap
        text: (i18n.language, i18n.t("pairing_no_iface"))
        color: Theme.accentText
        font.pixelSize: 12; font.family: Theme.fontSans
    }
}
