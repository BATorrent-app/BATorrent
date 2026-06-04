# Changelog

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
