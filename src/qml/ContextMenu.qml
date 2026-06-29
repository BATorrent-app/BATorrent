// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Right-click menu for a grid tile / list row. Carved out of Main.qml; reads window
// state via `win`, drives the shared input dialog passed in as `inputPrompt`, and
// raises the one-shot dialogs (remove/move/export/diagnose) as signals the parent
// wires. CtxItem/Sep are its own private styled menu primitives.
import QtQuick
import QtQuick.Controls.Basic
import "theme"
import "widgets"
Item {
    id: root
    property var win
    property var inputPrompt
    signal removeRequested()
    signal moveStorageRequested()
    signal exportRequested()
    signal diagnoseRequested()
    function popup() { ctxMenu.popup() }

    component CtxItem: MenuItem {
        id: ci
        implicitHeight: enabled ? 30 : 1
        visible: enabled
        padding: 0
        contentItem: Text {
            leftPadding: 14
            rightPadding: 14
            text: ci.text
            color: ci.highlighted ? Theme.t1 : Theme.t2
            font.pixelSize: 12
            font.family: Theme.fontSans
            verticalAlignment: Text.AlignVCenter
        }
        background: Rectangle {
            color: ci.highlighted ? Theme.hover : "transparent"
            radius: 5
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

    Menu {
        id: ctxMenu
        modal: true
        implicitWidth: 220
        background: Rectangle {
            color: Theme.panel
            border.color: Theme.hair
            border.width: 1
            radius: 8
        }
        delegate: CtxItem {}

        // Games lead the menu with an accent button (state-driven, Steam model):
        // Play when ready, else Install. A torrent is a game XOR a video, so only
        // one of gameCtx/playCtx is ever visible — both sit at the very top.
        MenuItem {
            id: gameCtx
            readonly property bool gReady: (win.selected, session.selectedGameState() === 4)
            visible: (win.selected, session.selectedIsGame() && session.selectedGameState() !== 5)
            height: visible ? 36 : 0
            implicitHeight: height
            padding: 0
            onTriggered: gameCtx.gReady ? session.playSelectedGame() : session.installSelectedGame()
            contentItem: Row {
                leftPadding: 12
                spacing: 9
                IconImg {
                    anchors.verticalCenter: parent.verticalCenter
                    src: gameCtx.gReady ? "qrc:/icons/play.svg" : "qrc:/icons/download.svg"; tint: Theme.accent; s: 13
                }
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: (i18n.language, gameCtx.gReady ? i18n.t("hub_gs_play") : i18n.t("hub_gs_install"))
                    color: Theme.accent
                    font.pixelSize: 13; font.weight: Font.DemiBold; font.family: Theme.fontSans
                }
            }
            background: Rectangle {
                radius: 6
                color: gameCtx.highlighted
                       ? Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.18)
                       : Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.07)
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
            contentItem: Row {
                leftPadding: 12
                spacing: 9
                IconImg {
                    anchors.verticalCenter: parent.verticalCenter
                    src: "qrc:/icons/play.svg"; tint: Theme.accent; s: 13
                }
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: (i18n.language, i18n.t("ctx_play"))
                    color: Theme.accent
                    font.pixelSize: 13; font.weight: Font.DemiBold; font.family: Theme.fontSans
                }
            }
            background: Rectangle {
                radius: 6
                color: playCtx.highlighted
                       ? Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.18)
                       : Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.07)
            }
        }

        // Common actions stay one click; the rest is grouped into submenus so
        // the menu doesn't run the whole height of the screen.
        CtxItem { text: (i18n.language, i18n.t("tb_pause")); enabled: !session.selectedPaused; onTriggered: session.pauseSelected() }
        CtxItem { text: (i18n.language, i18n.t("tb_resume")); enabled: session.selectedPaused; onTriggered: session.resumeSelected() }
        CtxItem { text: (i18n.language, i18n.t("ctx_open_folder")); onTriggered: session.openSaveFolder() }
        CtxItem { text: (i18n.language, i18n.t("ctx_copy_path")); onTriggered: session.copySelectedContentPath() }
        CtxItem {
            visible: Qt.platform.os === "windows"
            height: visible ? implicitHeight : 0
            text: (i18n.language, i18n.t("ctx_defender_exclude"))
            onTriggered: session.excludeTorrentFromDefender(torrentFilter.mapToSource(win.selected))
        }
        CtxItem {
            visible: session.selectedHasArchives
            height: visible ? implicitHeight : 0
            text: (i18n.language, i18n.t("ctx_extract"))
            onTriggered: inputPrompt.openWith(i18n.t("ctx_extract"), i18n.t("extract_password_label"),
                                              "", i18n.t("extract_password_ph"),
                                              function(pw){ session.extractSelected(pw) })
        }
        CtxItem { text: (i18n.language, i18n.t("ctx_rename")); onTriggered: inputPrompt.openWith(i18n.t("ctx_rename"), i18n.t("ctx_rename_prompt"), session.selectedName, "", function(t){ session.renameSelected(t) }) }
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
            CtxItem { text: (session.selectedSuperSeeding ? "✓ " : "") + (i18n.language, i18n.t("ctx_super_seeding")); onTriggered: session.setSelectedSuperSeeding(!session.selectedSuperSeeding) }
        }
        Menu {
            title: (i18n.language, i18n.t("ctx_grp_organize"))
            implicitWidth: 200
            delegate: CatItem {}
            background: Rectangle { color: Theme.panel; border.color: Theme.hair; border.width: 1; radius: 8 }
            CatItem { text: (session.selectedCategory() === "Apps"   ? "✓ " : "") + win.catLabel("Apps");   onTriggered: session.setSelectedCategory("Apps") }
            CatItem { text: (session.selectedCategory() === "Games"  ? "✓ " : "") + win.catLabel("Games");  onTriggered: session.setSelectedCategory("Games") }
            CatItem { text: (session.selectedCategory() === "Movies" ? "✓ " : "") + win.catLabel("Movies"); onTriggered: session.setSelectedCategory("Movies") }
            CatItem { text: (session.selectedCategory() === "Series" ? "✓ " : "") + win.catLabel("Series"); onTriggered: session.setSelectedCategory("Series") }
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
            CtxItem { text: (i18n.language, i18n.t("ctx_copy_magnet")); onTriggered: session.copyMagnetLink() }
            CtxItem { text: (i18n.language, i18n.t("ctx_copy_hash")); onTriggered: session.copyInfoHash() }
        }
        Menu {
            title: (i18n.language, i18n.t("ctx_fix_cover"))
            implicitWidth: 200
            delegate: CtxItem {}
            background: Rectangle { color: Theme.panel; border.color: Theme.hair; border.width: 1; radius: 8 }
            CtxItem { text: win.catLabel("Movies"); onTriggered: inputPrompt.openWith(i18n.t("ctx_fix_cover"), i18n.t("ctx_fix_cover_hint"), "", "Euphoria", function(t){ session.relinkSelectedCover(t, "movie") }) }
            CtxItem { text: win.catLabel("Series"); onTriggered: inputPrompt.openWith(i18n.t("ctx_fix_cover"), i18n.t("ctx_fix_cover_hint"), "", "Euphoria", function(t){ session.relinkSelectedCover(t, "series") }) }
            CtxItem { text: win.catLabel("Games"); onTriggered: inputPrompt.openWith(i18n.t("ctx_fix_cover"), i18n.t("ctx_fix_cover_hint"), "", "Cyberpunk 2077", function(t){ session.relinkSelectedCover(t, "game") }) }
            MenuSeparator { contentItem: Rectangle { implicitHeight: 1; color: Theme.hairSoft } }
            CtxItem { text: (i18n.language, i18n.t("ctx_no_cover")); onTriggered: session.clearSelectedCover() }
        }
        Menu {
            title: (i18n.language, i18n.t("ctx_grp_more"))
            implicitWidth: 230
            delegate: CtxItem {}
            background: Rectangle { color: Theme.panel; border.color: Theme.hair; border.width: 1; radius: 8 }
            CtxItem { text: (i18n.language, i18n.t("ctx_move_storage")); onTriggered: root.moveStorageRequested() }
            CtxItem { text: (i18n.language, i18n.t("ctx_force_recheck")); onTriggered: session.forceRecheckSelected() }
            CtxItem { text: (i18n.language, i18n.t("ctx_force_reannounce")); onTriggered: session.forceReannounceSelected() }
            CtxItem { text: (i18n.language, i18n.t("ctx_export_torrent")); onTriggered: root.exportRequested() }
            CtxItem { text: (i18n.language, i18n.t("ctx_why_slow")); onTriggered: root.diagnoseRequested() }
            CtxItem { text: session.selectedCompleted ? (i18n.language, i18n.t("ctx_unmark_completed_plain")) : (i18n.language, i18n.t("ctx_mark_completed_plain")); onTriggered: session.selectedCompleted ? session.unmarkSelectedCompleted() : session.markSelectedCompleted() }
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
        CtxItem { text: (i18n.language, i18n.t("ctx_remove")); onTriggered: root.removeRequested() }
    }
}
