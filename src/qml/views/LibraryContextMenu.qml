// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

import QtQuick
import QtQuick.Controls.Basic
import "../theme"
import "../widgets"

// Downloads library context menu. session/i18n/torrentFilter are context
// properties; dialogs and host helpers are injected as props.
Menu {
    id: root
    required property var host
    required property var controller
    required property var inputPrompt
    required property var removeDialog
    required property var setLocationDialog
    required property var exportDialog
    required property var diagnoseDialog

    component CatItem: MenuItem {
        id: ci
        implicitHeight: enabled ? 30 : 1
        visible: enabled
        padding: 0
        contentItem: Text {
            leftPadding: 14; rightPadding: 14
            text: ci.text
            color: ci.highlighted ? Theme.t1 : Theme.t2
            font.pixelSize: 12; font.family: Theme.fontSans
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }
        background: Rectangle {
            color: ci.highlighted ? Theme.hover : "transparent"
            radius: 5
        }
    }

    modal: true
    implicitWidth: 220
    // flush rows (the accent header reads as a band, not a floating pill);
    // just a breath of space under the last row so Remove isn't glued to
    // the card's bottom edge
    padding: 0
    bottomPadding: 6
    background: Rectangle {
        color: Theme.panel
        border.color: Theme.hair
        border.width: 1
        radius: 8
    }
    component CtxItem: MenuItem {
        id: ci
        property string iconSrc: ""
        // gutter aligns iconless rows (submenu titles) to the icon column
        property bool gutter: false
        // destructive: red icon + red text on hover (Remove)
        property bool destructive: false
        implicitHeight: enabled ? 30 : 1
        visible: enabled
        padding: 0
        contentItem: Item {
            IconImg {
                visible: ci.iconSrc !== ""
                src: ci.iconSrc; s: 14
                tint: ci.destructive ? Theme.accent
                                     : (ci.highlighted ? Theme.t1 : Theme.t3)
                x: 14
                anchors.verticalCenter: parent.verticalCenter
            }
            Text {
                anchors.fill: parent
                leftPadding: (ci.iconSrc !== "" || ci.gutter) ? 38 : 14
                rightPadding: 14
                text: ci.text
                color: ci.destructive && ci.highlighted ? Theme.accent
                      : ci.highlighted ? Theme.t1 : Theme.t2
                font.pixelSize: 12
                font.family: Theme.fontSans
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
            }
        }
        background: Rectangle {
            color: ci.highlighted
                   ? (ci.destructive ? Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.12) : Theme.hover)
                   : "transparent"
            radius: 5
            Behavior on color { ColorAnimation { duration: 80 } }
        }
        arrow: Text {
            visible: ci.subMenu
            text: "›"
            color: ci.highlighted ? Theme.t1 : Theme.t4
            font.pixelSize: 16
            font.family: Theme.fontSans
            x: ci.width - width - 12
            y: (ci.height - height) / 2
        }
    }
    component Sep: MenuSeparator { contentItem: Rectangle { implicitHeight: 1; color: Theme.hairSoft } }
    // submenu title rows are spawned from this delegate — the gutter keeps
    // their text on the same column as the icon rows above
    delegate: CtxItem { gutter: true }

    // gentle pop: fade + 4px rise, matching the app's page transitions
    enter: Transition {
        ParallelAnimation {
            NumberAnimation { property: "opacity"; from: 0.0; to: 1.0; duration: 120; easing.type: Easing.OutCubic }
            NumberAnimation { property: "y"; duration: 140; easing.type: Easing.OutCubic
                              from: root.y + 4; to: root.y }
        }
    }
    exit: Transition {
        NumberAnimation { property: "opacity"; from: 1.0; to: 0.0; duration: 80; easing.type: Easing.InCubic }
    }

    // Games lead the menu with an accent button (state-driven, Steam model):
    // Play when ready, else Install. A torrent is a game XOR a video, so only
    // one of gameCtx/playCtx is ever visible — both sit at the very top.
    MenuItem {
        id: gameCtx
        // depend on selectedHash (NOTIFY selectionChanged), not win.selected:
        // the QML row changes BEFORE the C++ selection commits, so binding to
        // it showed the PREVIOUS torrent's game state (movies got "Install")
        readonly property bool gReady: (session.selectedHash, session.selectedGameState() === 4)
        visible: (session.selectedHash, session.selectedIsGame() && session.selectedGameState() !== 5)
        height: visible ? 36 : 0
        implicitHeight: height
        padding: 0
        onTriggered: gameCtx.gReady ? session.playSelectedGame() : session.installSelectedGame()
        // same column geometry as CtxItem (icon x:14, text 38) so the
        // accent header lines up with the rows below it
        contentItem: Item {
            IconImg {
                x: 14
                anchors.verticalCenter: parent.verticalCenter
                src: gameCtx.gReady ? "qrc:/icons/play.svg" : "qrc:/icons/download.svg"; tint: Theme.accent; s: 14
            }
            Text {
                anchors.fill: parent
                leftPadding: 38
                text: (i18n.language, gameCtx.gReady ? i18n.t("hub_gs_play") : i18n.t("hub_gs_install"))
                color: Theme.accent
                font.pixelSize: 13; font.weight: Font.DemiBold; font.family: Theme.fontSans
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
            }
        }
        background: Rectangle {
            // full-width band: top corners follow the card's radius
            radius: 0
            topLeftRadius: 8; topRightRadius: 8
            color: gameCtx.highlighted
                   ? Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.18)
                   : Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.07)
            Behavior on color { ColorAnimation { duration: 80 } }
        }
    }
    // Play leads the menu as a minimalist accent button (not a plain row) so
    // the primary action for a video torrent stands out at a glance.
    MenuItem {
        id: playCtx
        visible: session.selectedHasVideo
        height: visible ? 36 : 0
        implicitHeight: height
        padding: 0
        onTriggered: session.playSelected()
        contentItem: Item {
            IconImg {
                x: 14
                anchors.verticalCenter: parent.verticalCenter
                src: "qrc:/icons/play.svg"; tint: Theme.accent; s: 14
            }
            Text {
                anchors.fill: parent
                leftPadding: 38
                text: (i18n.language, i18n.t("ctx_play"))
                color: Theme.accent
                font.pixelSize: 13; font.weight: Font.DemiBold; font.family: Theme.fontSans
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
            }
        }
        background: Rectangle {
            radius: 0
            topLeftRadius: 8; topRightRadius: 8
            color: playCtx.highlighted
                   ? Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.18)
                   : Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.07)
            Behavior on color { ColorAnimation { duration: 80 } }
        }
    }

    // Common actions stay one click; the rest is grouped into submenus so
    // the menu doesn't run the whole height of the screen.
    CtxItem { iconSrc: "qrc:/icons/pause.svg"; text: (i18n.language, i18n.t("ctx_pause_download")); enabled: !session.selectedPaused; onTriggered: session.pauseSelected() }
    CtxItem {
        // a completed torrent has no download to resume — what this action
        // actually does there is put it back to seeding; say so
        readonly property bool seedAgain: session.selectedDataDone || session.selectedCompleted
        iconSrc: seedAgain ? "qrc:/icons/upload.svg" : "qrc:/icons/play.svg"
        text: (i18n.language, seedAgain ? i18n.t("ctx_seed_again") : i18n.t("ctx_resume_download"))
        enabled: session.selectedPaused
        onTriggered: session.resumeSelected()
    }
    CtxItem { iconSrc: "qrc:/icons/open.svg"; text: (i18n.language, i18n.t("ctx_open_folder")); onTriggered: session.openSaveFolder() }
    // promoted out of the Copy submenu — the two most-reached actions (tester, MotrixNext ref)
    CtxItem { iconSrc: "qrc:/icons/magnet.svg"; text: (i18n.language, i18n.t("ctx_copy_magnet")); onTriggered: session.copyMagnetLink() }
    CtxItem { iconSrc: "qrc:/icons/copy.svg"; text: (i18n.language, i18n.t("ctx_copy_path")); onTriggered: session.copySelectedContentPath() }
    CtxItem {
        visible: Qt.platform.os === "windows"
        height: visible ? implicitHeight : 0
        iconSrc: "qrc:/icons/lock.svg"
        text: (i18n.language, i18n.t("ctx_defender_exclude"))
        onTriggered: session.excludeTorrentFromDefender(torrentFilter.mapToSource(controller.selected))
    }
    CtxItem {
        visible: session.selectedHasArchives
        height: visible ? implicitHeight : 0
        iconSrc: "qrc:/icons/download.svg"
        text: (i18n.language, i18n.t("ctx_extract"))
        onTriggered: inputPrompt.openWith(i18n.t("ctx_extract"), i18n.t("extract_password_label"),
                                          "", i18n.t("extract_password_ph"),
                                          function(pw){ session.extractSelected(pw) })
    }
    CtxItem { iconSrc: "qrc:/icons/pencil.svg"; text: (i18n.language, i18n.t("ctx_rename")); onTriggered: inputPrompt.openWith(i18n.t("ctx_rename"), i18n.t("ctx_rename_prompt"), session.selectedName, "", function(t){ session.renameSelected(t) }) }
    CtxItem {
        // only once the data is done — marking mid-download freezes the torrent
        visible: session.selectedDataDone || session.selectedCompleted
        height: visible ? implicitHeight : 0
        iconSrc: "qrc:/icons/check.svg"
        text: session.selectedCompleted ? (i18n.language, i18n.t("ctx_unmark_completed_plain")) : (i18n.language, i18n.t("ctx_mark_completed_plain"))
        onTriggered: session.selectedCompleted ? session.unmarkSelectedCompleted() : session.markSelectedCompleted()
    }
    Sep {}

    Menu {
        title: (i18n.language, i18n.t("ctx_grp_queue"))
        implicitWidth: 200
        delegate: CtxItem {}
        background: Rectangle { color: Theme.panel; border.color: Theme.hair; border.width: 1; radius: 8 }
        CtxItem { text: (i18n.language, i18n.t("ctx_queue_top")); onTriggered: session.queueTopSelected() }
        CtxItem { text: (i18n.language, i18n.t("ctx_queue_up")); onTriggered: session.queueUpSelected() }
        CtxItem { text: (i18n.language, i18n.t("ctx_queue_down")); onTriggered: session.queueDownSelected() }
        CtxItem { text: (i18n.language, i18n.t("ctx_queue_bottom")); onTriggered: session.queueBottomSelected() }
    }
    Menu {
        title: (i18n.language, i18n.t("ctx_grp_download"))
        implicitWidth: 230
        delegate: CtxItem {}
        background: Rectangle { color: Theme.panel; border.color: Theme.hair; border.width: 1; radius: 8 }
        CtxItem { text: (i18n.language, i18n.t("ctx_speed_down")); onTriggered: inputPrompt.openWith(i18n.t("ctx_speed_down"), i18n.t("prompt_speed_kbs"), String(session.selectedDownloadLimit()), "0", function(t){ session.setSelectedDownloadLimit(parseInt(t) || 0) }) }
        CtxItem { text: (i18n.language, i18n.t("ctx_speed_up")); onTriggered: inputPrompt.openWith(i18n.t("ctx_speed_up"), i18n.t("prompt_speed_kbs"), String(session.selectedUploadLimit()), "0", function(t){ session.setSelectedUploadLimit(parseInt(t) || 0) }) }
        CtxItem { text: (session.selectedSequential() ? "✓ " : "") + (i18n.language, i18n.t("ctx_sequential")); onTriggered: session.setSelectedSequential(!session.selectedSequential()) }
        CtxItem { text: (session.selectedForceStart ? "✓ " : "") + (i18n.language, i18n.t("ctx_force_start_plain")); onTriggered: session.setSelectedForceStart(!session.selectedForceStart) }
        // Only offered on a complete seed: libtorrent ignores the flag otherwise
        // ("if the torrent is not a seed, this flag has no effect") but still
        // reports it set, so the menu used to show a tick over a no-op. CtxItem
        // hides itself when disabled, which is the behaviour we want here.
        CtxItem {
            enabled: session.selectedDataDone
            text: (session.selectedSuperSeeding ? "✓ " : "") + (i18n.language, i18n.t("ctx_super_seeding"))
            onTriggered: session.setSelectedSuperSeeding(!session.selectedSuperSeeding)
        }
    }
    Menu {
        id: catMenu
        title: (i18n.language, i18n.t("ctx_grp_organize"))
        implicitWidth: 200
        delegate: CatItem {}
        background: Rectangle { color: Theme.panel; border.color: Theme.hair; border.width: 1; radius: 8 }

        // The four built-ins below are static (catLabel translates them).
        // Anything the user typed via "Other…" is appended here — before this,
        // a custom category was applied to the torrent but never appeared in
        // the list, so it looked like it hadn't been saved.
        onAboutToShow: customCats.model = host.customCategories()
        Instantiator {
            id: customCats
            model: []
            delegate: CatItem {
                required property var modelData
                text: (session.selectedCategory() === modelData ? "✓ " : "") + modelData
                onTriggered: session.setSelectedCategory(modelData)
            }
            onObjectAdded: function(index, object) { catMenu.insertItem(4 + index, object) }
            onObjectRemoved: function(index, object) { catMenu.removeItem(object) }
        }

        CatItem { text: (session.selectedCategory() === "Apps"   ? "✓ " : "") + host.catLabel("Apps");   onTriggered: session.setSelectedCategory("Apps") }
        CatItem { text: (session.selectedCategory() === "Games"  ? "✓ " : "") + host.catLabel("Games");  onTriggered: session.setSelectedCategory("Games") }
        CatItem { text: (session.selectedCategory() === "Movies" ? "✓ " : "") + host.catLabel("Movies"); onTriggered: session.setSelectedCategory("Movies") }
        CatItem { text: (session.selectedCategory() === "Series" ? "✓ " : "") + host.catLabel("Series"); onTriggered: session.setSelectedCategory("Series") }
        MenuSeparator { contentItem: Rectangle { implicitHeight: 1; color: Theme.hairSoft } }
        CatItem { text: (session.selectedCategory() === "" ? "✓ " : "") + (i18n.language, i18n.t("category_none")); onTriggered: session.setSelectedCategory("") }
        CatItem { text: (i18n.language, i18n.t("ctx_category_other")); onTriggered: inputPrompt.openWith(i18n.t("ctx_category"), i18n.t("prompt_category_name"), session.selectedCategory(), i18n.t("prompt_category_eg"), function(t){ session.setSelectedCategory(t) }) }
        MenuSeparator { contentItem: Rectangle { implicitHeight: 1; color: Theme.hairSoft } }
        CatItem { text: (i18n.language, i18n.t("ctx_add_tag")); onTriggered: inputPrompt.openWith(i18n.t("prompt_add_tag_title"), i18n.t("prompt_new_tag"), "", i18n.t("prompt_tag_eg"), function(t){ if (t.length === 0) return; var tags = session.selectedTagList(); if (tags.indexOf(t) < 0) { tags.push(t); session.setSelectedTags(tags) } }) }
        CatItem { text: (i18n.language, i18n.t("tracker_add")); onTriggered: inputPrompt.openWith(i18n.t("prompt_add_tracker_title"), i18n.t("prompt_tracker_url"), "", "udp://tracker:porta", function(t){ session.addTrackerToSelected(t) }) }
    }
    Menu {
        title: (i18n.language, i18n.t("ctx_grp_copy"))
        implicitWidth: 180
        delegate: CtxItem {}
        background: Rectangle { color: Theme.panel; border.color: Theme.hair; border.width: 1; radius: 8 }
        CtxItem { text: (i18n.language, i18n.t("ctx_copy_name")); onTriggered: session.copySelectedName() }
        CtxItem { text: (i18n.language, i18n.t("ctx_copy_hash")); onTriggered: session.copyInfoHash() }
    }
    Menu {
        title: (i18n.language, i18n.t("ctx_fix_cover"))
        implicitWidth: 200
        delegate: CtxItem {}
        background: Rectangle { color: Theme.panel; border.color: Theme.hair; border.width: 1; radius: 8 }
        CtxItem { text: host.catLabel("Movies"); onTriggered: inputPrompt.openWith(i18n.t("ctx_fix_cover"), i18n.t("ctx_fix_cover_hint"), "", "Euphoria", function(t){ session.relinkSelectedCover(t, "movie") }) }
        CtxItem { text: host.catLabel("Series"); onTriggered: inputPrompt.openWith(i18n.t("ctx_fix_cover"), i18n.t("ctx_fix_cover_hint"), "", "Euphoria", function(t){ session.relinkSelectedCover(t, "series") }) }
        CtxItem { text: host.catLabel("Games"); onTriggered: inputPrompt.openWith(i18n.t("ctx_fix_cover"), i18n.t("ctx_fix_cover_hint"), "", "Cyberpunk 2077", function(t){ session.relinkSelectedCover(t, "game") }) }
        MenuSeparator { contentItem: Rectangle { implicitHeight: 1; color: Theme.hairSoft } }
        CtxItem { text: (i18n.language, i18n.t("ctx_no_cover")); onTriggered: session.clearSelectedCover() }
    }
    Menu {
        title: (i18n.language, i18n.t("ctx_grp_more"))
        implicitWidth: 230
        delegate: CtxItem {}
        background: Rectangle { color: Theme.panel; border.color: Theme.hair; border.width: 1; radius: 8 }
        CtxItem { text: (i18n.language, i18n.t("ctx_move_storage")); onTriggered: setLocationDialog.open() }
        CtxItem { text: (i18n.language, i18n.t("ctx_force_recheck")); onTriggered: session.forceRecheckSelected() }
        CtxItem { text: (i18n.language, i18n.t("ctx_force_reannounce")); onTriggered: session.forceReannounceSelected() }
        CtxItem { text: (i18n.language, i18n.t("ctx_export_torrent")); onTriggered: exportDialog.open() }
        CtxItem { text: (i18n.language, i18n.t("ctx_why_slow")); onTriggered: { diagnoseDialog.body = session.diagnoseSelectedSlow(); diagnoseDialog.open() } }
        CtxItem { text: (i18n.language, i18n.t("ctx_stop_seeding")); onTriggered: session.stopSeedingSelected() }
        Menu {
            title: (i18n.language, i18n.t("ctx_seed_rules"))
            implicitWidth: 220
            delegate: CtxItem {}
            background: Rectangle { color: Theme.panel; border.color: Theme.hair; border.width: 1; radius: 8 }
            CtxItem { text: (i18n.language, i18n.t("ctx_seed_use_default")); onTriggered: { session.setSelectedStopAfter(-1); session.setSelectedMaxSeedDays(-1) } }
            CtxItem { text: (session.selectedStopAfter() === 1 ? "✓ " : "") + (i18n.language, i18n.t("ctx_stop_after_download")); onTriggered: session.setSelectedStopAfter(session.selectedStopAfter() === 1 ? 0 : 1) }
            CtxItem { text: (i18n.language, i18n.t("ctx_max_seed_time")); onTriggered: inputPrompt.openWith(i18n.t("ctx_max_seed_time"), i18n.t("ctx_max_seed_prompt"), String(Math.max(0, session.selectedMaxSeedDays())), "0", function(t){ session.setSelectedMaxSeedDays(parseInt(t) || 0) }) }
        }
    }
    Sep {}
    CtxItem { iconSrc: "qrc:/icons/trash.svg"; destructive: true; implicitHeight: enabled ? 34 : 1; text: (i18n.language, i18n.t("ctx_remove")); onTriggered: removeDialog.open() }
}
