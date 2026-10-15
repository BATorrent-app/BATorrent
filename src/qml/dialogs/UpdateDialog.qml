// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Updates, driven by the `updater` bridge (null in Store builds). Up to date:
// the answer, when it was checked, and the two settings that decide the next
// check. Available: the version, its size, what is in it, and Install.
import QtQuick
import QtQuick.Layouts
import "../theme"
import "../widgets"
import "../widgets/TimeAgo.js" as TimeAgo

ModalShell {
    id: dlg
    // the handoff's own width for this one (UTILITY-HANDOFF: modal 440px)
    cardWidth: 440
    cardHeight: col.implicitHeight
    // mid-install there is nothing to leave to
    dismissable: !downloading
    function open() { refresh(); opened = true }

    signal whatsNewRequested()
    signal releaseNotesRequested()

    readonly property bool hasUpdater: typeof updater !== "undefined" && updater !== null
    property string version: ""
    property string url: ""
    property string assetName: ""
    property string size: ""
    property var highlights: []
    property int pct: 0
    property bool downloading: false
    property bool restarting: false
    property string error: ""
    property real checkedAt: 0
    property bool autoCheck: true
    property int track: 0
    readonly property bool available: version.length > 0
    readonly property string appVer: typeof themeBridge !== "undefined" ? themeBridge.appVersion : ""

    function refresh() {
        if (typeof settings !== "undefined") {
            var a = settings.get("autoCheckUpdates")
            autoCheck = a === undefined || a === null || a === "" ? true : (a === true || a === "true")
            track = Number(settings.get("updateTrack")) === 1 ? 1 : 0
        }
        checkedAt = hasUpdater ? updater.lastChecked() : 0
    }
    function showAvailable(v, u, a) {
        version = v; url = u; assetName = a
        size = hasUpdater ? updater.releaseSize() : ""
        highlights = hasUpdater ? updater.releaseHighlights(3) : []
        downloading = false; restarting = false; pct = 0; error = ""
        open()
    }
    function showNone() {
        version = ""; downloading = false; error = ""
        open()
    }
    function showError(msg) {
        downloading = false
        error = msg
        open()
    }

    Connections {
        target: dlg.hasUpdater ? updater : null
        ignoreUnknownSignals: true
        function onProgress(percent) { dlg.pct = percent }
        function onReady() { dlg.restarting = true }
        // a failed silent (startup) check stays quiet; failures mid-download or
        // after a manual check still surface here
        function onFailed(message, silent) { if (!silent || dlg.opened) dlg.showError(message) }
    }

    function ago(secs) {
        if (!secs || secs <= 0) return ""
        return Date.now() / 1000 - secs < 90 ? i18n.t("upd_checked_now") : i18n.t("upd_checked").arg(TimeAgo.ago(secs, i18n.t))
    }


    ColumnLayout {
        id: col
        width: parent.width
        spacing: 0

        // title
        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 20; Layout.rightMargin: 12
            Layout.topMargin: 14; Layout.bottomMargin: 4
            Text { Layout.fillWidth: true; text: (i18n.language, i18n.t("update_title2")); color: Theme.t1; font.pixelSize: 17; font.weight: Font.DemiBold; font.family: Theme.fontSans }
            CloseButton {
                onClicked: dlg.dismiss()
            }
        }

        // ======== up to date ========
        ColumnLayout {
            visible: !dlg.available
            Layout.fillWidth: true
            Layout.topMargin: 14
            spacing: 8
            Rectangle {
                Layout.alignment: Qt.AlignHCenter
                width: 40; height: 40; radius: 20
                color: dlg.error !== "" ? Qt.rgba(Theme.warn.r, Theme.warn.g, Theme.warn.b, 0.14) : Qt.rgba(Theme.grn.r, Theme.grn.g, Theme.grn.b, 0.16)
                Text {
                    anchors.centerIn: parent
                    text: dlg.error !== "" ? "!" : "✓"
                    color: dlg.error !== "" ? Theme.warn : Theme.grn
                    font.pixelSize: 20; font.weight: Font.Bold; font.family: Theme.fontSans
                }
            }
            Text {
                Layout.alignment: Qt.AlignHCenter
                text: (i18n.language, i18n.t(dlg.error !== "" ? "upd_failed" : "upd_uptodate"))
                color: Theme.t1; font.pixelSize: 18; font.weight: Font.DemiBold; font.family: Theme.fontSans
            }
            Text {
                Layout.alignment: Qt.AlignHCenter
                Layout.maximumWidth: 380
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
                color: dlg.error !== "" ? Theme.warn : Theme.t3
                font.pixelSize: 13; font.family: Theme.fontSans; font.features: Theme.tnum
                text: {
                    i18n.language
                    if (dlg.error !== "") return dlg.error
                    var bits = [dlg.appVer, i18n.t(dlg.track === 1 ? "set_track_beta" : "set_track_stable")]
                    var c = dlg.ago(dlg.checkedAt)
                    if (c !== "") bits.push(c)
                    return bits.join(" · ")
                }
            }
            Text {
                Layout.alignment: Qt.AlignHCenter
                visible: dlg.error === ""
                text: (i18n.language, i18n.t("upd_whats_new")).arg(dlg.appVer.split(".").slice(0, 2).join("."))
                color: wnMa.containsMouse ? Theme.t1 : Theme.t2
                font.pixelSize: 13; font.underline: true; font.family: Theme.fontSans
                MouseArea { id: wnMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: { dlg.close(); dlg.whatsNewRequested() } }
            }
            // the two settings that decide the next check
            Item { Layout.preferredHeight: 10 }
            Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: Theme.hairSoft }
            RowLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 20; Layout.rightMargin: 20
                Layout.preferredHeight: 44
                Text { Layout.fillWidth: true; text: (i18n.language, i18n.t("upd_auto")); color: Theme.t1; font.pixelSize: 14; font.family: Theme.fontSans }
                TToggle {
                    on: dlg.autoCheck
                    onToggled: function (v) { dlg.autoCheck = v; if (typeof settings !== "undefined") settings.set("autoCheckUpdates", v) }
                }
            }
            Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: Theme.hairSoft }
            RowLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 20; Layout.rightMargin: 20
                Layout.preferredHeight: 48
                Text { Layout.fillWidth: true; text: (i18n.language, i18n.t("upd_channel")); color: Theme.t1; font.pixelSize: 14; font.family: Theme.fontSans }
                Segmented {
                    options: (i18n.language, [{ value: 0, label: i18n.t("set_track_stable") }, { value: 1, label: i18n.t("set_track_beta") }])
                    current: dlg.track
                    onPicked: function (v) { dlg.track = v; if (typeof settings !== "undefined") settings.set("updateTrack", v) }
                }
            }
        }

        // ======== available ========
        ColumnLayout {
            visible: dlg.available
            Layout.fillWidth: true
            Layout.leftMargin: 20; Layout.rightMargin: 20
            Layout.topMargin: 4; Layout.bottomMargin: 16
            spacing: 14
            Row {
                spacing: 10
                Text { text: dlg.version.split(".").slice(0, 2).join("."); color: Theme.t1; font.family: "New Rocker"; font.pixelSize: 44 }
                Text {
                    anchors.baseline: parent.children[0].baseline
                    text: (i18n.language, dlg.size !== "" ? i18n.t("upd_ready_size").arg(dlg.size) : i18n.t("upd_ready"))
                    color: Theme.t3; font.pixelSize: 13; font.family: Theme.fontSans; font.features: Theme.tnum
                }
            }
            Repeater {
                model: dlg.highlights
                delegate: RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    Text { Layout.alignment: Qt.AlignTop; text: "›"; color: Theme.t3; font.pixelSize: 14; font.family: Theme.fontSans }
                    Text { Layout.fillWidth: true; text: modelData; color: Theme.t1; font.pixelSize: 13; font.family: Theme.fontSans; wrapMode: Text.WordWrap; lineHeight: 1.3 }
                }
            }
            // installing
            Rectangle {
                visible: dlg.downloading
                Layout.fillWidth: true
                Layout.preferredHeight: 6
                radius: 3
                color: Qt.rgba(Theme.t1.r, Theme.t1.g, Theme.t1.b, 0.1)
                Rectangle {
                    width: parent.width * dlg.pct / 100
                    height: parent.height; radius: 3
                    color: Theme.down
                    Behavior on width { NumberAnimation { duration: Theme.durSlow; easing.type: Theme.easeOut } }
                }
            }
            Text {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                color: dlg.error !== "" ? Theme.warn : Theme.t3
                font.pixelSize: 13; font.family: Theme.fontSans
                text: (i18n.language, dlg.error !== "" ? dlg.error
                      : dlg.restarting ? i18n.t("update_restarting")
                      : i18n.t("upd_keeps_going"))
            }
        }

        // ======== footer ========
        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 60
            Rectangle { width: parent.width; height: 1; color: Theme.hairSoft }
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 20; anchors.rightMargin: 14
                spacing: 8
                Text {
                    visible: !dlg.available
                    text: (i18n.language, i18n.t("update_manual")) + " ↗"
                    color: dmMa.containsMouse ? Theme.t1 : Theme.t3
                    font.pixelSize: 13; font.family: Theme.fontSans
                    MouseArea {
                        id: dmMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                        onClicked: Qt.openUrlExternally(dlg.url.length > 0 ? dlg.url : "https://github.com/BATorrent-app/BATorrent/releases/latest")
                    }
                }
                BtnFlat {
                    visible: dlg.available && !dlg.downloading
                    text: (i18n.language, i18n.t("upd_full_notes"))
                    onClicked: { dlg.close(); dlg.releaseNotesRequested() }
                }
                Item { Layout.fillWidth: true }
                BtnFlat {
                    visible: !dlg.available
                    text: (i18n.language, i18n.t("release_notes_close"))
                    onClicked: dlg.close()
                }
                BtnFlat {
                    visible: dlg.available && !dlg.downloading
                    text: (i18n.language, i18n.t("upd_later"))
                    onClicked: dlg.close()
                }
                BtnFlat {
                    visible: dlg.available
                    primary: true
                    opacity: dlg.downloading ? 0.6 : 1
                    text: (i18n.language, dlg.downloading ? i18n.t("update_downloading_pct").arg(dlg.pct) : i18n.t("upd_install"))
                    onClicked: {
                        if (!dlg.hasUpdater || dlg.downloading) return
                        dlg.downloading = true
                        dlg.error = ""
                        updater.downloadAndInstall(dlg.url, dlg.assetName)
                    }
                }
            }
        }
    }
}
