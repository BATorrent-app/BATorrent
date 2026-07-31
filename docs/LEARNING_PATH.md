# BATorrent — learning path (agreement)

Durable note for future agents and the human. **Canonical copy** (tracked in
git). Local `internal/LEARNING_PATH.md` is a pointer only — `/internal/` is
gitignored.

**Do not start deep teaching while the map is still moving.** Product quality
gate first; learning is gated on **Ótimo**. Related local notes (may be
gitignored under `internal/`): `SPRINT_ROADMAP.md` (Ótimo gate),
`QUALITY_PLAN.md`, `REVIEW.md`.

---

## Priority until Ótimo

1. **Product quality first** until the codebase is judged **ótimo** (structure,
   hotspots, tests, navigable folders). Soft ceilings, peels, reorg, and test
   nets win over pedagogy.
2. **Do not** start deep function-by-function teaching while peels still move
   the map. Learning essays that freeze obsolete structure are an anti-pattern
   (see SPRINT_ROADMAP anti-patterns).
3. **Parallel:** C++/Qt teaching is **secondary** to product quality until Ótimo.
   Mentoring can happen in short asides; structural work is the main thread.

Ótimo exit criteria live in `SPRINT_ROADMAP.md` / `QUALITY_PLAN.md` / `REVIEW.md`
(local `internal/` when present). When the gate is declared — and only then —
switch to the learning pass below.

---

## Skill level (do not mix these up)

Two different gaps:

1. **Languages they already know** (JS/TS/Python/Java-ish): syntax is fine;
   “I know syntax but struggle to invent solutions” means **rusty problem-solving
   muscle** — inventing decompositions, not missing `if`/`for`.
2. **C++/Qt:** **zero tooling literacy**. Do **not** assume they know the
   package.json equivalent, build commands, include paths, moc, qrc, CMake
   targets, etc. Treat the toolchain as new ground.

Rebuild logic muscle with small reps in known languages; teach BATorrent’s
C++/Qt map explicitly (survival card below) before assuming any of it.

---

## After Ótimo — how to learn

**Before / alongside** function-by-function vertical slices, give a short
**C++/Qt survival card for BATorrent** (section below). Then learn **vertically
by user action**, not by random file walks.

### First slice (validated)

**Watch → `playFile` → `PlayerWindow`**

The user already validated player playback manually. Start here: follow one
user action end-to-end (QML entry → bridge/call → player surface), not a
directory dump.

### At each stop

For every function / component / signal on the path:

- **What goes in** (args, properties, model roles, signals)
- **What comes out** (return, side effects, emitted signals)
- **Why the name exists** (what idea it names in this codebase)

Names like `modelData` only after knowing **which** Repeater / list owns them.
Context first; identifier second.

### Session shape

- Keep sessions short: **30–45 min**
- Rebuild logic muscle with **small reps** (one vertical slice per sitting,
  not a whole subsystem lecture)

### Goal of the learning pass

Understand enough that **obvious comments aren't needed**. Keep only *why*
comments (quirks, libtorrent/Qt constraints, non-obvious invariants) — same bar
as `CLAUDE.md`.

---

## C++/Qt survival card for BATorrent

Concise map — teach this before/alongside the first vertical slice:

| Concept | Here |
|---------|------|
| `package.json` + build graph | `CMakeLists.txt` |
| Canonical build | `scripts/dev-build-fork.sh` (sources `.env` for embedded keys) |
| Out dir / binary | `build-fork/`; run `./build-fork/BATorrent.app/Contents/MacOS/BATorrent` (kill stale instances; don’t `open` the installed copy) |
| Includes | Target has `-Isrc` → `#include "services/..."` (root-relative from `src/`; no `../`) |
| QML packing | `src/resources.qrc` |
| After any `.qml` edit | Rebuild + launch with `BAT_QML_STRICT=warn`; read stderr (QML errors are runtime-only) |
| Tests | Catch2 under `tests/` |

moc / qrc / CMake are part of “how the app exists,” not optional trivia — explain
when a change touches them.

---

## Mindset (Qt / C++)

Qt/C++ feel hard because **APIs are half the language** and ideas span many
files — plus the tooling above is unfamiliar. That is normal; rusty
problem-splitting in *known* languages is a separate issue from C++ literacy.
Prefer named stops on a user path over holding the whole graph at once.

---

## QML note (durable)

**QML ≠ CSS.** A `.qml` file is structure + logic + inline style together.

- **Theme** is the CSS-like bit (tokens, shared look).
- **`*Compute` leaves** are logic-only **by design** — do not treat them as
  “views that forgot styling.”

When teaching QML, say what each file’s job is (host composition, chrome,
compute, theme) before diving into bindings.

---

## Agent checklist

- [ ] Ótimo gate declared (or work is structural peel/reorg/test — not teaching)
- [ ] Survival card covered before/alongside first vertical slice (no assumed npm/CMake literacy)
- [ ] Teaching follows a user action vertically
- [ ] First learning slice: Watch → playFile → PlayerWindow
- [ ] Stops explain in / out / name; no orphan `modelData` lectures
- [ ] Sessions 30–45 min; small reps
- [ ] Comment bar: why only; learning should reduce narrating comments, not add them
- [ ] Do not confuse “rusty inventing solutions” (known langs) with C++/Qt zero tooling
- [ ] C++ pedagogy remains secondary until Ótimo
