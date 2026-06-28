# BATorrent — engineering bar for Claude

This is the quality bar for the codebase. The point is code that reads as *crafted*,
not accreted. When a change would lower the bar "just this once", stop and flag it.

## Architecture (where things live)

```
src/
  main.cpp              app entry + single-instance + engine-split branch
  torrent/              the engine: SessionManager, IEngine, types  (the core)
  bridges/              C++ <-> QML glue (QObject bridges + the poster model)
  services/             non-UI app logic, grouped by domain:
    metadata/           metadataresolver, releasepick, nameparser
    subtitles/          subtitlesearch, subtitleparser
    discovery/          discoveryservice, addonmanager, gamesourcemanager
    security/           defender, suspiciousscan, secretstore, crashhandler, archivescan
    integrations/       realdebrid, discordrpc, notifier, geoip, updater, rssmanager, installerprofile
    platform/           logger, translator, statshistory, utils, qrcodegen
  ipc/                  engine-split child process protocol
  webui/                remote web server
  qml/                  UI — views/ overlays/ dialogs/ windows/ widgets/ theme/
```

- **Includes are root-relative from `src/`** (the target has `-Isrc`): write
  `#include "services/metadata/metadataresolver.h"`, never `../../app/x.h`. No
  relative `../` include paths — they're a structural smell and break on every move.
- New non-UI logic goes under the matching `services/<domain>/`. If it doesn't fit
  an existing domain, that's a signal to discuss, not to drop it in a catch-all.

## Comments

- Write far fewer comments. Default to none. Only comment when the *why* is
  non-obvious: a hidden constraint, a subtle libtorrent/Qt quirk, a workaround for a
  specific bug. Don't narrate *what* the code does — good names already do that.
- One line is almost always enough. No multi-line blocks explaining context that
  belongs in a commit message or PR.
- Don't re-explain control flow, a function's obvious purpose, or what a signal is for.
- Good (keep): `// file_progress is required for file_completed_alert — without it the .!bt suffix never gets stripped`.
- Bad (delete): `// loop through the torrents and update each one`.

## Structure & size

- **Soft ceilings:** ~400 lines per C++ file, ~300 per QML file, ~60 per function.
  Over that isn't forbidden but is a prompt to split. The monoliths
  (sessionmanager.cpp, Main.qml, qmlsessionbridge.cpp) are known debt — don't grow them.
- **One class per header** in `bridges/` (the 16-class mega-header is being split).
- A bridge is glue: it adapts engine/service calls to QML. Business logic belongs in
  `services/` or `torrent/`, not in a bridge and never in QML/JS.

## Correctness & robustness (we have a real crash rate to bring down)

- Guard external input and every fallible boundary (network bodies, file parsing,
  user paths, libtorrent handles). No happy-path-only code. The hardened
  `unzipFirstSrt` is the standard: 64-bit offsets, bounds checks.
- Prefer RAII / smart ownership; no raw new/delete without a clear owner.
- Network access is async via QNetworkAccessManager, `deleteLater()` on the reply,
  with a guard for the object outliving the request. Mirror subtitlesearch / realdebrid.

## Tests

- New non-UI logic ships **with** a test (tests/, Catch2). Pure functions (parsers,
  selection logic) especially — they're cheap to lock down.
- Don't change engine/parser behaviour without a characterization test pinning the
  old behaviour first. Refactoring without a net is how slop becomes regressions.

## Consistency (the anti-slop rule)

- Match the surrounding code's idioms, naming, and patterns. Don't introduce a second
  way to do a thing that already has a way.
- **No new duplication.** If you copy a block, extract it.
- **i18n:** every new UI string key goes in **all 8** `translations/*.json`, not just
  en/pt; no duplicate keys (last-wins shadows silently); every `i18n.t()/tr_()` key
  must exist. (See the project memory on i18n parity.)

## Build / verify (non-negotiable before "done")

- Build via `scripts/dev-build-fork.sh` (sources .env for the embedded keys).
- After **any** `.qml` edit, **launch** the binary and read stderr — QML errors are
  runtime-only. `BAT_QML_STRICT=warn` surfaces them loudly.
- Run the freshly built `./build-fork/BATorrent.app/Contents/MacOS/BATorrent`
  (kill stale instances first); don't `open` the installed copy.

## Working agreement

- Quality over feature velocity right now. We're in a code-review / de-slop pass
  (see internal/REVIEW.md). Smaller diffs, reviewed, tested, consistent.
- Flag incomplete work and missing external steps proactively. Don't lower the bar
  silently to ship faster — surface the tradeoff.
