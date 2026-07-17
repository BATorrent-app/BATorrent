// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// One settings row: label/note/badge on the left, a type-driven control on
// the right (toggle, path, select, theme, slider, ...). `sw` is the owning
// SettingsView (prefs accessors + active custom-theme profile); the three
// dialogs it can't own are routed back as signals.
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import "theme"
import "widgets"

ColumnLayout {
    id: rowRoot
    property var sw
    signal renameProfileRequested()
    signal colorPickRequested(string role, color cur)
    signal bgImageRequested()
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
        IconImg { src: "qrc:/icons/triangle-alert.svg"; tint: Theme.amber; s: 13; Layout.alignment: Qt.AlignTop }
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
            : sw.boolPref(field)
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
                onClicked: rowRoot.renameProfileRequested()
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
            readonly property string cur: (sw.ap && sw.ap[field.role]) ? sw.ap[field.role] : "#000000"
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
                onClicked: rowRoot.colorPickRequested(field.role, parent.cur)
            }
        }
    }
    Component {
        id: cBgImage
        RowLayout {
            spacing: Theme.sp2
            TFld {
                implicitWidth: 210; implicitHeight: 30; mono: true
                text: (sw.ap && sw.ap.image) ? sw.ap.image : ""
                placeholder: (i18n.language, i18n.t("set_custom_bgimage"))
                readonly: true
            }
            BtnFlat { text: (i18n.language, i18n.t("settings_browse")); sm: true; onClicked: rowRoot.bgImageRequested() }
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
                Binding { target: opacitySlider; property: "value"; value: (sw.ap && sw.ap.opacity !== undefined) ? sw.ap.opacity : 55 }
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
                text: ((sw.ap && sw.ap.opacity !== undefined) ? sw.ap.opacity : 55) + "%"
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
    Component { id: cButton; BtnFlat { text: field.btn || ""; sm: false; onClicked: sw.runButtonAction(field.action) } }

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
                onClicked: sw.openPathPicker(field.key, field.file === true)
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
