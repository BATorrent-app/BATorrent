// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Source: BATorrent Settings.html + batorrent-settings.css (+ settings-data.js)
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import QtQuick.Dialogs
import "theme"
import "widgets"

Rectangle {
    id: win
    color: Theme.bg

    property int sec: 0
    property int reloadTick: 0       // bumped by a reset to force the rows to re-read their values
    property bool isCurrent: false   // true when this is the active nav page (gates the Esc/⌘W shortcuts)
    signal closed()                  // wired by Main to jump back to Downloads

    // a stored bool pref, falling back to the field's `on` default when unset —
    // so a default-on toggle reads ON on a fresh profile (matches runtime behavior).
    function boolPref(field) {
        if (typeof settings === "undefined" || field.key === undefined) return field.on === true
        var v = settings.get(field.key)
        return (v === undefined || v === null || v === "") ? (field.on === true) : (v === true)
    }

    // active custom-theme profile map; re-read on any profile data change
    readonly property var ap: (typeof themeBridge !== "undefined" && themeBridge.customProfiles.length > 0)
                              ? themeBridge.customProfiles[themeBridge.activeProfile] : ({})

    // Instant-apply settings: Esc / ⌘W just leave the page (nothing to confirm).
    Shortcut { enabled: win.isCurrent; sequences: [StandardKey.Cancel]; onActivated: win.closed() }
    Shortcut { enabled: win.isCurrent; sequences: [StandardKey.Close];  onActivated: win.closed() }

    SettingsSchema { id: schema }

    // group consecutive fields into { label, rows[] } blocks (a "group" field starts a new card).
    property string query: ""

    // Called from the command palette to jump straight to an option (fills the
    // search box, which drives the global filter below).
    function searchFor(text) { searchFld.text = text }

    function fieldMatches(f, q) {
        if (!f || f.type === "group" || f.type === "warning") return false
        var hay = (String(f.label || "") + " " + String(f.note || "") + " " + String(f.key || "")).toLowerCase()
        return hay.indexOf(q) >= 0
    }
    // Global search: when the box has text, gather matching fields from EVERY
    // section (not just the open one) — otherwise a search finds nothing useful.
    function searchBlocks(q) {
        var matched = []
        for (var s = 0; s < schema.sections.length; s++) {
            var fields = schema.sections[s]
            for (var i = 0; i < fields.length; i++)
                if (fieldMatches(fields[i], q)) matched.push(fields[i])
        }
        return buildBlocks(matched)
    }

    // ---- reset to defaults ----
    // The schema is the single source of truth for a field's default: a toggle's
    // `on`, a select/segmented/number's `value`, else empty. set() applies live,
    // exactly like a user edit — so a reset needs no separate defaults table.
    function defaultForField(f) {
        if (!f || f.key === undefined) return undefined      // no key ⇒ not resettable here
        if (f.type === "toggle") return f.on === true
        if (f.value !== undefined) return f.value
        return ""                                            // text / password / path
    }
    function resetFields(fields) {
        if (typeof settings === "undefined") return
        for (var i = 0; i < fields.length; i++) {
            var d = win.defaultForField(fields[i])
            if (d !== undefined) settings.set(fields[i].key, d)
        }
        win.reloadTick++                                     // rebuild the rows so they show the defaults
    }
    function resetSection() { win.resetFields(schema.sections[win.sec]) }
    function resetAll() {
        for (var s = 0; s < schema.sections.length; s++) win.resetFields(schema.sections[s])
    }

    // Hidden rows are dropped, and a group with no remaining rows is omitted entirely
    // (so we never render an orphan header over an empty card).
    function buildBlocks(fields) {
        var blocks = []
        var cur = null
        for (var i = 0; i < fields.length; i++) {
            var f = fields[i]
            if (f.type === "group") {
                cur = { label: f.label, rows: [] }
            } else {
                if (f.hidden === true) continue
                if (!cur) { cur = { label: "", rows: [] } }
                if (cur.rows.length === 0 && blocks.indexOf(cur) < 0) blocks.push(cur)
                cur.rows.push(f)
            }
        }
        return blocks
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // ===== titlebar =====
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 36
            color: Theme.elev
            Text { anchors.centerIn: parent; text: (i18n.language, i18n.t("settings_heading")); color: Theme.t2; font.pixelSize: 13; font.weight: Font.DemiBold; font.family: Theme.fontSans }
            Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hairSoft }
        }

        // ===== sbody =====
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            // ----- sidebar (.sside 226) -----
            Rectangle {
                Layout.preferredWidth: 226
                Layout.fillHeight: true
                color: Theme.panel
                Rectangle { anchors.right: parent.right; width: 1; height: parent.height; color: Theme.hair }

                ColumnLayout {
                    anchors.fill: parent
                    anchors.topMargin: Theme.sp3
                    anchors.bottomMargin: Theme.sp3
                    anchors.leftMargin: Theme.sp2
                    anchors.rightMargin: Theme.sp2
                    spacing: 0

                    // .ssearch
                    TFld {
                        id: searchFld
                        Layout.fillWidth: true
                        Layout.preferredHeight: 32
                        Layout.bottomMargin: Theme.sp3
                        icon: "qrc:/icons/search.svg"
                        clearable: true
                        placeholder: (i18n.language, i18n.t("set_search_config_ph"))
                        onTextChanged: win.query = text.trim().toLowerCase()
                    }

                    // .snav
                    ListView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        spacing: 1
                        model: schema.navs
                        boundsBehavior: Flickable.StopAtBounds

                        delegate: Rectangle {
                            width: ListView.view.width
                            height: 34
                            radius: 7
                            color: win.sec === index ? Theme.sel : (nrMa.containsMouse ? Theme.hover : "transparent")

                            // .nrow.on::before left bar
                            Rectangle {
                                visible: win.sec === index
                                anchors.left: parent.left
                                anchors.leftMargin: -2
                                anchors.verticalCenter: parent.verticalCenter
                                width: 2
                                height: 20
                                radius: 1
                                color: Theme.accent
                            }

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 10
                                anchors.rightMargin: 10
                                spacing: Theme.sp3
                                IconImg {
                                    Layout.alignment: Qt.AlignVCenter
                                    src: modelData.icon
                                    tint: win.sec === index ? Theme.accent : Theme.t3
                                    s: 16
                                }
                                Text {
                                    Layout.fillWidth: true
                                    text: modelData.nav
                                    color: win.sec === index ? Theme.t1 : (nrMa.containsMouse ? Theme.t1 : Theme.t2)
                                    font.pixelSize: 13
                                    font.family: Theme.fontSans
                                    elide: Text.ElideRight
                                }
                            }
                            MouseArea { id: nrMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: win.sec = index }
                        }
                    }
                }
            }

            // ----- content (.scontent) -----
            Flickable {
                id: contentScroll
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                contentWidth: width
                contentHeight: contentCol.implicitHeight + 2 * Theme.sp5
                boundsBehavior: Flickable.StopAtBounds
                // Tester feedback: the default glide over-ran the target setting.
                // Decelerate harder and cap the top speed so a wheel notch lands
                // precisely, while staying smooth (no per-notch jump).
                flickDeceleration: 6500
                maximumFlickVelocity: 2500
                WheelScroller { flick: contentScroll }

                ColumnLayout {
                    id: contentCol
                    x: Theme.sp5
                    y: Theme.sp5
                    width: contentScroll.width - 2 * Theme.sp5
                    spacing: 0

                    // .shead
                    Eyebrow { text: schema.heads[win.sec].eyebrow }
                    RowLayout {
                        Layout.fillWidth: true
                        Layout.topMargin: 6
                        spacing: Theme.sp3
                        Text {
                            Layout.fillWidth: true
                            text: schema.heads[win.sec].h
                            color: Theme.t1
                            font.pixelSize: 19
                            font.weight: Font.DemiBold
                            font.letterSpacing: -0.3
                            font.family: Theme.fontSans
                        }
                        // reset just this section; hidden while searching (the
                        // filtered view spans sections, so it'd be ambiguous)
                        BtnFlat {
                            sm: true
                            visible: win.query.length === 0
                            icon: "qrc:/icons/replay.svg"
                            text: (i18n.language, i18n.t("set_reset_section"))
                            onClicked: { confirmReset.scope = "section"; confirmReset.open() }
                        }
                    }
                    Text {
                        Layout.topMargin: 8
                        Layout.maximumWidth: 540
                        Layout.bottomMargin: Theme.sp5
                        wrapMode: Text.WordWrap
                        text: schema.heads[win.sec].sub
                        color: Theme.t3
                        font.pixelSize: 12
                        font.family: Theme.fontSans
                        lineHeight: 1.5
                    }

                    // blocks (glabel + card)
                    Repeater {
                        model: win.query.length > 0 ? win.searchBlocks(win.query) : (win.reloadTick, win.buildBlocks(schema.sections[win.sec]))
                        delegate: ColumnLayout {
                            required property var modelData
                            required property int index
                            Layout.fillWidth: true
                            spacing: 0

                            Text {
                                visible: modelData.label !== ""
                                Layout.topMargin: index === 0 ? 0 : Theme.sp5
                                Layout.bottomMargin: Theme.sp2
                                Layout.leftMargin: 2
                                text: modelData.label
                                color: Theme.t4
                                font.pixelSize: 10
                                font.weight: Font.Bold
                                font.letterSpacing: 0.8
                                font.family: Theme.fontSans
                                font.capitalization: Font.AllUppercase
                            }

                            // .card
                            Rectangle {
                                Layout.fillWidth: true
                                radius: 11
                                color: Theme.panel
                                border.color: Theme.hair
                                border.width: 1
                                implicitHeight: rowsCol.implicitHeight

                                ColumnLayout {
                                    id: rowsCol
                                    anchors.left: parent.left
                                    anchors.right: parent.right
                                    anchors.leftMargin: Theme.sp4
                                    anchors.rightMargin: Theme.sp4
                                    spacing: 0

                                    Repeater {
                                        model: modelData.rows
                                        delegate: SettingsRow {
                                            required property var modelData
                                            required property int index
                                            Layout.fillWidth: true
                                            sw: win
                                            field: modelData
                                            isLast: index === (rowsCol.parent.parent.modelData ? 0 : 0)  // border handled inside
                                            showDivider: true
                                            onRenameProfileRequested: { renameDlg.idx = themeBridge.activeProfile; renameField.text = themeBridge.customProfiles[themeBridge.activeProfile].name; renameDlg.open() }
                                            onColorPickRequested: function (role, cur) { colorDlg.targetRole = role; colorDlg.selectedColor = cur; colorDlg.open() }
                                            onBgImageRequested: bgImageDlg.open()
                                        }
                                    }
                                }
                            }
                        }
                    }

                    Item { Layout.preferredHeight: Theme.sp5 }
                }
            }
        }

        // ===== footer (.sfoot) =====
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 56
            color: Theme.elev
            Rectangle { anchors.top: parent.top; width: parent.width; height: 1; color: Theme.hair }
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Theme.sp5
                anchors.rightMargin: 20
                spacing: 8
                // a change auto-saves; flash "Saved" the moment it does so the
                // user never wonders whether Close discards their edits
                IconImg {
                    src: "qrc:/icons/check.svg"; s: 13
                    tint: Theme.grn
                    opacity: savedFlash.running ? 1 : 0.55
                }
                Text {
                    id: savedText
                    text: savedFlash.running ? (i18n.language, i18n.t("set_saved_now"))
                                             : (i18n.language, i18n.t("set_changes_instant"))
                    color: savedFlash.running ? Theme.grn : Theme.t4
                    font.pixelSize: 11; font.family: Theme.fontSans
                    Behavior on color { ColorAnimation { duration: 200 } }
                }
                Timer { id: savedFlash; interval: 1400 }
                Connections {
                    target: typeof settings !== "undefined" ? settings : null
                    function onChanged() { savedFlash.restart() }
                }
                Item { Layout.fillWidth: true }
                BtnFlat {
                    text: (i18n.language, i18n.t("set_reset_all"))
                    icon: "qrc:/icons/replay.svg"
                    onClicked: { confirmReset.scope = "all"; confirmReset.open() }
                }
                // "Done" (not "Close"): nothing is discarded — every edit is
                // already saved, so the button just dismisses the window
                BtnFlat { primary: true; text: (i18n.language, i18n.t("btn_done")); onClicked: win.closed() }
            }
        }
    }


    // custom-theme dialogs
    ColorDialog {
        id: colorDlg
        property string targetRole: ""
        // QML color.toString() is "#AARRGGBB" (alpha first); the last 6 hex
        // digits are RRGGBB — slice(0,7) would wrongly keep '#'+alpha+RR.
        onAccepted: if (typeof themeBridge !== "undefined" && targetRole !== "")
                        themeBridge.setProfileColor(themeBridge.activeProfile, targetRole,
                                                    "#" + selectedColor.toString().slice(-6))
    }
    FileDialog {
        id: bgImageDlg
        title: (i18n.language, i18n.t("set_custom_bgimage"))
        nameFilters: ["Images (*.png *.jpg *.jpeg *.webp *.bmp)"]
        // Store a decoded plain filesystem path (selectedFile is a percent-
        // encoded URL); Theme.bgImageSource re-encodes it for Image.source.
        onAccepted: if (typeof themeBridge !== "undefined") {
            var u = selectedFile.toString().replace(/^file:\/\/\/?/, Qt.platform.os === "windows" ? "" : "/")
            themeBridge.setProfileImage(themeBridge.activeProfile, decodeURIComponent(u))
        }
    }
    // action buttons (default-app / pair phone)
    function runButtonAction(a) {
        if (a === "default") {
            var ok = (typeof settings !== "undefined") && settings.setAsDefaultApp()
            infoDlg.title = (i18n.language, i18n.t("set_default_app"))
            infoDlg.message = ok ? (i18n.language, i18n.t("settings_default_success"))
                                 : (i18n.language, i18n.t("settings_default_failed"))
            infoDlg.open()
        } else if (a === "pair") {
            pairDlg.open()
        } else if (a === "telegram") {
            if (typeof settings !== "undefined") settings.testTelegram()
        } else if (a === "defender") {
            var dok = (typeof settings !== "undefined") && settings.excludeFromDefender()
            infoDlg.title = "Windows Defender"
            infoDlg.message = dok ? i18n.t("defender_exclude_ok").replace(/:?\s*%1\s*$/, "")
                                  : (i18n.language, i18n.t("defender_exclude_fail"))
            infoDlg.open()
        } else if (a === "proxyMullvad") {
            if (typeof settings !== "undefined") settings.applyProxyPreset("mullvad")
            win.showInfo(i18n.t("set_proxy_presets"), i18n.t("set_proxy_preset_applied"))
        } else if (a === "proxyLeakTest") {
            if (typeof settings !== "undefined") settings.proxyLeakTest()
            win.showInfo(i18n.t("set_proxy_leaktest"), i18n.t("set_proxy_leaktest_running"))
        } else if (a === "exportSettings") { backupSaveDlg.mode = "export"; backupSaveDlg.currentFile = "batorrent_settings.json"; backupSaveDlg.open() }
        else if (a === "fullBackup")      { backupSaveDlg.mode = "backup"; backupSaveDlg.currentFile = "batorrent-backup.bat"; backupSaveDlg.open() }
        else if (a === "importSettings")  { backupOpenDlg.mode = "import"; backupOpenDlg.open() }
        else if (a === "fullRestore")     { backupOpenDlg.mode = "restore"; backupOpenDlg.open() }
    }
    function showInfo(title, msg) { infoDlg.title = title; infoDlg.message = msg; infoDlg.open() }
    FileDialog {
        id: backupSaveDlg
        property string mode: ""
        fileMode: FileDialog.SaveFile
        nameFilters: mode === "backup" ? ["BATorrent backup (*.bat)"] : ["JSON (*.json)"]
        onAccepted: if (typeof settings !== "undefined") {
            var p = selectedFile.toString()
            win.showInfo(i18n.t("settings_advanced"), mode === "backup" ? settings.fullBackup(p) : settings.exportSettings(p))
        }
    }
    FileDialog {
        id: backupOpenDlg
        property string mode: ""
        fileMode: FileDialog.OpenFile
        nameFilters: mode === "restore" ? ["BATorrent backup (*.bat)"] : ["JSON (*.json)"]
        onAccepted: if (typeof settings !== "undefined") {
            var p = selectedFile.toString()
            win.showInfo(i18n.t("settings_advanced"), mode === "restore" ? settings.fullRestore(p) : settings.importSettings(p))
        }
    }

    Connections {
        target: typeof settings !== "undefined" ? settings : null
        ignoreUnknownSignals: true
        function onTelegramTestResult(ok, message) {
            infoDlg.title = (i18n.language, i18n.t("settings_telegram_test"))
            infoDlg.message = message
            infoDlg.open()
        }
        function onProxyLeakTestResult(ok, message) {
            infoDlg.title = (i18n.language, i18n.t("set_proxy_leaktest"))
            infoDlg.message = ok ? i18n.t("set_proxy_leaktest_ok").replace("%1", message)
                                 : i18n.t("set_proxy_leaktest_fail").replace("%1", message)
            infoDlg.open()
        }
    }

    function openPathPicker(key, isFile) {
        if (isFile) { pathFileDlg.targetKey = key; pathFileDlg.open() }
        else { pathFolderDlg.targetKey = key; pathFolderDlg.open() }
    }
    function decodeLocalPath(u) {
        var s = u.toString()
        // file:///C:/x → C:/x (win) ; file:///Users/x → /Users/x (mac/linux) ;
        // file://server/share → //server/share (UNC)
        if (s.startsWith("file:///")) s = s.substring(Qt.platform.os === "windows" ? 8 : 7)
        else if (s.startsWith("file://")) s = "//" + s.substring(7)
        return decodeURIComponent(s)
    }
    FolderDialog {
        id: pathFolderDlg
        property string targetKey: ""
        onAccepted: if (typeof settings !== "undefined" && targetKey !== "")
                        settings.set(targetKey, win.decodeLocalPath(selectedFolder))
    }
    FileDialog {
        id: pathFileDlg
        property string targetKey: ""
        onAccepted: if (typeof settings !== "undefined" && targetKey !== "")
                        settings.set(targetKey, win.decodeLocalPath(selectedFile))
    }

    PairingDialog { id: pairDlg }

    // BatDialog, not QtQuick Dialog: the native dialog ignored the app theme
    // (white card in dark mode) and localized its OK button to the OS language
    // (Arabic for some users) instead of the app's language.
    BatDialog {
        id: infoDlg
        property string message: ""
        cardW: 440; cardH: 260
        showCancel: false
        Text {
            Layout.fillWidth: true
            text: infoDlg.message
            color: Theme.t2; font.pixelSize: 13; font.family: Theme.fontSans
            wrapMode: Text.WordWrap; lineHeight: 1.4
        }
    }

    // confirm a reset-to-defaults (section or all) before it wipes the user's edits
    BatDialog {
        id: confirmReset
        property string scope: "section"
        cardW: 440; cardH: 240
        showCancel: true
        title: (i18n.language, i18n.t("set_reset_confirm_title"))
        okText: (i18n.language, i18n.t("set_reset_all"))
        onAccepted: scope === "all" ? win.resetAll() : win.resetSection()
        Text {
            Layout.fillWidth: true
            text: confirmReset.scope === "all"
                  ? (i18n.language, i18n.t("set_reset_confirm_all"))
                  : (i18n.language, i18n.t("set_reset_confirm_section"))
            color: Theme.t2; font.pixelSize: 13; font.family: Theme.fontSans
            wrapMode: Text.WordWrap; lineHeight: 1.4
        }
    }

    // rename the active profile
    Dialog {
        id: renameDlg
        property int idx: 0
        anchors.centerIn: parent
        modal: true
        title: (i18n.language, i18n.t("set_custom_rename"))
        standardButtons: Dialog.Ok | Dialog.Cancel
        onAccepted: if (themeBridge && renameField.text.length > 0) themeBridge.renameProfile(idx, renameField.text)
        contentItem: TFld {
            id: renameField
            implicitWidth: 240; implicitHeight: 32
        }
    }
}

