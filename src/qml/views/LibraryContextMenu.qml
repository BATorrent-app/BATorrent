// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

import QtQuick
import QtQuick.Controls.Basic
import "../theme"
import "../widgets"

// Downloads right-click menu. Same structure for every item, with what does
// not apply left out: Play or Watch only when there is something to play;
// Pause upload, Pause or Resume by state; Extract only for archives; the
// Defender exclusion only for games; Fix cover only when there is one; "Mark
// as…" names the type (watched, played, done). Everything that copies sits in
// Copy ▸. session/i18n/torrentFilter are context properties; dialogs and host
// helpers are injected.
DMenu {
    id: root
    required property var host
    required property var controller
    required property var inputPrompt
    required property var removeDialog
    required property var setLocationDialog
    required property var exportDialog
    required property var diagnoseDialog

    // Read when the menu opens: the row under a right-click is a series group
    // (open or closed), and the selection covers more than one torrent. With
    // several, pause and resume both stay offered: the first one's state says
    // nothing about the rest.
    property bool onGroup: false
    property bool groupOpen: false
    property bool multi: false
    property string cat: ""
    onAboutToShow: {
        var row = controller.selected
        var keyed = controller.keyedRows
        onGroup = keyed && torrentFilter.isGroup(row)
        groupOpen = onGroup && torrentFilter.isGroupExpanded(row)
        multi = session.selectedRows().length > 1
        cat = session.selectedCategory()
    }

    // depend on selectedHash (NOTIFY selectionChanged): the QML row changes
    // before the C++ selection commits, and the menu must read the new one
    readonly property bool isGame: (session.selectedHash, session.selectedIsGame())
    readonly property int gameState: (session.selectedHash, session.selectedGameState())
    readonly property bool done: session.selectedDataDone || session.selectedCompleted
    // a direct download (a link, not a torrent): once finished it shares
    // nothing, so there is nothing left to pause or resume
    readonly property bool directDone: done && !multi && String(session.selectedInfoHash).indexOf("httpdl-") === 0
    readonly property string markKey: isGame || cat === "Games" ? "mark_played"
                                     : (session.selectedHasVideo || cat === "Movies" || cat === "Series") ? "mark_watched"
                                     : "ctx_mark_completed_plain"
    width: 248

    // ---- the one thing this item is for ----
    DMenuItem {
        hideWhenDisabled: true
        enabled: root.isGame && root.gameState !== 5
        tone: "primary"
        iconSrc: root.gameState === 4 ? "qrc:/icons/play.svg" : "qrc:/icons/download.svg"
        text: (i18n.language, root.gameState === 4 ? i18n.t("ov_act_play") : i18n.t("hub_gs_install"))
        onTriggered: root.gameState === 4 ? session.playSelectedGame() : session.installSelectedGame()
    }
    DMenuItem {
        hideWhenDisabled: true
        enabled: !root.isGame && session.selectedHasVideo
        tone: "primary"
        iconSrc: "qrc:/icons/play.svg"
        text: (i18n.language, i18n.t("ov_act_watch"))
        onTriggered: session.playSelected()
    }
    DMenuItem {
        hideWhenDisabled: true
        enabled: root.onGroup
        iconSrc: "qrc:/icons/chevron.svg"
        text: (i18n.language, root.groupOpen ? i18n.t("lib_group_close") : i18n.t("lib_group_open"))
        onTriggered: controller.toggleGroup(controller.selected)
    }
    // A finished item has no download to pause: what pausing stops there is
    // the upload, so it says so.
    DMenuItem {
        hideWhenDisabled: true
        enabled: (!session.selectedPaused || root.multi) && !root.directDone
        iconSrc: "qrc:/icons/pause.svg"
        text: (i18n.language, root.done ? i18n.t("ctx_pause_upload") : i18n.t("tb_pause"))
        onTriggered: session.pauseSelected()
    }
    DMenuItem {
        hideWhenDisabled: true
        enabled: (session.selectedPaused || root.multi) && !root.directDone
        iconSrc: root.done ? "qrc:/icons/upload.svg" : "qrc:/icons/play.svg"
        text: (i18n.language, root.done ? i18n.t("ctx_seed_again") : i18n.t("tb_resume"))
        onTriggered: session.resumeSelected()
    }
    DMenuSep {}
    DMenuItem { iconSrc: "qrc:/icons/folder.svg"; text: (i18n.language, i18n.t("ctx_open_folder")); shortcutText: "Ctrl+E"; onTriggered: session.openSaveFolder() }
    DMenuItem {
        hideWhenDisabled: true
        enabled: session.selectedHasArchives
        iconSrc: "qrc:/icons/download.svg"
        text: (i18n.language, i18n.t("ctx_extract"))
        onTriggered: inputPrompt.openWith(i18n.t("ctx_extract"), i18n.t("extract_password_label"),
                                          "", i18n.t("extract_password_ph"),
                                          function(pw){ session.extractSelected(pw) })
    }
    DMenuItem { iconSrc: "qrc:/icons/pencil.svg"; text: (i18n.language, i18n.t("ctx_rename")); shortcutText: "F2"; onTriggered: host.promptRenameSelected() }
    DMenuItem {
        // only once the data is done: marking mid-download freezes the torrent
        hideWhenDisabled: true
        enabled: root.done
        iconSrc: "qrc:/icons/check.svg"
        text: (i18n.language, session.selectedCompleted ? i18n.t("ctx_unmark_completed_plain") : i18n.t(root.markKey))
        onTriggered: session.selectedCompleted ? session.unmarkSelectedCompleted() : session.markSelectedCompleted()
    }
    DMenuItem {
        hideWhenDisabled: true
        enabled: Qt.platform.os === "windows" && (root.isGame || root.cat === "Games")
        iconSrc: "qrc:/icons/shield-check.svg"
        text: (i18n.language, i18n.t("ctx_defender_exclude"))
        // a group row has no torrent of its own: its first episode's folder
        // is the one the episodes share
        onTriggered: {
            var rows = typeof torrentFilter.sourceRowsAt === "function"
                       ? torrentFilter.sourceRowsAt(controller.selected)
                       : [torrentFilter.mapToSource(controller.selected)]
            if (rows.length > 0) session.excludeTorrentFromDefender(rows[0])
        }
    }
    DMenuSep {}

    DMenu {
        sub: true
        title: (i18n.language, i18n.t("ctx_grp_copy"))
        iconSrc: "qrc:/icons/copy.svg"
        implicitWidth: 220
        // a magnet for a torrent, the address for a direct download
        DMenuItem {
            readonly property string kind: session.selectedCopyKind
            iconSrc: kind === "magnet" ? "qrc:/icons/magnet.svg" : "qrc:/icons/link.svg"
            text: (i18n.language, kind === "magnet" ? i18n.t(root.multi ? "tb_copy_magnets" : "ctx_copy_magnet")
                                  : i18n.t(root.multi || kind === "mixed" ? "tb_copy_links" : "ctx_copy_link"))
            onTriggered: session.copyMagnetLink()
        }
        DMenuItem { iconSrc: "qrc:/icons/folder.svg"; text: (i18n.language, i18n.t("ctx_copy_path")); onTriggered: session.copySelectedContentPath() }
        DMenuItem { iconSrc: "qrc:/icons/abc.svg"; text: (i18n.language, i18n.t("ctx_copy_name")); onTriggered: session.copySelectedName() }
        // a direct download has no info hash
        DMenuItem { hideWhenDisabled: true; enabled: session.selectedCopyKind === "magnet"; iconSrc: "qrc:/icons/hash.svg"; text: (i18n.language, i18n.t("ctx_copy_hash")); onTriggered: session.copyInfoHash() }
    }
    DMenu {
        sub: true
        title: (i18n.language, i18n.t("ctx_grp_queue"))
        iconSrc: "qrc:/icons/sliders.svg"
        implicitWidth: 210
        DMenuItem { iconSrc: "qrc:/icons/chevrons-up.svg"; text: (i18n.language, i18n.t("ctx_queue_top")); onTriggered: session.queueTopSelected() }
        DMenuItem { iconSrc: "qrc:/icons/chevron-up.svg"; text: (i18n.language, i18n.t("ctx_queue_up")); onTriggered: session.queueUpSelected() }
        DMenuItem { iconSrc: "qrc:/icons/chevron.svg"; text: (i18n.language, i18n.t("ctx_queue_down")); onTriggered: session.queueDownSelected() }
        DMenuItem { iconSrc: "qrc:/icons/chevrons-down.svg"; text: (i18n.language, i18n.t("ctx_queue_bottom")); onTriggered: session.queueBottomSelected() }
    }
    DMenu {
        sub: true
        title: (i18n.language, i18n.t("ctx_grp_speed"))
        iconSrc: "qrc:/icons/arrow-down.svg"
        implicitWidth: 240
        DMenuItem { iconSrc: "qrc:/icons/download.svg"; text: (i18n.language, i18n.t("ctx_speed_down")); onTriggered: inputPrompt.openWith(i18n.t("ctx_speed_down"), i18n.t("prompt_speed_kbs"), String(session.selectedDownloadLimit()), "0", function(t){ session.setSelectedDownloadLimit(parseInt(t) || 0) }) }
        DMenuItem { iconSrc: "qrc:/icons/upload.svg"; text: (i18n.language, i18n.t("ctx_speed_up")); onTriggered: inputPrompt.openWith(i18n.t("ctx_speed_up"), i18n.t("prompt_speed_kbs"), String(session.selectedUploadLimit()), "0", function(t){ session.setSelectedUploadLimit(parseInt(t) || 0) }) }
        DMenuItem { iconSrc: "qrc:/icons/arrows-sort.svg"; checkable: true; checked: session.selectedSequential(); text: (i18n.language, i18n.t("ctx_sequential")); onTriggered: session.setSelectedSequential(!session.selectedSequential()) }
        DMenuItem { iconSrc: "qrc:/icons/bolt.svg"; checkable: true; checked: session.selectedForceStart; text: (i18n.language, i18n.t("ctx_force_start_plain")); onTriggered: session.setSelectedForceStart(!session.selectedForceStart) }
        // Only on a complete seed: libtorrent ignores the flag otherwise but
        // still reports it set, so the menu would tick a no-op.
        DMenuItem {
            hideWhenDisabled: true
            enabled: session.selectedDataDone
            iconSrc: "qrc:/icons/share.svg"
            checkable: true; checked: session.selectedSuperSeeding
            text: (i18n.language, i18n.t("ctx_super_seeding"))
            onTriggered: session.setSelectedSuperSeeding(!session.selectedSuperSeeding)
        }
    }
    DMenu {
        sub: true
        id: catMenu
        title: (i18n.language, i18n.t("ctx_grp_category_tags"))
        iconSrc: "qrc:/icons/hub.svg"
        implicitWidth: 220
        // The built-ins are static (catLabel translates them); categories the
        // user typed through "Other…" are appended when the menu opens.
        onAboutToShow: customCats.model = host.customCategories()
        Instantiator {
            id: customCats
            model: []
            delegate: DMenuItem {
                required property var modelData
                checkable: true
                checked: root.cat === modelData
                text: modelData
                onTriggered: session.setSelectedCategory(modelData)
            }
            onObjectAdded: function(index, object) { catMenu.insertItem(host.presetCats.length + index, object) }
            onObjectRemoved: function(index, object) { catMenu.removeItem(object) }
        }
        DMenuItem { checkable: true; checked: root.cat === "Apps";   text: host.catLabel("Apps");   onTriggered: session.setSelectedCategory("Apps") }
        DMenuItem { checkable: true; checked: root.cat === "Games";  text: host.catLabel("Games");  onTriggered: session.setSelectedCategory("Games") }
        DMenuItem { checkable: true; checked: root.cat === "Movies"; text: host.catLabel("Movies"); onTriggered: session.setSelectedCategory("Movies") }
        DMenuItem { checkable: true; checked: root.cat === "Series"; text: host.catLabel("Series"); onTriggered: session.setSelectedCategory("Series") }
        DMenuItem { checkable: true; checked: root.cat === "Compressed"; text: host.catLabel("Compressed"); onTriggered: session.setSelectedCategory("Compressed") }
        DMenuItem { checkable: true; checked: root.cat === "Music"; text: host.catLabel("Music"); onTriggered: session.setSelectedCategory("Music") }
        DMenuItem { checkable: true; checked: root.cat === "Images"; text: host.catLabel("Images"); onTriggered: session.setSelectedCategory("Images") }
        DMenuItem { checkable: true; checked: root.cat === "Documents"; text: host.catLabel("Documents"); onTriggered: session.setSelectedCategory("Documents") }
        DMenuItem { checkable: true; checked: root.cat === "Torrent"; text: host.catLabel("Torrent"); onTriggered: session.setSelectedCategory("Torrent") }
        DMenuSep {}
        DMenuItem { checkable: true; checked: root.cat === ""; text: (i18n.language, i18n.t("category_none")); onTriggered: session.setSelectedCategory("") }
        DMenuItem { text: (i18n.language, i18n.t("ctx_category_other")); onTriggered: inputPrompt.openWith(i18n.t("ctx_category"), i18n.t("prompt_category_name"), root.cat, i18n.t("prompt_category_eg"), function(t){ session.setSelectedCategory(t) }) }
        DMenuSep {}
        DMenuItem { text: (i18n.language, i18n.t("ctx_add_tag")); onTriggered: inputPrompt.openWith(i18n.t("prompt_add_tag_title"), i18n.t("prompt_new_tag"), "", i18n.t("prompt_tag_eg"), function(t){ if (t.length === 0) return; var tags = session.selectedTagList(); if (tags.indexOf(t) < 0) { tags.push(t); session.setSelectedTags(tags) } }) }
        DMenuItem { text: (i18n.language, i18n.t("tracker_add")); onTriggered: inputPrompt.openWith(i18n.t("prompt_add_tracker_title"), i18n.t("prompt_tracker_url"), "", "udp://tracker:porta", function(t){ session.addTrackerToSelected(t) }) }
    }
    DMenu {
        sub: true
        id: coverMenu
        title: (i18n.language, i18n.t("ctx_fix_cover"))
        iconSrc: "qrc:/icons/photo-off.svg"
        implicitWidth: 210
        DMenuItem { iconSrc: "qrc:/icons/movie.svg"; text: host.catLabel("Movies"); onTriggered: inputPrompt.openWith(i18n.t("ctx_fix_cover"), i18n.t("ctx_fix_cover_hint"), "", "Euphoria", function(t){ session.relinkSelectedCover(t, "movie") }) }
        DMenuItem { iconSrc: "qrc:/icons/device-tv.svg"; text: host.catLabel("Series"); onTriggered: inputPrompt.openWith(i18n.t("ctx_fix_cover"), i18n.t("ctx_fix_cover_hint"), "", "Euphoria", function(t){ session.relinkSelectedCover(t, "series") }) }
        DMenuItem { iconSrc: "qrc:/icons/device-gamepad-2.svg"; text: host.catLabel("Games"); onTriggered: inputPrompt.openWith(i18n.t("ctx_fix_cover"), i18n.t("ctx_fix_cover_hint"), "", "Cyberpunk 2077", function(t){ session.relinkSelectedCover(t, "game") }) }
        DMenuSep {}
        DMenuItem { iconSrc: "qrc:/icons/photo-off.svg"; text: (i18n.language, i18n.t("ctx_no_cover")); onTriggered: session.clearSelectedCover() }
    }
    DMenu {
        sub: true
        title: (i18n.language, i18n.t("ctx_grp_advanced"))
        iconSrc: "qrc:/icons/ellipsis.svg"
        implicitWidth: 240
        DMenuItem { iconSrc: "qrc:/icons/folder-symlink.svg"; text: (i18n.language, i18n.t("ctx_move_storage")); onTriggered: setLocationDialog.open() }
        DMenuItem { iconSrc: "qrc:/icons/refresh.svg"; text: (i18n.language, i18n.t("ctx_force_recheck")); onTriggered: session.forceRecheckSelected() }
        DMenuItem { iconSrc: "qrc:/icons/broadcast.svg"; text: (i18n.language, i18n.t("ctx_force_reannounce")); onTriggered: session.forceReannounceSelected() }
        DMenuItem { iconSrc: "qrc:/icons/file-export.svg"; text: (i18n.language, i18n.t("ctx_export_torrent")); onTriggered: exportDialog.open() }
        DMenuItem { iconSrc: "qrc:/icons/gauge.svg"; text: (i18n.language, i18n.t("ctx_why_slow")); onTriggered: { diagnoseDialog.body = session.diagnoseSelectedSlow(); diagnoseDialog.open() } }
        DMenuItem { iconSrc: "qrc:/icons/stop.svg"; text: (i18n.language, i18n.t("ctx_stop_seeding")); onTriggered: session.stopSeedingSelected() }
        DMenu {
            sub: true
            title: (i18n.language, i18n.t("ctx_seed_rules"))
            iconSrc: "qrc:/icons/upload.svg"
            implicitWidth: 240
            DMenuItem { iconSrc: "qrc:/icons/replay.svg"; text: (i18n.language, i18n.t("ctx_seed_use_default")); onTriggered: { session.setSelectedStopAfter(-1); session.setSelectedMaxSeedDays(-1) } }
            DMenuItem { iconSrc: "qrc:/icons/check.svg"; checkable: true; checked: session.selectedStopAfter() === 1; text: (i18n.language, i18n.t("ctx_stop_after_download")); onTriggered: session.setSelectedStopAfter(session.selectedStopAfter() === 1 ? 0 : 1) }
            DMenuItem { iconSrc: "qrc:/icons/clock.svg"; text: (i18n.language, i18n.t("ctx_max_seed_time")); onTriggered: inputPrompt.openWith(i18n.t("ctx_max_seed_time"), i18n.t("ctx_max_seed_prompt"), String(Math.max(0, session.selectedMaxSeedDays())), "0", function(t){ session.setSelectedMaxSeedDays(parseInt(t) || 0) }) }
        }
    }
    DMenuSep {}
    DMenuItem { iconSrc: "qrc:/icons/trash.svg"; tone: "danger"; text: (i18n.language, i18n.t("ctx_remove_short")); shortcutText: "Del"; onTriggered: removeDialog.open() }

    // Fix cover only means something when there is a cover. A submenu has
    // no `visible` of its own: its row is the menu item that opens it.
    Component.onCompleted: {
        for (var i = 0; i < count; ++i) {
            var it = itemAt(i)
            if (it && it.subMenu === coverMenu) {
                it.hideWhenDisabled = true
                it.enabled = Qt.binding(function () { return (session.selectedHash, session.selectedPoster.length > 0) })
            }
        }
    }
}
