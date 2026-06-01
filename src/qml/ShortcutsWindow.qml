// Keyboard shortcuts reference. Static data (ported from shortcutsdialog.cpp), no bridge.
import QtQuick
import QtQuick.Layouts
import "theme"
import "widgets"

Window {
    id: win
    width: 560
    height: 620
    minimumWidth: 520
    minimumHeight: 480
    color: Theme.bg
    title: (i18n.language, i18n.t("shortcuts_title2"))

    readonly property var groups: [
        { title: (i18n.language, i18n.t("shortcuts_app")), rows: [
            { k: "Ctrl+,", d: (i18n.language, i18n.t("sc_open_settings")) },
            { k: "Ctrl+Q", d: (i18n.language, i18n.t("sc_quit")) },
            { k: "Ctrl+W", d: (i18n.language, i18n.t("sc_close_window")) },
            { k: "F1", d: (i18n.language, i18n.t("sc_show_shortcuts")) },
            { k: "Ctrl+Shift+L", d: (i18n.language, i18n.t("sc_open_logs")) }
        ] },
        { title: (i18n.language, i18n.t("search_source_torrents")), rows: [
            { k: "Ctrl+O", d: (i18n.language, i18n.t("sc_add_file")) },
            { k: "Ctrl+U", d: (i18n.language, i18n.t("sc_add_magnet")) },
            { k: "Ctrl+N", d: (i18n.language, i18n.t("sc_create")) },
            { k: "Delete", d: (i18n.language, i18n.t("sc_remove")) },
            { k: "Shift+Delete", d: (i18n.language, i18n.t("sc_remove_data")) },
            { k: "Ctrl+A", d: (i18n.language, i18n.t("sc_select_all")) },
            { k: (i18n.language, i18n.t("key_space")), d: (i18n.language, i18n.t("sc_pause_resume")) },
            { k: "Ctrl+R", d: (i18n.language, i18n.t("sc_recheck")) },
            { k: "Ctrl+F", d: (i18n.language, i18n.t("sc_focus_search")) }
        ] },
        { title: (i18n.language, i18n.t("shortcuts_queue")), rows: [
            { k: "Ctrl+↑", d: (i18n.language, i18n.t("sc_queue_up")) },
            { k: "Ctrl+↓", d: (i18n.language, i18n.t("sc_queue_down")) },
            { k: "Ctrl+Home", d: (i18n.language, i18n.t("sc_queue_top")) },
            { k: "Ctrl+End", d: (i18n.language, i18n.t("sc_queue_bottom")) }
        ] },
        { title: (i18n.language, i18n.t("shortcuts_view")), rows: [
            { k: "Ctrl+1", d: (i18n.language, i18n.t("sc_show_all")) },
            { k: "Ctrl+2", d: (i18n.language, i18n.t("sc_show_downloading")) },
            { k: "Ctrl+3", d: (i18n.language, i18n.t("sc_show_seeding")) },
            { k: "Ctrl+4", d: (i18n.language, i18n.t("sc_show_completed")) },
            { k: "Ctrl+5", d: (i18n.language, i18n.t("sc_show_active")) },
            { k: "Ctrl+6", d: (i18n.language, i18n.t("sc_show_inactive")) },
            { k: "F5", d: (i18n.language, i18n.t("sc_refresh_view")) },
            { k: "Ctrl+Shift+T", d: (i18n.language, i18n.t("sc_toggle_theme")) }
        ] }
    ]

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 36
            color: Theme.elev
            Text { anchors.centerIn: parent; text: (i18n.language, i18n.t("shortcuts_title2")); color: Theme.t2; font.pointSize: 12.5; font.weight: Font.DemiBold; font.family: Theme.fontSans }
            Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hairSoft }
        }

        Flickable {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            contentWidth: width
            contentHeight: col.implicitHeight + 2 * Theme.sp5
            boundsBehavior: Flickable.StopAtBounds

            ColumnLayout {
                id: col
                x: Theme.sp5
                y: Theme.sp5
                width: parent.width - 2 * Theme.sp5
                spacing: Theme.sp5

                Repeater {
                    model: win.groups
                    delegate: ColumnLayout {
                        required property var modelData
                        Layout.fillWidth: true
                        spacing: Theme.sp2

                        Text {
                            text: modelData.title.toUpperCase()
                            color: Theme.t4
                            font.pointSize: 10; font.weight: Font.Bold; font.letterSpacing: 0.8
                            font.family: Theme.fontSans
                        }
                        Repeater {
                            model: modelData.rows
                            delegate: RowLayout {
                                required property var modelData
                                Layout.fillWidth: true
                                spacing: Theme.sp4
                                Rectangle {
                                    Layout.preferredWidth: 140
                                    implicitHeight: kc.implicitHeight + 8
                                    radius: 6
                                    color: Theme.field
                                    border.color: Theme.hair
                                    border.width: 1
                                    Text {
                                        id: kc
                                        anchors.centerIn: parent
                                        text: modelData.k
                                        color: Theme.t2
                                        font.pointSize: 11.5
                                        font.family: Theme.fontMono
                                    }
                                }
                                Text {
                                    Layout.fillWidth: true
                                    text: modelData.d
                                    color: Theme.t2
                                    font.pointSize: 12
                                    font.family: Theme.fontSans
                                    wrapMode: Text.WordWrap
                                }
                            }
                        }
                    }
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
