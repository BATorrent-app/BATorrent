#!/usr/bin/env bash
# Apply the BATorrent libtorrent fork patches onto the vendored submodule.
#
# The submodule tracks upstream arvidn/libtorrent; our engine patches live as
# versioned diffs in third_party/patches/ instead of as un-committed working-tree
# edits (which vanish on a fresh clone). Run this after `git submodule update`,
# before building with BAT_LIBTORRENT_SOURCE=ON.
#
# Idempotent: resets the submodule to its pinned commit, then re-applies every
# patch — so running it twice is a no-op, not a double-apply failure.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

SUB=third_party/libtorrent
PATCHES=third_party/patches

if [[ ! -e "$SUB/.git" ]]; then
  echo "error: submodule $SUB not initialized." >&2
  echo "       run: git submodule update --init --recursive" >&2
  exit 1
fi

# Pristine tracked tree, so re-runs start clean (drops any prior patch state).
git -C "$SUB" checkout -- .

shopt -s nullglob
patches=("$PATCHES"/*.patch)
if [[ ${#patches[@]} -eq 0 ]]; then
  echo "no patches in $PATCHES — nothing to apply."
  exit 0
fi

for p in "${patches[@]}"; do
  echo "applying $(basename "$p")"
  git -C "$SUB" apply "$ROOT/$p"
done
echo "fork patches applied (${#patches[@]})."
