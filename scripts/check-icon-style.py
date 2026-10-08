#!/usr/bin/env python3
"""Icon style gate: one stroke weight, and a declared list of exceptions.

The set drifted once already — six solid icons sat among seven stroked ones in
the same toolbar row, with no rule the eye could learn, because the convention
lived in a commit message instead of a check. An icon that wants to break the
rule has to say so here, in a diff someone reviews.

Usage: scripts/check-icon-style.py
"""
import re
import sys
from pathlib import Path

ICONS = Path(__file__).resolve().parent.parent / "src" / "icons"
STROKE = "2.2"

# Solid by design: the transport controls, the two carets (an outline at caret
# size is a smudge), and the donate heart, which is a symbol rather than an
# action.
SOLID = {
    "pause.svg", "play.svg", "stop.svg",
    "caret-down-fill.svg", "caret-up-fill.svg",
    "heart.svg",
}

# Heavier on purpose: these are drawn small and need the extra weight.
HEAVIER = {
    "chevron-bold.svg": "2.4",
    "close-bold.svg": "2.4",
    "lock-solid.svg": "2",
    "lock-open-solid.svg": "2",
}

WIDTH_RE = re.compile(r'stroke-width="([0-9.]+)"')


def main() -> int:
    if not ICONS.is_dir():
        print(f"icon-style: {ICONS} not found", file=sys.stderr)
        return 2

    problems = []
    for svg in sorted(ICONS.glob("*.svg")):
        name = svg.name
        widths = set(WIDTH_RE.findall(svg.read_text(encoding="utf-8")))

        if not widths:
            if name not in SOLID:
                problems.append(
                    f"{name}: solid, but not in the SOLID list. Either give it "
                    f'stroke-width="{STROKE}" or add it here with a reason.')
            continue

        if name in SOLID:
            problems.append(f"{name}: listed as SOLID but carries a stroke.")
            continue

        want = HEAVIER.get(name, STROKE)
        off = {w for w in widths if w != want}
        if off:
            problems.append(
                f"{name}: stroke-width {sorted(off)} — expected {want}.")

    stale = sorted((SOLID | set(HEAVIER)) - {p.name for p in ICONS.glob("*.svg")})
    for name in stale:
        problems.append(f"{name}: listed as an exception but no longer exists.")

    total = len(list(ICONS.glob("*.svg")))
    if problems:
        print(f"icon-style: {len(problems)} problem(s) across {total} icons")
        for p in problems:
            print(f"  {p}")
        return 1

    print(f"icon-style: {total} icons, one stroke weight ({STROKE}), "
          f"{len(SOLID)} declared solid — OK")
    return 0


if __name__ == "__main__":
    sys.exit(main())
