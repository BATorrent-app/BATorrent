# BATorrent — CI flow

What must be green before merge/release, and what is optional noise you can
ignore on routine PRs (especially Dependabot Actions bumps).

**Related:** [TESTING.md](TESTING.md) (test strategy) · workflows in
`.github/workflows/`.

---

## Pyramid

```
  Every product PR / push to main (required)
  ┌─────────────────────────────────────────────────────────────┐
  │  Build & Release   Catch2 allowlist + Linux/Win/macOS build │
  │                    + QML smoke (reusable workflow call)     │
  │  i18n parity       Python key/usage check (path-filtered)   │
  │  QML Smoke         also on push to main (src/QML paths)     │
  └─────────────────────────────────────────────────────────────┘
                              │
  Weekly / manual / label `ci-deep` (optional — do not block merge)
  ┌─────────────────────────────────────────────────────────────┐
  │  Sanitizers · ThreadSanitizer · clang-tidy · cppcheck       │
  │  CodeQL · Semgrep · OSV-Scanner                             │
  └─────────────────────────────────────────────────────────────┘
                              │
  Release / store (manual or tag)
  ┌─────────────────────────────────────────────────────────────┐
  │  Build & Release on tag v*  → GitHub release + artifacts    │
  │  Microsoft Store (MSIX)     → workflow_dispatch only        │
  │  Scorecard                  → weekly (+ branch protection)  │
  └─────────────────────────────────────────────────────────────┘
```

---

## What runs when

| Workflow | PR (product paths) | PR (Actions-only / docs) | Push `main` | Tag `v*` | Schedule | Manual |
|----------|--------------------|---------------------------|-------------|----------|----------|--------|
| **Build & Release** | Yes (path-filtered) | No | No (tags only for push) | Yes + release | — | Yes |
| **i18n parity** | If translations/src touch | No | If paths match | — | — | — |
| **QML Smoke** | Via Build & Release call | No | If `src`/QML paths | Via Build & Release | — | Yes |
| Sanitizers / TSan / clang-tidy / cppcheck / CodeQL / Semgrep / OSV | Only if label `ci-deep` | Same | — | — | Mon 06:00 UTC | Yes |
| Scorecard | — | — | — | — | Mon 06:00 UTC | Yes |
| Microsoft Store | — | — | — | — | — | Yes |
| Claude Code Review | — | — | — | — | — | Yes (PR # input) |

“Product paths” for Build & Release: `src/`, `tests/`, `translations/`,
`CMakeLists.txt`, `cmake/`, `scripts/`, `third_party/`, and the
`build.yml` / `qml-smoke.yml` workflow files themselves (so action-pin bumps
that touch the real gate still rebuild).

---

## What “green” means

| Check | Green means |
|-------|-------------|
| **Build & Release → test** | Catch2 allowlist binaries all print `All tests passed`; WebUI `node --test` passes; i18n script passes |
| **Build & Release → build-\*** | Linux AppImage, Windows installer, macOS DMG artifacts upload |
| **Build & Release → qml-smoke** | Offscreen boot has no QML load/type errors; Qt Quick Test (`test_qml`) passes |
| **Build & Release → release** | Only on `v*` tags; needs test + qml-smoke + all three platform builds |
| **i18n parity** | `scripts/check-i18n-parity.py` exits 0 (all 9 locales, no missing keys) |
| Deep scanners | Informational / debt burn-down — red here does **not** mean “do not merge” unless you opted in with `ci-deep` |

Concurrency: one run per workflow + event + ref; superseded PR/dispatch runs
cancel. Tag runs never cancel (half-published release is worse than a duplicate
build).

---

## Dependabot

- **Actions bumps that only touch scanner/scorecard workflows** → no Build &
  Release, no deep scanners on the PR. Merge after a glance at the diff.
- **Actions bumps that touch `build.yml` / `qml-smoke.yml`** → full product
  gate runs (verifies the pin in the real build).
- Deep scanners stay on the weekly cron; they are not re-run per bump.

---

## Opting into deep CI on a PR

1. Add the label **`ci-deep`** to the pull request, **or**
2. Actions tab → pick the workflow → Run workflow → select the PR branch.

Do this for engine/threading-sensitive changes, not for copy or workflow pins.

---

## Known debt (gated, not deleted)

| Workflow | Why gated |
|----------|-----------|
| **clang-tidy** | Large warning backlog; fails closed on any warning — weekly burn-down |
| **cppcheck** | Qt `QDataStream >>` false positives historically; paths updated + suppressions; still not a merge gate |
| **Claude Code Review** | Was auto-on every PR and failed with nobody watching — manual only |

Fix the debt or keep them gated. Do not re-enable as required PR checks until
they are reliably green on `main`.
