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
                WheelScroller { flick: contentScroll }

                ColumnLayout {
                    id: contentCol
                    x: Theme.sp5
                    y: Theme.sp5
                    width: contentScroll.width - 2 * Theme.sp5
                    spacing: 0

                    // .shead
                    Eyebrow { text: schema.heads[win.sec].eyebrow }
                    Text {
                        Layout.topMargin: 6
                        text: schema.heads[win.sec].h
                        color: Theme.t1
                        font.pixelSize: 19
                        font.weight: Font.DemiBold
                        font.letterSpacing: -0.3
                        font.family: Theme.fontSans
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
                        model: win.query.length > 0 ? win.searchBlocks(win.query) : win.buildBlocks(schema.sections[win.sec])
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
                                            field: modelData
                                            isLast: index === (rowsCol.parent.parent.modelData ? 0 : 0)  // border handled inside
                                            showDivider: true
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
                Text { text: (i18n.language, i18n.t("set_changes_instant")); color: Theme.t4; font.pixelSize: 11; font.family: Theme.fontSans }
                Item { Layout.fillWidth: true }
                BtnFlat { primary: true; text: (i18n.language, i18n.t("btn_close")); onClicked: win.closed() }
            }
        }
    }

    // ---- single settings row (one field) ----
    component SettingsRow: ColumnLayout {
        property var field
        property bool showDivider: true
        property bool isLast: false
        spacing: 0

        // custom-only rows collapse unless the custom theme is active; win-only collapse off Windows
        readonly property bool rowVisible: (!field.customOnly || Theme.name === "custom")
                                           && (!field.winOnly || Qt.platform.os === "windows")
                                           && field.hidden !== true
        visible: rowVisible
        Layout.preferredHeight: rowVisible ? -1 : 0

        // .srow — label/note on the left, control on the right. The text column
        // fills the space left of the control, so a long label/note word-wraps
        // to a second line instead of running into the control.
        RowLayout {
            id: srow
            visible: field.type !== "warning"
            Layout.fillWidth: true
            Layout.topMargin: 13
            Layout.bottomMargin: 13
            spacing: Theme.sp4

            // .smeta
            ColumnLayout {
                Layout.fillWidth: true
                Layout.minimumWidth: 120
                spacing: 4
                RowLayout {
                    spacing: Theme.sp2
                    Layout.fillWidth: true
                    Text {
                        text: field.label
                        color: Theme.t1
                        font.pixelSize: 13
                        font.family: Theme.fontSans
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }
                    // badge
                    Rectangle {
                        visible: field.badge !== undefined
                        implicitWidth: bdg.implicitWidth + 14
                        implicitHeight: 18
                        radius: 999
                        color: Theme.field
                        border.color: Theme.hair
                        border.width: 1
                        Text { id: bdg; anchors.centerIn: parent; text: field.badge || ""; color: Theme.t3; font.pixelSize: 10; font.family: Theme.fontMono }
                    }
                }
                Text {
                    visible: field.note !== undefined
                    Layout.fillWidth: true          // wrap within the text column, never under the control
                    text: field.note || ""
                    color: Theme.t4
                    font.pixelSize: 11
                    font.family: Theme.fontSans
                    wrapMode: Text.WordWrap
                    lineHeight: 1.5
                }
            }

            // .sctrl — control by type. Wide controls (path, grow text) fill the
            // remaining width (capped) and shrink instead of overflowing the card.
            // Fixed-width control on the right; the text column (fillWidth) takes the
            // rest and wraps. Making the control fillWidth makes it fight a long note
            // for space and get pushed off-screen, so we keep it a fixed size.
            Loader {
                id: ctrl
                Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                visible: field.type !== "warning"
                sourceComponent: {
                    switch (field.type) {
                    case "toggle": return cToggle
                    case "path": return cPath
                    case "timerange": return cTimeRange
                    case "days": return cDays
                    case "anime": return cAnime
                    case "number": return cNumber
                    case "text": return cText
                    case "password": return cText
                    case "select": return cSelect
                    case "segmented": return cSegmented
                    case "theme": return cTheme
                    case "appicon": return cAppIcon
                    case "debrid": return cDebridPicker
                    case "debridtoken": return cDebridToken
                    case "button": return cButton
                    case "iface": return cIface
                    case "profiles": return cProfiles
                    case "color": return cColor
                    case "bgimage": return cBgImage
                    case "slider": return cSlider
                    }
                    return null
                }
            }
        }

        // warning: full-width amber banner (the .srow above is hidden for it)
        RowLayout {
            visible: field.type === "warning"
            Layout.fillWidth: true
            Layout.topMargin: 12
            Layout.bottomMargin: 12
            spacing: Theme.sp3
            Text { text: "⚠"; color: Theme.amber; font.pixelSize: 13; Layout.alignment: Qt.AlignTop }
            Text {
                Layout.fillWidth: true
                visible: field.type === "warning"
                text: field.text || ""
                color: Theme.t3
                font.pixelSize: 11
                font.family: Theme.fontSans
                wrapMode: Text.WordWrap
                lineHeight: 1.45
            }
        }

        Rectangle { visible: showDivider; Layout.fillWidth: true; Layout.preferredHeight: 1; color: Theme.hairSoft }

        // ---- control components ----
        Component { id: cToggle; TToggle {
            on: (field.key === "torrentSearchEnabled" && typeof addons !== "undefined") ? addons.torrentSearchEnabled
                : (field.key === "autoTrackers" && typeof addons !== "undefined") ? addons.autoTrackers
                : (field.key === "followSystem" && typeof themeBridge !== "undefined") ? themeBridge.followSystem
                : win.boolPref(field)
            onToggled: function(v) {
                if (field.key === "torrentSearchEnabled" && typeof addons !== "undefined") addons.torrentSearchEnabled = v
                else if (field.key === "autoTrackers" && typeof addons !== "undefined") addons.autoTrackers = v
                else if (field.key === "followSystem" && typeof themeBridge !== "undefined") themeBridge.followSystem = v
                else if (typeof settings !== "undefined" && field.key !== undefined) settings.set(field.key, v)
            }
        } }
        Component {
            id: cAnime
            TToggle {
                on: Theme.anime
                onToggled: function(v) { Theme.setAnime(v) }
            }
        }
        Component {
            id: cTheme
            TSelect {
                // options index ↔ Theme.name
                readonly property var names: ["dark", "light", "midnight", "sakura", "darkstar", "matrix", "custom"]
                implicitWidth: 180
                model: field.options || []
                currentIndex: Math.max(0, names.indexOf(Theme.name))
                onActivated: function(i) { Theme.setName(names[i]) }
            }
        }
        // app-icon picker — clickable tiles, independent of the UI theme
        Component {
            id: cAppIcon
            Row {
                spacing: 7
                Repeater {
                    model: (typeof themeBridge !== "undefined") ? themeBridge.appIcons() : []
                    delegate: Rectangle {
                        id: tile
                        required property var modelData
                        readonly property bool sel: typeof themeBridge !== "undefined"
                            && (themeBridge.appIconChoice, themeBridge.appIconChoice === modelData.key)
                        width: 44; height: 44; radius: 11
                        color: tile.sel ? Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.12) : "transparent"
                        border.width: 2
                        border.color: tile.sel ? Theme.accent : Theme.hair
                        Image {
                            anchors.centerIn: parent
                            width: 34; height: 34
                            source: tile.modelData.preview
                            sourceSize: Qt.size(68, 68)
                            fillMode: Image.PreserveAspectFit
                            smooth: true
                        }
                        MouseArea {
                            id: tileMa
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: if (typeof themeBridge !== "undefined") themeBridge.setAppIcon(tile.modelData.key)
                        }
                        ToolTip.text: tile.modelData.key
                        ToolTip.visible: tileMa.containsMouse
                        ToolTip.delay: 400
                    }
                }
            }
        }
        // ---- custom-theme controls (operate on the active profile) ----
        Component {
            id: cProfiles
            RowLayout {
                spacing: Theme.sp2
                TSelect {
                    implicitWidth: 150
                    model: {
                        var names = []
                        if (typeof themeBridge !== "undefined")
                            for (var i = 0; i < themeBridge.customProfiles.length; i++)
                                names.push(themeBridge.customProfiles[i].name)
                        return names
                    }
                    currentIndex: themeBridge ? themeBridge.activeProfile : 0
                    onActivated: function(i) { if (themeBridge) themeBridge.activeProfile = i }
                }
                BtnFlat { text: "+"; sm: true; onClicked: if (themeBridge) themeBridge.activeProfile = themeBridge.addProfile() }
                BtnFlat {
                    text: (i18n.language, i18n.t("set_custom_rename")); sm: true
                    onClicked: { renameDlg.idx = themeBridge.activeProfile; renameField.text = themeBridge.customProfiles[themeBridge.activeProfile].name; renameDlg.open() }
                }
                BtnFlat {
                    text: (i18n.language, i18n.t("set_custom_clear")); sm: true
                    enabled: themeBridge && themeBridge.customProfiles.length > 1
                    onClicked: if (themeBridge) themeBridge.removeProfile(themeBridge.activeProfile)
                }
            }
        }
        Component {
            id: cColor
            RowLayout {
                spacing: Theme.sp2
                readonly property string cur: (win.ap && win.ap[field.role]) ? win.ap[field.role] : "#000000"
                Rectangle {
                    implicitWidth: 26; implicitHeight: 26; radius: 6
                    color: parent.cur
                    border.color: Theme.hair; border.width: 1
                }
                TFld {
                    implicitWidth: 110; implicitHeight: 30; mono: true
                    text: parent.cur
                    placeholder: "#rrggbb"
                    onEdited: function(t) {
                        var v = t.charAt(0) === "#" ? t : "#" + t
                        if (/^#[0-9a-fA-F]{6}$/.test(v) && themeBridge)
                            themeBridge.setProfileColor(themeBridge.activeProfile, field.role, v.toLowerCase())
                    }
                }
                BtnFlat {
                    text: "…"; sm: true
                    onClicked: { colorDlg.targetRole = field.role; colorDlg.selectedColor = parent.cur; colorDlg.open() }
                }
            }
        }
        Component {
            id: cBgImage
            RowLayout {
                spacing: Theme.sp2
                TFld {
                    implicitWidth: 210; implicitHeight: 30; mono: true
                    text: (win.ap && win.ap.image) ? win.ap.image : ""
                    placeholder: (i18n.language, i18n.t("set_custom_bgimage"))
                    readonly: true
                }
                BtnFlat { text: (i18n.language, i18n.t("settings_browse")); sm: true; onClicked: bgImageDlg.open() }
                BtnFlat { text: (i18n.language, i18n.t("set_custom_clear")); sm: true; onClicked: if (themeBridge) themeBridge.setProfileImage(themeBridge.activeProfile, "") }
            }
        }
        Component {
            id: cSlider
            RowLayout {
                spacing: Theme.sp2
                Slider {
                    id: opacitySlider
                    implicitWidth: 150
                    from: 0; to: 100; stepSize: 1
                    onMoved: if (themeBridge) themeBridge.setProfileOpacity(themeBridge.activeProfile, Math.round(value))
                    // Binding (not a plain `value:`) so dragging doesn't break the link.
                    Binding { target: opacitySlider; property: "value"; value: (win.ap && win.ap.opacity !== undefined) ? win.ap.opacity : 55 }
                    background: Rectangle {
                        x: parent.leftPadding; y: parent.topPadding + parent.availableHeight / 2 - height / 2
                        width: parent.availableWidth; height: 4; radius: 2; color: Theme.track
                        Rectangle { width: parent.width * parent.parent.visualPosition; height: parent.height; radius: 2; color: Theme.accent }
                    }
                    handle: Rectangle {
                        x: parent.leftPadding + parent.visualPosition * (parent.availableWidth - width)
                        y: parent.topPadding + parent.availableHeight / 2 - height / 2
                        implicitWidth: 16; implicitHeight: 16; radius: 8
                        color: Theme.accent; border.color: Theme.bg; border.width: 2
                    }
                }
                Text {
                    text: ((win.ap && win.ap.opacity !== undefined) ? win.ap.opacity : 55) + "%"
                    color: Theme.t3; font.pixelSize: 11; font.family: Theme.fontMono
                    Layout.preferredWidth: 36
                }
            }
        }
        Component {
            id: cNumber
            RowLayout {
                spacing: Theme.sp2
                Rectangle {
                    implicitWidth: 92; implicitHeight: 30; radius: 7
                    color: Theme.field; border.color: Theme.hair; border.width: 1
                    TextInput {
                        anchors.fill: parent; anchors.rightMargin: 10
                        text: { var v = (typeof settings !== "undefined" && field.key !== undefined) ? settings.get(field.key) : field.value; return (v === undefined || v === null) ? "" : String(v) }
                        color: Theme.t1
                        font.pixelSize: 12; font.family: Theme.fontMono
                        horizontalAlignment: TextInput.AlignRight
                        verticalAlignment: TextInput.AlignVCenter
                        onEditingFinished: if (typeof settings !== "undefined" && field.key !== undefined) settings.set(field.key, text)
                    }
                }
                Text { visible: field.suffix !== undefined; text: field.suffix || ""; color: Theme.t4; font.pixelSize: 11; font.family: Theme.fontMono }
            }
        }
        Component {
            id: cText
            TFld {
                implicitWidth: field.w === "grow" ? 300 : field.w === "w-md" ? 210 : field.w === "w-sm" ? 120 : 180
                implicitHeight: 30
                mono: field.mono === true
                password: field.type === "password"
                text: (field.key === "torrentSearchUrl" && typeof addons !== "undefined") ? addons.torrentSearchUrl
                      : (typeof settings !== "undefined" && field.key !== undefined) ? settings.get(field.key) : (field.value || "")
                placeholder: field.placeholder || ""
                onEdited: function(t) {
                    if (field.key === "torrentSearchUrl" && typeof addons !== "undefined") addons.torrentSearchUrl = t
                    else if (typeof settings !== "undefined" && field.key !== undefined) settings.set(field.key, t)
                }
            }
        }
        Component {
            id: cIface
            TSelect {
                implicitWidth: 220
                model: (typeof settings !== "undefined") ? settings.networkInterfaces() : []
                currentIndex: {
                    var cur = (typeof settings !== "undefined") ? (settings.get("outgoingInterface") || "") : ""
                    if (cur === "") return 0
                    for (var i = 1; i < model.length; i++)
                        if (model[i].split(" — ")[0] === cur) return i
                    return 0
                }
                onActivated: function(i) {
                    if (typeof settings !== "undefined")
                        settings.set("outgoingInterface", i === 0 ? "" : model[i].split(" — ")[0])
                }
            }
        }
        Component {
            id: cSelect
            TSelect {
                implicitWidth: 180
                model: field.options || []
                icons: field.icons || []
                currentIndex: {
                    if (field.isLang) return i18n.language
                    var v = (typeof settings !== "undefined" && field.key !== undefined) ? settings.get(field.key) : field.value
                    return (v === undefined || v === null || v === "") ? 0 : v
                }
                onActivated: function(i) { if (field.isLang) i18n.setLanguage(i); else if (typeof settings !== "undefined" && field.key !== undefined) settings.set(field.key, i) }
            }
        }
        Component {
            id: cSegmented
            Rectangle {
                implicitWidth: segR.implicitWidth + 4
                implicitHeight: 29
                radius: 8
                color: Theme.field
                border.color: Theme.hair
                border.width: 1
                property int curIdx: {
                    var v = (typeof settings !== "undefined" && field.key !== undefined) ? settings.get(field.key) : field.value
                    return (v === undefined || v === null || v === "") ? 0 : v
                }
                Row {
                    id: segR
                    anchors.centerIn: parent
                    spacing: 2
                    Repeater {
                        model: field.options || []
                        delegate: Rectangle {
                            height: 25
                            implicitWidth: segLbl.implicitWidth + 22
                            radius: 6
                            color: index === parent.parent.parent.curIdx ? Theme.sel : "transparent"
                            Text {
                                id: segLbl
                                anchors.centerIn: parent
                                text: modelData
                                color: index === parent.parent.parent.curIdx ? Theme.accentText : Theme.t3
                                font.pixelSize: 11
                                font.weight: Font.Medium
                                font.family: Theme.fontSans
                            }
                            MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: { parent.parent.parent.curIdx = index; if (typeof settings !== "undefined" && field.key !== undefined) settings.set(field.key, index) } }
                        }
                    }
                }
            }
        }
        Component { id: cButton; BtnFlat { text: field.btn || ""; sm: false; onClicked: win.runButtonAction(field.action) } }

        Component {
            id: cDebridPicker
            RowLayout {
                spacing: Theme.sp2
                Repeater {
                    model: typeof debrid !== "undefined" ? debrid.providers : []
                    BtnFlat {
                        text: (debrid.providerId === modelData.id ? "● " : "")
                              + modelData.name
                              + (modelData.hasToken ? " ✓" : "")
                        sm: true
                        onClicked: debrid.providerId = modelData.id
                    }
                }
            }
        }
        Component {
            id: cDebridToken
            RowLayout {
                spacing: Theme.sp2
                readonly property bool connected: typeof debrid !== "undefined" && debrid.authed
                Text {
                    visible: parent.connected
                    text: parent.connected
                        ? "✓ " + debrid.accountName + " · " + debrid.accountPlan
                          + (debrid.expiry ? " · " + debrid.expiry : "")
                        : ""
                    color: Theme.grn; font.pixelSize: 12; font.family: Theme.fontSans
                    elide: Text.ElideRight; Layout.maximumWidth: 280
                }
                TFld {
                    id: debTok
                    visible: !parent.connected
                    implicitWidth: 220; implicitHeight: 30; mono: true; password: true
                    placeholder: (i18n.language, i18n.t("set_rd_token_ph"))
                    onEdited: function(t) { if (typeof debrid !== "undefined") debrid.setToken(t) }
                }
                BtnFlat {
                    text: parent.connected ? (i18n.language, i18n.t("set_rd_disconnect"))
                                           : (i18n.language, i18n.t("set_rd_connect"))
                    sm: true
                    onClicked: {
                        if (typeof debrid === "undefined") return
                        if (parent.connected) debrid.clearToken()
                        else debrid.setToken(debTok.text)
                    }
                }
            }
        }
        Component {
            id: cPath
            RowLayout {
                id: pathRow
                spacing: Theme.sp2
                function curPath() {
                    return (typeof settings !== "undefined" && field.key !== undefined)
                        ? (settings.get(field.key) || "") : (field.value || "")
                }
                TFld {
                    id: pathFld
                    implicitWidth: 220; implicitHeight: 30; mono: true
                    placeholder: field.placeholder || ""
                    // Driven imperatively (not bound) so it survives both the user
                    // typing into it AND a Browse pick — settings.get() isn't a
                    // reactive property, so a plain binding never refreshed on Browse.
                    Component.onCompleted: text = pathRow.curPath()
                    onEdited: function(t) {
                        if (typeof settings !== "undefined" && field.key !== undefined)
                            settings.set(field.key, t)
                    }
                    Connections {
                        target: typeof settings !== "undefined" ? settings : null
                        ignoreUnknownSignals: true
                        function onChanged() {
                            var v = pathRow.curPath()
                            if (pathFld.text !== v) pathFld.text = v
                        }
                    }
                }
                BtnFlat {
                    text: (i18n.language, i18n.t("settings_browse")); sm: true
                    onClicked: win.openPathPicker(field.key, field.file === true)
                }
                BtnFlat {
                    visible: field.file !== true
                    text: (i18n.language, i18n.t("set_custom_clear")); sm: true
                    onClicked: if (typeof settings !== "undefined" && field.key !== undefined) settings.set(field.key, "")
                }
            }
        }
        // scheduler from/to hour (0–23)
        Component {
            id: cTimeRange
            RowLayout {
                spacing: Theme.sp2
                Repeater {
                    model: [{ k: "scheduleFromHour" }, { k: "scheduleToHour" }]
                    delegate: Row {
                        spacing: Theme.sp2
                        Text { visible: index === 1; text: "—"; color: Theme.t4; font.pixelSize: 12; anchors.verticalCenter: parent.verticalCenter }
                        Rectangle {
                            width: 64; height: 30; radius: 7
                            color: Theme.field; border.color: Theme.hair; border.width: 1
                            TextInput {
                                anchors.fill: parent; anchors.margins: 6
                                text: (typeof settings !== "undefined") ? String(settings.get(modelData.k)) : "0"
                                color: Theme.t1; font.pixelSize: 12; font.family: Theme.fontMono
                                horizontalAlignment: TextInput.AlignHCenter; verticalAlignment: TextInput.AlignVCenter
                                validator: IntValidator { bottom: 0; top: 23 }
                                onEditingFinished: if (typeof settings !== "undefined") settings.set(modelData.k, Math.max(0, Math.min(23, parseInt(text) || 0)))
                            }
                        }
                        Text { text: "h"; color: Theme.t4; font.pixelSize: 11; anchors.verticalCenter: parent.verticalCenter }
                    }
                }
            }
        }
        // scheduler active days — bitmask, bit0=Mon … bit6=Sun
        Component {
            id: cDays
            Row {
                spacing: 4
                property int mask: (typeof settings !== "undefined") ? (settings.get("scheduleDays") || 0) : 0
                Repeater {
                    model: [(i18n.language, i18n.t("day_mon")), i18n.t("day_tue"), i18n.t("day_wed"), i18n.t("day_thu"), i18n.t("day_fri"), i18n.t("day_sat"), i18n.t("day_sun")]
                    delegate: Rectangle {
                        width: 30; height: 30; radius: 7
                        readonly property bool sel: (parent.mask & (1 << index)) !== 0
                        // tint + border like the filter pills — seven solid accent
                        // squares in a row read as a warning, not a selection
                        color: sel ? Theme.accentTint : Theme.field
                        border.color: sel ? Theme.accent : Theme.hair; border.width: 1
                        Text { anchors.centerIn: parent; text: modelData; color: parent.sel ? Theme.accentText : Theme.t3; font.pixelSize: 11; font.weight: parent.sel ? Font.DemiBold : Font.Medium; font.family: Theme.fontSans }
                        MouseArea {
                            anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                            onClicked: if (typeof settings !== "undefined") {
                                parent.parent.mask ^= (1 << index)
                                settings.set("scheduleDays", parent.parent.mask)
                            }
                        }
                    }
                }
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

    Dialog {
        id: infoDlg
        property string message: ""
        anchors.centerIn: parent
        modal: true
        standardButtons: Dialog.Ok
        contentItem: Text {
            text: infoDlg.message
            color: Theme.t1; font.pixelSize: 12; font.family: Theme.fontSans
            wrapMode: Text.WordWrap
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

