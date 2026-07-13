# Changelog

## v4.5.0 — "Find"

### Added
- **One place to find everything.** Search and Discover merged into a single
  **Find** page: browse the featured billboard and poster shelves (movies,
  series, games, your list), or just start typing — the big search bar shrinks
  and docks as results take over, and clearing it puts you right back where you
  were browsing. Navigation slims down to three tabs: Downloads · Find · HUB.
- **"Mark as completed" grew up.** It now lives in the main right-click menu
  (shown once a download finishes), and the seeding limits you set (ratio or
  time) mark the torrent completed automatically instead of leaving it paused
  forever.
- **Setting a game executable now confirms it** with a toast, and the game is
  playable immediately — even if the torrent is still finishing other files.

### Fixed
- **Games: Play always answers.** A finished game that was still seeding
  counted as "downloading", so Play silently did nothing until you found
  "mark as completed" buried in a submenu. Fixed at the root — plus: launch
  failures now say so (toast + folder), executables that need admin rights get
  a proper UAC prompt on Windows, and .app bundles launch correctly on macOS.
- **Windows: mouse-wheel scrolling is fast now** — spinning the wheel used to
  crawl while dragging flew; wheel notches now build up speed properly.
- **Windows: the tray menu opens at your click**, not in a corner it guessed.
- **Two boot-crash protections.** A broken update that leaves a mismatched
  engine DLL on disk (our top crash in the wild) is now detected with a clear
  "re-download" dialog instead of a crash loop; and after any crashed start,
  the next boot begins with a fresh engine state instead of re-feeding
  whatever killed the last one.
- **Light theme contrast pass:** the game card's red button no longer hides
  its own label, the HUB's continue rails stop rendering as black slabs, and
  the grid's hover shadow no longer smudges on light backgrounds.
- Grid cards: a long status ("Download finished — seeding paused") elides
  instead of overlapping the size; peers/files/trackers empty states centered;
  peers table breathes at the top.

### Changed
- The top bar leads with the bat: glyph-only logo (the wordmark still fronts
  the splash, About and classic layout), and the grid's selection ring is
  accent red.

## v4.4.1

A hotfix for v4.4.0, which failed to launch on Windows.

### Fixed
- **Windows: the app launches again.** v4.4.0 shipped a QML shadow effect
  (`RectangularShadow`) that failed to resolve on Windows, silently failing
  the whole UI to load — no crash, no error, the app just quit. Replaced
  with the same shadow technique already used everywhere else in the app.

## v4.4.0 — "Dublado"

### Added
- **Releases in your language, finally.** With "Prefer my language" on (the default),
  the bundled stream source is asked for your-language releases — dubbed included —
  so they lead the list instead of drowning at the bottom. App in Portuguese?
  Dual-audio releases now show up first, not at position 19.
- **Source manager with a localized catalog.** A new "Sources" button in search
  lists every torrent source with a switch, plus a one-tap catalog grouped by
  region — Russian/CIS trackers (RuTor, Kinozal, NNM-Club via TorAPI), a
  Brazilian preset (Comando/BluDV… via torrent-indexer), and Jackett for
  everything else. Turn on what fits where you live.
- **BitSearch** ships on by default — a multi-language aggregator (TPB/1337x/
  YTS/nyaa across languages), so non-English releases surface out of the box.
- **Series → season → episode.** Picking a series now groups its releases by
  season, with an episode picker (and complete/season-pack tabs), instead of one
  flat 400-row list. Stremio series resolve per episode instead of a dead bare id.
- **Screenshots in the detail drawer** — a strip of backdrops (movies/series) or
  in-game stills (games) when you open a result.
- **Dubbed / Subtitled / Original filter.** One tap at the top of search results —
  see only dubbed releases, only subtitled, or only original audio, in your own
  language. It knows the difference a viewer actually cares about; ordering can't.
- **Free up space, without leaving the app.** Click the disk bar in the sidebar
  to open a panel of every torrent, sortable by size or age, with a one-click
  delete. When a download won't fit, the "won't fit" dialogs (Search and the
  add-torrent dialog) now offer a direct "Free up space…" link sized to exactly
  how much you're short. Adding a `.torrent` with a known size that won't fit no
  longer bypasses the warning even with "always use default path" on.

### Changed
- **Two view modes, not three: Grid × Classic.** Plain "List" is gone — Classic
  supersedes it. Classic is now a real power-user table: seeds, peers, ratio (red
  under 1.0, green above), live ETA, and sortable columns, cover-less with raw
  release names in monospace. Every column header sorts.

### Fixed
- **Progress never claims "100%" early anymore.** A torrent at 99.95% now shows
  99.9% until it actually finishes — every progress display floors instead of
  rounding, so "100%" is back to being a promise.
- **The what's-new screen no longer loses the dev note when a hotfix replaces a
  release** (as 4.3.1 replaced 4.3.0): notes now belong to the release line.
- **The player now shows a spinner when it actually runs out of buffer**, not just
  when Qt's own stall detection fires (which a growing torrent file never
  reliably triggers) — playback freezing with no feedback is now a spinner
  instead of a silent stop.
- **Fixed two crashes** found via crash reporting: one when a torrent's
  selection state went stale (e.g. removed while still selected), one at
  app shutdown when a very-late log message could still reach the log file
  after it had already closed.

### Security
- **Web UI password hardened**: stored as salted PBKDF2 (100k iterations) instead
  of unsalted SHA-256. Existing setups upgrade automatically on the next login.

## v4.3.1

A hotfix for v4.3.0, which failed to launch on Windows.

### Fixed
- **Windows: the app launches again.** v4.3.0 shipped the wrong libtorrent DLL next
  to the executable — a build that didn't match the one the app was compiled
  against — so Windows refused to start with a "procedure entry point not found"
  error. The correct engine library is now packaged. (#20)

### Under the hood
- Code-review pass over the engine's IPC layer, networking services, the web UI,
  and the QML views: hardened the IPC frame parser against malformed input, added
  request timeouts to every network client, sped up release-name parsing, and
  fixed a handful of latent QML property bugs. No behaviour changes you'll notice —
  the app is just steadier.

## v4.3.0 — "Continue"

The first release since v4.1.0, and a big one. Pick up where you left off, see what's actually worth downloading, and feel the hundreds of small decisions that finally make the whole app *fit*: bigger heros and real swarm health, automatic subtitles, a command palette, and a long list of long-standing bugs put to rest.

### Highlights
- **Continue watching / playing heros** — the HUB opens on your last movie and last game as large cards: progress, time left, hours played, last played, and a one-click **Resume**.
- **Command palette** — press **Ctrl/⌘+K** anywhere: jump to any torrent by name, pause/resume everything, toggle alt speed, open any page or window (it can jump straight to any Settings section). One box, fuzzy search, no mouse.
- **Player rebuilt** — a solid title bar showing the file's quality/audio, a two-row control deck, a buffer-ahead HUD, and resume that actually sticks (it no longer restarts streamed torrents from zero).
- **Automatic subtitles** — the built-in player now finds and downloads subtitles for **movies and series on its own**, no account needed, with a **language picker** to pull subs in any language. Still has manual load and ±0.5 s sync nudges; power users can plug their own OpenSubtitles account for a bigger quota.
- **Search by language** — a language filter on movie/series results (with a per-result language badge and a `DUB` marker) so you can go straight to a dubbed or your-language release.
- **Finished a movie? Play now** — when a download completes, a one-click *Play now* opens it in the player.
- **Real source health in Discover & Search** — the featured title shows a live **"N sources · best X GB · seeds"** line (one cached lookup), and the torrent list gets a green/amber/red **seed-health bar**.
- **Discover type filter** — All / Games / Movies / Series, with a hybrid hero (ambient art + crisp poster) that always has a banner, even for games with no backdrop.
- **Best match, explicitly** — search surfaces the top title as its own hero instead of just first in the grid.
- **Torrents explain why they're stuck** — hover a stalled torrent's state and it tells you: no peers yet, no seeds connected, peers not uploading, or the actual error. The eternal silent "stalled" finally speaks.
- **Unified window chrome (macOS)** — the native title bar is gone; the window controls sit inside the app. No more gray strip fighting your theme. (Windows/Linux keep native chrome for now.)
- **Delete sends data to the trash** — "remove with files" moves data to the system trash instead of erasing it permanently. Recoverable by design (with an opt-in **"Delete permanently"** when you're low on disk).
- **Free disk space in the status bar**, next to your totals.

### Fixed
- **The window remembers its size** — it reopens at the size you left it instead of snapping back to the default on every launch.
- **The listen port stops resetting** — after you change the port, the Settings field no longer flips back to the old value (it was reading the socket before libtorrent finished re-binding).
- **Paused torrents stay paused** across restarts (they were quietly resuming on launch).
- **Removed files no longer linger** — the hidden `.parts` sidecar is cleaned up, and the **"Delete permanently"** option skips the Trash when you're low on disk.
- **The Downloads search box** no longer stays stuck red after one click, and **Ctrl/⌘+K opens ready to type**.
- The **repack filter** opens on the first click again.
- **Movies no longer offer "Install"** — a film still downloading (its files end in `.!bt`) could be mistaken for a game; the Install/Play vs Watch choice now reads the actual file evidence.
- **Games with the `.exe` buried in subfolders** are detected as games again — and **Play** opens the game instead of a stray 30-second intro clip it found first.
- **Mouse-wheel scrolling** moves a sensible amount per notch on Discover, the HUB, Search and Settings (it used to crawl one line at a time).
- **(Windows) the tray right-click menu** anchors to the tray icon again instead of drifting.
- **The nav-rail activity card no longer vanishes** when downloads are paused (e.g. disk-low auto-pause) — it keeps showing your downloads or seeding.
- **Mixed-language UI** — search source/category names, result counts, status texts, the port diagnostics and the nav rail no longer show Portuguese in a non-Portuguese UI. (All 8 languages got a full pass.)
- **Duplicate search results** are deduplicated by infohash across providers.
- **HTML entities in result titles** (`&ndash;` and friends) now render as real characters.
- **Settings labels** no longer leak `&` accelerator marks ("&Export Settings").
- **Peers tab** country column no longer overlaps the IP header.
- **Update check errors** no longer pop a dialog on silent startup checks.
- **Fully translated, everywhere** — the new subtitle, search-by-language and disk-guard strings are now localized across all eight languages instead of falling back to English, the HUB empty-state shows its intended text again (it was stuck on a stale duplicate), and a diagnostics line that leaked a raw `detail_seeds` label now reads "Seeds".

### Polish
- A disk-aware search footer warns when results won't fit your free space, and **adding a too-big torrent now asks first** ("needs X, Y free — add anyway?") instead of silently piling up.
- "Get best" stepped back from red so it stops competing with the Search button.
- The Discover hero rotates with clickable dots.
- **Esc closes every dialog and window; Enter confirms** — including stacked dialogs.
- **Keyboard focus rings** on every control (Tab around the whole app) and **press feedback** on every button.
- **Semantic state colors**: completed reads green, seeding amber — a 100% red pill no longer looks like an error.
- **Anime accent art** becomes a subtle watermark in list view so row data always wins the contrast fight (grid keeps it bold).
- **Search results without covers** get per-title generated placeholders instead of a repeated logo.
- **Richer Statistics window** — live speed graph, per-state counts, all-time vs session columns.
- **Real empty states** for RSS (with a call-to-action) and search (with a hint when nothing matches).
- Discover skeletons follow the theme and pulse while loading; search shows a real spinner.
- About dialog links to GitHub, releases and the privacy policy.

### Under the hood
- **Fewer crashes, fixed faster** — opt-in crash reporting now captures the rare crash with a symbolicated stack, and crash detection offers a **one-click pre-filled GitHub issue** (opt-in toast, with the log tail for review). Nothing is ever sent automatically; the privacy policy is unchanged.
- The app keeps a tiny **local daily usage history** (bytes, torrents added/completed per category) — fully offline, feeds upcoming statistics features.
- Shared menu/dialog components, motion/focus design tokens, and a stats-history test suite.


## v4.1.0 — "Parity"

A community-driven release: everything here came straight from your reports and requests. It closes the remaining gaps with qBittorrent and fixes the Windows settings/tray/splash issues several of you hit.

### Fixed
- **Settings now actually save.** A whole class of preferences — speed limits (and the alternative limits), max active downloads, seed ratio, listen port, max connections, DHT/uTP/encryption, VPN interface, kill switch and proxy — weren't being persisted and reset to defaults on every launch. They now round-trip correctly. (Thanks to everyone who reported "the upload limit always goes back to 0".)
- **Splash and tray toggles stick on Windows.** Turning off the startup animation (or "close to tray") no longer reverts — the Windows registry stored these booleans as integers and the UI was misreading them.
- **Close-to-tray hint.** The first time the window hides to the tray you get a one-time notification, so the app doesn't look like it vanished (Windows 11 tucks new tray icons into the overflow).
- **macOS Dock icon size.** The icon filled its canvas edge-to-edge and rendered larger than neighbouring apps; it now uses the standard safe-area padding.
- **Native file picker language.** The "Torrent file / All files" filter in the open dialog follows the app language instead of being hard-coded.

### Added — qBittorrent parity
- **Alternative speed limits toggle** — a turtle button in the toolbar flips your throttled limits on/off instantly, independent of the scheduler.
- **Follow system theme** — switch light/dark automatically with the OS (Settings → Appearance).
- **Pre-allocate disk space** — reserve the full file size up front to reduce fragmentation (Settings → Downloads).
- **Recheck data on add** — optionally force a hash check when adding a torrent, so existing or partial files on disk are detected.
- **Port status indicator** — a 🟢/🟡/🔴 dot in the status bar shows whether your listen port looks reachable (UPnP/NAT-PMP + listen state; fully local, no external check).
- **Add torrent from URL** — File → Add torrent from URL (Ctrl+U) fetches a remote `.torrent` and routes it through the normal add dialog.
- **Export .torrent** — right-click a torrent → Export .torrent to save its metadata file.

### Already there (in case you missed it)
- **Watch folder** — auto-add `.torrent` files dropped into a monitored directory (Settings → Files). This release just surfaces it.
- Incomplete files already carry a **`.!bt`** suffix until they finish.

### Under the hood
- Regression tests for the settings-persistence and Windows boolean bugs.
- A new **Qt Quick Test** harness covering the startup splash and the design-system widgets.

## v4.0.0 — "Hub"

BATorrent becomes a media hub: **find → download → search → watch/play**, all around the cover-art identity. A left nav rail (collapsible) swaps between pages with smooth transitions.

### Onboarding & updates
- **Welcome / What's New screen** — one screen that greets you on first install and, after each update, carries a personal note from the dev plus that version's highlights and a link to the full release notes. (Born from a real need: a past broken auto-update left no way to reach users between releases.)
- **Guided interactive tour** — a coach-marks walkthrough (dimmed backdrop, callouts with arrows pointing at the nav rail, adding a torrent, Discover, Search, HUB and Settings). Runs once automatically after the first welcome/update screen — on a fresh install and once for everyone updating into this release — is skippable, and re-runnable anytime from **Help → Interactive tutorial**.

### Appearance
- **App-icon picker** (Settings → Appearance) — choose the live Dock/taskbar icon from a set of styles, **independent of the UI theme** (so a dark icon pairs with a light theme). Doesn't change the file-manager icon (`.app`/`.exe`), which comes from the signed bundle. Icon pack contributed by **@dkindratyuk-web** (#15).

### Discover
- A new **Discover** page: rotating hero + rows of trending/popular **movies, series and games** (TMDB + IGDB), cover-art forward. Click anything to search for it.

### Search, rebuilt
- **Title-first search**: type once and pick the actual work (e.g. *God of War Ragnarök*) from a poster grid before seeing downloads — one cover per title, games and movies mixed by relevance.
- Then drill into that title's **downloads** with real filters: **quality, source, repacker, provider/origin and seeders**, plus a relevance sort that matches whole words (so "blast" ≠ "last").
- Each result shows **where it came from** (RuTracker, Torrents, …); raw torrents resolve covers on demand. A "raw results" escape hatch is always available.

### HUB — watch & play
- **Watch movies in a built-in player** with **resume** (it remembers where you stopped) and a **watched-% bar** on each poster — stream while it's still downloading.
- **Continue watching** and **Continue playing** rails up top, then your movies and games libraries.
- **Games**: launch from the hub — it auto-detects the executable (or you set it once), with Install/Open-folder actions.

### Reliability
- **Auto-update hardening** — the downloaded installer is verified against its published size before it runs (no more bricking on a truncated/blocked download), a boot-crash **safe mode** offers recovery (reset settings / get latest) if startup fails twice, and the update dialog always has a manual **"Download manually"** fallback.
- **The Peers tab no longer lags on large swarms** — failed geo-IP lookups are negative-cached and the peer list only refreshes while the tab is open.

### Fixes
- A completed torrent that you'd **streamed no longer re-announces "download complete" on every launch** (its skipped sidecar files no longer make it look unfinished).

### Other
- Fully translated across all **8 languages**.

## v3.0.4

### Fixes
- **The app now launches on a clean Windows install** — it was missing the Visual C++ runtime; the build now ships the MSVC runtime DLLs, so the installer, the portable build and winget all work without a separate redistributable.
- **macOS Dock icon** no longer appears transparent and can now be customized — the app stopped overriding the Dock tile at runtime and leaves the bundled app icon in place. (#14)
- Enlarged the bat within the **macOS app icon** so it fills the rounded square better.

### Search & add
- **Restored `thunder://` link decoding** in Smart Paste — paste a Xunlei `thunder://` link and it's decoded to the underlying magnet / torrent and added.

### Games
- **Game search now matches across words and labels the source.** Typing "god of war fitgirl" finds FitGirl's repack (and typing just "fitgirl" lists all of theirs), and results show the repacker (FitGirl, DODI, RUNE, TENOKE, …) so you can see where a download comes from.

### Streaming
- **Hardened "stream while downloading"** — it opens the correct file even with the incomplete-file (`.!bt`) suffix, prefers VLC / mpv / IINA (which play a still-downloading file) with a clean fallback to your default player on Windows, macOS and Linux, and stops waiting if the torrent has no seeders.

## v3.0.3

### Games
- New **"All" search** that queries every source at once (game catalogs + torrent indexers) and merges the results — picking a single source is now optional. Game search consumes Hydra-format community catalogs (a default is seeded on first run, and you can remove it), downloads with cover art and clean titles, and caches catalogs for instant reuse.

### Covers & titles
- Adding a game now shows the **right name and cover instantly**, no restart needed. The matcher combines several signals — the torrent's file list, edition/qualifier stripping (`Early Access`, `Complete Edition`, `GOTY`), apostrophes (`Baldur's` = `Baldurs`), roman/arabic numerals (`GTA V` = `GTA 5`) and Cyrillic titles — and validates the API result instead of guessing, so it stops landing on the wrong title.

### Fixes
- A **completed torrent could start re-downloading** when the `.!bt` incomplete-file mapping desynced from disk. It now reconciles against what's actually on disk and self-heals on launch.
- **Added torrents vanished on restart** unless they had downloaded data — every added torrent now persists immediately.
- The **Peers tab no longer freezes** on large swarms (9k+ peers).
- The **welcome dialog no longer reappears** on Windows after you tick "don't show again".

### Interface
- Reworked README with demo GIFs and **localized screenshots** (the app shown in your own language).
- **Ukrainian** added — now eight UI languages.
- About: the Ukrainian flag renders correctly and the **Donate** button opens GitHub Sponsors.

## v3.0.2

### Phone pairing & WebUI
- The browser WebUI was **reskinned to match the desktop app** — same dark palette, Inter font, flat surfaces, the real BATorrent logo (it was a random bat before), and a proper magnet icon. It now looks like the same product, not a separate dashboard.
- **Pairing is one tap and zero typing**: the generated WebUI password is now copyable, and the QR code carries the credentials — scanning it from your phone logs straight in (no typing the IP or password), then drops the credentials from the address bar.

### Search
- Two new providers: **RuTor** (CIS sources, no login, via a public TorAPI relay) and **Torrents-CSV**.
- Results are **sorted by seeders** (healthiest first), and each search now times out after 15 s so one dead provider can't hang the UI.

### Files & trackers
- **Per-file priority** is back: right-click a file in the detail panel to set Skip / Low / Normal / High.
- **Rename an individual file** inside a torrent (double-click or the file menu), separate from renaming the torrent.
- **Remove a tracker** from a torrent (the ✕ on a tracker row); adding was already there.
- **Smart Paste on Ctrl+V** — paste a magnet, a 40-char info-hash, or a `.torrent` URL straight from the clipboard and it's added immediately (text fields still paste text normally).

### Covers & titles
- Anime fansub naming (`[Group] Title - NN`) now resolves to the right show.
- Audio channel layouts in titles (`DDP5.1`, `7.1`, …) are stripped so they don't pollute cover matching.

### Under the hood
- **The legacy QWidget interface is gone.** QML had been the only UI since 3.0.0 (reachable old code lived behind a hidden `--legacy` flag); with parity confirmed, the entire QWidget layer — main window, every dialog, the theme manager — was removed (~13,400 lines). The four restored actions above were features that backend already supported but the QML port had never wired.
- **macOS:** the WebUI password *hash* moved out of the keychain into app settings, so launching the app no longer pops a login-keychain password prompt on unsigned builds. The actual password still lives in the keychain.
- Cleanup: ~400 orphaned translation strings and a batch of dead code removed; internal duplication collapsed; an `ARCHITECTURE.md` added for contributors. Unit / security / memory tests and the ASan/UBSan/TSan sanitizers stay green.

---

## v3.0.1

### Windows / Linux
- **Restored the menu bar** (File, Torrent, Settings, Help — including Preferences, Check for Updates, and About). It had vanished on Windows because the previous bar only rendered as a macOS-style global menu; it now draws inside the window while macOS keeps the native global menu.

### Covers & titles
- Much stronger title parsing: leading release-site prefixes (`www.foo.com - `, `[ tracker.net ] - `) are stripped, and for episodes only the show name before SxxExx is used — so `www.UIndex.org - Euphoria US S03E08 in God We trust` resolves as Euphoria.
- Grid and list tiles no longer show a blank label when the cover hasn't resolved — they fall back to the parsed title, then the raw torrent name. List mode now matches grid.
- Episode tiles show SxxExx, so several episodes of one show are distinguishable.
- **Fix a wrong cover** from the right-click menu: re-link a torrent to the correct Movie / Series / Game title, or clear it with "No cover". The override is remembered and never overwritten by auto-matching.

### Notifications
- Finished / error / kill-switch / RSS events show as real OS notifications again (visible when the window is minimized), not only in-app toasts.

### Misc
- The portable Windows download is now named `BATorrent-windows-x86_64-portable.zip` so the installer is the obvious choice.

---

## v3.0.0

### New interface — full QML rewrite
- **Entire UI rebuilt in Qt Quick / QML**, replacing the QWidget interface — every screen ported: main window, settings, add/create torrent, search, RSS, statistics, diagnostics, inspector, log viewer, pairing, shortcuts, removed history, welcome, about, release notes
- Real-time speed graph, functional detail tabs (general, peers, files, trackers, pieces), drag-and-drop, native menu bar, and right-click context menus
- Multi-select, column sorting, grid reorder with animations, full-row hover, and arrow-key navigation
- Poster/cover art grid with TMDB / IGDB metadata resolution and localized synopses

### Theming
- **Custom theme profiles** — create, rename, and delete full palettes (background, panel, text + three accent colors), each with an optional background image and adjustable opacity
- Five built-in themes; midnight shifted from purple to blue; per-platform mono/sans fonts
- OS-scheme-aware logo so the white mark stays visible on light Windows taskbars/trays

### Startup splash v2.0
- Minimalist stroke animation that draws the bat outline, fills uniformly, and fades in the wordmark — no audio, shown every launch with a toggle in Settings

### System integration
- System tray with left-click restore and a right-click menu (speed, pause/resume all, quit); rich tray popup with live counts and DOWN/UP speeds
- Desktop notifications for finished / error / kill-switch / RSS events
- Discord Rich Presence and a fixed auto-updater

### Localization
- GeoIP peer country flags
- All windows and dialogs translated across 7 languages with live language switching

### Fixes
- Welcome dialog now shows again on first launch
- About and Release Notes pull real data (app version, linked-library versions, and the changelog) instead of hardcoded values
- "Active" filter and its count now agree — both mean *actually transferring* (idle seeders are no longer listed as active)
- Dropping several .torrent files at once shows the add dialog for each in sequence
- Reject duplicate torrents on add; deselect on empty-area click

### Windows
- **"Open containing folder"** now reliably opens the parent and highlights the torrent's file/folder, instead of landing in Documents/Downloads with nothing selected (uses the shell API directly, like qBittorrent)
- Cover/poster art loads correctly (fixed `file:` URL handling)
- Fixed a UI freeze when selecting a torrent
- Text rendering and fonts now match macOS — crisp, same size and weight
- Faster, smoother startup (windows load lazily) and a sharper splash animation
- Status colors corrected: completed is green, seeding amber, paused gray

---

## v2.6.1

### Critical fix
- **Auto-updater broken since v2.5.0** — the "Check for updates" button and silent startup check were both failing silently due to accumulated signal connections. Fixed by disconnecting stale handlers before each check.
- Added redirect policy and 15-second timeout to update API requests
- Users on v2.5.0 through v2.6.0 must update manually this one time — the updater will work correctly from v2.6.1 onward

---

## v2.6.0

### Search plugin system
- **Multiple search providers** with configurable URL templates and JSON response mapping
- **Built-in providers:** The Pirate Bay (apibay), Nyaa.si — ready to use out of the box
- **Custom providers:** define your own URL template, JSON array path, and field mappings (name, hash, size, seeders, leechers)
- Provider selector in the search dialog alongside the existing Stremio source

### Translation system rewrite
- Migrated 683+ translation keys × 7 languages from hardcoded C++ to JSON files
- `translator.cpp` reduced from 5,615 lines to 62 lines
- JSON files in `translations/` directory, loaded via Qt resources at runtime
- Translators can now contribute by editing JSON — no C++ knowledge required
- `tr_()` shortcut and English fallback work exactly as before

### Category temp paths
- Assigning a category with a save path to a downloading torrent automatically updates the download destination
- With temp path active: updates the intended final path (auto-moves on completion)
- Without temp path: calls `move_storage` immediately to the category's save path

### Release workflow
- `CHANGELOG.md` as the source of truth for GitHub Release descriptions
- Release job automatically extracts the version-specific section on tag push
- All existing releases (v1.3 through v2.5.3) retroactively received proper descriptions

---

## v2.5.3

### New features
- **Temp download path** — download to a staging folder first, auto-move to the save path on completion. Keeps media servers (Plex, Jellyfin, Sonarr) from scanning partial files.
- **Content layout options** — Original / Create subfolder / No subfolder controls how multi-file torrents are laid out on disk.
- **Excluded file patterns** — regex rules (semicolon-separated) to auto-skip files when adding a torrent. Common patterns: `\.nfo$`, `\.txt$`, `sample`.

### Improvements
- Advanced settings tab fully translated (42 keys × 7 languages)
- Run on Complete and Watched Folder labels/tooltips translated
- User-agent now uses dynamic APP_VERSION instead of hardcoded string
- Peer fingerprint updated to match current version

### Fixes
- CI: release job now has `actions/checkout` + `actions:write` permission for post-release triggers
- Post-release MSIX and Homebrew triggers use `continue-on-error` to prevent marking the release as failed

---

## v2.5.2

### Stability (from qBittorrent code analysis)
- Try-catch around the entire `processAlerts` loop body — a single bad alert no longer crashes the app
- `active_checking=1` — only one torrent rechecks at a time (prevents OOM on 96GB+ torrents)
- `checking_mem_usage=512` — explicit memory budget for piece checking (8MB)
- Cache invalidation in `forceRecheck()` — root cause of the 96GB recheck crash
- `alert_queue_size=1000000` — generous queue so disk-full storms don't silently drop alerts
- Crash loop guard — `startupInProgress` flag in QSettings; skips resume data on crash-during-boot
- Rate-limited `file_error_alert` emissions (1 per 30s) — disk-full no longer generates hundreds of notifications per second
- Auto-pause all downloads on disk-full detection
- Per-torrent error deduplication
- Handlers for `fastresume_rejected_alert`, `torrent_checked_alert`, `alerts_dropped_alert`, `storage_moved_failed_alert`

### Advanced settings (18 libtorrent tunables)
- Disk I/O: async threads, hashing threads, file pool size, checking memory, send buffer watermark
- Connections: global limit, connection speed, unchoke slots, per-torrent max uploads/connections
- Algorithms: choking (fixed slots / rate-based), seed choking (round robin / fastest upload / anti-leech)
- Toggles: rate-limit IP overhead, exempt LAN peers from speed limits

### Automation
- **Run on complete** — external command with template variables (%N=name, %D=path, %H=hash, %Z=size, %F=file)
- **Watched folder** — auto-add `.torrent` files every 10s, move to `.processed/` after adding
- **Torrent export directory** — auto-copy `.torrent` files to a backup folder on add
- **Download queue** with stalled-torrent detection (10KB/s for 60s = frees the queue slot)

### Power user features
- **Super seeding** mode for initial distribution
- **Force start** — bypass active-downloads queue cap for a single torrent
- **Per-torrent rate limits** (download + upload, persisted by info-hash)
- **Per-torrent stop-after-download and max seed time** (overrides global defaults)
- **Bandwidth scheduler** — alternative speed profile with hour-of-day + day-of-week schedule
- **Auto-complete** — mark torrent as Completed after configurable seeding window

### Polish
- Undo remove with toast + recently removed history (last 50 torrents, one-click restore)
- `diagnoseSlow()` diagnostic for stuck torrents
- Low disk space warning (<1GB remaining)
- Pause finished torrents on file errors (external drive unplugged)
- Opportunistic resume data saves on `piece_finished_alert` (rate-limited 1/min/torrent)
- File logging with rotation (5MB/file, keep 3) + log viewer dialog with level filter and export

---

## v2.5.0

### Privacy & private trackers
- **PT Mode** — one-toggle compliance: disables DHT/PEX/LSD, forces anonymous handshake, announces to every tier
- **Tor proxy preset** — one-click fill SOCKS5 127.0.0.1:9050
- **Anti-leecher blocking** — auto-detects and bans Xunlei, QQDownload, Baidu Netdisk P2P by peer_id prefix

### Notifications & integrations
- **Telegram webhook** — download complete, kill switch, RSS auto-download, errors pushed to any chat via bot token
- **Discord Rich Presence** — shows download progress in Discord profile with action buttons
- **Native OS notifications** via QSystemTrayIcon::showMessage

### Discovery & content
- **Smart Paste (Ctrl+V)** — magnet links, info hashes, and thunder:// links from clipboard
- **Torrent Inspector** — preview .torrent metadata before adding
- **RSS feed presets** — one-click add Nyaa, Sukebei, Linux Tracker
- **Thunder:// link decoding** — automatic decode of Xunlei's proprietary format

### WebUI & remote
- **QR code pairing** — scan to open WebUI on phone, no IP typing needed
- **Gitee update mirror** — alternative update source for users in China

### Interface
- Multi-tag system (free-form, multiple per torrent)
- Force Start queue bypass
- Recently removed history (last 50, one-click restore)
- Full backup/restore of settings + resume data
- Inline rename (F2)
- Resume on network change via QNetworkInformation
- Changelog popup after version bump
- Speed display in bytes or bits (togglable)
- Locale-aware number formatting
- 7 UI languages: EN, PT, ZH, JA, RU, ES, DE
