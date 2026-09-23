# Privacy Policy

**Last updated:** September 23, 2026

## Summary

BATorrent has **no telemetry, no analytics, no ads and no user accounts**, and it never sells or shares your data. It makes a small number of network connections to do its job (listed below), none of them to profile you. The official builds also send a crash report when the app crashes; see [Crash reports](#crash-reports). Everything else stays on your machine.

## What the app does NOT do

- No telemetry
- No analytics
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
| Sentry | Crash report | Only after the app crashes | No setting yet; see below |

These services receive only what the request needs, for example a title you searched for, or a peer's IP address to look up its country. They get no account, identity or usage data, because the app collects none.

## Crash reports

The builds on the GitHub releases page (Windows, macOS and Linux, including the
winget and Homebrew packages that use them) include [Sentry](https://sentry.io)
crash reporting. The Microsoft Store build does not.

- A report is sent only when the app or its engine process crashes. Nothing is
  sent while it runs normally.
- The report contains the crash stack (a minidump of the crashed process), the
  operating system and CPU type, and the BATorrent and libtorrent versions. A
  minidump is a snapshot of part of the process memory, so it can contain bits of
  whatever the app was handling at that moment, such as a file or torrent name.
- Logs, settings and your torrent list are not attached.
- Like any server, Sentry sees the IP address the report comes from.
- There is no switch to turn it off in the app yet. Builds you compile yourself
  don't include it unless you set `BAT_SENTRY_DSN`.
- Reports are used only to find and fix crashes.

The app also writes its own crash notes to `<AppData>/BATorrent/crashes/`. Those
stay on your machine; after a crash the app may offer to open a pre-filled GitHub
issue, and nothing is sent unless you submit it.

## Data stored locally

All data is stored on your machine only:

- **Settings:** `QSettings` (Registry on Windows, plist on macOS, config file on Linux)
- **Resume data:** `<AppData>/BATorrent/resume/`, torrent state for restart recovery
- **Logs:** `<AppData>/BATorrent/logs/`, local log files, never transmitted
- **Crash data:** `<AppData>/BATorrent/crashes/` and the Sentry queue in `<AppData>/BATorrent/sentry-*`
- **Credentials:** Telegram bot token, WebUI password, and media-server keys stored via the OS keychain (macOS Keychain, Windows DPAPI) when available, or QSettings otherwise

## Open source

BATorrent is open source under the MIT license. The full source code is available at [github.com/Mateuscruz19/BATorrent](https://github.com/Mateuscruz19/BATorrent) for audit.

## Contact

For privacy questions: [GitHub Issues](https://github.com/Mateuscruz19/BATorrent/issues) or Discord ([Mateus Cruz](https://discord.com/users/241995362057977856)).
