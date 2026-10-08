#!/usr/bin/env python3
"""Motion gate: durations and curves come from Theme, not from the file.

Theme already defines the scale (durFast/durBase/durSlow/durExit/durShow and
three curves), but most animations were written with a number chosen by eye in
that one file. Thirty-eight distinct durations, four of them within 40ms of
each other, is why the motion reads as hand-made: there is no shared beat.

This is a ratchet, not a wall. The counts already in the tree are recorded in
motion-baseline.json; a file may not exceed its own count, so new code has to
use the tokens while the existing ones are migrated down. Lowering a count is
always allowed, and --update rewrites the baseline after a migration.

Usage:
  scripts/check-motion-tokens.py            # gate
  scripts/check-motion-tokens.py --update   # re-record after migrating
"""
import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
QML = ROOT / "src" / "qml"
BASELINE = Path(__file__).resolve().parent / "motion-baseline.json"

# Theme.qml is where the scale is defined, so the raw values belong there.
EXEMPT = {"theme/Theme.qml"}

RAW_DURATION = re.compile(r"\bduration:\s*\d+")
RAW_EASING = re.compile(r"\beasing\.type:\s*Easing\.")


def scan() -> dict:
    counts = {}
    for qml in sorted(QML.rglob("*.qml")):
        rel = qml.relative_to(QML).as_posix()
        if rel in EXEMPT:
            continue
        text = qml.read_text(encoding="utf-8")
        n = len(RAW_DURATION.findall(text)) + len(RAW_EASING.findall(text))
        if n:
            counts[rel] = n
    return counts


def main() -> int:
    counts = scan()

    if "--update" in sys.argv:
        BASELINE.write_text(json.dumps(counts, indent=2, sort_keys=True) + "\n",
                            encoding="utf-8")
        print(f"motion: baseline rewritten — {sum(counts.values())} raw values "
              f"in {len(counts)} files")
        return 0

    if not BASELINE.exists():
        print("motion: no baseline; run with --update once to record the "
              "current state", file=sys.stderr)
        return 2

    base = json.loads(BASELINE.read_text(encoding="utf-8"))
    over = {f: (n, base.get(f, 0)) for f, n in counts.items() if n > base.get(f, 0)}

    if over:
        print(f"motion: {len(over)} file(s) added raw durations or curves")
        for f, (now, was) in sorted(over.items()):
            print(f"  {f}: {was} -> {now}")
        print("\nUse the Theme tokens: durFast/durBase/durSlow/durExit/durShow,"
              "\neaseIn/easeOut/easePop. A value that genuinely needs its own"
              "\nnumber should say why in a comment, and raise the baseline"
              "\ndeliberately with --update.")
        return 1

    total, baseline_total = sum(counts.values()), sum(base.values())
    gone = baseline_total - total
    print(f"motion: {total} raw values (baseline {baseline_total}"
          + (f", {gone} migrated" if gone > 0 else "") + ") — OK")
    return 0


if __name__ == "__main__":
    sys.exit(main())
