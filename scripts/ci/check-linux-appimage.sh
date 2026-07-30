#!/usr/bin/env bash
# Fail the Linux AppImage if it would not open: missing libs, stock libtorrent,
# or QML boot errors. Run from the repo root after the AppImage is built.
set -euo pipefail

APPIMAGE="${1:?usage: check-linux-appimage.sh path/to/BATorrent-*.AppImage}"
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
WORKDIR=$(mktemp -d)
trap 'rm -rf "$WORKDIR"' EXIT

cp "$APPIMAGE" "$WORKDIR/app.AppImage"
chmod +x "$WORKDIR/app.AppImage"
cd "$WORKDIR"
./app.AppImage --appimage-extract >/dev/null

BIN=$(find squashfs-root -type f -name BATorrent | head -1)
if [[ -z "$BIN" ]]; then
  echo "::error::BATorrent binary missing inside AppImage"
  exit 1
fi
echo "binary: $BIN"

# Match AppRun's library search so ldd/boot see bundled .so's, not the host's.
libdirs=$(find squashfs-root -type d \( -name lib -o -name lib64 \) | tr '\n' ':')
export LD_LIBRARY_PATH="${libdirs}${LD_LIBRARY_PATH:-}"

# Fork symbol guard (issue #32)
lib=$(find squashfs-root -name 'libtorrent-rasterbar.so*' | head -1)
if [[ -z "$lib" ]]; then
  echo "::error::no libtorrent in the AppImage — launch would crash"
  exit 1
fi
if ! nm -D --defined-only "$lib" 2>/dev/null | grep -q set_geo_local_fn; then
  echo "::error::bundled libtorrent lacks set_geo_local_fn — stock lib shipped (issue #32)"
  exit 1
fi
echo "OK fork libtorrent: $lib"

# Unresolved shared libs against the bundled tree
missing=$(ldd "$BIN" 2>/dev/null | grep 'not found' || true)
if [[ -n "$missing" ]]; then
  echo "::error::AppImage binary has unresolved libs (with AppDir on LD_LIBRARY_PATH):"
  echo "$missing"
  exit 1
fi
echo "OK ldd: no 'not found'"

# Actual boot of the *packaged* tree (not the unpackaged build/).
# Prefer AppRun so RPATH/FUSE layout matches what users execute.
BOOT=./squashfs-root/AppRun
[[ -x "$BOOT" ]] || BOOT="$BIN"
export QT_QPA_PLATFORM=offscreen
export BAT_QML_STRICT=warn
set +e
timeout 25 "$BOOT" >boot.log 2>&1
rc=$?
set -e
echo "=== packaged boot log (tail) ==="
tail -40 boot.log || true

# timeout → 124 is success for us (still running). Early crash → fail.
if [[ $rc -ne 0 && $rc -ne 124 ]]; then
  echo "::error::packaged AppImage exited early (rc=$rc) — would not open for users"
  exit 1
fi
if grep -Eq 'failed to load component|is not a type|TypeError:|ReferenceError:|Unable to assign|Invalid property assignment|Cannot specify .* anchors for items inside|is instantiated recursively|error while loading shared libraries|cannot open shared object' boot.log; then
  echo "::error::QML/loader errors on packaged AppImage boot"
  grep -En 'failed to load component|is not a type|TypeError:|ReferenceError:|Unable to assign|Invalid property assignment|Cannot specify|instantiated recursively|error while loading shared libraries|cannot open shared object' boot.log | head -20
  exit 1
fi

echo "Linux AppImage package smoke OK"
