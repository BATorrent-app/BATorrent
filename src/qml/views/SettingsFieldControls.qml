// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Pref / path / schedule / debrid SettingsRow control Components.
// field + sw are host props from SettingsRow: never `field: field`.
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import "../theme"
import "../widgets"

// Item (not QtObject): Component children need a default property.
Item {
    id: root
    width: 0; height: 0; visible: false
    property var field
    property var sw

    // One height and type size for every input on the page, so a row with a
    // select lines up with the row of text field under it.
    readonly property int ctlH: 42
    readonly property int ctlFont: 15
    readonly property real ctlRadius: Theme.rad(9)

    property Component number: cNumber
    property Component text: cText
    property Component iface: cIface
    property Component select: cSelect
    property Component segmented: cSegmented
    property Component button: cButton
    property Component debrid: cDebridPicker
    property Component debridtoken: cDebridToken
    property Component path: cPath
    property Component timerange: cTimeRange
    property Component days: cDays
    property Component langlist: cLangList

    Component {
        id: cNumber
        RowLayout {
            spacing: Theme.sp2
            Rectangle {
                implicitWidth: 108; implicitHeight: root.ctlH; radius: root.ctlRadius
                color: Theme.field; border.color: Theme.hair; border.width: 1
                TextInput {
                    anchors.fill: parent; anchors.rightMargin: 14
                    text: { var v = (typeof settings !== "undefined" && root.field.key !== undefined) ? settings.get(root.field.key) : root.field.value; return (v === undefined || v === null) ? "" : String(v) }
                    color: Theme.t1
                    font.pixelSize: root.ctlFont; font.family: Theme.fontSans; font.features: Theme.tnum
                    horizontalAlignment: TextInput.AlignRight
                    verticalAlignment: TextInput.AlignVCenter
                    onEditingFinished: if (typeof settings !== "undefined" && root.field.key !== undefined) settings.set(root.field.key, text)
                    // Flush on teardown: Windows focus quirk before editingFinished.
                    Component.onDestruction: if (typeof settings !== "undefined" && root.field.key !== undefined
                        && String(settings.get(root.field.key)) !== text) settings.set(root.field.key, text)
                }
            }
            Text { visible: root.field.suffix !== undefined; text: root.field.suffix || ""; color: Theme.t3; font.pixelSize: 14; font.family: Theme.fontSans; font.features: Theme.tnum }
        }
    }
    Component {
        id: cText
        TFld {
            implicitWidth: root.field.w === "grow" ? 330 : root.field.w === "w-md" ? 260 : root.field.w === "w-sm" ? 150 : 220
            implicitHeight: root.ctlH
            radius: root.ctlRadius
            fontSize: root.field.mono === true ? 14 : 15
            mono: root.field.mono === true
            password: root.field.type === "password"
            text: (root.field.key === "torrentSearchUrl" && typeof addons !== "undefined") ? addons.torrentSearchUrl
                  : (typeof settings !== "undefined" && root.field.key !== undefined) ? settings.get(root.field.key) : (root.field.value || "")
            placeholder: root.field.placeholder || ""
            onEdited: function(t) {
                if (root.field.key === "torrentSearchUrl" && typeof addons !== "undefined") addons.torrentSearchUrl = t
                else if (typeof settings !== "undefined" && root.field.key !== undefined) settings.set(root.field.key, t)
            }
            Component.onDestruction: {
                if (root.field.key === undefined || root.field.key === "torrentSearchUrl") return
                if (typeof settings !== "undefined" && String(settings.get(root.field.key)) !== text)
                    settings.set(root.field.key, text)
            }
        }
    }
    Component {
        id: cIface
        TSelect {
            implicitWidth: 260
            implicitHeight: root.ctlH
            radius: root.ctlRadius
            font.pixelSize: root.ctlFont
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
            implicitWidth: 220
            implicitHeight: root.ctlH
            radius: root.ctlRadius
            font.pixelSize: root.ctlFont
            model: root.field.options || []
            icons: root.field.icons || []
            currentIndex: {
                if (root.field.isLang) return i18n.language
                // Index 0 is "same as the app"; stored value is index minus one (-1 = follow).
                if (root.field.isContentLang) {
                    var cv = (typeof settings !== "undefined") ? settings.get("contentLanguage") : undefined
                    return (cv === undefined || cv === null || cv === "" || cv < 0) ? 0 : cv + 1
                }
                var v = (typeof settings !== "undefined" && root.field.key !== undefined) ? settings.get(root.field.key) : root.field.value
                return (v === undefined || v === null || v === "") ? 0 : v
            }
            onActivated: function(i) {
                if (root.field.isLang) i18n.setLanguage(i)
                else if (root.field.isContentLang) {
                    if (typeof settings !== "undefined") settings.set("contentLanguage", i - 1)
                } else if (typeof settings !== "undefined" && root.field.key !== undefined) settings.set(root.field.key, i)
            }
        }
    }
    Component {
        id: cSegmented
        Rectangle {
            implicitWidth: segR.implicitWidth + 8
            implicitHeight: 42
            radius: Theme.rad(10)
            color: Theme.field
            border.color: Theme.hair
            border.width: 1
            property int curIdx: {
                var v = (typeof settings !== "undefined" && root.field.key !== undefined) ? settings.get(root.field.key) : root.field.value
                return (v === undefined || v === null || v === "") ? 0 : v
            }
            Row {
                id: segR
                anchors.centerIn: parent
                spacing: 2
                Repeater {
                    model: root.field.options || []
                    delegate: Rectangle {
                        height: 34
                        implicitWidth: segLbl.implicitWidth + 28
                        radius: Theme.rad(7)
                        color: index === parent.parent.parent.curIdx ? Theme.sel : "transparent"
                        Text {
                            id: segLbl
                            anchors.centerIn: parent
                            text: modelData
                            color: index === parent.parent.parent.curIdx ? Theme.t1 : Theme.t3
                            font.pixelSize: 14
                            font.weight: Font.Medium
                            font.family: Theme.fontSans
                        }
                        MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: { parent.parent.parent.curIdx = index; if (typeof settings !== "undefined" && root.field.key !== undefined) settings.set(root.field.key, index) } }
                    }
                }
            }
        }
    }
    Component {
        id: cButton
        BtnFlat { text: root.field.btn || ""; lg: true; onClicked: root.sw.runButtonAction(root.field.action) }
    }
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
                color: Theme.grn; font.pixelSize: 14; font.family: Theme.fontSans
                elide: Text.ElideRight; Layout.maximumWidth: 320
            }
            TFld {
                id: debTok
                visible: !parent.connected
                implicitWidth: 260; implicitHeight: root.ctlH; radius: root.ctlRadius; fontSize: 14; mono: true; password: true
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
        // One field with clear and Browse inside it: three loose controls in a
        // row read as three settings.
        Rectangle {
            id: pathRow
            implicitWidth: 330
            implicitHeight: root.ctlH
            radius: root.ctlRadius
            color: Theme.field
            border.color: pathInput.activeFocus ? Theme.accent : Theme.hair
            border.width: 1
            function curPath() {
                return (typeof settings !== "undefined" && root.field.key !== undefined)
                    ? (settings.get(root.field.key) || "") : (root.field.value || "")
            }
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 14
                anchors.rightMargin: 5
                spacing: 4
                TextField {
                    id: pathInput
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    background: null
                    padding: 0
                    verticalAlignment: TextInput.AlignVCenter
                    color: Theme.t1
                    placeholderText: root.field.placeholder || ""
                    placeholderTextColor: Theme.t4
                    font.pixelSize: 14
                    font.family: Theme.fontMono
                    // Imperative text: settings.get() isn't reactive; binding wouldn't refresh on Browse.
                    Component.onCompleted: text = pathRow.curPath()
                    onEditingFinished: if (typeof settings !== "undefined" && root.field.key !== undefined)
                                           settings.set(root.field.key, text)
                    Connections {
                        target: typeof settings !== "undefined" ? settings : null
                        ignoreUnknownSignals: true
                        function onChanged() {
                            var v = pathRow.curPath()
                            if (pathInput.text !== v) pathInput.text = v
                        }
                    }
                }
                Rectangle {
                    visible: root.field.file !== true && pathInput.text.length > 0
                    Layout.preferredWidth: 30; Layout.preferredHeight: 30
                    radius: Theme.rad(6)
                    color: clrMa.containsMouse ? Theme.hover : "transparent"
                    IconImg { anchors.centerIn: parent; src: "qrc:/icons/close.svg"; s: 12; tint: clrMa.containsMouse ? Theme.t1 : Theme.t3 }
                    MouseArea {
                        id: clrMa
                        anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                        onClicked: if (typeof settings !== "undefined" && root.field.key !== undefined) settings.set(root.field.key, "")
                    }
                }
                Rectangle {
                    Layout.preferredHeight: 32
                    Layout.preferredWidth: brLbl.implicitWidth + 24
                    radius: Theme.rad(6)
                    color: brMa.containsMouse ? Theme.hover : Theme.elev
                    border.color: Theme.hair; border.width: 1
                    Text {
                        id: brLbl
                        anchors.centerIn: parent
                        text: (i18n.language, i18n.t("settings_browse"))
                        color: Theme.t1
                        font.pixelSize: 14; font.weight: Font.Medium; font.family: Theme.fontSans
                    }
                    MouseArea {
                        id: brMa
                        anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                        onClicked: root.sw.openPathPicker(root.field.key, root.field.file === true)
                    }
                }
            }
        }
    }
    Component {
        id: cLangList
        // An ordered list of language codes stored comma-separated: the first
        // one the file has wins. Chips in priority order, each can move up or
        // go; the select at the end adds one.
        ColumnLayout {
            id: ll
            spacing: Theme.sp2
            property var codes: {
                var v = (typeof settings !== "undefined" && root.field.key !== undefined) ? settings.get(root.field.key) : ""
                return String(v || "").split(",").filter(function (c) { return c.length > 0 })
            }
            readonly property var all: root.sw.schemaLanguages
            function nameOf(code) {
                for (var i = 0; i < all.length; i++) if (all[i].code === code) return all[i].name
                return code
            }
            function save(list) {
                ll.codes = list
                if (typeof settings !== "undefined" && root.field.key !== undefined) settings.set(root.field.key, list.join(","))
            }
            readonly property var remaining: all.filter(function (l) { return ll.codes.indexOf(l.code) < 0 })

            Flow {
                Layout.preferredWidth: 360
                Layout.alignment: Qt.AlignRight
                layoutDirection: Qt.RightToLeft
                spacing: 6
                visible: ll.codes.length > 0
                Repeater {
                    // reversed so the right-to-left flow still reads 1, 2, 3
                    model: ll.codes.slice().reverse()
                    delegate: Rectangle {
                        required property string modelData
                        required property int index
                        readonly property int pos: ll.codes.length - 1 - index
                        height: 34
                        width: chipRow.implicitWidth + 16
                        radius: Theme.rad(8)
                        color: Theme.field
                        border.color: Theme.hair; border.width: 1
                        RowLayout {
                            id: chipRow
                            anchors.centerIn: parent
                            spacing: 6
                            Text { text: (pos + 1) + "."; color: Theme.t4; font.pixelSize: 13; font.family: Theme.fontSans; font.features: Theme.tnum }
                            Text { text: ll.nameOf(modelData); color: Theme.t1; font.pixelSize: 14; font.family: Theme.fontSans }
                            IconImg {
                                visible: pos > 0
                                src: "qrc:/icons/chevron-bold.svg"; rotation: 180; s: 11
                                tint: upMa.containsMouse ? Theme.t1 : Theme.t3
                                MouseArea {
                                    id: upMa
                                    anchors.fill: parent; anchors.margins: -4
                                    hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                                    onClicked: {
                                        var l = ll.codes.slice(); var t = l[pos - 1]; l[pos - 1] = l[pos]; l[pos] = t
                                        ll.save(l)
                                    }
                                }
                            }
                            IconImg {
                                src: "qrc:/icons/close.svg"; s: 11
                                tint: rmMa.containsMouse ? Theme.t1 : Theme.t3
                                MouseArea {
                                    id: rmMa
                                    anchors.fill: parent; anchors.margins: -4
                                    hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                                    onClicked: { var l = ll.codes.slice(); l.splice(pos, 1); ll.save(l) }
                                }
                            }
                        }
                    }
                }
            }
            TSelect {
                Layout.alignment: Qt.AlignRight
                visible: ll.remaining.length > 0
                implicitWidth: 220
                implicitHeight: root.ctlH
                radius: root.ctlRadius
                font.pixelSize: root.ctlFont
                displayText: (i18n.language, i18n.t("set_lang_add"))
                model: ll.remaining.map(function (l) { return l.name })
                currentIndex: -1
                onActivated: function (i) {
                    var l = ll.codes.slice(); l.push(ll.remaining[i].code); ll.save(l)
                    currentIndex = -1
                }
            }
        }
    }
    Component {
        id: cTimeRange
        RowLayout {
            spacing: Theme.sp2
            Repeater {
                model: [{ k: "scheduleFromHour" }, { k: "scheduleToHour" }]
                delegate: Row {
                    spacing: Theme.sp2
                    Text { visible: index === 1; text: "—"; color: Theme.t4; font.pixelSize: 15; anchors.verticalCenter: parent.verticalCenter }
                    Rectangle {
                        width: 72; height: root.ctlH; radius: root.ctlRadius
                        color: Theme.field; border.color: Theme.hair; border.width: 1
                        TextInput {
                            anchors.fill: parent; anchors.margins: 6
                            text: (typeof settings !== "undefined") ? String(settings.get(modelData.k)) : "0"
                            color: Theme.t1; font.pixelSize: root.ctlFont; font.family: Theme.fontSans; font.features: Theme.tnum
                            horizontalAlignment: TextInput.AlignHCenter; verticalAlignment: TextInput.AlignVCenter
                            validator: IntValidator { bottom: 0; top: 23 }
                            onEditingFinished: if (typeof settings !== "undefined") settings.set(modelData.k, Math.max(0, Math.min(23, parseInt(text) || 0)))
                        }
                    }
                    Text { text: "h"; color: Theme.t3; font.pixelSize: 14; anchors.verticalCenter: parent.verticalCenter }
                }
            }
        }
    }
    Component {
        id: cDays
        Row {
            spacing: 4
            property int mask: (typeof settings !== "undefined") ? (settings.get("scheduleDays") || 0) : 0
            Repeater {
                model: [(i18n.language, i18n.t("day_mon")), i18n.t("day_tue"), i18n.t("day_wed"), i18n.t("day_thu"), i18n.t("day_fri"), i18n.t("day_sat"), i18n.t("day_sun")]
                delegate: Rectangle {
                    width: 40; height: 40; radius: root.ctlRadius
                    readonly property bool sel: (parent.mask & (1 << index)) !== 0
                    color: sel ? Theme.accentTint : Theme.field
                    border.color: sel ? Theme.accent : Theme.hair; border.width: 1
                    Text { anchors.centerIn: parent; text: modelData; color: parent.sel ? Theme.accentText : Theme.t3; font.pixelSize: 13; font.weight: parent.sel ? Font.DemiBold : Font.Medium; font.family: Theme.fontSans }
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
