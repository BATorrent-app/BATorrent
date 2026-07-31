// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Pref / path / schedule / debrid SettingsRow control Components.
// field + sw are host props from SettingsRow — never `field: field`.
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import "theme"
import "widgets"

// Item (not QtObject): Component children need a default property.
Item {
    id: root
    width: 0; height: 0; visible: false
    property var field
    property var sw

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

    Component {
        id: cNumber
        RowLayout {
            spacing: Theme.sp2
            Rectangle {
                implicitWidth: 92; implicitHeight: 30; radius: 7
                color: Theme.field; border.color: Theme.hair; border.width: 1
                TextInput {
                    anchors.fill: parent; anchors.rightMargin: 10
                    text: { var v = (typeof settings !== "undefined" && root.field.key !== undefined) ? settings.get(root.field.key) : root.field.value; return (v === undefined || v === null) ? "" : String(v) }
                    color: Theme.t1
                    font.pixelSize: 12; font.family: Theme.fontMono
                    horizontalAlignment: TextInput.AlignRight
                    verticalAlignment: TextInput.AlignVCenter
                    onEditingFinished: if (typeof settings !== "undefined" && root.field.key !== undefined) settings.set(root.field.key, text)
                    // Flush on teardown — Windows focus quirk before editingFinished.
                    Component.onDestruction: if (typeof settings !== "undefined" && root.field.key !== undefined
                        && String(settings.get(root.field.key)) !== text) settings.set(root.field.key, text)
                }
            }
            Text { visible: root.field.suffix !== undefined; text: root.field.suffix || ""; color: Theme.t4; font.pixelSize: 11; font.family: Theme.fontMono }
        }
    }
    Component {
        id: cText
        TFld {
            implicitWidth: root.field.w === "grow" ? 300 : root.field.w === "w-md" ? 210 : root.field.w === "w-sm" ? 120 : 180
            implicitHeight: 30
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
            implicitWidth: segR.implicitWidth + 4
            implicitHeight: 29
            radius: 8
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
                        MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: { parent.parent.parent.curIdx = index; if (typeof settings !== "undefined" && root.field.key !== undefined) settings.set(root.field.key, index) } }
                    }
                }
            }
        }
    }
    Component {
        id: cButton
        BtnFlat { text: root.field.btn || ""; sm: false; onClicked: root.sw.runButtonAction(root.field.action) }
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
                return (typeof settings !== "undefined" && root.field.key !== undefined)
                    ? (settings.get(root.field.key) || "") : (root.field.value || "")
            }
            TFld {
                id: pathFld
                implicitWidth: 220; implicitHeight: 30; mono: true
                placeholder: root.field.placeholder || ""
                // Imperative text — settings.get() isn't reactive; binding wouldn't refresh on Browse.
                Component.onCompleted: text = pathRow.curPath()
                onEdited: function(t) {
                    if (typeof settings !== "undefined" && root.field.key !== undefined)
                        settings.set(root.field.key, t)
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
                onClicked: root.sw.openPathPicker(root.field.key, root.field.file === true)
            }
            BtnFlat {
                visible: root.field.file !== true
                text: (i18n.language, i18n.t("set_custom_clear")); sm: true
                onClicked: if (typeof settings !== "undefined" && root.field.key !== undefined) settings.set(root.field.key, "")
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
