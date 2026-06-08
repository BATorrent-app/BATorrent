# Privacy Policy

**Last updated:** June 8, 2026

## Summary

BATorrent has **no telemetry, no analytics, no crash reporting, no ads, and no user accounts**, and it never sells or shares your data for advertising or tracking. It does make a small number of **functional** network connections to do its job (listed below) — none of them exist to profile you. Everything else stays on your machine.

## What the app does NOT do

- No telemetry
- No analytics
- No crash reporting
- No usage tracking
- No advertising
- No user accounts
- No selling or sharing of your data for advertising/tracking

## Network connections the app makes

BATorrent only connects to the network to perform a feature you're using. Most are optional and can be turned off:

| Connection | Purpose | When | Can be disabled? |
|---|---|---|---|
| BitTorrent peers | Download/upload torrent data | When you add a torrent | Remove the torrent |
| Tracker announces | Find peers for your torrents | When a torrent is active | Remove trackers |
| DHT network | Decentralized peer discovery | When DHT is enabled | Settings → Network → uncheck DHT |
| Search providers | Run a torrent/game search | Only when you search | Searching is opt-in; manage in Settings → Add-ons |
| The Movie Database (TMDB) | Movie/series titles, posters and details | When you use Discover or title search | Don't use Discover/Search |
| IGDB | Game titles, covers and details | When you use game discovery/search | Don't use game search |
| ipinfo.io | Resolve a peer's country flag in the Peers tab | Only while the Peers tab is open | Don't open the Peers tab |
| Community game catalogs | Game search sources you add yourself | When you use game search | Remove the catalog in Settings |
| GitHub API | Check for app updates | On startup (silent) | Settings → Update source → Disabled |
| Gitee API | Check for app updates (China mirror) | Only if selected | Settings → Update source |
| Plex / Jellyfin | Send finished media to your own server | Only if you configure a server | Don't configure a media server |
| Telegram API | Send notifications you configured | Only if a bot token is set | Remove the bot token in Settings |
| api.ipify.org | IP-leak test in Diagnostics | Only when you click "Test outgoing IP" | Don't click the button |
| Discord | Show "now playing/downloading" rich presence | Only if Discord is running and the option is on | Settings → uncheck Discord presence |
| WebUI (localhost) | Remote control from your browser | Only if WebUI is enabled | Settings → WebUI → uncheck |

These third-party services (TMDB, IGDB, ipinfo.io, etc.) receive only what's needed for the request — for example a title you searched for, or a peer's IP address to look up its country. They are not sent any account, identity, or usage data, because the app collects none.

## Data stored locally

All data is stored on your machine only:

- **Settings:** `QSettings` (Registry on Windows, plist on macOS, config file on Linux)
- **Resume data:** `<AppData>/BATorrent/resume/` — torrent state for restart recovery
- **Logs:** `<AppData>/BATorrent/logs/` — local log files, never transmitted
- **Credentials:** Telegram bot token, WebUI password, and media-server keys stored via the OS keychain (macOS Keychain, Windows DPAPI) when available, or QSettings otherwise

## Open source

BATorrent is open source under the MIT license. The full source code is available at [github.com/Mateuscruz19/BATorrent](https://github.com/Mateuscruz19/BATorrent) for audit.

## Contact

For privacy questions: [GitHub Issues](https://github.com/Mateuscruz19/BATorrent/issues) or Discord ([Mateus Cruz](https://discord.com/users/241995362057977856)).
