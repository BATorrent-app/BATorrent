# BATorrent — Test Strategy

How we raise **change-without-fear** confidence to the same bar as structural Ótimo
(`CLAUDE.md` soft ceilings + peels). This is the durable product plan. Sprint
rosters and working boards live under gitignored `internal/` when needed.

**Related:** [ARCHITECTURE.md](ARCHITECTURE.md) · Catch2 harness in `tests/` ·
QML boot smoke in `.github/workflows/qml-smoke.yml`.

---

## Goals

1. **Regressions that already hurt users must fail in CI** before they ship
   (poster paths, resume keys, playback URL invariants, parser edges).
2. **Extracted pure seams stay netted** — every new collaborator under
   `services/` / `torrent/` that is pure or near-pure ships with Catch2 cases.
3. **Engine glue stays characterizable where behaviour is product-visible**,
   without chasing line coverage through libtorrent alert loops.
4. **QML confidence is layered**: widget Quick Test → offscreen boot smoke →
   optional scripted Watch/Search/library paths — never “90% QML coverage.”

Success looks like structural Ótimo: a reviewer can change a peel or seam and
know the net will catch the regressions that matter.

---

## Current state (honest)

| Signal | Reality (2026-07-31) |
|--------|----------------------|
| Catch2 binaries | **40** `tests/test_*.cpp` (~**400+** `TEST_CASE`/`SCENARIO`) |
| Test LOC | ~6.7k lines of tests vs ~**~55k+** product C++ / **~22k** QML |
| Strength | Parsers, security scanners, release pick/rank, session **pure** resume helpers, settings helpers, HTTP range/plan, VPN config |
| Weakness | `QmlSessionBridge` playback/library paths, most of SessionManager alert/lifecycle glue, DiscoveryService network orchestration, almost all product QML surfaces |
| CI Catch2 allowlist | `build.yml` runs peel + metadata suites including `test_sessionresume`, `test_sessionremove`, `test_metadatamatch`, `test_sessionconfig`, parse/discovery helpers, `test_hublogic`, `test_settingshelpers`, plus prior core binaries. Local-HTTP / VPN integration suites stay local-first |
| QML | 9 Quick Test files (controls/Splash/Toast) + **boot-offscreen** smoke — no Watch / Search / Hub journey automation |
| Coverage tooling | **Not wired** in CMake. Apple Clang ships `llvm-cov` / `llvm-profdata` on macOS; optional local recipe below. Do not treat % lines as the gate |

Structural peels (SessionResume, MetadataMatch, AddonParse, Discovery assemble/search,
settings helpers, HubLogic, …) already moved risk into testable units. The gap is
**product behaviour nets** and **CI completeness**, not “more files for their own sake.”

Discovery peel tip: `discoveryservice.cpp` is **677** after assemble/search extract
(`c6a253c`). Prefer fakes at the network boundary and tests on the pure
collaborators — do not reopen that peel for coverage theatre.

---

## Inventory → domain map

Rough map of `tests/test_*.cpp` to product domains. Counts are `TEST_CASE` /
`SCENARIO` (approx.).

| Domain | Suites | Cases (≈) | What is locked | Primary gaps |
|--------|--------|-----------|----------------|--------------|
| **torrent / engine** | `sessionconfig`, `sessionresume`, `sessionremove`, `ipblocklist`, `bandwidthschedule`, `magnettrackers`, `proxycontroller`, chunks of `unit` | ~100+ | Resume naming/migration/corrupt policy; incomplete-suffix reconcile; finish move + emit mute; remove disposition × missing targets; config round-trips; empty-session API | Full alert_cast branches; tick/scheduler simulation; streaming piece priority |
| **bridges** | `bridge` | 16 | Headless bridge construct + fixture torrent add paths; **playFile / streamUrl / clearResume** | library/watchlist; selection/diagnose |
| **discovery** | `tmdbparse`, `igdbparse`, `addonparse`, `discoverysearch`, `discoveryassemble`, `hublogic`, `gamesource` | ~40 | JSON→card mappers; assemble dedupe; search query helpers; HubLogic | `DiscoveryService` QNAM orchestration; AddonManager network + gen counters |
| **metadata** | `nameparser`, `metadatamatch`, `searchranker`, `releasegroup`, `releasetrust`, `gamereleasepick`, `episodegroup`, `mkvchapters`, `unit` (ReleasePick/AudioMode) | ~100+ | Title parse, poster locate + AppData sibling + hash casing, ranking/trust | Full `MetadataResolver` fetch pipeline (intentional — network) |
| **security** | `security`, `memguard`, archive chunks in `unit` | ~30+ | SuspiciousScan, PasswordHash, archive volume rules | Platform Defender edges (OS-dependent) |
| **downloads** | `httpdownload`, `httpdownloadmanager`, `rangeplan`, `filehostresolver` | ~19 | Range plan, finish/remove persistence | Rare redirect/host edge cases |
| **vpn** | `vpnmanager`, `wireguardconfig` | ~15 | Config parse + manager persistence (test AppData) | Real tunnel bring-up (non-goal in unit CI) |
| **integrations** | `debrid`, `geoipdb`, `unit` (RSS, Updater, InstallerProfile) | ~40+ | Debrid pick, GeoIP DB, RSS/Atom, installer magic | Live GitHub/RD network |
| **platform** | `settingshelpers`, `contentlanguage`, `unit` (utils, translator, stats) | ~25+ | Backup/policy helpers, i18n basics | File association OS hooks |
| **subtitles** | `unit` | ~5 | Parser formats; optional Gestdown net case | Search ranking under failure modes |
| **ipc** | `ipcprotocol` | 6 | Frame/serialize contract | Engine-child process E2E |
| **webui** | `unit` + `tests/webui/test_webui.mjs` | ~15 + JS | HTTP API schema smoke | Auth/session hardening matrix |
| **games / install** | `gameinstallstate`, `gameexedetect`, `gamereleasepick` | ~28 | Install state machine, exe detect | Bridge gameinstall glue |
| **QML** | `tests/qml/tst_*.qml` + qml-smoke workflow | 9 + boot | Control widgets, Splash dismiss | Watch / Search / library covers |
| **misc / harness** | `memory`, `unit` trash/migration | — | ASan-oriented memory; trash env | — |

---

## Test pyramid (this app)

```
                 ┌─────────────────────────────┐
                 │  Optional: scripted smoke     │  Watch / Search / library
                 │  (Qt Quick Test journeys or   │  only for critical paths
                 │   BAT_SMOKE_* boot extensions)│
                 └────────────▲────────────────┘
                              │
                 ┌────────────┴────────────────┐
                 │  Thin integration             │  In-process SessionManager +
                 │  (Catch2 + QNAM fakes /       │  fixture .torrent; WebServer;
                 │   QStandardPaths test mode)   │  HttpDownloadManager persistence
                 └────────────▲────────────────┘
                              │
                 ┌────────────┴────────────────┐
                 │  Characterization of seams    │  SessionResume, MetadataMatch,
                 │  (pin behaviour before peel)  │  alert-finish pure helpers
                 └────────────▲────────────────┘
                              │
                 ┌────────────┴────────────────┐
                 │  Pure unit (Catch2)           │  Parsers, pickers, policies,
                 │  Fast, no network, fixtures   │  path math, IPC codecs
                 └─────────────────────────────┘
```

**Rules of thumb**

- Prefer **pure unit** when peeling (already the house style).
- Prefer **characterization** before changing SessionManager / bridge behaviour
  that users already depend on.
- Prefer **thin integration** only when the bug lives in wiring (signals, QSettings
  keys, resume file I/O) — keep fixtures offline (`create_torrent` private torrents,
  `QStandardPaths::setTestModeEnabled(true)`).
- Prefer **QML automation** only for irreversible UX contracts (play starts,
  cover resolves, search results bind) — not for styling.

---

## Phased plan → Ótimo-on-tests

### P0 — Characterization nets for past regressions

**Why:** These already shipped as user-visible bugs or near-misses. Structural peels
helped; the net must stay in CI.

| Regression / invariant | Seam to pin | Suggested home |
|------------------------|-------------|----------------|
| Poster cache / nested AppData / hash casing | `MetadataMatch::legacyAppDataSibling`, `locatePosterFile`, `canonicalInfoHash` | Extend `test_metadatamatch` (cases exist — **ensure CI runs this binary**) |
| Resume file keys & legacy dir migration | `SessionResume::*` | `test_sessionresume` (exists — **ensure CI**) |
| Corrupt resume quarantine vs recheck | `corruptResumeAction` | same |
| Incomplete `.!bt` path reconcile | `reconcileIncompleteSuffix` | same |
| `playFile` path invariants | Bound checks + stream URL shape `http://127.0.0.1:<port>/stream/<hash>/<fileIndex>`; no-op when hash/index/port invalid; sequential + priority side-effects observable via session API | New focused cases in `test_bridge` or `test_playback.cpp` (extract pure URL builder if glue fights you) |
| Continue-watching resume keys | `resume_<hash>_<fileIndex>` (+ `_dur` / `_at`) clear/set contract | Bridge/settings helpers — characterize `clearResume` / writers |

**P0 DoD**

- [x] CI `build.yml` Catch2 allowlist includes at least: `test_sessionresume`,
      `test_metadatamatch`, `test_sessionconfig`, `test_addonparse`,
      `test_tmdbparse`, `test_igdbparse`, `test_settingshelpers`,
      `test_discoveryassemble`, `test_discoverysearch`, `test_hublogic`
- [x] Explicit `playFile` / stream URL characterization cases (even if thin)
- [ ] No behaviour change to those seams without updating the net first

### P1 — SessionManager extractable seams + alert finish

**Why:** Engine is the crash/regression surface. Alerts finish already partly
peeled (`sessionmanager_alerts_finish.cpp`); resume characterization landed
(`c9f3d68`).

| Work | Approach |
|------|----------|
| Finish / rename / `.!bt` strip product rules | Extract remaining pure decision tables; Catch2 tables |
| `removeTorrent` flag matrix | Characterize deleteFiles × permanent × missing on temp dirs |
| Scheduler / bandwidth already partly covered | Keep; avoid full session tick simulation |
| Query / info projections | Prefer pure mappers over live handles |

**P1 DoD**

- [x] Finish move destination + payload/emit mute tables (`SessionResume::*`,
      `test_sessionresume`)
- [x] `removalDisposition` + `existingRemovalTargets` on temp dirs
      (`test_sessionremove`); CI allowlist includes the binary
- [ ] Query/info pure mappers (deferred — live-handle projections still glue)
- [ ] No requirement to cover every `alert_cast` branch

### P2 — Discovery / network boundary fakes

**Why:** Discovery is product-critical and still glue-heavy after the assemble/search
peel. Network is flaky in CI.

| Work | Approach |
|------|----------|
| Keep expanding pure mappers | TMDB/IGDB/AddonParse fixtures (ongoing) |
| `DiscoveryService` | Inject `QNetworkAccessManager` **or** a thin `IHttpGet` fake; drive assemble/search finishers with fixture bodies |
| `AddonManager` search | Fixture provider JSON through AddonParse (done) + one fake QNAM gen-counter race test if cheap |

**P2 DoD:** Shelf/search finish paths can be exercised offline; live TMDB/IGDB
remain manual / rare integration tags (`[net]`).

### P3 — Critical QML / scripted smoke

**Why:** QML errors are runtime-only; boot smoke already catches type/load failures.

| Work | Approach |
|------|----------|
| Keep widget Quick Tests | Existing `tests/qml/tst_*` |
| Extend boot smoke carefully | Optional `BAT_SMOKE_*` hooks for deferred loaders (pattern already in qml-smoke) |
| Watch / Search / library | Prefer **one** Qt Quick Test journey each **or** a documented manual checklist in release — not both bloated |
| Covers | Assert bridge-exposed poster path helpers under test mode, not pixel diffs |

**P3 DoD:** A broken Watch→play wiring or Search results model bind fails either
Catch2 (C++ seam) or a named smoke/Quick Test — not only a human report.

---

## Non-goals

- **90% line coverage** on SessionManager, libtorrent glue, or QML.
- Full VPN tunnel / Defender / Store packaging in Catch2.
- Replacing qml-smoke with a giant UI robot.
- Testing vendored `third_party/libtorrent` (upstream has its own suite).
- Mechanical split of `discoveryservice.cpp` solely to inflate coverage.
- Committing agent curricula, `.cursor/` rules, or learning notebooks as product docs.

---

## Definition of Done — “test Ótimo”

Matches structural confidence, not a coverage percentage.

Declare **test Ótimo** when **all** of the following hold:

1. **P0 nets in CI** — poster/AppData, resume keys, playFile/stream invariants cannot
   regress silently on the default PR build.
2. **Peel discipline** — every new pure collaborator lands with Catch2; peels do not
   land “and tests later.”
3. **Engine seams** — alert-finish / remove / resume policy changes require updating
   characterization tests (P1 largely done or consciously deferred with REVIEW notes).
4. **Discovery** — offline fixture path exists for assemble/search finish (P2); live
   API remains optional.
5. **QML** — boot smoke green on PRs; at least one automated or checklist-gated path
   for Watch and Search (P3).
6. **CI allowlist ≈ CMake targets that are fast/offline** — no long-lived orphan
   binaries that only run on one developer’s machine.
7. **Team confidence** — willing to change SessionManager / bridges / discovery without
   “who knows what breaks,” same emotional bar as structural Ótimo in
   `internal/SPRINT_ROADMAP.md`.

Line coverage may be sampled locally as a **gap finder**, never as a ship gate.

---

## Suggested sprint-sized workstreams

Parallelize only when file ownership is disjoint. Serialize `tests/CMakeLists.txt`,
`.github/workflows/build.yml`, and shared fixtures headers.

| Stream | Size | Owns | Forbidden | Deliverable |
|--------|------|------|-----------|-------------|
| **T0 CI allowlist** | S | `.github/workflows/build.yml` (+ maybe `tests/CMakeLists.txt` `check` target list) | Product `src/` behaviour | P0 binaries in PR CI |
| **T0b playFile net** | S–M | `tests/test_bridge.cpp` or NEW `tests/test_playback.cpp`, optional tiny pure helper under `bridges/session/` or `torrent/` | Discovery, QML views | Stream URL + guard characterization |
| **T1 alert/remove** | M | `sessionmanager_alerts_finish.cpp`, persistence/lifecycle **helpers only**, `test_sessionresume` / `test_sessionremove` | bridges/*, qml/*, discovery | Tables for finish + remove |
| **T2 discovery fakes** | M | `discoveryservice.*` (inject seam only), `discoveryassemble`/`discoverysearch` tests, fixture JSON under `tests/fixtures/` | sessionmanager peels, qml | Offline shelf/search finish |
| **T3 QML journeys** | M | `tests/qml/**`, optionally `qml-smoke.yml` env flags | Catch2 engine rewrites | Watch + Search smoke or Quick Test |
| **T-cov (optional)** | S | NEW `scripts/coverage-macos.sh` + doc only; **no default CMake ON** | Forcing coverage in CI | Local HTML/text report for gap triage |

### Optional macOS coverage recipe (not installed / not wired)

Apple Command Line Tools already provide `llvm-cov` and `llvm-profdata`. A future
opt-in might look like:

```bash
# Example only — do not enable in default developer builds until scripted.
cmake -B build-cov -DBAT_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS="-fprofile-instr-generate -fcoverage-mapping"
cmake --build build-cov -j --target test_metadatamatch test_sessionresume
LLVM_PROFILE_FILE="cov-%p.profraw" ./build-cov/tests/test_metadatamatch
llvm-profdata merge -sparse cov-*.profraw -o cov.profdata
llvm-cov report ./build-cov/tests/test_metadatamatch -instr-profile=cov.profdata \
  -ignore-filename-regex='(third_party|catch2|_deps)/'
```

If this fights Qt/MOC or the fat `COMMON_SOURCES` link model, **stop** and keep
inventory-driven prioritization — do not break `scripts/dev-build-fork.sh`.

---

## How to run tests locally

```bash
# Configure + build (same keys as the app when needed)
scripts/dev-build-fork.sh   # or cmake -B build-fork -DBAT_BUILD_TESTS=ON …

# Single suite
./build-fork/tests/test_sessionresume
./build-fork/tests/test_metadatamatch

# Broader
ctest --test-dir build-fork --output-on-failure

# QML Quick Test (offscreen)
QT_QPA_PLATFORM=offscreen ./build-fork/tests/test_qml

# Product QML boot (mandatory after .qml edits)
BAT_QML_STRICT=warn ./build-fork/BATorrent.app/Contents/MacOS/BATorrent
```

Tags: Catch2 tags follow domains (`[sessionresume]`, `[metadatamatch]`, `[bridge]`,
…); filter with `./test_unit "[unit]"` etc.

Network-tagged cases (e.g. Gestdown) may be skipped or slow — keep them out of the
default CI allowlist unless hardened.

---

## Maintenance rules

1. **Characterization before behaviour change** on engine/bridge contracts users rely on.
2. **One concern per suite** — avoid growing `test_unit.cpp` further; new domains get
   `test_<domain>.cpp`.
3. **Test mode AppData** — always `QStandardPaths::setTestModeEnabled(true)` when
   touching QSettings / resume / posters.
4. **Fixtures offline** — private torrents, checked-in JSON; no DHT.
5. **Update this doc** when CI allowlist or pyramid layers change materially.
