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
   muscle** — inventing decompositions, not missing `if`/`for`. Concrete
   example of the target skill: `CommandPalette.qml` `score()` (substring /
   subsequence fuzzy-match scoring) currently reads like cave drawings — the
   goal is to invent and understand that kind of small algorithm, not just
   recite syntax.
2. **C++/Qt:** **zero background**. Do **not** assume they know the
   package.json equivalent, build commands, include paths, moc, qrc, CMake
   targets, what a pointer is, or how C++ relates to C. Treat fundamentals and
   the toolchain as new ground from zero.

Rebuild logic muscle with small reps in known languages; teach BATorrent’s
C++/Qt map explicitly (survival card below) before assuming any of it.

---

## Goals of the learning phase (after Ótimo)

1. **Dust off programming** — regain the ability to *invent and understand*
   solutions to small algorithms (decompositions, scoring, matching), not only
   follow along with finished code.
2. **Deep fundamentals from zero** — relationship of C++ to C, modern C++
   patterns, the toolchain, and Qt/QML. No assumed prior knowledge; define
   terms when first used; build each idea on what was just taught.

---

## Teaching style (non-negotiable)

1. **No superficiality.** Prefer months of specialist-depth over a few hours
   that only produce junior-level gloss. Choose depth every time.
2. **Time is not the constraint.** Willing to spend a year on a single area
   (e.g. the BitTorrent protocol alone) if that is what depth requires. Do not
   rush breadth.
3. **No haste.** Do not skip stages. Do not jump ahead to “the interesting
   part.” Prerequisites before payoffs.
4. **Assume nothing.** Explain end-to-end without presuming prior knowledge
   (not even “you know what a pointer is”). Define terms when first used.
   Relate new ideas to what was just built, not to unspoken industry lore.
5. **Still respect the gate.** Quality / Ótimo cleanup first; then learning.
   When teaching: survival card → vertical Watch slice → deepen; within each
   topic go specialist-deep.

### Anti-patterns for the teacher

- No “as you know…” / “obviously…”
- No skipping “boring” prerequisites to get to the fun part
- No survey-course pace (breadth over depth, “we’ll cover X later”)
- No assuming JS/TS knowledge transfers cleanly to C++ (ownership, memory,
  compilation, types, build graph are different worlds)

---

## After Ótimo — how to learn

**Before / alongside** function-by-function vertical slices, give a short
**C++/Qt survival card for BATorrent** (section below). Then learn **vertically
by user action**, not by random file walks. Within each topic, go
specialist-deep — do not skim and move on.

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
Context first; identifier second. Define every term the first time it appears.

### Session shape

- Keep sessions short: **30–45 min**
- Rebuild logic muscle with **small reps** (one vertical slice per sitting,
  not a whole subsystem lecture)
- Depth across many sessions beats a shallow tour in one sitting

### Goal of the learning pass

Understand enough that **obvious comments aren't needed**. Keep only *why*
comments (quirks, libtorrent/Qt constraints, non-obvious invariants) — same bar
as `CLAUDE.md`.

---

## C++/Qt survival card for BATorrent

Concise map — teach this before/alongside the first vertical slice. Explain
each row from zero; do not assume the left column is already known:

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
Depth over speed: one solid foundation beat a thin map of everything.

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
- [ ] Sessions 30–45 min; small reps; specialist depth within each topic
- [ ] No superficiality / no haste / assume nothing (define terms; no skipped stages)
- [ ] Comment bar: why only; learning should reduce narrating comments, not add them
- [ ] Do not confuse “rusty inventing solutions” (known langs) with C++/Qt from-zero literacy
- [ ] C++ pedagogy remains secondary until Ótimo
- [ ] Teacher anti-patterns avoided (“as you know”, skipped prereqs, survey pace, JS→C++ transfer assumptions)
