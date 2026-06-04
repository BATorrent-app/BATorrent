# Privacy Policy

**Last updated:** May 24, 2026

## Summary

BATorrent does not collect, transmit, or store any personal data. Period.

## What the app does NOT do

- No telemetry
- No analytics
- No crash reporting
- No usage tracking
- No advertising
- No user accounts
- No data shared with third parties

## Network connections the app makes

BATorrent only initiates network connections that **you explicitly configure**:

| Connection | Purpose | When | Can be disabled? |
|---|---|---|---|
| BitTorrent peers | Download/upload torrent data | When you add a torrent | Remove the torrent |
| Tracker announces | Find peers for your torrents | When a torrent is active | Remove trackers |
| DHT network | Decentralized peer discovery | When DHT is enabled | Settings → Network → uncheck DHT |
| GitHub API | Check for app updates | On startup (silent) | Settings → Update source → Disabled |
| Gitee API | Check for app updates (China mirror) | Only if selected | Settings → Update source |
| Telegram API | Send notifications you configured | Only if bot token is set | Remove bot token in Settings |
| api.ipify.org | IP leak test in Diagnostics | Only when you click "Test outgoing IP" | Don't click the button |
| WebUI (localhost) | Remote control from your browser | Only if WebUI is enabled | Settings → WebUI → uncheck |

## Data stored locally

All data is stored on your machine only:

- **Settings:** `QSettings` (Registry on Windows, plist on macOS, config file on Linux)
- **Resume data:** `<AppData>/BATorrent/resume/` — torrent state for restart recovery
- **Logs:** `<AppData>/BATorrent/logs/` — local log files, never transmitted
- **Credentials:** Telegram bot token and WebUI password stored via OS keychain (macOS Keychain, Windows DPAPI) when available, or QSettings otherwise

## Open source

BATorrent is open source under the MIT license. The full source code is available at [github.com/Mateuscruz19/BATorrent](https://github.com/Mateuscruz19/BATorrent) for audit.

## Contact

For privacy questions: [GitHub Issues](https://github.com/Mateuscruz19/BATorrent/issues) or Discord ([Mateus Cruz](https://discord.com/users/241995362057977856)).
