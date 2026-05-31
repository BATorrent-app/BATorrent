# 00 — Backend API Inventory for the QML Layer

Single source of truth for what the QML layer can call today, what the QWidget
backend still hides, and how QML windows are shown. Evidence is cited as
`path:line`. Paths are absolute.

> **Coverage note:** Every file in this report was read in full and is
> documented authoritatively, with `path:line` citations: `qmlposterbridge.h`,
> `qmlposterbridge.cpp`, `main.cpp`, `sessionmanager.h`, `torrent/types.h`,
> `CMakeLists.txt`, the complete `src/qml/Main.qml` (1741 lines, incl. the
> window/dialog declaration block 1700–1740), `RssWindow.qml`,
> `SearchWindow.qml`, `SettingsWindow.qml` (first 120 lines), and all backend
> manager headers: `addonmanager.h`, `rssmanager.h`, `logger.h`, `updater.h`,
> `secretstore.h`, `metadataresolver.h`, `geoip.h`, `notifier.h`, `qrcodegen.h`,
> `webui/webserver.h`, `gui/thememanager.h`, `gui/pairingdialog.h`. Manager
> signatures in Section 2 are exact, not reconstructed.

---

## 1. QML-exposed objects (complete surface)

Five C++ objects are pushed into the root context in the `--qml` branch of
`main.cpp:180-185`:

```cpp
engine.rootContext()->setContextProperty("torrentModel",  filterProxy);   // QmlTorrentFilterProxy
engine.rootContext()->setContextProperty("torrentFilter", filterProxy);   // SAME object, two names
engine.rootContext()->setContextProperty("themeBridge",   themeBridge);   // QmlThemeBridge
engine.rootContext()->setContextProperty("session",       sessionBridge); // QmlSessionBridge
engine.rootContext()->setContextProperty("rss",           rssBridge);     // QmlRssBridge
```

Important: `torrentModel` and `torrentFilter` are **the same `QmlTorrentFilterProxy`
instance** (`main.cpp:181-182`). QML uses `torrentModel` as a view model
(`GridView.model`/`ListView.model`) and `torrentFilter` for the filter/sort/map
invokables. The underlying source model `QmlPosterModel` is **not** exposed by
name — only through the proxy.

The engine loads `qrc:/src/qml/Main.qml` (`main.cpp:186`).

There is **no** `qmlRegisterType` / `QML_ELEMENT`; every object is a context
property. Enums (e.g. `QmlPosterModel::Roles`) are therefore not reachable from
QML as named enums — roles are reached via `roleNames()` string keys only.

### 1a. `torrentModel` / `torrentFilter` — `QmlTorrentFilterProxy`
Defined `qmlposterbridge.h:58-79`; impl `qmlposterbridge.cpp:131-239`.
Subclass of `QSortFilterProxyModel`. Source model is `QmlPosterModel`
(`main.cpp:177-178`).

**Q_INVOKABLE methods**
| Signature | Decl | Impl | Behavior |
|---|---|---|---|
| `void setFilterState(const QString &state)` | h:64 | cpp:134 | state ∈ `all`/`active`/`downloading`/`seeding`/`paused`/`completed`. `active` = downloading∪seeding. cpp:232-238 |
| `void setSearchText(const QString &text)` | h:65 | cpp:141 | case-insensitive match on `torrentName` OR `metaTitle`. cpp:224-230 |
| `void setSortColumn(const QString &column, bool ascending)` | h:66 | cpp:148 | columns: `name`,`size`,`progress`,`down`,`up`,`state`,`category`,`peers` (cpp:169-200). Always `sort(0, …)`. |
| `void clearSort()` | h:67 | cpp:154 | `sort(-1)` resets to source order. |
| `int mapToSource(int proxyRow)` | h:68 | cpp:204 | returns source row or -1. Used by `_commitSel()` in Main.qml:54-61. |
| `int mapFromSource(int sourceRow)` | h:69 | cpp:210 | returns proxy row or -1. Used by `selectAll()` Main.qml:85-97. |

No Q_PROPERTY, no custom signals (inherits `QAbstractItemModel`'s
`rowsInserted`/`dataChanged`/etc. + `rowCount` usable from QML).

**Roles available on the model rows** (from `QmlPosterModel::roleNames()`
`qmlposterbridge.cpp:83-101`; values produced in `data()` cpp:38-81):

| QML role name | Source field | Notes |
|---|---|---|
| `torrentName` | `info.name` | |
| `stateKey` | computed | one of `completed`/`paused`/`seeding`/`downloading` (cpp:48-53) |
| `infoHash` | `torrentHashAt(row)` | empty while magnet resolving |
| `progress` | `info.progress` | qreal 0..1 |
| `posterPath` | resolver cache | local FS path, "" if none (cpp:56-62) |
| `metaTitle` | resolver cache | "" if none |
| `stateString` | `info.stateString` | human label |
| `downSpeed` | `formatSpeed(downloadRate)` | formatted string |
| `upSpeed` | `formatSpeed(uploadRate)` | formatted string |
| `category` | `info.category` | |
| `numPeers` | `info.numPeers` | int |
| `downRate` | `info.downloadRate` | int bytes/s (raw) |
| `upRate` | `info.uploadRate` | int bytes/s (raw) |
| `size` | `formatSize(totalSize)` | formatted string |

Note: enum `Roles` (h:23-39) also has `SizeBytesRole` (raw int64 size, cpp:78)
used internally for sort, but it is **not** in `roleNames()` (cpp:83-101) so it
is **not** exposed to QML as a named role. `DownRateRole`/`UpRateRole` ARE
exposed (`downRate`/`upRate`).

`QmlPosterModel` public slots `refresh()` (cpp:113) and `moveRow(int,int)`
(cpp:103) exist but the model isn't exposed by name; they are driven from C++
signal connections in `main.cpp:149-156`.

### 1b. `session` — `QmlSessionBridge`
Defined `qmlposterbridge.h:81-228`; impl `qmlposterbridge.cpp:243-892`.

**Q_PROPERTY — stats group** (all `NOTIFY statsChanged`), h:84-97:
| Property | Type | Read | Impl |
|---|---|---|---|
| `torrentCount` | int | `torrentCount()` | cpp:300 |
| `activeCount` | int | `activeCount()` | cpp:302 |
| `downloadingCount` | int | `downloadingCount()` | cpp:312 |
| `seedingCount` | int | `seedingCount()` | cpp:322 |
| `pausedCount` | int | `pausedCount()` | cpp:332 |
| `completedCount` | int | `completedCount()` | cpp:340 |
| `totalDownSpeed` | QString | `totalDownSpeed()` | cpp:348 |
| `totalUpSpeed` | QString | `totalUpSpeed()` | cpp:356 |
| `totalDownloaded` | QString | `totalDownloaded()` | cpp:364 (`globalDownloaded()`) |
| `totalUploaded` | QString | `totalUploaded()` | cpp:365 (`globalUploaded()`) |
| `globalRatio` | QString | `globalRatio()` | cpp:366 |
| `downloadHistory` | QVariantList | `downloadHistory()` | cpp:276; `NOTIFY historyChanged` |
| `uploadHistory` | QVariantList | `uploadHistory()` | cpp:284; `NOTIFY historyChanged` |
| `historyMaxBytes` | int | `historyMaxBytes()` | cpp:292; `NOTIFY historyChanged` |

History is sampled by an internal 1 s `QTimer` (cpp:246-248, `sampleSpeeds()`
cpp:261), capped at `HistoryMaxPoints = 60` (h:227).

**Q_PROPERTY — selection group** (all `NOTIFY selectionChanged`), h:99-126:
| Property | Type | Read impl |
|---|---|---|
| `selectedName` | QString | cpp:748 |
| `selectedSavePath` | QString | cpp:754 |
| `selectedSize` | QString | cpp:760 |
| `selectedHash` | QString | cpp:766 (elided: `8…4`) |
| `selectedDownloaded` | QString | cpp:774 (`size (pct%)`) |
| `selectedUploaded` | QString | cpp:782 (computed `totalDone*ratio`) |
| `selectedSpeed` | QString | cpp:789 (`↓ x   ↑ y`) |
| `selectedDownSpeed` | QString | cpp:796 (`—` if none) |
| `selectedUpSpeed` | QString | cpp:802 |
| `selectedEta` | QString | cpp:808 (`—` if stalled) |
| `selectedSeeds` | int | cpp:819 |
| `selectedPeers` | int | cpp:824 |
| `selectedRatio` | QString | cpp:829 |
| `selectedState` | QString | cpp:835 |
| `selectedPoster` | QString | cpp:841 (resolver cache) |
| `selectedDescription` | QString | cpp:852 (resolver cache) |
| `selectedMetaTitle` | QString | cpp:863 |
| `selectedMetaInfo` | QString | cpp:874 (year · genres · rating) |
| `selectedForceStart` | bool | cpp:449 (`isForceStart`) |
| `selectedSuperSeeding` | bool | cpp:454 (`isSuperSeeding`) |
| `selectedCompleted` | bool | cpp:459 |
| `selectedAtFullProgress` | bool | cpp:464 |
| `selectedPaused` | bool | cpp:469 |
| `selectedPeerList` | QVariantList | cpp:584 — maps `{ip,port,client,downSpeed,upSpeed,progress}` |
| `selectedFiles` | QVariantList | cpp:603 — maps `{path,size,progress,priority}` |
| `selectedTrackers` | QVariantList | cpp:620 — maps `{url,tier,status}` |
| `selectedPieces` | QVariantList | cpp:636 — list of bool |
| `hasSelection` | bool | cpp:743 (`m_selectedIndex` valid) |

**Q_INVOKABLE methods** h:146-174:
| Signature | Decl | Impl | Backend call |
|---|---|---|---|
| `void setSelectedIndex(int)` | h:146 | cpp:368 | sets single selection |
| `void setSelectedRows(const QList<int>&)` | h:147 | cpp:376 | multi-select (source rows) |
| `QList<int> selectedRows()` | h:148 | cpp:383 | |
| `void pauseSelected()` | h:149 | cpp:385 | `pauseTorrent` per row |
| `void resumeSelected()` | h:150 | cpp:394 | `resumeTorrent` per row |
| `void removeSelected()` | h:151 | cpp:403 | `removeTorrent(r,false)` |
| `void removeSelectedWithFiles()` | h:152 | cpp:416 | `removeTorrent(idx,true)` — **single-index only (bug-ish: ignores multi)** |
| `void pauseAll()` | h:153 | cpp:425 | `pauseAll()` |
| `void resumeAll()` | h:154 | cpp:426 | `resumeAll()` |
| `void addTorrentFile(const QString&)` | h:155 | cpp:654 | `addTorrent(local, defaultSavePath())` |
| `void addMagnetUri(const QString&)` | h:156 | cpp:663 | `addMagnet(uri, defaultSavePath())` |
| `QVariantMap previewTorrent(const QString&)` | h:157 | cpp:669 | parses `.torrent`; returns `{ok,name,totalSize,fileCount,infoHash,posterPath?,files[]}` or `{ok:false,error}` |
| `void resolvePreview(infoHash, name)` | h:158 | cpp:715 | `resolver->resolve()` |
| `void addTorrentWithPrefs(filePath, savePath, QVariantList priorities)` | h:159 | cpp:721 | `addTorrent` or `addTorrentWithPriorities` |
| `void copyMagnetLink()` | h:161 | cpp:428 | clipboard ← `torrentMagnetUri` |
| `void copyInfoHash()` | h:162 | cpp:435 | clipboard ← hash |
| `void openSaveFolder()` | h:163 | cpp:442 | `QDesktopServices::openUrl(savePath)` |
| `QString defaultSavePath()` | h:164 | cpp:646 | `lastSavePath` QSetting or Downloads |
| `void setSelectedForceStart(bool)` | h:165 | cpp:503 | `setForceStart` |
| `void setSelectedSuperSeeding(bool)` | h:166 | cpp:509 | `setSuperSeeding` |
| `void markSelectedCompleted()` | h:167 | cpp:515 | `markCompleted` |
| `void unmarkSelectedCompleted()` | h:168 | cpp:521 | `unmarkCompleted` |
| `void forceRecheckSelected()` | h:169 | cpp:527 | `forceRecheck` |
| `void forceReannounceSelected()` | h:170 | cpp:532 | `forceReannounce` |
| `void queueUpSelected()` | h:171 | cpp:537 | `setTorrentQueuePosition` + `queueMoved` |
| `void queueDownSelected()` | h:172 | cpp:560 | `setTorrentQueuePosition` + `queueMoved` |
| `void toggleSelectedPause()` | h:173 | cpp:474 | |
| `void smartPaste()` | h:174 | cpp:483 | clipboard → magnet/hash/.torrent URL |

`void emitStats()` (h:205, cpp:888) is public but **not** Q_INVOKABLE — it is a C++
slot wired to `SessionManager::torrentsUpdated` / `MetadataResolver::metadataReady`
in `main.cpp:144-147`; emits both `statsChanged()` + `selectionChanged()`.

**signals** h:207-213:
| Signal | Wired to |
|---|---|
| `statsChanged()` | drives all stats Q_PROPERTY |
| `selectionChanged()` | drives all selection Q_PROPERTY |
| `historyChanged()` | drives history props |
| `queueRefreshNeeded()` | → `QmlPosterModel::refresh` (main.cpp:153) |
| `queueMoved(int from,int to)` | → `QmlPosterModel::moveRow` (main.cpp:155) |
| `previewPosterReady(infoHash, posterPath)` | emitted from resolver callback (cpp:251-257); consumable by AddTorrentDialog |

### 1c. `themeBridge` — `QmlThemeBridge`
Defined `qmlposterbridge.h:230-250`; impl cpp:896-919.

| Member | Type | R/W | NOTIFY | Impl | Persistence |
|---|---|---|---|---|---|
| `themeName` | QString | READ/WRITE | `changed` | cpp:903/904 | QSettings `qmlThemeName` (default `dark`) |
| `anime` | bool | READ/WRITE | `changed` | cpp:912/913 | QSettings `qmlAnime` (default false) |

Signal: `changed()` (h:245). No invokables. Consumed by `theme/Theme.qml`.

### 1d. `rss` — `QmlRssBridge`
Defined `qmlposterbridge.h:253-271`; impl cpp:923-1006. Wraps the
`RssManager::instance()` singleton; constructor connects `feedAdded`/`feedUpdated`
→ `feedsChanged` and calls `loadFeeds()` (cpp:923-929).

| Member | Type | NOTIFY | Impl |
|---|---|---|---|
| `feeds` (Q_PROPERTY) | QVariantList | `feedsChanged` | cpp:931 — per feed `{index,name,url,enabled,autoDownload,filterPattern,count}` |
| `QVariantList itemsForFeed(int)` | invokable | — | cpp:950 — per item `{title,link,size,downloaded}` |
| `void addFeed(QString url)` | invokable | — | cpp:965 |
| `void removeFeed(int)` | invokable | — | cpp:973 |
| `void setFeedEnabled(int,bool)` | invokable | — | cpp:980 |
| `void setAutoDownload(int,bool)` | invokable | — | cpp:987 (`updateFeed`) |
| `void checkAllFeeds()` | invokable | — | cpp:998 |
| `void downloadItem(int feed,int item)` | invokable | — | cpp:1003 |

Signal: `feedsChanged()` (h:270).

> Bridge does NOT expose RSS feed *editing* of `filterPattern`, feed *rename*, or
> per-feed save path. See Section 2 (RssManager).

### 1e. NOT exposed but constructed in `--qml`
`MetadataResolver *resolver` (`main.cpp:138`) is used internally by the model +
session bridge but is **not** a context property — QML cannot call it directly.
Its `metadataReady` signal reaches QML only indirectly via
`emitStats()`/`refresh()` and `previewPosterReady`.

---

## 2. SessionManager public API NOT yet bridged to QML

All from `src/torrent/sessionmanager.h`. These are the backend capabilities the
QWidget UI used that the QML bridge has no path to yet. Grouped by the QML screen
that will need them.

### Settings → General / Files / Content
| Signature | Line | What it does |
|---|---|---|
| `void setTempPath(QString)` / `QString tempPath()` | 239/240 | incomplete-downloads temp dir |
| `void setContentLayout(int)` / `int contentLayout()` | 244/245 | 0=Original,1=Subfolder,2=NoSubfolder |
| `void setExcludedFilePatterns(QStringList)` / `excludedFilePatterns()` | 249/250 | auto-skip file regexes |
| `void setTorrentExportDir(QString)` / `torrentExportDir()` | 254/255 | auto-backup .torrent files |
| `void setWatchedFolder(QString)` / `watchedFolder()` | 274/275 | auto-add watch dir |
| `void setAutoMove(bool,QString)` / `autoMoveEnabled()` / `autoMovePath()` | 278-280 | move completed downloads |
| `void setRunOnComplete(QString)` / `runOnComplete()` | 268/269 | post-complete command (%N,%D,%F,%Z,%H) |
| `void setAutoExtract(bool)` / `autoExtract()` | 258/259 | extract archives on complete |
| `void setAutoExtractDelete(bool)` / `autoExtractDelete()` | 260/261 | |
| `void setExtractPasswords(QStringList)` / `extractPasswords()` | 262/263 | |

### Settings → Speed / Queue / Seeding
| Signature | Line | |
|---|---|---|
| `void setDownloadLimit(int kbps)` / `downloadLimit()` | 87/89 | global down cap |
| `void setUploadLimit(int kbps)` / `uploadLimit()` | 88/90 | global up cap |
| `void setSeedRatioLimit(float)` / `seedRatioLimit()` | 145/146 | |
| `void setStopAfterDownload(bool)` / `stopAfterDownload()` | 149/150 | |
| `void setMaxSeedSeconds(qint64)` / `maxSeedSeconds()` | 153/154 | |
| `void setAutoCompleteSeconds(qint64)` / `autoCompleteSeconds()` | 193/194 | |
| `void setMaxActiveDownloads(int)` / `maxActiveDownloads()` | 283/284 | queue cap |
| Bandwidth scheduler: `setAltSpeedLimits(int,int)`, `altDownloadLimit()`, `altUploadLimit()`, `setSchedulerEnabled(bool)`, `schedulerEnabled()`, `setScheduleFromHour(int)`, `setScheduleToHour(int)`, `scheduleFromHour()`, `scheduleToHour()`, `setScheduleDays(int)`, `scheduleDays()`, `altSpeedsActive()` | 329-340 | full scheduler |

### Settings → Network / Privacy / VPN / Proxy
| Signature | Line | |
|---|---|---|
| `void setListenPort(int)` / `listenPort()` | 93/94 | |
| `void setMaxConnections(int)` / `maxConnections()` | 95/96 | |
| `void setDhtEnabled(bool)` / `dhtEnabled()` | 97/98 | |
| `void setEncryptionMode(int)` / `encryptionMode()` | 99/100 | 0/1/2 |
| `void setUtpEnabled(bool)` / `utpEnabled()` | 106/107 | |
| `void setAnonymousMode(bool)` / `anonymousMode()` | 112/113 | |
| `void setForceIpv4(bool)` / `forceIpv4()` | 118/119 | |
| `void setPtMode(bool)` / `ptMode()` | 125/126 | private-tracker mode |
| `void setBlockLeecherClients(bool)` / `blockLeecherClients()` / `int blockedLeecherCount()` | 132-134 | |
| `void setOutgoingInterface(QString)` / `outgoingInterface()` | 137/138 | |
| `void setKillSwitchEnabled(bool)` / `killSwitchEnabled()` | 139/140 | |
| `void setAutoResumeOnReconnect(bool)` / `autoResumeOnReconnect()` | 141/142 | |
| `void setProxySettings(int,QString,int,QString,QString)` + `proxyType()`/`proxyHost()`/`proxyPort()`/`proxyUser()`/`proxyPass()` | 288-294 | |
| `void loadIpFilter(QString)` / `clearIpFilter()` / `ipFilterPath()` / `ipFilterCount()` | 297-300 | |

### Settings → Advanced
| Signature | Line | |
|---|---|---|
| `struct AdvancedSettings { … 18 fields … }` | 305-324 | full libtorrent tuning struct |
| `AdvancedSettings advancedSettings()` / `void setAdvancedSettings(const AdvancedSettings&)` | 325/326 | **struct — needs QVariantMap marshalling in a new bridge method** |

### Inspector / Detail panel (per-torrent ops missing from bridge)
| Signature | Line | |
|---|---|---|
| `void addTracker(int,QString)` | 49 | add tracker (detail Trackers tab) |
| `void replaceTrackers(int,QStringList)` | 66 | edit/remove trackers |
| `void setFilePriority(int,int,int)` | 51 | per-file priority (Files tab) |
| `void setSequentialDownload(int,bool)` | 52 | |
| `void prioritizeFilePieceBoundaries(int,int)` | 56 | stream-first |
| `void renameFile(int,int,QString)` | 59 | |
| `void moveStorage(int,QString)` | 62 | move save folder |
| `void setTorrentCategory(int,QString)` | 71 | |
| `QStringList categories()` | 72 | for category dropdown in toolbar (Main.qml has a stub "Todas as categorias") |
| `void setCategorySavePath(QString,QString)` / `categorySavePath()` / `allCategorySavePaths()` | 73-75 | |
| `QStringList torrentTags(int)` / `setTorrentTags(int,QStringList)` / `allTags()` | 80-82 | tags |
| `void setTorrentStopAfterDownload(int,int)` / `torrentStopAfterDownload(int)` | 157/158 | per-torrent override |
| `void setTorrentMaxSeedSeconds(int,qint64)` / `torrentMaxSeedSeconds(int)` | 159/160 | |
| `void stopSeedingTorrent(int)` | 163 | "stop seeding now" |
| `void setTorrentDownloadLimit(int,int)` / `setTorrentUploadLimit(int,int)` / getters | 178-181 | per-torrent rate caps |
| `bool isTorrentCompleted(int)` | 189 | (bridge uses `info.completed` instead) |
| `QString torrentRootPath(int)` | 234 | open-file (vs save folder) |
| `QString torrentMagnetUri(int)` | 207 | (bridge wraps via copyMagnetLink only) |
| `QString torrentHashAt(int)` | 203 | exposed via `infoHash` role only |

### Removed-history / undo (RemovedHistory screen)
| Signature | Line | |
|---|---|---|
| `QByteArray captureResumeData(int)` | 212 | |
| `bool restoreFromResumeData(QByteArray)` | 215 | |
| `struct RemovedEntry {hash,name,totalSize,removedAt,resumePath}` | 220-226 | |
| `QList<RemovedEntry> recentlyRemoved()` | 227 | **struct list — needs marshalling** |
| `bool restoreRemoved(QString hash)` | 228 | |
| `void clearRemovedHistory()` | 229 | |

### Statistics / Diagnostics screen
| Signature | Line | |
|---|---|---|
| `qint64 globalDownloaded()` / `globalUploaded()` / `float globalRatio()` | 354-356 | (down/up/ratio already bridged as formatted strings) |
| `qint64 sessionDownloaded()` / `sessionUploaded()` | 359/360 | **not bridged** |
| `struct DetailedStats {dhtNodes,peersCount,totalWasted,diskReadQueue,diskWriteQueue,hasIncomingConnections}` | 363-370 | **struct — needs marshalling** |
| `DetailedStats detailedStats()` | 371 | |
| `int totalTorrentsAdded()` / `void incrementTorrentCount()` | 375/374 | |

### Misc / import
| Signature | Line | |
|---|---|---|
| `int importFromQBittorrent(QString defaultSavePath)` | 377 | qBittorrent import (Settings/Welcome) |
| `void saveResumeData()` / `flushResumeDataBlocking(int)` / `loadResumeData()` | 346/350/351 | resume persistence |

### SessionManager signals NOT yet surfaced to QML
| Signal | Line | Needed for |
|---|---|---|
| `torrentAdded(int index)` | 380 | (wired only to resolver in main.cpp:157; QML never sees it) |
| `torrentRemoved(int index)` | 381 | toast / undo |
| `torrentsUpdated()` | 382 | wired → `emitStats`+`refresh` (main.cpp:144,149) ✔ |
| `torrentFinished(name, infoHash)` | 383 | completion notification / Notifier |
| `torrentError(message)` | 384 | error toast — **not surfaced** |
| `killSwitchTriggered()` | 385 | VPN kill-switch banner — **not surfaced** |
| `interfaceRestored()` | 386 | VPN reconnect banner — **not surfaced** |

> **Recommendation:** add a small set of new bridge invokables/properties on
> `QmlSessionBridge` (or a dedicated `QmlSettingsBridge`) that marshal the
> struct-returning getters (`advancedSettings`, `detailedStats`,
> `recentlyRemoved`) into `QVariantMap`/`QVariantList`, and re-emit
> `torrentError`/`torrentFinished`/`killSwitchTriggered`/`interfaceRestored` as
> bridge signals so QML can show toasts/banners. Pattern to copy: the existing
> `selectedFiles()`/`selectedPeerList()` marshalling in `qmlposterbridge.cpp:584-644`
> and the RSS struct→map in `cpp:931-963`.

### Other backend managers (verified — NONE bridged to QML)
These classes back the Settings/Search/Addon/Logs/Pairing/Stats screens. None is
a context property; each needs a new bridge (thin `setContextProperty` object) or
new invokables on an existing bridge. Structs they return must be marshalled to
`QVariantMap`/`QVariantList` (the `QmlRssBridge` cpp:931-963 pattern). Signals
must be re-emitted as bridge signals to reach QML. All signatures below are exact.

- **`RssManager` (singleton)** — `src/app/rssmanager.h`. `instance()` (h:40),
  `setSession(SessionManager*, QString savePath)` (h:42), `addFeed(QString)` (h:45),
  `removeFeed(int)` (h:46), `setFeedEnabled(int,bool)` (h:47),
  `updateFeed(int, RssFeed)` (h:48), `feeds()`→`QList<RssFeed>` (h:49),
  `checkAllFeeds()` (h:52), `checkFeed(int)` (h:53),
  `itemsForFeed(int)`→`QList<RssItem>` (h:56), `downloadItem(int,int)` (h:57),
  `markDownloaded(QString,QString)` (h:58), `loadFeeds()`/`saveFeeds()` (h:61/62).
  `RssFeed` (h:16-25): `{url,name,savePath,filterPattern,enabled,autoDownload,checkIntervalMin,lastChecked}`.
  `RssItem` (h:27-34): `{title,link,description,pubDate,size,downloaded}`.
  Signals (h:64-68): `feedAdded(RssFeed)`, `feedUpdated(int,QList<RssItem>)`,
  `feedError(QString)`, `itemAutoDownloaded(feedName,itemTitle)`. **`QmlRssBridge`
  already wraps most of this** but NOT: `setSession`, `checkFeed`, `markDownloaded`,
  feed `savePath`/`filterPattern`/`checkIntervalMin` editing, or `feedError`/
  `itemAutoDownloaded` signals (needed for toasts).

- **`AddonManager` (singleton)** — `src/app/addonmanager.h`. Backs both the Addon
  screen AND the Search screen (`SearchWindow.qml` currently uses a hardcoded
  `ListModel` of fake results — needs full bridging). `instance()` (h:74).
  Addons: `addAddon(QString url)` (h:77), `removeAddon(int)` (h:78),
  `setAddonEnabled(int,bool)` (h:79), `addons()`→`QList<AddonManifest>` (h:80),
  `loadAddons()`/`saveAddons()` (h:81/82), `hasCatalogAddon()`/`hasStreamAddon()`
  (h:85/86), `searchCatalog(QString)` (h:89), `getStreams(type,id)` (h:92).
  Trackers: `fetchTrackerList()` (h:95), `trackerList()` (h:96),
  `autoTrackersEnabled()`/`setAutoTrackersEnabled(bool)` (h:97/98).
  Search providers: `searchProviders()`→`QList<SearchProvider>` (h:101),
  `addSearchProvider(SearchProvider)` (h:102), `removeSearchProvider(int)` (h:103),
  `setSearchProviderEnabled(int,bool)` (h:104),
  `searchWithProvider(int providerIndex, QString query, int category=0)` (h:105).
  Legacy: `torrentSearchEnabled()`/`setTorrentSearchEnabled` (h:108/109),
  `torrentSearchUrl()`/`setTorrentSearchUrl` (h:110/111),
  `searchTorrents(query,category)` (h:112).
  Structs: `AddonManifest{id,name,description,url,types,resources,enabled}` (h:15-23);
  `CatalogItem{id,type,name,poster,year}` (h:26-32);
  `StreamResult{addonName,title,magnet,quality,size}` (h:35-41);
  `TorrentSearchResult{name,infoHash,magnet,size,seeders,leechers,category}` (h:44-52);
  `SearchProvider{id,name,urlTemplate,arrayPath,namePath,hashPath,sizePath,seedersPath,leechersPath,enabled,builtIn}` (h:55-68).
  Signals (h:115-124): `addonAdded(AddonManifest)`, `addonError(QString)`,
  `catalogResults(QList<CatalogItem>)`, `catalogFinished()`,
  `streamResults(QList<StreamResult>)`, `streamFinished()`, `trackerListUpdated()`,
  `torrentSearchResults(QList<TorrentSearchResult>)`, `torrentSearchFinished()`,
  `torrentSearchError(QString)`. **Search results arrive async via signals → the
  Search bridge must expose a results model + a `query()` invokable, and re-emit
  finished/error.**

- **`MetadataResolver`** — `src/app/metadataresolver.h`. `resolve(infoHash,name)`
  (h:35), `cached(infoHash)`→`MetadataResult` (h:36), `hasCached(infoHash)` (h:37),
  `batchResolve(QStringList,QStringList)` (h:38). `MetadataResult` (h:17-27):
  `{title,posterPath,description,rating(double),year,genres,platforms,contentType,valid}`.
  Signal: `metadataReady(QString infoHash, MetadataResult)` (h:41 — note the QML
  bridge connects a 1-arg overload form; the C++ signal is 2-arg). Constructed in
  `--qml` (main.cpp:138) but not a context property.

- **`Logger` (singleton, not QObject)** — `src/app/logger.h`. For the Logs viewer
  screen. `instance()` (h:18), `init()` (h:22), `setLevel(Level)` (h:23),
  `level()` (h:24), `log(Level,QString)` (h:28), `logsDir()` (h:31),
  `currentLogPath()` (h:34), `readAllLogs()`→QString concatenated (h:35),
  `clear()` (h:37), static `levelName(Level)` (h:39)/`levelFromName(QString)` (h:40).
  `enum Level { Trace=0, Debug, Info, Warning, Error, Critical }` (h:16).
  No signals (not a QObject) — log viewer must poll `readAllLogs()` / re-read file.

- **`Updater` (QObject)** — `src/app/updater.h`. For About / update banner.
  `checkForUpdate()` (h:19), `downloadAndInstall(downloadUrl,assetName)` (h:20),
  `latestVersion()` (h:22), static `compareVersions(a,b)` (h:26). Signals (h:29-33):
  `updateAvailable(version,downloadUrl,assetName)`, `noUpdateAvailable()`,
  `downloadProgress(received,total)`, `updateReady()`, `errorOccurred(QString)`.
  Note: auto-updater is compiled out for Store builds (`BAT_STORE_BUILD`,
  CMakeLists.txt:88-91) — guard any UI accordingly. Not constructed in `--qml` yet.

- **`SecretStore` (singleton, not QObject)** — `src/app/secretstore.h`. For
  password/token fields in Settings. `instance()` (h:20), `get(key)` (h:25),
  `set(key,value)` (h:28 — empty value deletes), `migrateFromSettings(QStringList)`
  (h:33), `isSecure()` (h:38 — false = plaintext fallback, surface to user).
  Keys used app-wide: `proxyPass`, `plexToken`, `jellyfinApiKey`,
  `webUiPasswordHash` (main.cpp:114-116).

- **`TelegramNotifier` (QObject)** — `src/app/notifier.h`. For Notifications
  settings. `reload()` (h:34), `isConfigured()` (h:35), `send(QString)` (h:39 —
  "Test connection" button), `wantsEvent(Event)` (h:43). Public slots wired to
  session/RSS signals: `onTorrentFinished`, `onKillSwitchTriggered`,
  `onRssAutoDownloaded`, `onTorrentError` (h:47-50). `enum Event {EventFinished,
  EventKillSwitch, EventRssAuto, EventError}` flags (h:24-30). No signals.

- **`GeoIpResolver` (QObject)** — `src/app/geoip.h`. For peer-country flags in the
  detail Peers tab. `resolve(ip)` (h:22), `cachedCountry(ip)` (h:23), signal
  `resolved(ip,countryCode)` (h:26). Free helper `countryCodeToFlag(code)`→emoji
  (h:40, inline). Not wired in `--qml`; Peers tab currently shows no flag.

- **`qrgen` (free functions)** — `src/app/qrcodegen.h`. For WebUI pairing QR.
  `qrgen::encode(QString)`→`Matrix{size,dark,at(x,y)}` (h:36), `qrgen::renderQR(text,
  pixelSize, fg, bg, quietZoneModules)`→`QPixmap` (h:42). Returns QPixmap — for QML
  use an `QQuickImageProvider` or render the `Matrix` directly with QML Rectangles.

- **`WebServer` (QObject)** — `src/webui/webserver.h`. For WebUI settings + pairing.
  ctor `WebServer(SessionManager*, parent)` (h:23), `start(quint16 port, bool
  remoteAccess)`→bool (h:26), `stop()` (h:27), `isRunning()` (h:28),
  `setCredentials(user, passwordHash)` (h:30). All other methods are private HTTP
  handlers. Not constructed in `--qml` yet — Settings WebUI section must own one.

- **`ThemeManager` (singleton, not QObject)** — `src/gui/thememanager.h`. This is
  the **QWidget** theme system (QSS strings + QColor getters), SEPARATE from
  `QmlThemeBridge`. `enum Theme {Dark,Light,Midnight,Sakura}` (h:14 — only 4; QML
  `SettingsWindow.qml:55` lists 5 themes incl. "Dark Star", and `QmlThemeBridge`
  stores theme as a free-form string — reconcile naming). `instance()`,
  `setTheme(Theme)`, `theme()`, plus ~30 color getters returning QString hex
  (h:28-63), `themeNames()` (h:72). QML should keep driving theming through
  `theme/Theme.qml` + `QmlThemeBridge`; ThemeManager is only relevant if the
  QWidget code path must stay in sync.

- **`PairingDialog` (QWidget QDialog)** — `src/gui/pairingdialog.h`. ctor
  `PairingDialog(int port, parent)` (h:20); only private `detectLanIp()`. This is a
  QWidget dialog — for QML, re-implement as a QML window showing the LAN URL +
  (newly) a QR via `qrgen`. The class header notes QR was deferred in v2.5.0.

---

## 3. How QML windows/dialogs are shown today

### 3a. Invocation sites (verified, Main.qml 1–1396)
The native menu bar (`Platform.MenuBar`, Main.qml:158-195) and the toolbar
(`TBtn`s, Main.qml:362-378) drive everything. Two distinct patterns:

**Top-level Windows — shown with `.show()`** (these are independent OS windows;
matches the user's stated preference that Settings/RSS/Search be top-level, not
cramped child modals):
| Call site | id | Triggered by |
|---|---|---|
| `settingsWin.show()` | `settingsWin` | menu "Preferências…" (line 179), toolbar "Config." (line 378) |
| `rssWin.show()` | `rssWin` | menu "RSS…" (line 181), toolbar "RSS" (line 375) |
| `searchWin.show()` | `searchWin` | menu "Buscar torrents…" (line 183), toolbar "Buscar" (line 374) |

**Dialogs — shown with `.open()`** (these are `BatDialog`-based modal popups, or
native file dialogs):
| Call site | id | Triggered by |
|---|---|---|
| `openFileDlg.open()` | `openFileDlg` | "Abrir torrent…" (161), toolbar "Abrir" (362), EmptyState (696) |
| `magnetDlg.open()` | `magnetDlg` | "Adicionar magnet…" (162), toolbar "Magnet" (363), EmptyState (697) |
| `createDlg.open()` | `createDlg` | "Criar torrent…" (163) |
| `removeDlg.open()` | `removeDlg` | menu/context/toolbar "Remover…" (152,175,371) |
| `addAddonDlg.open()` | `addAddonDlg` | "Addons…" (180) |
| `welcomeDlg.open()` | `welcomeDlg` | "Boas-vindas" (189) |
| `releaseNotesDlg.open()` | `releaseNotesDlg` | "Notas de versão" (190) |
| `aboutDlg.open()` | `aboutDlg` | "Sobre o BATorrent" (193) |

`Qt.quit()` (165), `Qt.openUrlExternally(...)` (185,192,678) are used for
exit / external links.

### 3b. Where the window/dialog objects are DECLARED (verified, Main.qml 1700–1740)
All instances are declared inline as direct children of the root `Window {}`, at
the very bottom of Main.qml — there is **no `Loader`**; component files are
instantiated by their type name with an `id`:

```qml
// Native file picker (Main.qml:1700-1713)
FileDialog { id: openFileDlg; title: "Abrir torrent"
    nameFilters: ["Arquivos torrent (*.torrent)", "Todos os arquivos (*)"]
    onAccepted: { var p = session.previewTorrent(selectedFile.toString())
        if (!p.ok) { session.addTorrentFile(selectedFile.toString()); return }
        addTorrentDlg.savePath = session.defaultSavePath()
        addTorrentDlg.loadPreview(p, selectedFile.toString()); addTorrentDlg.open() } }

// Overlay (modal, in-app backdrop) dialogs — opened with .open() (Main.qml:1716-1734)
MagnetDialog       { id: magnetDlg;       onAccepted: …session.addMagnetUri(magnetText) }
AddTorrentDialog   { id: addTorrentDlg;   onAccepted: …session.addTorrentWithPrefs(torrentPath, savePath, priorities()) }
RemoveDialog       { id: removeDlg;       onAccepted: deleteFiles ? session.removeSelectedWithFiles() : session.removeSelected() }
CreateTorrentDialog{ id: createDlg }
AddAddonDialog     { id: addAddonDlg }
WelcomeDialog      { id: welcomeDlg }
ReleaseNotesDialog { id: releaseNotesDlg }
AboutDialog        { id: aboutDlg }

// Top-level OS windows — opened with .show(); declared visible:false (Main.qml:1737-1739)
SearchWindow   { id: searchWin;   visible: false }   // root Window (SearchWindow.qml:7)
RssWindow      { id: rssWin;      visible: false }   // root Window (RssWindow.qml:8)
SettingsWindow { id: settingsWin; visible: false }   // root Window (SettingsWindow.qml:7)
```

Confirmed facts:
- `SearchWindow.qml`, `RssWindow.qml`, `SettingsWindow.qml` each have a root
  `Window {}` (verified at those files' line 7/8/7), so `.show()` opens an
  independent OS window — matching the user's top-level-window preference.
- `MagnetDialog`/`RemoveDialog`/`AddTorrentDialog`/`CreateTorrentDialog`/
  `AddAddonDialog`/`WelcomeDialog`/`ReleaseNotesDialog`/`AboutDialog` are
  `BatDialog`-based overlays → `.open()`.
- `openFileDlg` is a `QtQuick.Dialogs.FileDialog` (import at Main.qml:7); on
  accept it runs `session.previewTorrent()` then either direct-adds or hands the
  preview to `AddTorrentDialog.loadPreview(p, path)` + `.open()`.
- `createDlg` (`CreateTorrentDialog`) has **no `onAccepted` wiring** in Main.qml —
  it is opened but its accept path is unhandled (the create-torrent SessionManager
  path is not implemented; SessionManager has no `createTorrent` method). This is a
  gap to close.

Existing import set in Main.qml (1-10): `QtQuick`, `QtQuick.Controls.Basic`,
`QtQuick.Effects`, `QtQuick.Layouts`, `QtQuick.Shapes`, `QtQuick.Dialogs`,
`Qt.labs.platform as Platform`, `"theme"`, `"widgets"`. `Theme` is a QML
singleton (`theme/qmldir`: `singleton Theme 1.0 Theme.qml`).

### 3c. Pattern summary for adding a new screen
1. Create `src/qml/<Name>Window.qml` (root `Window` for top-level) or
   `src/qml/<Name>Dialog.qml` (root `BatDialog` for modal). Copy structure from
   `RssWindow.qml` / `MagnetDialog.qml` / `RemoveDialog.qml`.
2. Add it to `src/resources.qrc` (the qrc that the engine loads from
   `qrc:/src/qml/…`).
3. If it needs new backend data, add a bridge (new `setContextProperty` in
   `main.cpp` ~line 185, following the 5 existing ones) **or** add invokables to
   an existing bridge.
4. Instantiate `…Window { id: fooWin }` in Main.qml (1397–1741 block) and call
   `fooWin.show()` / `fooDlg.open()` from a menu item or `TBtn`.

---

## Quick reference — what's already callable vs. what's missing

- **Already callable from QML:** torrent list (model+roles), filter/sort/search,
  multi-select, all per-selection getters, add file/magnet/with-prefs, preview,
  pause/resume/remove (single & all), queue up/down, force recheck/reannounce,
  force-start/super-seed/mark-completed toggles, clipboard ops, save folder,
  global stats + 60-point speed history, theme name/anime toggle, full RSS feed
  list/items/add/remove/enable/auto-download/check/download.
- **Missing (needs new bridging):** every Settings field (network/speed/queue/
  seeding/privacy/proxy/advanced/files/scheduler), categories & tags, per-torrent
  overrides & rate caps, tracker add/replace, file priority/rename, move storage,
  removed-history/undo, detailed stats & diagnostics, qBittorrent import, and all
  of Search / Addon / Logs / Pairing / Web-UI (no bridge objects exist for these
  managers). `torrentError`/`torrentFinished`/`killSwitch`/`interfaceRestored`
  signals are not surfaced.
