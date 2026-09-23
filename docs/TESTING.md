# Testing

Unit and integration tests live in `tests/` and use Catch2. QML widgets have
Qt Quick Tests in `tests/qml/`, and CI boots the full QML tree offscreen on every
PR (`.github/workflows/qml-smoke.yml`). See [CI.md](CI.md) for what has to be
green before a merge.

## What is covered

| Area | Suites | What they check | Not covered |
|------|--------|-----------------|-------------|
| torrent / engine | `sessionconfig`, `sessionresume`, `sessionremove`, `contentprobe`, `ipblocklist`, `bandwidthschedule`, `magnettrackers`, `proxycontroller`, parts of `unit` | Resume file naming, migration and corrupt-file handling; `.!bt` suffix cleanup; finish moves; remove with missing files; config round-trips; missing-content detection | Every libtorrent alert branch; streaming piece priority |
| bridges | `bridge` | Headless bridge setup, adding fixture torrents, `playFile` / stream URL / `clearResume` | Library and watchlist glue |
| discovery | `tmdbparse`, `igdbparse`, `addonparse`, `addonfinish`, `discoverysearch`, `discoveryassemble`, `discoveryfinish`, `hublogic`, `gamesource` | JSON to card mapping, dedupe, search queries, offline shelf and search results, stale addon responses being dropped | Live TMDB / IGDB calls (tagged `[net]`, run by hand) |
| metadata | `nameparser`, `metadatamatch`, `searchranker`, `releasegroup`, `releasetrust`, `gamereleasepick`, `episodegroup`, `mkvchapters` | Title parsing, poster lookup, ranking and trust | The full metadata fetch, which needs the network |
| security | `security`, `memguard`, parts of `unit` | Suspicious file scan, password hashing, archive volume rules | OS antivirus behaviour |
| downloads | `httpdownload`, `httpdownloadmanager`, `rangeplan`, `filehostresolver` | Range planning, persistence of finished and removed downloads | Unusual redirects and file hosts |
| vpn | `vpnmanager`, `wireguardconfig` | Config parsing and saved profiles | Bringing up a real tunnel |
| integrations | `debrid`, `geoipdb`, parts of `unit` | Debrid picks, GeoIP lookups, RSS/Atom, updater and installer checks | Live GitHub / Real-Debrid calls |
| platform | `settingshelpers`, `contentlanguage`, parts of `unit` | Settings backup, i18n basics, stats | OS file association hooks |
| ipc | `ipcprotocol` | Frame encoding between the UI and the engine process | The two processes end to end |
| webui | `unit`, `tests/webui/test_webui.mjs` | HTTP API responses | Auth hardening |
| QML | `tests/qml/tst_*.qml` | Controls, splash, toasts, search filtering and sorting, continue watching, the get-and-watch overlay, library selection | Full page flows, pixel comparisons |

`test_memory` is meant to run under ASan. `fuzz_nameparser` is a libFuzzer target.

## Running them

```bash
scripts/dev-build-fork.sh          # or: cmake -B build-fork -DBAT_BUILD_TESTS=ON ...

./build-fork/tests/test_sessionresume
ctest --test-dir build-fork --output-on-failure

QT_QPA_PLATFORM=offscreen ./build-fork/tests/test_qml

# after touching .qml files, boot the app with strict QML warnings
BAT_QML_STRICT=warn ./build-fork/BATorrent.app/Contents/MacOS/BATorrent
```

Catch2 tags follow the suite names (`[sessionresume]`, `[bridge]`, ...), so
`./test_unit "[unit]"` filters as usual. Cases tagged `[net]` hit real servers and
stay out of CI.

## Rules

1. If users depend on how the engine or a bridge behaves, write a test that pins
   the current behaviour before changing it.
2. New code in `services/` or `torrent/` that has no Qt event loop gets its own
   tests in the same change.
3. New areas get their own `test_<area>.cpp`. `test_unit.cpp` is already too big.
4. Anything touching QSettings, resume files or posters calls
   `QStandardPaths::setTestModeEnabled(true)`.
5. Tests stay offline: small private torrents and JSON fixtures, no DHT.
6. Line coverage is useful for finding gaps, but it doesn't decide whether a
   release ships. SessionManager, libtorrent glue and QML are not chasing a number.
