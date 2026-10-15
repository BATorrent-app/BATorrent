// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// The first-run setup, over the whole window. Four questions (language,
// look, how the library reads, where downloads go) and a last page that
// offers the tour. Every answer is written the moment it is picked, and the
// wizard wears the theme being chosen, so the preview on the right and the
// page around it are the app itself.
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Dialogs
import QtQuick.Layouts
import "../theme"
import "../widgets"
import "../views"

Item {
    id: wiz
    anchors.fill: parent
    z: 300
    visible: opened || anim > 0

    // the window's own strip (title, window buttons) stays usable above it
    property real topInset: 0
    // the app itself, under the wizard: the preview is a live picture of it
    property Item appView: null
    property bool opened: false
    property real anim: 0
    Behavior on anim { NumberAnimation { duration: Theme.durSlower; easing.type: Theme.easeOut } }
    onOpenedChanged: {
        anim = opened ? 1 : 0
        if (opened) {
            DialogStack.push(wiz)
            step = 0
            refresh()
        } else DialogStack.pop(wiz)
    }
    Component.onDestruction: DialogStack.pop(wiz)

    signal accepted()      // finished: the host offers the tour
    signal rejected()      // closed early
    signal tourRequested()

    function open() { opened = true }
    function close() { opened = false }

    property int step: 0
    readonly property int lastStep: 3            // 4 is the done page
    // fade + 14px rise between steps
    property real stepIn: 1      // the fade, 380ms
    property real stepRise: 1    // the 14px rise, 480ms
    function go(n) {
        if (n === step || n < 0 || n > lastStep + 1) return
        step = n
        stepIn = 0
        stepRise = 0
        stepAnim.restart()
    }
    ParallelAnimation {
        id: stepAnim
        NumberAnimation { target: wiz; property: "stepIn"; from: 0; to: 1; duration: 380; easing.type: Easing.OutCubic }
        NumberAnimation { target: wiz; property: "stepRise"; from: 0; to: 1; duration: Theme.reduceMotion ? 0 : 480; easing.type: Easing.OutCubic }
    }

    // ---- what is set (mirrors, so the chips restate a choice at once) ----
    SettingsSchema { id: schema }
    property int contentLang: -1
    property bool classic: false
    property bool keepSharing: true
    property var folders: []
    function refresh() {
        if (typeof settings === "undefined") return
        var cv = settings.get("contentLanguage")
        contentLang = (cv === undefined || cv === null || cv === "" || cv < 0) ? -1 : cv
        classic = settings.getBool("classicMode")
        keepSharing = !settings.getBool("stopAfterDownload")
        folders = typeof session !== "undefined" ? session.typeFolders() : []
    }
    Connections {
        target: typeof settings !== "undefined" ? settings : null
        ignoreUnknownSignals: true
        function onChanged() { if (wiz.opened) wiz.refresh() }
    }
    Connections {
        target: typeof session !== "undefined" ? session : null
        ignoreUnknownSignals: true
        function onTypeFoldersChanged() { wiz.folders = session.typeFolders() }
    }

    readonly property var themeKeys: ["default", "cinema", "light", "midnight", "sakura", "darkstar", "matrix"]
    function themeName(k) {
        return k === "default" ? i18n.t("theme_default") : k === "cinema" ? i18n.t("theme_cinema")
             : k === "light" ? i18n.t("set_theme_light") : k === "midnight" ? "Midnight"
             : k === "sakura" ? "Sakura" : k === "darkstar" ? "Dark Star" : "Matrix"
    }
    readonly property string bgMode: typeof themeBridge !== "undefined" ? themeBridge.backgroundMode : "theme"
    readonly property bool themeHasArt: Theme.p && Theme.p.art ? true : false
    function shotFor(k) {
        var file = k === "cinema" ? "dark" : k
        var art = bgMode === "theme" && k !== "default"
        return "qrc:/images/theme-shots/" + file + "-" + (art ? "bg" : "clean") + ".png"
    }

    FolderDialog {
        id: folderDlg
        property string kind: ""
        onAccepted: if (typeof session !== "undefined")
            session.setTypeFolder(kind, session.urlToLocalPath(selectedFolder.toString()))
    }
    FileDialog {
        id: imageDlg
        nameFilters: ["Images (*.png *.jpg *.jpeg *.webp)"]
        onAccepted: if (typeof themeBridge !== "undefined" && typeof session !== "undefined") {
            themeBridge.userImage = session.urlToLocalPath(selectedFile.toString())
            themeBridge.backgroundMode = "user"
        }
    }

    Shortcut {
        sequences: [StandardKey.Cancel]
        enabled: wiz.opened && DialogStack.topItem === wiz
        onActivated: { wiz.rejected(); wiz.close() }
    }

    // ---- the page ----
    Rectangle {
        id: page
        x: 0; y: wiz.topInset
        width: parent.width; height: parent.height - wiz.topInset
        color: Theme.bg
        opacity: wiz.anim
        Behavior on color { ColorAnimation { duration: 500 } }
        // swallow everything meant for the app underneath
        MouseArea { anchors.fill: parent; acceptedButtons: Qt.AllButtons; onWheel: function (w) { w.accepted = true } }

        // the theme's art, fading out toward the text on the left
        ThemeArt { anchors.fill: parent; opacity: wiz.step === wiz.lastStep + 1 ? 0.6 : 1 }
        Rectangle {
            anchors.fill: parent
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0.0; color: Theme.bg }
                GradientStop { position: 0.3; color: Theme.bg }
                GradientStop { position: 0.62; color: Qt.rgba(Theme.bg.r, Theme.bg.g, Theme.bg.b, 0) }
            }
        }
        Image {
            anchors.fill: parent
            visible: Theme.scanlines
            source: "qrc:/images/themes/scanline.png"
            fillMode: Image.Tile
            opacity: 0.5
            z: 10
        }

        // logo row
        Row {
            x: 20; y: 10
            spacing: 10
            Image { source: Theme.logoSource; width: 26; height: 26; sourceSize: Qt.size(52, 52) }
            Text {
                visible: Theme.kanji
                text: "桜"
                color: Theme.accent
                font.pixelSize: 18; font.weight: Font.Bold; font.family: Theme.fontDisplay
                anchors.verticalCenter: parent.verticalCenter
            }
        }
        // close: leaving early is fine, everything here is in Settings too
        Rectangle {
            anchors.right: parent.right; anchors.rightMargin: 14; y: 8
            width: 32; height: 32; radius: Theme.rad(6)
            color: closeMa.containsMouse ? Theme.hover : "transparent"
            IconImg { anchors.centerIn: parent; src: "qrc:/icons/close.svg"; s: 14; tint: closeMa.containsMouse ? Theme.t1 : Theme.t3 }
            MouseArea { id: closeMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: { wiz.rejected(); wiz.close() } }
        }

        // ================= steps 1-4 =================
        Item {
            id: stepsArea
            visible: wiz.step <= wiz.lastStep
            x: 0; y: 46
            width: parent.width
            height: parent.height - 46 - 92

            // left column
            Flickable {
                id: leftScroll
                // 8px of slack round the column, so a picked tile's ring is not cut
                readonly property real pad: 8
                x: Math.min(64, page.width * 0.05) - pad
                y: 34 - pad
                width: Math.min(470, page.width * 0.42) + 2 * pad
                height: parent.height - 34 + pad
                contentHeight: left.implicitHeight + 16 + pad
                clip: true
                boundsBehavior: Flickable.StopAtBounds
                interactive: contentHeight > height

                ColumnLayout {
                    id: left
                    x: leftScroll.pad; y: leftScroll.pad
                    width: leftScroll.width - 2 * leftScroll.pad
                    spacing: 18
                    opacity: wiz.stepIn
                    transform: Translate { y: Theme.reduceMotion ? 0 : 14 * (1 - wiz.stepRise) }

                    Text {
                        text: (i18n.language, i18n.t("wiz_step")).arg(wiz.step + 1).arg(wiz.lastStep + 1)
                        color: Theme.accentText
                        font.pixelSize: 12; font.weight: Font.Bold; font.letterSpacing: 1.2; font.family: Theme.fontSans
                    }
                    Text {
                        Layout.fillWidth: true
                        text: (i18n.language, i18n.t(["wiz_t_lang", "wiz_t_look", "wiz_t_view", "wiz_t_where"][wiz.step]))
                        color: Theme.t1
                        font.family: Theme.fontDisplay
                        font.pixelSize: Theme.key === "matrix" ? 40 : Math.min(60, Theme.displaySize(50))
                        font.weight: Theme.tWeight(Font.Bold)
                        font.capitalization: Theme.tCaps
                        font.italic: Theme.tItalic
                        font.letterSpacing: Theme.tTrack(50, -1)
                        lineHeightMode: Text.FixedHeight
                        lineHeight: Math.round(font.pixelSize * 1.06)
                        wrapMode: Text.WordWrap
                    }
                    Text {
                        Layout.fillWidth: true
                        text: (i18n.language, i18n.t(["wiz_b_lang", "wiz_b_look", "wiz_b_view", "wiz_b_where"][wiz.step]))
                        color: Theme.t2
                        font.pixelSize: 16; font.family: Theme.fontSans
                        lineHeight: 1.5
                        wrapMode: Text.WordWrap
                    }

                    // ---- 1: language ----
                    ColumnLayout {
                        visible: wiz.step === 0
                        Layout.fillWidth: true
                        Layout.topMargin: 6
                        spacing: 20
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 9
                            Text { text: (i18n.language, i18n.t("wiz_lang_ui")); color: Theme.t1; font.pixelSize: 14; font.weight: Font.DemiBold; font.family: Theme.fontSans }
                            Flow {
                                Layout.fillWidth: true
                                spacing: 7
                                Repeater {
                                    model: schema.languageNames
                                    delegate: WizChip {
                                        text: modelData
                                        on: i18n.language === index
                                        onPicked: i18n.setLanguage(index)
                                    }
                                }
                            }
                        }
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 9
                            Text { text: (i18n.language, i18n.t("wiz_lang_content")); color: Theme.t1; font.pixelSize: 14; font.weight: Font.DemiBold; font.family: Theme.fontSans }
                            Flow {
                                Layout.fillWidth: true
                                spacing: 7
                                Repeater {
                                    model: [(i18n.language, i18n.t("set_content_language_same"))].concat(schema.languageNames)
                                    delegate: WizChip {
                                        text: modelData
                                        on: wiz.contentLang === index - 1
                                        onPicked: if (typeof settings !== "undefined") settings.set("contentLanguage", index - 1)
                                    }
                                }
                            }
                            Text {
                                Layout.fillWidth: true
                                text: (i18n.language, i18n.t("set_content_language_note"))
                                color: Theme.t3; font.pixelSize: 13; font.family: Theme.fontSans
                                wrapMode: Text.WordWrap; lineHeight: 1.4
                            }
                        }
                    }

                    // ---- 2: look ----
                    ColumnLayout {
                        visible: wiz.step === 1
                        Layout.fillWidth: true
                        Layout.topMargin: 4
                        spacing: 18
                        GridLayout {
                            Layout.fillWidth: true
                            columns: 4
                            columnSpacing: 10
                            rowSpacing: 12
                            Repeater {
                                model: wiz.themeKeys
                                delegate: ColumnLayout {
                                    id: tcell
                                    readonly property bool on: Theme.key === modelData
                                    Layout.fillWidth: true
                                    Layout.preferredWidth: 1
                                    spacing: 6
                                    Item {
                                        Layout.fillWidth: true
                                        Layout.preferredHeight: width * 352 / 560
                                        Rectangle {
                                            anchors.fill: parent
                                            anchors.margins: -4
                                            radius: 10
                                            color: "transparent"
                                            border.width: 2
                                            border.color: Theme.accent
                                            visible: tcell.on
                                        }
                                        RoundedImage {
                                            anchors.fill: parent
                                            radius: 7
                                            sources: [wiz.shotFor(modelData)]
                                            sourceWidth: 240
                                        }
                                        Rectangle {
                                            anchors.fill: parent
                                            radius: 7
                                            color: "transparent"
                                            border.width: 1
                                            border.color: Theme.hair
                                            visible: !tcell.on
                                        }
                                        transform: Translate { y: tcell.on ? -3 : 0; Behavior on y { NumberAnimation { duration: Theme.durSlow } } }
                                        MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: Theme.setName(modelData) }
                                    }
                                    Text {
                                        text: (i18n.language, wiz.themeName(modelData))
                                        color: tcell.on ? Theme.t1 : Theme.t3
                                        font.pixelSize: 13; font.weight: tcell.on ? Font.Bold : Font.Normal; font.family: Theme.fontSans
                                    }
                                }
                            }
                        }
                        ColumnLayout {
                            spacing: 9
                            Text { text: (i18n.language, i18n.t("wiz_bg")); color: Theme.t1; font.pixelSize: 14; font.weight: Font.DemiBold; font.family: Theme.fontSans }
                            Segmented {
                                optionHeight: 34
                                fontSize: 14
                                // Default has no art: "Theme art" says why it can't be picked
                                options: (i18n.language, [
                                    { value: "clean", label: i18n.t("wiz_bg_clean") },
                                    { value: "theme", label: i18n.t("wiz_bg_theme"), disabled: !wiz.themeHasArt, tip: wiz.themeHasArt ? "" : i18n.t("wiz_bg_none") },
                                    { value: "user", label: i18n.t("wiz_bg_user") }
                                ])
                                current: wiz.bgMode === "theme" && !wiz.themeHasArt ? "clean" : wiz.bgMode
                                onPicked: function (v) {
                                    if (typeof themeBridge === "undefined") return
                                    if (v === "user" && themeBridge.userImage === "") { imageDlg.open(); return }
                                    themeBridge.backgroundMode = v
                                }
                            }
                        }
                    }

                    // ---- 3: library ----
                    RowLayout {
                        visible: wiz.step === 2
                        Layout.fillWidth: true
                        Layout.topMargin: 6
                        spacing: 12
                        Repeater {
                            model: [false, true]
                            delegate: Rectangle {
                                id: vcard
                                readonly property bool isList: modelData
                                readonly property bool on: wiz.classic === isList
                                Layout.fillWidth: true
                                Layout.preferredWidth: 1
                                implicitHeight: vcol.implicitHeight + 28
                                radius: Theme.radCard(10)
                                color: on ? Qt.rgba(Theme.t1.r, Theme.t1.g, Theme.t1.b, 0.07) : Qt.rgba(Theme.panel.r, Theme.panel.g, Theme.panel.b, 0.82)
                                border.width: on ? 2 : 1
                                border.color: on ? Theme.t1 : Theme.hair
                                ColumnLayout {
                                    id: vcol
                                    x: 14; y: 14
                                    width: parent.width - 28
                                    spacing: 12
                                    Rectangle {
                                        Layout.fillWidth: true
                                        Layout.preferredHeight: 120
                                        radius: 6
                                        color: Theme.nav
                                        clip: true
                                        // grid: posters and a side panel
                                        Row {
                                            visible: !vcard.isList
                                            x: 9; y: 9
                                            spacing: 6
                                            Grid {
                                                columns: 4; spacing: 5
                                                Repeater {
                                                    model: 4
                                                    delegate: Rectangle {
                                                        width: (vcol.width - 18 - 6 - (vcol.width - 18) * 0.28 - 15) / 4
                                                        height: 68
                                                        radius: 3
                                                        color: [Theme.accent, Theme.panel, Theme.hover, Theme.t3][index]
                                                        opacity: index === 0 ? 0.7 : 1
                                                    }
                                                }
                                            }
                                            Rectangle { width: (vcol.width - 18) * 0.28; height: 102; radius: 4; color: Theme.panel }
                                        }
                                        // list: dense rows and a bottom panel
                                        Column {
                                            visible: vcard.isList
                                            x: 9; y: 9
                                            width: parent.width - 18
                                            spacing: 5
                                            Repeater {
                                                model: [Theme.down, Theme.doneFill, Theme.doneFill, Theme.t4, Theme.doneFill]
                                                delegate: Row {
                                                    spacing: 6
                                                    Rectangle { width: vcol.width - 18 - 40; height: 5; radius: 2; color: Theme.t3; opacity: 0.5; anchors.verticalCenter: parent.verticalCenter }
                                                    Rectangle { width: 34; height: 7; radius: 4; color: modelData }
                                                }
                                            }
                                            Rectangle { width: parent.width; height: 120 - 18 - 5 * 12 - 2; radius: 4; color: Theme.panel }
                                        }
                                    }
                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        spacing: 3
                                        Text { text: (i18n.language, i18n.t(vcard.isList ? "wiz_view_list" : "wiz_view_grid")); color: Theme.t1; font.pixelSize: 16; font.weight: Font.DemiBold; font.family: Theme.fontSans }
                                        Text {
                                            Layout.fillWidth: true
                                            text: (i18n.language, i18n.t(vcard.isList ? "wiz_view_list_d" : "wiz_view_grid_d"))
                                            color: Theme.t3; font.pixelSize: 13; font.family: Theme.fontSans
                                            wrapMode: Text.WordWrap; lineHeight: 1.35
                                        }
                                    }
                                }
                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: if (typeof settings !== "undefined") settings.set("classicMode", vcard.isList)
                                }
                            }
                        }
                    }

                    // ---- 4: where ----
                    ColumnLayout {
                        visible: wiz.step === 3
                        Layout.fillWidth: true
                        Layout.topMargin: 4
                        spacing: 14
                        Rectangle {
                            Layout.fillWidth: true
                            implicitHeight: fcol.implicitHeight
                            radius: Theme.radCard(10)
                            color: Qt.rgba(Theme.panel.r, Theme.panel.g, Theme.panel.b, 0.82)
                            border.color: Theme.hair; border.width: 1
                            Column {
                                id: fcol
                                width: parent.width
                                Repeater {
                                    model: wiz.folders
                                    delegate: Item {
                                        width: fcol.width
                                        height: 60
                                        Rectangle { visible: index > 0; width: parent.width; height: 1; color: Theme.hairSoft }
                                        ColumnLayout {
                                            x: 14
                                            width: parent.width - 28 - chBtn.width - 12
                                            anchors.verticalCenter: parent.verticalCenter
                                            spacing: 2
                                            Text {
                                                text: (i18n.language, i18n.t(modelData.kind === "game" ? "wiz_f_games" : modelData.kind === "video" ? "wiz_f_video" : "wiz_f_other"))
                                                color: Theme.t1; font.pixelSize: 14; font.weight: Font.DemiBold; font.family: Theme.fontSans
                                            }
                                            Row {
                                                Layout.fillWidth: true
                                                spacing: 0
                                                clip: true
                                                Text { id: fpath; text: modelData.path; color: Theme.t2; font.pixelSize: 13; font.family: Theme.fontMono; elide: Text.ElideMiddle; width: Math.min(implicitWidth, parent.width - ffree.implicitWidth) }
                                                Text { id: ffree; text: modelData.free !== "" ? "  ·  " + (i18n.language, i18n.t("addw_free")).arg(modelData.free) : ""; color: Theme.t3; font.pixelSize: 13; font.family: Theme.fontSans }
                                            }
                                        }
                                        BtnFlat {
                                            id: chBtn
                                            anchors.right: parent.right; anchors.rightMargin: 14
                                            anchors.verticalCenter: parent.verticalCenter
                                            text: (i18n.language, i18n.t("wiz_change"))
                                            onClicked: { folderDlg.kind = modelData.kind; folderDlg.open() }
                                        }
                                    }
                                }
                            }
                        }
                        Rectangle {
                            Layout.fillWidth: true
                            implicitHeight: seedCol.implicitHeight + 28
                            radius: Theme.radCard(10)
                            color: Qt.rgba(Theme.panel.r, Theme.panel.g, Theme.panel.b, 0.82)
                            border.color: Theme.hair; border.width: 1
                            RowLayout {
                                x: 14; y: 14
                                width: parent.width - 28
                                spacing: 16
                                ColumnLayout {
                                    id: seedCol
                                    Layout.fillWidth: true
                                    spacing: 5
                                    Text { text: (i18n.language, i18n.t("wiz_seed")); color: Theme.t1; font.pixelSize: 14; font.weight: Font.DemiBold; font.family: Theme.fontSans }
                                    Text {
                                        Layout.fillWidth: true
                                        text: (i18n.language, i18n.t("wiz_seed_body"))
                                        color: Theme.t3; font.pixelSize: 13; font.family: Theme.fontSans
                                        wrapMode: Text.WordWrap; lineHeight: 1.4
                                    }
                                }
                                TToggle {
                                    Layout.alignment: Qt.AlignTop
                                    on: wiz.keepSharing
                                }
                            }
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: if (typeof settings !== "undefined") settings.set("stopAfterDownload", wiz.keepSharing)
                            }
                        }
                    }
                }
            }

            // the preview, turned slightly toward the text
            Item {
                id: pvSlot
                x: leftScroll.x + leftScroll.width - leftScroll.pad + 56
                y: 34
                width: Math.max(0, page.width - x)
                height: parent.height - 34
                visible: width > 240
                clip: false
                Rectangle {
                    id: pv
                    // the window below its title strip, at the window's own shape
                    readonly property real srcW: wiz.appView ? wiz.appView.width : 560
                    readonly property real srcH: wiz.appView ? Math.max(1, wiz.appView.height - wiz.topInset) : 352
                    width: Math.min(820, Math.max(320, pvSlot.width - 24))
                    height: width * srcH / srcW
                    anchors.verticalCenter: parent.verticalCenter
                    radius: 12
                    color: Theme.nav
                    border.color: Theme.hair; border.width: 1
                    clip: true
                    transform: [
                        Rotation { origin.x: 0; origin.y: pv.height / 2; axis { x: 0; y: 1; z: 0 } angle: -11 },
                        Rotation { origin.x: 0; origin.y: pv.height / 2; axis { x: 1; y: 0; z: 0 } angle: 3 }
                    ]
                    // drawn flat into a texture and then turned: text laid out
                    // on the turned plane came out with crooked glyphs
                    layer.enabled: true
                    layer.smooth: true
                    layer.mipmap: true
                    layer.textureSize: Qt.size(width * 2, height * 2)
                    ShaderEffectSource {
                        anchors.fill: parent
                        anchors.margins: 1
                        sourceItem: wiz.opened ? wiz.appView : null
                        sourceRect: Qt.rect(0, wiz.topInset, pv.srcW, pv.srcH)
                        live: true
                        hideSource: false
                        smooth: true
                        mipmap: true
                        textureSize: Qt.size(pv.srcW, pv.srcH)
                    }
                }
            }
        }

        // ================= done =================
        ColumnLayout {
            visible: wiz.step > wiz.lastStep
            anchors.centerIn: parent
            width: Math.min(760, parent.width - 120)
            spacing: 26
            opacity: wiz.stepIn
            transform: Translate { y: Theme.reduceMotion ? 0 : 14 * (1 - wiz.stepRise) }
            Image { Layout.alignment: Qt.AlignHCenter; source: Theme.logoSource; Layout.preferredWidth: 88; Layout.preferredHeight: 88; sourceSize: Qt.size(176, 176) }
            Text {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                text: (i18n.language, i18n.t("wiz_done"))
                color: Theme.t1
                font.family: Theme.fontDisplay
                font.pixelSize: Theme.key === "matrix" ? 56 : Math.min(80, Theme.displaySize(70))
                font.weight: Theme.tWeight(Font.Bold)
                font.capitalization: Theme.tCaps
                font.italic: Theme.tItalic
                font.letterSpacing: Theme.tTrack(70, -1.4)
                wrapMode: Text.WordWrap
            }
            Text {
                Layout.alignment: Qt.AlignHCenter
                Layout.preferredWidth: Math.min(620, parent.width)
                horizontalAlignment: Text.AlignHCenter
                text: (i18n.language, i18n.t("wiz_done_body"))
                color: Theme.t2; font.pixelSize: 18; font.family: Theme.fontSans
                wrapMode: Text.WordWrap; lineHeight: 1.5
            }
            Flow {
                Layout.alignment: Qt.AlignHCenter
                Layout.preferredWidth: Math.min(implicitW, parent.width)
                readonly property real implicitW: { var w = 0; for (var i = 0; i < children.length; ++i) w += children[i].width + spacing; return w - spacing }
                spacing: 8
                Repeater {
                    model: [
                        ["wiz_sum_lang", schema.languageNames[i18n.language] || ""],
                        ["wiz_sum_theme", wiz.themeName(Theme.key)],
                        ["wiz_sum_view", i18n.t(wiz.classic ? "wiz_view_list" : "wiz_view_grid")],
                        ["wiz_sum_share", i18n.t(wiz.keepSharing ? "wiz_on" : "wiz_off")]
                    ]
                    delegate: Rectangle {
                        width: sumRow.implicitWidth + 28; height: 32
                        radius: Theme.rad(8)
                        color: Theme.panel
                        border.color: Theme.hair; border.width: 1
                        Row {
                            id: sumRow
                            anchors.centerIn: parent
                            spacing: 8
                            Text { text: (i18n.language, i18n.t(modelData[0])); color: Theme.t3; font.pixelSize: 14; font.family: Theme.fontSans }
                            Text { text: modelData[1]; color: Theme.t1; font.pixelSize: 14; font.weight: Font.DemiBold; font.family: Theme.fontSans }
                        }
                    }
                }
            }
            Row {
                Layout.alignment: Qt.AlignHCenter
                Layout.topMargin: 6
                spacing: 10
                WizButton {
                    text: (i18n.language, i18n.t("wiz_explore"))
                    onClicked: { wiz.accepted(); wiz.close() }
                }
                WizButton {
                    primary: true
                    text: (i18n.language, i18n.t("wiz_tour"))
                    onClicked: { wiz.close(); wiz.tourRequested() }
                }
            }
        }

        // ================= footer =================
        Item {
            visible: wiz.step <= wiz.lastStep
            anchors.bottom: parent.bottom
            width: parent.width
            height: 92
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Math.min(64, page.width * 0.05)
                anchors.rightMargin: 40
                spacing: 24
                Row {
                    spacing: 6
                    Repeater {
                        model: wiz.lastStep + 1
                        delegate: Rectangle {
                            width: index === wiz.step ? 34 : 14
                            height: 5
                            radius: 3
                            color: index <= wiz.step ? Theme.accent : Qt.rgba(Theme.t1.r, Theme.t1.g, Theme.t1.b, 0.18)
                            Behavior on width { NumberAnimation { duration: 400; easing.type: Easing.OutCubic } }
                            Behavior on color { ColorAnimation { duration: 300 } }
                            MouseArea { anchors.fill: parent; anchors.margins: -6; cursorShape: Qt.PointingHandCursor; onClicked: wiz.go(index) }
                        }
                    }
                }
                Text { text: (i18n.language, i18n.t("wiz_applied")); color: Theme.t3; font.pixelSize: 13; font.family: Theme.fontSans }
                Item { Layout.fillWidth: true }
                WizButton {
                    visible: wiz.step > 0
                    flat: true
                    text: (i18n.language, i18n.t("welcome_back"))
                    onClicked: wiz.go(wiz.step - 1)
                }
                WizButton {
                    primary: true
                    arrow: true
                    text: (i18n.language, i18n.t(wiz.step === wiz.lastStep ? "wiz_start" : "welcome_next"))
                    onClicked: wiz.go(wiz.step + 1)
                }
            }
        }
    }

    component WizChip: Rectangle {
        id: chip
        property string text
        property bool on: false
        signal picked()
        width: chipTxt.implicitWidth + 26
        height: 36
        radius: Theme.rad(8)
        color: on ? Qt.rgba(Theme.t1.r, Theme.t1.g, Theme.t1.b, 0.1) : (chipMa.containsMouse ? Qt.rgba(Theme.t1.r, Theme.t1.g, Theme.t1.b, 0.05) : "transparent")
        border.width: on ? 1.5 : 1
        border.color: on ? Theme.t1 : Theme.hair
        Behavior on color { ColorAnimation { duration: Theme.durBase } }
        Text {
            id: chipTxt
            anchors.centerIn: parent
            text: chip.text
            color: chip.on ? Theme.t1 : Theme.t2
            font.pixelSize: 14; font.weight: chip.on ? Font.DemiBold : Font.Normal; font.family: Theme.fontSans
        }
        MouseArea { id: chipMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: chip.picked() }
    }

    component WizButton: Rectangle {
        id: btn
        property string text
        property bool primary: false
        property bool flat: false
        property bool arrow: false
        signal clicked()
        width: btnRow.implicitWidth + (primary ? 52 : 44)
        height: flat ? 46 : 48
        radius: Theme.rad(8)
        color: primary ? (btnMa.containsMouse ? Qt.lighter(Theme.accent, 1.1) : Theme.accent)
                       : (btnMa.containsMouse ? Theme.hover : "transparent")
        border.width: primary || flat ? 0 : 1
        border.color: Theme.hair
        Behavior on color { ColorAnimation { duration: 300 } }
        Rectangle {
            visible: btn.primary && Theme.glow
            anchors.fill: parent; anchors.margins: -6
            radius: parent.radius + 6
            color: "transparent"
            border.width: 6; border.color: Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.25)
        }
        Row {
            id: btnRow
            anchors.centerIn: parent
            spacing: 10
            Text {
                text: btn.text
                color: btn.primary ? Theme.inkOnAccent : Theme.t2
                font.pixelSize: 16; font.weight: btn.primary ? Font.DemiBold : Font.Normal; font.family: Theme.fontSans
            }
            Text {
                visible: btn.arrow
                text: "→"
                color: Theme.inkOnAccent
                font.pixelSize: 15; font.family: Theme.fontSans
            }
        }
        MouseArea { id: btnMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: btn.clicked() }
    }
}
