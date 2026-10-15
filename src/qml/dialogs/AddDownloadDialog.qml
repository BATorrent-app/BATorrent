// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// One window to add anything and confirm it. A .torrent file is the main way
// in; a magnet, a direct download link or an info-hash goes in the paste
// field below it. One item opens its files in groups (game data, languages,
// extras), because that is where a download that does not fit gets fixed;
// several become a list where each says what it is.
//
// Magnets fetch their metadata in here: the engine holds them out of the
// library until Add, and Cancel drops them.
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Dialogs
import QtQuick.Layouts
import QtQuick.Shapes
import "../theme"
import "../widgets"

Item {
    id: dlg
    anchors.fill: parent
    z: 200
    visible: opened || anim > 0

    property bool opened: false
    property real anim: 0
    Behavior on anim { NumberAnimation { duration: 150; easing.type: Theme.easeOut } }
    onOpenedChanged: {
        anim = opened ? 1 : 0
        if (opened) {
            DialogStack.push(dlg)
            dlg.folders = typeof session !== "undefined" ? session.typeFolders() : []
            pasteArea.forceActiveFocus()
        } else DialogStack.pop(dlg)
    }
    Component.onDestruction: DialogStack.pop(dlg)

    signal chooseFilesRequested(string infoHash)
    signal freeSpaceRequested(double targetBytes, string name, string path)
    signal showInLibraryRequested(string infoHash)

    function open() { opened = true }
    function close() { opened = false }
    // Cancel, Esc, the backdrop: whatever was waiting in here goes away
    function reject() {
        for (var i = 0; i < items.count; ++i) dlg.drop(items.get(i))
        dlg.reset()
        close()
    }
    function reset() {
        items.clear()
        previews = ({})
        sel = ({})
        selRev++
        showTree = false
        destPath = ""
        destTouched = false
        pasteArea.text = ""
        gameWarn = false
    }
    // a clipboard magnet the window noticed: opened with it already in
    function openWithText(text) {
        if (!opened) reset()
        pasteArea.text = text
        open()
    }
    function openWithFiles(paths) {
        if (!opened) reset()
        addInputs(paths, false)
        open()
    }

    // ---- items ----
    // key: the info-hash where there is one, else the text itself
    ListModel { id: items }
    // ListModel roles are not bindable through get(): this counts every change
    property int itemsRev: 0
    Connections {
        target: items
        function onDataChanged() { dlg.itemsRev++ }
        function onCountChanged() { dlg.itemsRev++ }
    }
    property var previews: ({})      // key -> session preview map
    property int previewRev: 0
    property int tick: 0             // the fetching clocks
    Timer {
        interval: 1000; repeat: true
        running: dlg.opened && dlg.fetchingCount > 0
        onTriggered: dlg.tick++
    }
    readonly property int fetchingCount: { var n = 0; for (var i = 0; i < items.count; ++i) if (items.get(i).state === "fetching") ++n; return dlg.itemsRev >= 0 ? n : 0 }

    function indexOfKey(key) {
        for (var i = 0; i < items.count; ++i) if (items.get(i).key === key) return i
        return -1
    }
    function addInputs(list, fromPaste) {
        if (typeof session === "undefined") return
        for (var i = 0; i < list.length; ++i) {
            var raw = String(list[i]).trim()
            if (raw.length === 0) continue
            var d = session.describeInput(raw)
            if (!d || !d.kind) continue
            var key = d.hash ? d.hash : d.value
            if (indexOfKey(key) >= 0) continue
            var src = d.kind === "magnet" ? "magnet" : d.kind === "webFile" ? "direct" : "torrent"
            items.append({ key: key, src: src, kind: d.kind, value: d.value, line: raw, fromPaste: fromPaste,
                           path: d.kind === "torrentFile" ? d.value : "",
                           name: d.name || "", hash: d.hash || "", host: d.host || "",
                           joins: d.joins || "", poster: "", type: "", sizeBytes: 0, fileCount: 0,
                           sourceFile: "", resumable: false, state: "fetching", started: Date.now() })
            if (d.kind === "torrentFile") loadTorrent(key, d.value)
            else if (d.kind === "torrentUrl") session.fetchTorrentUrl(d.value)
            else if (d.kind === "webFile") session.probeUrl(d.value)
            else if (d.inLibrary) setField(key, "state", "dup")
            else session.stageMagnet(d.value)
        }
        dlg.pickDest()
    }
    function setField(key, role, v) {
        var i = indexOfKey(key)
        if (i >= 0) items.setProperty(i, role, v)
    }
    function loadTorrent(key, path) {
        var p = session.previewTorrent(path)
        if (!p || !p.ok) { setField(key, "state", "failed"); return }
        var i = indexOfKey(key)
        if (i < 0) return
        // the item is known by its hash from here on
        if (p.infoHash && indexOfKey(p.infoHash) >= 0 && indexOfKey(p.infoHash) !== i) { items.remove(i); return }
        if (p.infoHash) items.setProperty(i, "key", p.infoHash)
        items.setProperty(i, "hash", p.infoHash || "")
        items.setProperty(i, "path", path)
        applyPreview(p.infoHash || key, p)
    }
    function applyPreview(key, p) {
        var i = indexOfKey(key)
        if (i < 0) return
        var m = previews
        m[key] = p
        previews = m
        previewRev++
        items.setProperty(i, "name", p.cleanTitle || p.name || "")
        items.setProperty(i, "sizeBytes", p.totalSizeBytes || 0)
        items.setProperty(i, "fileCount", p.fileCount || 0)
        items.setProperty(i, "type", p.kind || "other")
        items.setProperty(i, "joins", p.joins || "")
        if (p.sourceFile) items.setProperty(i, "sourceFile", p.sourceFile)
        items.setProperty(i, "poster", p.posterPath ? posterUrl(p.posterPath) : "")
        items.setProperty(i, "state", p.inLibrary ? "dup" : "ready")
        if (!p.posterPath && p.infoHash) session.resolvePreview(p.infoHash, p.name)
        if (items.count === 1) initSelection(p)
        dlg.pickDest()
    }
    function posterUrl(path) {
        return (Qt.platform.os === "windows" ? "file:///" : "file://") + encodeURI(path)
    }
    // what leaves with it: a magnet waiting in the engine
    function drop(it) {
        if (it.src === "magnet" && it.hash && it.state !== "dup" && typeof session !== "undefined")
            session.discardStaged(it.hash)
    }
    function removeAt(i) {
        var it = items.get(i)
        drop(it)
        var line = it.line, fromPaste = it.fromPaste
        items.remove(i)
        if (fromPaste) {
            pasteSync.enabled = false
            pasteArea.text = pasteArea.text.split("\n").filter(function (l) { return l.trim() !== line }).join("\n")
            pasteSync.enabled = true
        }
        if (items.count === 1) {
            var p = previews[items.get(0).key]
            if (p) initSelection(p)
        }
    }
    // The paste field is the list's other half: a line removed there leaves
    // the list, a line added joins it.
    function syncPaste() {
        var lines = pasteArea.text.split("\n").map(function (l) { return l.trim() }).filter(function (l) { return l.length > 0 })
        for (var i = items.count - 1; i >= 0; --i) {
            var it = items.get(i)
            if (it.fromPaste && lines.indexOf(it.line) < 0) { drop(it); items.remove(i) }
        }
        var fresh = []
        for (var j = 0; j < lines.length; ++j) {
            var seen = false
            for (var k = 0; k < items.count; ++k) if (items.get(k).line === lines[j]) { seen = true; break }
            if (!seen) fresh.push(lines[j])
        }
        addInputs(fresh, true)
    }
    Timer { id: pasteTimer; interval: 250; onTriggered: dlg.syncPaste() }

    Connections {
        target: typeof session !== "undefined" ? session : null
        ignoreUnknownSignals: true
        function onStagedPreview(hash, p) { if (dlg.indexOfKey(hash) >= 0) dlg.applyPreview(hash, p) }
        function onPreviewPosterReady(hash, poster) {
            var i = dlg.indexOfKey(hash)
            if (i < 0) return
            items.setProperty(i, "poster", dlg.posterUrl(poster))
            var p = dlg.previews[hash]
            if (p) p.posterPath = poster
        }
        function onUrlProbed(url, info) {
            var i = dlg.indexOfKey(url)
            if (i < 0) return
            items.setProperty(i, "state", "ready")
            if (info.name) items.setProperty(i, "name", info.name)
            items.setProperty(i, "sizeBytes", info.sizeBytes > 0 ? info.sizeBytes : 0)
            items.setProperty(i, "resumable", info.resumable === true)
            items.setProperty(i, "type", info.kind || "other")
            if (info.host) items.setProperty(i, "host", info.host)
            dlg.pickDest()
        }
        function onTorrentUrlFetched(url, path) {
            var i = dlg.indexOfKey(url)
            if (i < 0) return
            if (path === "") { items.setProperty(i, "state", "failed"); return }
            dlg.loadTorrent(url, path)
        }
        function onTypeFoldersChanged() { dlg.folders = session.typeFolders() }
    }

    // ---- one item: its files ----
    readonly property bool single: items.count === 1 && itemsRev >= 0 && previewRev >= 0 && previews[items.get(0).key] !== undefined
                                   && items.get(0).state === "ready"
    readonly property var preview: single ? previews[items.get(0).key] : null
    readonly property string singlePoster: single && itemsRev >= 0 ? items.get(0).poster : ""
    // the language someone reads: the content language, else the app's
    readonly property string wantLang: {
        var codes = ["en", "pt", "zh", "ja", "ru", "es", "de", "uk", "tr"]
        var cl = typeof settings !== "undefined" ? settings.get("contentLanguage") : -1
        var li = (cl === undefined || cl === null || cl === "" || cl < 0) ? i18n.language : cl
        return codes[li] || "en"
    }
    property var sel: ({})           // fileIndex -> on
    property int selRev: 0
    property bool showTree: false
    property bool gameWarn: false
    property var fileSize: []
    function initSelection(p) {
        var s = {}, sz = []
        var fs = p.files || []
        for (var i = 0; i < fs.length; ++i) {
            if (fs[i].dir || fs[i].fileIndex < 0) continue
            s[fs[i].fileIndex] = true
            sz[fs[i].fileIndex] = fs[i].sizeBytes || 0
        }
        sel = s
        fileSize = sz
        selRev++
        gameWarn = false
    }
    function setFiles(list, v) {
        var s = sel
        for (var i = 0; i < list.length; ++i) s[list[i]] = v
        sel = s
        selRev++
    }
    function countOn(list) {
        var n = 0
        for (var i = 0; i < list.length; ++i) if (sel[list[i]]) ++n
        return selRev >= 0 ? n : 0
    }
    function bytesOn(list) {
        var b = 0
        for (var i = 0; i < list.length; ++i) if (sel[list[i]]) b += fileSize[list[i]] || 0
        return selRev >= 0 ? b : 0
    }
    readonly property int selCount: { var n = 0; for (var k in sel) if (sel[k]) ++n; return selRev >= 0 ? n : 0 }
    readonly property double selBytes: { var b = 0; for (var k in sel) if (sel[k]) b += fileSize[k] || 0; return selRev >= 0 ? b : 0 }
    function priorities() {
        if (!preview || selCount === preview.fileCount) return []
        var out = []
        for (var i = 0; i < preview.fileCount; ++i) out.push(sel[i] ? 4 : 0)
        return out
    }

    // ---- how much, and where ----
    readonly property double needBytes: {
        if (single) return selBytes
        var b = 0
        for (var i = 0; i < items.count; ++i) {
            var it = items.get(i)
            if (it.state !== "dup" && it.state !== "failed") b += it.sizeBytes
        }
        return itemsRev >= 0 ? b : 0
    }
    readonly property int addCount: {
        var n = 0
        for (var i = 0; i < items.count; ++i) {
            var s = items.get(i).state
            if (s !== "dup" && s !== "failed") ++n
        }
        return itemsRev >= 0 ? n : 0
    }
    readonly property int skippedCount: {
        var n = 0
        for (var i = 0; i < items.count; ++i) if (items.get(i).state === "dup") ++n
        return itemsRev >= 0 ? n : 0
    }

    property var folders: []
    property var customDests: []
    property string destPath: ""
    property bool destTouched: false
    property int freeRev: 0
    Timer { interval: 2000; repeat: true; running: dlg.opened; onTriggered: dlg.freeRev++ }
    readonly property var dests: {
        var out = [], seen = {}
        var all = folders.concat(customDests)
        for (var i = 0; i < all.length; ++i) {
            var f = all[i]
            var k = String(f.path).toLowerCase()
            if (seen[k]) continue
            seen[k] = true
            out.push({ path: f.path, name: f.name })
        }
        return out
    }
    function freeAt(path) { return typeof session !== "undefined" && freeRev >= 0 ? session.freeBytesAt(path) : -1 }
    function fits(path) { var f = freeAt(path); return f < 0 || needBytes <= f }
    function destName(path) {
        for (var i = 0; i < dests.length; ++i) if (dests[i].path === path) return dests[i].name
        return path
    }
    // The folder for what is being added: a game goes where games go. The
    // biggest item decides for a mixed list.
    function pickDest() {
        if (destTouched || typeof session === "undefined") return
        var kind = "other", big = -1
        for (var i = 0; i < items.count; ++i) {
            var it = items.get(i)
            if (it.type && it.sizeBytes > big) { big = it.sizeBytes; kind = it.type }
        }
        var p = session.folderFor(kind)
        for (var j = 0; j < folders.length; ++j)
            if (String(folders[j].path).toLowerCase() === String(p).replace(/\//g, "\\").toLowerCase()
                || String(folders[j].path).toLowerCase() === String(p).toLowerCase()) { destPath = folders[j].path; return }
        destPath = p
    }
    onFoldersChanged: if (destPath === "" && folders.length > 0) pickDest()

    readonly property bool destFits: destPath === "" || fits(destPath)
    readonly property var fitsElsewhere: {
        if (destFits) return null
        for (var i = 0; i < dests.length; ++i)
            if (dests[i].path !== destPath && fits(dests[i].path)) return dests[i]
        return null
    }
    // The person's own language, when the repack has it as a pack of its own
    readonly property var keepChip: {
        if (!single || !preview) return null
        var want = wantLang
        var gs = preview.groups || []
        for (var g = 0; g < gs.length; ++g) {
            if (gs[g].kind !== "lang") continue
            var match = null
            for (var c = 0; c < gs[g].chips.length; ++c) {
                var ch = gs[g].chips[c]
                if (String(ch.lang).split("-")[0] === want && (!match || (want === "pt" && ch.lang === "pt-BR"))) match = ch
            }
            if (!match || match.inBase) return null
            var saves = bytesOn(gs[g].files) - bytesOn(match.files)
            return saves > 0 ? { chip: match, group: gs[g], saves: saves } : null
        }
        return null
    }

    function commit(paused, choose) {
        if (typeof session === "undefined") return
        var dest = destPath !== "" ? destPath : session.defaultSavePath()
        var prios = single ? priorities() : []
        var lastHash = ""
        for (var i = 0; i < items.count; ++i) {
            var it = items.get(i)
            if (it.state === "dup" || it.state === "failed") continue
            var hold = paused || choose
            if (it.src === "magnet") session.commitStaged(it.hash, dest, prios, hold)
            else if (it.src === "direct") session.addHttpUrl(it.value, dest)
            else if (it.path !== "") session.addTorrentFileEx(it.path, dest, prios, hold)
            else session.addAnything(it.value, dest)      // a .torrent link that never arrived
            if (it.hash) lastHash = it.hash
        }
        items.clear()                 // added: nothing left to discard
        reset()
        close()
        if (choose && lastHash !== "") chooseFilesRequested(lastHash)
    }

    function fmt(b) { return typeof session !== "undefined" ? session.humanSize(b) : "" }
    function clock(ms) {
        var s = Math.max(0, Math.floor((Date.now() - ms) / 1000))
        return Math.floor(s / 60) + ":" + (s % 60 < 10 ? "0" : "") + (s % 60)
    }
    function typeLabel(t) {
        return t === "game" ? i18n.t("addw_type_game") : t === "video" ? i18n.t("addw_type_video") : ""
    }

    Shortcut {
        sequences: [StandardKey.Cancel]
        enabled: dlg.opened && DialogStack.topItem === dlg
        onActivated: dlg.reject()
    }

    FileDialog {
        id: browseDlg
        fileMode: FileDialog.OpenFiles
        title: (i18n.language, i18n.t("dlg_open_torrent"))
        nameFilters: [(i18n.language, i18n.t("filter_torrent_files")), (i18n.language, i18n.t("filter_all_files"))]
        onAccepted: {
            var l = []
            for (var i = 0; i < selectedFiles.length; ++i) l.push(selectedFiles[i].toString())
            dlg.addInputs(l, false)
        }
    }
    FolderDialog {
        id: otherDlg
        onAccepted: if (typeof session !== "undefined") {
            var p = session.urlToLocalPath(selectedFolder.toString())
            var parts = p.split(/[\\\/]/).filter(function (s) { return s.length > 0 })
            var c = dlg.customDests.slice()
            c.push({ path: p, name: parts.length > 0 ? parts[parts.length - 1] : p })
            dlg.customDests = c
            dlg.destPath = p
            dlg.destTouched = true
        }
    }

    // ---- backdrop ----
    Rectangle {
        anchors.fill: parent
        opacity: dlg.anim
        color: Theme.isDark ? Qt.rgba(0, 0, 0, 0.6) : Qt.rgba(20/255, 20/255, 28/255, 0.36)
        MouseArea { anchors.fill: parent; onClicked: dlg.reject(); onWheel: function (w) { w.accepted = true } }
    }

    // the whole window takes a drop while this is open
    DropArea {
        id: dropAll
        anchors.fill: parent
        onEntered: function (drag) {
            var ok = false
            if (typeof session !== "undefined") {
                if (drag.hasUrls) for (var i = 0; i < drag.urls.length; ++i) if (session.inputKind(drag.urls[i].toString()) !== "") ok = true
                if (!ok && drag.hasText) ok = session.inputKind(drag.text) !== ""
            }
            drag.accepted = ok
        }
        onDropped: function (drop) {
            var l = []
            if (drop.hasUrls) for (var i = 0; i < drop.urls.length; ++i) l.push(drop.urls[i].toString())
            else if (drop.hasText) l = drop.text.split("\n")
            dlg.addInputs(l, false)
            drop.accept()
        }
    }

    // ---- card ----
    Rectangle {
        id: card
        anchors.centerIn: parent
        width: Math.min(600, dlg.width - 32)
        height: Math.min(col.implicitHeight, dlg.height - 48)
        opacity: dlg.anim
        scale: Theme.reduceMotion ? 1 : (0.97 + 0.03 * dlg.anim)
        radius: Theme.rad(12)
        color: Theme.elev
        border.color: Theme.isDark ? Qt.rgba(1, 1, 1, 0.1) : Qt.rgba(0, 0, 0, 0.14)
        border.width: 1
        clip: true
        MouseArea { anchors.fill: parent; onWheel: function (w) { w.accepted = true } }

        ColumnLayout {
            id: col
            anchors.fill: parent
            spacing: 0

            // title
            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: 56
                Text {
                    anchors.left: parent.left; anchors.leftMargin: 22
                    anchors.verticalCenter: parent.verticalCenter
                    text: (i18n.language, i18n.t("addw_title"))
                    color: Theme.t1; font.pixelSize: 17; font.weight: Font.DemiBold; font.family: Theme.fontSans
                }
                Rectangle {
                    anchors.right: parent.right; anchors.rightMargin: 12
                    anchors.verticalCenter: parent.verticalCenter
                    width: 32; height: 32; radius: Theme.rad(6)
                    color: xMa.containsMouse ? Theme.hover : "transparent"
                    IconImg { anchors.centerIn: parent; src: "qrc:/icons/close.svg"; s: 14; tint: xMa.containsMouse ? Theme.t1 : Theme.t3 }
                    MouseArea { id: xMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: dlg.reject() }
                }
            }

            Flickable {
                id: bodyScroll
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredHeight: body.implicitHeight + 22
                clip: true
                contentWidth: width
                contentHeight: body.implicitHeight + 22
                boundsBehavior: Flickable.StopAtBounds
                ScrollBar.vertical: ScrollBar { policy: bodyScroll.contentHeight > bodyScroll.height ? ScrollBar.AsNeeded : ScrollBar.AlwaysOff }

                ColumnLayout {
                    id: body
                    x: 22; y: 2
                    width: bodyScroll.width - 44
                    spacing: 16

                    // ---- drop ----
                    Item {
                        Layout.fillWidth: true
                        Layout.preferredHeight: items.count === 0 ? 170 : 44
                        Behavior on Layout.preferredHeight { enabled: !Theme.reduceMotion; NumberAnimation { duration: Theme.durBase; easing.type: Theme.easeOut } }
                        Rectangle {
                            anchors.fill: parent
                            radius: Theme.rad(items.count === 0 ? 10 : 9)
                            color: dropAll.containsDrag ? Theme.accentTint : (items.count === 0 ? Theme.bg : "transparent")
                        }
                        Shape {
                            id: dash
                            anchors.fill: parent
                            readonly property real r: Theme.rad(10)
                            readonly property real e: 0.75
                            ShapePath {
                                strokeColor: dropAll.containsDrag ? Theme.accent
                                             : Theme.isDark ? Qt.rgba(1, 1, 1, items.count === 0 ? 0.22 : 0.18) : Qt.rgba(0, 0, 0, 0.25)
                                strokeWidth: 1.5
                                strokeStyle: ShapePath.DashLine
                                dashPattern: [4, 3]
                                fillColor: "transparent"
                                startX: dash.r; startY: dash.e
                                PathLine { x: dash.width - dash.r; y: dash.e }
                                PathArc { x: dash.width - dash.e; y: dash.r; radiusX: dash.r - dash.e; radiusY: dash.r - dash.e }
                                PathLine { x: dash.width - dash.e; y: dash.height - dash.r }
                                PathArc { x: dash.width - dash.r; y: dash.height - dash.e; radiusX: dash.r - dash.e; radiusY: dash.r - dash.e }
                                PathLine { x: dash.r; y: dash.height - dash.e }
                                PathArc { x: dash.e; y: dash.height - dash.r; radiusX: dash.r - dash.e; radiusY: dash.r - dash.e }
                                PathLine { x: dash.e; y: dash.r }
                                PathArc { x: dash.r; y: dash.e; radiusX: dash.r - dash.e; radiusY: dash.r - dash.e }
                            }
                        }
                        // empty: the big target
                        Column {
                            anchors.centerIn: parent
                            spacing: 12
                            visible: items.count === 0
                            Column {
                                anchors.horizontalCenter: parent.horizontalCenter
                                spacing: 4
                                Text {
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    text: (i18n.language, i18n.t("addw_drop"))
                                    color: Theme.t1; font.pixelSize: 17; font.weight: Font.DemiBold; font.family: Theme.fontSans
                                }
                                Text {
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    text: (i18n.language, i18n.t("addw_drop_sub"))
                                    color: Theme.t3; font.pixelSize: 13; font.family: Theme.fontSans
                                }
                            }
                            Rectangle {
                                anchors.horizontalCenter: parent.horizontalCenter
                                width: browseTxt.implicitWidth + 32; height: 36
                                radius: Theme.rad(8)
                                color: browseMa.containsMouse ? Qt.darker(Theme.t1, 1.08) : Theme.t1
                                Text {
                                    id: browseTxt
                                    anchors.centerIn: parent
                                    text: (i18n.language, i18n.t("addw_browse"))
                                    color: Theme.bg; font.pixelSize: 14; font.weight: Font.DemiBold; font.family: Theme.fontSans
                                }
                                MouseArea { id: browseMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: browseDlg.open() }
                            }
                        }
                        // with items: a slim strip for more
                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 14; anchors.rightMargin: 6
                            visible: items.count > 0
                            Text {
                                Layout.fillWidth: true
                                text: (i18n.language, i18n.t("addw_drop_more"))
                                color: Theme.t3; font.pixelSize: 13; font.family: Theme.fontSans
                            }
                            Rectangle {
                                Layout.preferredWidth: browse2.implicitWidth + 24; Layout.preferredHeight: 30
                                radius: Theme.rad(6)
                                color: browse2Ma.containsMouse ? Qt.lighter(Theme.hover, 1.2) : Theme.hover
                                Text {
                                    id: browse2
                                    anchors.centerIn: parent
                                    text: (i18n.language, i18n.t("addw_browse_short"))
                                    color: Theme.t1; font.pixelSize: 13; font.family: Theme.fontSans
                                }
                                MouseArea { id: browse2Ma; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: browseDlg.open() }
                            }
                        }
                    }

                    // ---- paste ----
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 8
                        Text {
                            visible: items.count === 0
                            text: (i18n.language, i18n.t("addw_or_paste"))
                            color: Theme.t3; font.pixelSize: 14; font.family: Theme.fontSans
                        }
                        TArea {
                            id: pasteArea
                            Layout.fillWidth: true
                            Layout.preferredHeight: Math.min(120, Math.max(46, pasteArea.text.split("\n").length * 17 + 24))
                            placeholder: (i18n.language, i18n.t("addw_paste_hint"))
                            wrap: false
                            onTextChanged: if (pasteSync.enabled) pasteTimer.restart()
                            QtObject { id: pasteSync; property bool enabled: true }
                        }
                    }

                    // ---- several items ----
                    Rectangle {
                        Layout.fillWidth: true
                        visible: items.count > 0 && !dlg.single
                        implicitHeight: itemCol.implicitHeight
                        radius: Theme.rad(9)
                        color: "transparent"
                        border.color: Theme.hairSoft; border.width: 1
                        Column {
                            id: itemCol
                            width: parent.width
                            Repeater {
                                model: items
                                delegate: Item {
                                    id: row
                                    width: itemCol.width
                                    height: 64
                                    opacity: model.state === "dup" ? 0.55 : 1
                                    readonly property bool late: model.state === "fetching" && (dlg.tick >= 0) && (Date.now() - model.started) > 120000
                                    Rectangle { visible: index > 0; width: parent.width; height: 1; color: Theme.hairSoft }
                                    RowLayout {
                                        anchors.fill: parent
                                        anchors.leftMargin: 12; anchors.rightMargin: 8
                                        spacing: 12
                                        Rectangle {
                                            Layout.preferredWidth: 32; Layout.preferredHeight: 44
                                            radius: Theme.rad(4)
                                            color: Theme.hover
                                            Text {
                                                anchors.centerIn: parent
                                                visible: model.poster === ""
                                                text: model.src === "direct" ? String(model.name).split(".").pop().toUpperCase().slice(0, 4) : ""
                                                color: Theme.t3; font.pixelSize: 9; font.weight: Font.Bold; font.family: Theme.fontSans
                                            }
                                            Spinner { anchors.centerIn: parent; visible: model.poster === "" && model.state === "fetching"; s: 14 }
                                            RoundedImage { anchors.fill: parent; visible: model.poster !== ""; sources: model.poster !== "" ? [model.poster] : []; radius: Theme.rad(4) }
                                        }
                                        ColumnLayout {
                                            Layout.fillWidth: true
                                            spacing: 3
                                            Text {
                                                Layout.fillWidth: true
                                                readonly property bool mono: model.name === ""
                                                text: mono ? model.value : model.name
                                                color: mono ? Theme.t2 : Theme.t1
                                                font.pixelSize: mono ? 12 : 14; font.weight: mono ? Font.Normal : Font.DemiBold
                                                font.family: mono ? Theme.fontMono : Theme.fontSans
                                                elide: mono ? Text.ElideMiddle : Text.ElideRight
                                            }
                                            Row {
                                                Layout.fillWidth: true
                                                spacing: 6
                                                clip: true
                                                Text {
                                                    text: (i18n.language, i18n.t(model.src === "magnet" ? "addw_src_magnet" : model.src === "direct" ? "addw_src_direct" : "addw_src_torrent"))
                                                    color: Theme.t2; font.pixelSize: 12; font.weight: Font.DemiBold; font.family: Theme.fontSans
                                                }
                                                Text {
                                                    width: row.width - 220
                                                    elide: Text.ElideRight
                                                    color: model.state === "failed" || row.late ? Theme.amber : Theme.t3
                                                    font.pixelSize: 12; font.family: Theme.fontSans; font.features: Theme.tnum
                                                    text: {
                                                        i18n.language; dlg.tick
                                                        if (model.state === "dup") return i18n.t("addw_dup")
                                                        if (model.state === "failed") return i18n.t(model.src === "direct" ? "addw_probe_failed" : "addw_read_failed")
                                                        if (model.state === "fetching") {
                                                            if (model.src === "direct") return model.host
                                                            return row.late ? i18n.t("addw_still_fetching").arg(dlg.clock(model.started))
                                                                            : i18n.t("addw_fetching").arg(dlg.clock(model.started))
                                                        }
                                                        if (model.src === "direct")
                                                            return model.host + (model.resumable ? " · " + i18n.t("addw_resumable") : "")
                                                        if (model.joins !== "") {
                                                            var j = model.joins.split("\t")
                                                            return j.length > 1 ? i18n.t("addw_joins_season").arg(j[0]).arg(j[1]) : i18n.t("addw_joins").arg(j[0])
                                                        }
                                                        var bits = []
                                                        if (model.sourceFile !== "") bits.push(model.sourceFile)
                                                        if (dlg.typeLabel(model.type) !== "") bits.push(dlg.typeLabel(model.type))
                                                        if (model.fileCount > 1) bits.push(i18n.t("addw_n_files").arg(model.fileCount))
                                                        return bits.join(" · ")
                                                    }
                                                }
                                            }
                                        }
                                        Rectangle {
                                            visible: model.state === "dup"
                                            Layout.preferredWidth: showTxt.implicitWidth + 20; Layout.preferredHeight: 26
                                            radius: Theme.rad(6)
                                            color: showMa.containsMouse ? Theme.hover : "transparent"
                                            border.color: Theme.hair; border.width: 1
                                            Text { id: showTxt; anchors.centerIn: parent; text: (i18n.language, i18n.t("addw_show")); color: Theme.t2; font.pixelSize: 12; font.family: Theme.fontSans }
                                            MouseArea {
                                                id: showMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                                                onClicked: { var h = model.hash; dlg.reject(); dlg.showInLibraryRequested(h) }
                                            }
                                        }
                                        Text {
                                            visible: model.state !== "dup" && model.sizeBytes > 0
                                            text: dlg.fmt(model.sizeBytes)
                                            color: Theme.t2; font.pixelSize: 13; font.family: Theme.fontSans; font.features: Theme.tnum
                                        }
                                        Rectangle {
                                            Layout.preferredWidth: 24; Layout.preferredHeight: 24
                                            radius: Theme.rad(5)
                                            color: rmMa.containsMouse ? Theme.hover : "transparent"
                                            IconImg { anchors.centerIn: parent; src: "qrc:/icons/close.svg"; s: 11; tint: rmMa.containsMouse ? Theme.t1 : Theme.t4 }
                                            MouseArea { id: rmMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: dlg.removeAt(index) }
                                        }
                                    }
                                }
                            }
                        }
                    }

                    // ---- one item ----
                    ColumnLayout {
                        Layout.fillWidth: true
                        visible: dlg.single
                        spacing: 14
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 14
                            Rectangle {
                                Layout.preferredWidth: 52; Layout.preferredHeight: 74
                                radius: Theme.rad(5)
                                color: Theme.hover
                                Image {
                                    anchors.centerIn: parent; width: 24; height: 24
                                    visible: dlg.singlePoster === ""
                                    source: Theme.logoSource; sourceSize: Qt.size(48, 48); opacity: 0.45
                                }
                                RoundedImage {
                                    anchors.fill: parent
                                    radius: Theme.rad(5)
                                    visible: dlg.singlePoster !== ""
                                    sources: dlg.singlePoster !== "" ? [dlg.singlePoster] : []
                                }
                            }
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 4
                                Text {
                                    Layout.fillWidth: true
                                    text: dlg.single ? items.get(0).name : ""
                                    color: Theme.t1; font.pixelSize: 18; font.weight: Font.DemiBold; font.family: Theme.fontSans
                                    wrapMode: Text.WordWrap; maximumLineCount: 2; elide: Text.ElideRight
                                }
                                Text {
                                    Layout.fillWidth: true
                                    elide: Text.ElideRight
                                    color: Theme.t3; font.pixelSize: 13; font.family: Theme.fontSans
                                    text: {
                                        i18n.language
                                        if (!dlg.preview) return ""
                                        var it = items.get(0), p = dlg.preview, bits = []
                                        if (dlg.typeLabel(p.kind) !== "") bits.push(dlg.typeLabel(p.kind))
                                        if (p.year > 0) bits.push(p.year)
                                        if (p.repacker) bits.push(p.repacker)
                                        if (it.joins !== "") {
                                            var j = it.joins.split("\t")
                                            bits.push(j.length > 1 ? i18n.t("addw_joins_season").arg(j[0]).arg(j[1]) : i18n.t("addw_joins").arg(j[0]))
                                        }
                                        if (it.sourceFile !== "") bits.push(i18n.t("addw_from").arg(it.sourceFile))
                                        else if (it.src === "magnet") bits.push(i18n.t("addw_src_magnet"))
                                        return bits.join(" · ")
                                    }
                                }
                            }
                            Rectangle {
                                Layout.alignment: Qt.AlignTop
                                Layout.preferredWidth: 24; Layout.preferredHeight: 24
                                radius: Theme.rad(5)
                                color: rm1Ma.containsMouse ? Theme.hover : "transparent"
                                IconImg { anchors.centerIn: parent; src: "qrc:/icons/close.svg"; s: 11; tint: rm1Ma.containsMouse ? Theme.t1 : Theme.t4 }
                                MouseArea { id: rm1Ma; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: dlg.removeAt(0) }
                            }
                        }

                        // groups, then every file on request
                        Rectangle {
                            Layout.fillWidth: true
                            implicitHeight: grpCol.implicitHeight
                            radius: Theme.rad(9)
                            color: "transparent"
                            border.color: Theme.hairSoft; border.width: 1
                            Column {
                                id: grpCol
                                width: parent.width
                                Repeater {
                                    model: dlg.preview ? dlg.preview.groups : []
                                    delegate: AddFileGroup {
                                        width: grpCol.width
                                        group: modelData
                                        first: index === 0
                                        host: dlg
                                    }
                                }
                                // the plain tree: always for an unknown layout, on request otherwise
                                AddFileTree {
                                    width: grpCol.width
                                    visible: dlg.preview && (dlg.showTree || (dlg.preview.groups || []).length === 0)
                                    rows: dlg.preview ? dlg.preview.files : []
                                    host: dlg
                                    divider: dlg.preview && (dlg.preview.groups || []).length > 0
                                }
                                Item {
                                    width: grpCol.width; height: 38
                                    visible: dlg.preview && (dlg.preview.groups || []).length > 0
                                    Rectangle { width: parent.width; height: 1; color: Theme.hairSoft }
                                    Text {
                                        anchors.left: parent.left; anchors.leftMargin: 14; anchors.verticalCenter: parent.verticalCenter
                                        text: (i18n.language, i18n.t(dlg.showTree ? "addw_hide_files" : "addw_show_files")).arg(dlg.preview ? dlg.preview.fileCount : 0)
                                        color: allMa.containsMouse ? Theme.t1 : Theme.t3; font.pixelSize: 13; font.family: Theme.fontSans
                                    }
                                    Text {
                                        anchors.right: parent.right; anchors.rightMargin: 14; anchors.verticalCenter: parent.verticalCenter
                                        text: dlg.preview ? (i18n.language, i18n.t("addw_of_files")).arg(dlg.selCount).arg(dlg.preview.fileCount).arg(dlg.fmt(dlg.selBytes)) : ""
                                        color: Theme.t3; font.pixelSize: 13; font.family: Theme.fontSans; font.features: Theme.tnum
                                    }
                                    MouseArea { id: allMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: dlg.showTree = !dlg.showTree }
                                }
                            }
                        }
                        Text {
                            Layout.fillWidth: true
                            visible: dlg.gameWarn
                            text: (i18n.language, i18n.t("addw_game_warn"))
                            color: Theme.amber; font.pixelSize: 12; font.family: Theme.fontSans
                            wrapMode: Text.WordWrap
                        }
                    }

                    // ---- where ----
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 10
                        RowLayout {
                            Layout.fillWidth: true
                            Text { text: (i18n.language, i18n.t("addw_save_to")); color: Theme.t3; font.pixelSize: 14; font.family: Theme.fontSans }
                            Item { Layout.fillWidth: true }
                            Text {
                                visible: dlg.needBytes > 0
                                color: dlg.destFits ? Theme.t2 : Theme.amber
                                font.pixelSize: 13; font.family: Theme.fontSans; font.features: Theme.tnum
                                text: {
                                    i18n.language
                                    if (!dlg.destFits) return i18n.t("addw_needs").arg(dlg.fmt(dlg.needBytes))
                                    var f = dlg.freeAt(dlg.destPath)
                                    return f >= 0 ? i18n.t("addw_of_free").arg(dlg.fmt(dlg.needBytes)).arg(dlg.fmt(f)) : dlg.fmt(dlg.needBytes)
                                }
                            }
                        }
                        Flow {
                            Layout.fillWidth: true
                            spacing: 8
                            Repeater {
                                model: dlg.dests
                                delegate: Rectangle {
                                    id: dchip
                                    readonly property bool on: modelData.path === dlg.destPath
                                    readonly property double free: dlg.freeAt(modelData.path)
                                    readonly property bool ok: free < 0 || dlg.needBytes <= free
                                    width: Math.max(dName.implicitWidth, dSub.implicitWidth) + 24
                                    height: 44
                                    radius: Theme.rad(8)
                                    color: on ? Theme.hover : (dMa.containsMouse ? Theme.hover : "transparent")
                                    border.width: on ? 1.5 : 1
                                    border.color: on ? (ok ? Theme.t1 : Theme.amber) : Theme.hair
                                    Column {
                                        anchors.verticalCenter: parent.verticalCenter
                                        x: 12
                                        spacing: 1
                                        Text { id: dName; text: modelData.name; color: dchip.on ? Theme.t1 : Theme.t2; font.pixelSize: 14; font.weight: dchip.on ? Font.DemiBold : Font.Medium; font.family: Theme.fontSans }
                                        Text {
                                            id: dSub
                                            visible: text !== ""
                                            color: !dchip.ok ? Theme.amber : (dlg.needBytes > 0 ? Theme.grn : Theme.t4)
                                            font.pixelSize: 12; font.family: Theme.fontSans; font.features: Theme.tnum
                                            text: {
                                                i18n.language
                                                if (dchip.free < 0) return ""
                                                if (dlg.needBytes <= 0) return i18n.t("addw_free").arg(dlg.fmt(dchip.free))
                                                return dchip.ok ? i18n.t("addw_left_after").arg(dlg.fmt(dchip.free - dlg.needBytes))
                                                                : i18n.t("addw_short").arg(dlg.fmt(dlg.needBytes - dchip.free))
                                            }
                                        }
                                    }
                                    MouseArea {
                                        id: dMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                                        onClicked: { dlg.destPath = modelData.path; dlg.destTouched = true }
                                    }
                                    ToolTip.visible: dMa.containsMouse
                                    ToolTip.delay: 600
                                    ToolTip.text: modelData.path
                                }
                            }
                            Rectangle {
                                width: oTxt.implicitWidth + 24; height: 44
                                radius: Theme.rad(8)
                                color: oMa.containsMouse ? Theme.hover : "transparent"
                                border.color: Theme.hair; border.width: 1
                                Text { id: oTxt; anchors.centerIn: parent; text: (i18n.language, i18n.t("addw_other")); color: Theme.t2; font.pixelSize: 14; font.weight: Font.Medium; font.family: Theme.fontSans }
                                MouseArea { id: oMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: otherDlg.open() }
                            }
                        }
                        // the real ways out when it does not fit
                        Rectangle {
                            Layout.fillWidth: true
                            visible: !dlg.destFits && dlg.needBytes > 0
                            implicitHeight: warnCol.implicitHeight + 24
                            radius: Theme.rad(9)
                            color: Qt.rgba(Theme.amber.r, Theme.amber.g, Theme.amber.b, 0.08)
                            border.color: Qt.rgba(Theme.amber.r, Theme.amber.g, Theme.amber.b, 0.3); border.width: 1
                            ColumnLayout {
                                id: warnCol
                                x: 12; y: 12
                                width: parent.width - 24
                                spacing: 8
                                Text {
                                    Layout.fillWidth: true
                                    text: (i18n.language, i18n.t("addw_no_fit")).arg(dlg.destName(dlg.destPath))
                                    color: Theme.amber; font.pixelSize: 13; font.family: Theme.fontSans
                                    wrapMode: Text.WordWrap
                                }
                                Flow {
                                    Layout.fillWidth: true
                                    spacing: 8
                                    BtnFlat {
                                        visible: dlg.fitsElsewhere !== null
                                        text: dlg.fitsElsewhere ? (i18n.language, i18n.t("addw_use")).arg(dlg.fitsElsewhere.name) : ""
                                        onClicked: { dlg.destPath = dlg.fitsElsewhere.path; dlg.destTouched = true }
                                    }
                                    BtnFlat {
                                        visible: dlg.keepChip !== null
                                        text: dlg.keepChip ? (i18n.language, i18n.t("addw_keep_only")).arg(dlg.keepChip.chip.label).arg(dlg.fmt(dlg.keepChip.saves)) : ""
                                        onClicked: {
                                            var k = dlg.keepChip
                                            dlg.setFiles(k.group.files, false)
                                            dlg.setFiles(k.chip.files, true)
                                        }
                                    }
                                    BtnFlat {
                                        text: (i18n.language, i18n.t("search_wontfit_freeup"))
                                        onClicked: {
                                            var need = Math.max(0, dlg.needBytes - Math.max(0, dlg.freeAt(dlg.destPath)))
                                            var what = dlg.single ? items.get(0).name : i18n.t("addw_sum_items").arg(dlg.addCount)
                                            var where = dlg.destPath
                                            dlg.reject()
                                            dlg.freeSpaceRequested(need, what, where)
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // ---- footer ----
            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: 64
                Rectangle { width: parent.width; height: 1; color: Theme.hairSoft }
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 22; anchors.rightMargin: 16
                    spacing: 10
                    Text {
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                        color: Theme.t3; font.pixelSize: 13; font.family: Theme.fontSans; font.features: Theme.tnum
                        text: {
                            i18n.language
                            if (items.count === 0) return ""
                            if (dlg.single) {
                                var p = dlg.preview
                                if (!p) return ""
                                return dlg.selCount === p.fileCount
                                    ? i18n.t("addw_sum_files").arg(p.fileCount).arg(dlg.fmt(dlg.selBytes))
                                    : i18n.t("addw_sum_some").arg(dlg.selCount).arg(p.fileCount).arg(dlg.fmt(dlg.selBytes))
                            }
                            var s = i18n.t(dlg.addCount === 1 ? "addw_sum_item" : "addw_sum_items").arg(dlg.addCount)
                            if (dlg.needBytes > 0) s += " · " + dlg.fmt(dlg.needBytes)
                            if (dlg.skippedCount > 0) s += " · " + i18n.t("addw_sum_skipped").arg(dlg.skippedCount)
                            return s
                        }
                    }
                    BtnFlat { text: (i18n.language, i18n.t("btn_cancel")); onClicked: dlg.reject() }
                    // Add, with how to add behind the arrow
                    Item {
                        id: split
                        readonly property bool ready: dlg.addCount > 0 && (!dlg.single || dlg.selCount > 0)
                        Layout.preferredWidth: addTxt.implicitWidth + 32 + 32
                        Layout.preferredHeight: 36
                        opacity: ready ? 1 : 0.4
                        Rectangle {
                            id: addMain
                            width: parent.width - 32; height: parent.height
                            radius: Theme.rad(8)
                            color: addMa.containsMouse && split.ready ? Qt.lighter(Theme.accent, 1.13) : Theme.accent
                            Rectangle { anchors.right: parent.right; width: Theme.rad(8); height: parent.height; color: parent.color }
                            Text {
                                id: addTxt
                                anchors.centerIn: parent
                                text: (i18n.language, dlg.addCount > 1 ? i18n.t("addw_add_n").arg(dlg.addCount) : i18n.t("addw_add"))
                                color: Theme.inkOnAccent; font.pixelSize: 14; font.weight: Font.DemiBold; font.family: Theme.fontSans
                            }
                            MouseArea { id: addMa; anchors.fill: parent; hoverEnabled: true; enabled: split.ready; cursorShape: Qt.PointingHandCursor; onClicked: dlg.commit(false, false) }
                        }
                        Rectangle {
                            x: parent.width - 32; width: 32; height: parent.height
                            radius: Theme.rad(8)
                            color: arrMa.containsMouse && split.ready ? Qt.lighter(Theme.accentDark, 1.13) : Theme.accentDark
                            Rectangle { width: Theme.rad(8); height: parent.height; color: parent.color }
                            Rectangle { width: 1; height: parent.height; color: Qt.rgba(0, 0, 0, 0.25) }
                            IconImg { anchors.centerIn: parent; src: "qrc:/icons/chevron.svg"; s: 11; tint: Theme.inkOnAccent }
                            MouseArea { id: arrMa; anchors.fill: parent; hoverEnabled: true; enabled: split.ready; cursorShape: Qt.PointingHandCursor; onClicked: addMenu.popup(0, height + 4) }
                            DMenu {
                                id: addMenu
                                DMenuItem { text: (i18n.language, i18n.t("addw_add_start")); onTriggered: dlg.commit(false, false) }
                                DMenuItem { text: (i18n.language, i18n.t("addw_add_paused")); onTriggered: dlg.commit(true, false) }
                                DMenuItem { text: (i18n.language, i18n.t("addw_add_choose")); onTriggered: dlg.commit(true, true) }
                            }
                        }
                    }
                }
            }
        }
    }
}
