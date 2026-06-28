#!/usr/bin/env bash
# Static lint of the QML sources, tuned as a REGRESSION GATE — not a bug finder.
#
# Without project type-info, qmllint can't resolve our C++-registered objects
# (session, i18n, Theme) or custom widgets (TFld, etc.), so these categories are
# almost entirely noise HERE and are silenced:
#   unqualified       — every session./i18n./Theme. access (C++ context props)
#   comma             — our deliberate `(i18n.language, i18n.t(...))` re-eval idiom
#   missing-property  — props on custom delegates qmllint sees as plain Item
#   unused-imports    — informational
#
# What's left fails the build and reliably means a real bug:
#   duplicate-property-binding, equality-type-coercion (== vs ===),
#   property-override (shadowing a built-in like `state`/`data`), unresolved-type…
#
# Usage:
#   scripts/qml-lint.sh           # report real-signal warnings + count (exit 0)
#   scripts/qml-lint.sh --strict  # same, but exit !=0 if any remain (CI gate)
#   scripts/qml-lint.sh --all     # full unfiltered report (browse all categories)
set -uo pipefail
cd "$(dirname "$0")/.."

STRICT=0
[[ "${1:-}" == "--strict" ]] && { STRICT=1; shift; }

QMLLINT="${QMLLINT:-$(command -v qmllint 2>/dev/null || true)}"
if [[ -z "$QMLLINT" ]]; then
  for base in "$(brew --prefix qtdeclarative 2>/dev/null)" "$(brew --prefix qt 2>/dev/null)" "$HOME/Qt"; do
    [[ -n "$base" ]] || continue
    found=$(find -L "$base" -name qmllint -type f 2>/dev/null | head -1)   # -L: follow Homebrew opt symlink
    [[ -n "$found" ]] && { QMLLINT="$found"; break; }
  done
fi
[[ -z "$QMLLINT" ]] && { echo "qmllint not found — set QMLLINT=/path/to/qmllint" >&2; exit 127; }

mapfile -t FILES < <(find src/qml -name '*.qml')

if [[ "${1:-}" == "--all" ]]; then
  exec "$QMLLINT" -I src/qml "${FILES[@]}"
fi

OUT=$("$QMLLINT" -I src/qml \
  --unqualified disable \
  --comma disable \
  --missing-property disable \
  --unused-imports disable \
  "${FILES[@]}" 2>&1)
printf '%s\n' "$OUT"

# qmllint's own exit code ignores plain warnings, so gate on the count ourselves.
n=$(printf '%s\n' "$OUT" | grep -c '^Warning:')
if [[ "$n" -eq 0 ]]; then
  echo "qml-lint: clean (real-signal categories)"
  exit 0
fi
echo "qml-lint: $n real-signal warning(s)"
[[ "$STRICT" -eq 1 ]] && exit 1
exit 0
