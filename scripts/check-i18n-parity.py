#!/usr/bin/env python3
"""Fail CI when translation locales drift, duplicate keys, or referenced keys miss.

Parity:   every translations/*.json must share the same key set as en.json.
Dupes:    no file may repeat a key (json.loads keeps last-wins silently).
Usage:    every i18n.t("key") / tr_("key") in src/ must exist in en.json.

Run:      python3 scripts/check-i18n-parity.py
Exit 0 on success, 1 on drift. Prints a concise report either way.
"""
from __future__ import annotations

import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
TRANS = ROOT / "translations"
SRC = ROOT / "src"

# Keys that are built dynamically (prefix + suffix) — skip exact-match requirement
# when the source only has the prefix. Listed explicitly so we still verify the
# composed forms that are known at check time.
DYNAMIC_PREFIXES = (
    "hub_gs_",  # hub game state fragments assembled in QML
)


def _load_object(path: Path) -> tuple[dict, list[str]]:
    """Parse a JSON object; return (data, duplicate_keys)."""
    dups: list[str] = []

    def hook(pairs: list[tuple[str, object]]) -> dict:
        out: dict = {}
        for k, v in pairs:
            if k in out:
                dups.append(k)
            out[k] = v
        return out

    data = json.loads(path.read_text(encoding="utf-8"), object_pairs_hook=hook)
    if not isinstance(data, dict):
        raise SystemExit(f"{path.name}: expected a JSON object")
    return data, dups


def load_locales() -> tuple[dict[str, set[str]], list[str]]:
    out: dict[str, set[str]] = {}
    errs: list[str] = []
    for p in sorted(TRANS.glob("*.json")):
        data, dups = _load_object(p)
        out[p.name] = set(data)
        if dups:
            uniq = sorted(set(dups))
            sample = uniq[:5]
            more = f" (+{len(uniq) - 5} more)" if len(uniq) > 5 else ""
            errs.append(f"{p.name}: {len(uniq)} duplicate key(s) (e.g. {sample}){more}")
    if "en.json" not in out:
        raise SystemExit("translations/en.json missing")
    return out, errs


def check_parity(locales: dict[str, set[str]]) -> list[str]:
    en = locales["en.json"]
    errs: list[str] = []
    for name, keys in locales.items():
        missing = sorted(en - keys)
        extra = sorted(keys - en)
        if missing:
            errs.append(f"{name}: missing {len(missing)} keys (e.g. {missing[:5]})")
        if extra:
            errs.append(f"{name}: extra {len(extra)} keys not in en (e.g. {extra[:5]})")
    return errs


# i18n.t("key") / i18n.t('key') and C++ tr_("key")
KEY_RE = re.compile(
    r"""(?:i18n\.t|tr_)\(\s*(["'])([A-Za-z0-9_.-]+)\1"""
)


def collect_used_keys() -> set[str]:
    used: set[str] = set()
    for path in list(SRC.rglob("*.qml")) + list(SRC.rglob("*.cpp")) + list(SRC.rglob("*.h")):
        text = path.read_text(encoding="utf-8", errors="replace")
        for m in KEY_RE.finditer(text):
            used.add(m.group(2))
    return used


def check_used(en: set[str], used: set[str]) -> list[str]:
    errs: list[str] = []
    missing = sorted(
        k for k in used
        if k not in en and not any(k.startswith(p) for p in DYNAMIC_PREFIXES)
    )
    if missing:
        sample = missing[:20]
        more = f" (+{len(missing) - 20} more)" if len(missing) > 20 else ""
        errs.append(f"referenced keys missing from en.json: {sample}{more}")
    return errs


def main() -> int:
    locales, errs = load_locales()
    errs.extend(check_parity(locales))
    used = collect_used_keys()
    errs.extend(check_used(locales["en.json"], used))

    print(f"i18n: {len(locales)} locales, {len(locales['en.json'])} en keys, "
          f"{len(used)} referenced keys scanned")
    if errs:
        for e in errs:
            print(f"ERROR: {e}", file=sys.stderr)
        return 1
    print("i18n parity + key usage OK")
    return 0


if __name__ == "__main__":
    sys.exit(main())
