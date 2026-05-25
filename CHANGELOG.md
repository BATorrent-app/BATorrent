# Changelog

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
