#!/usr/bin/env bash
# Fail the macOS .app if macdeployqt left it unloadable (missing frameworks /
# dylibs, QML boot errors). Pass the path to BATorrent.app.
set -euo pipefail

APP="${1:?usage: check-macos-bundle.sh path/to/BATorrent.app}"
BIN="$APP/Contents/MacOS/BATorrent"

if [[ ! -x "$BIN" ]]; then
  echo "::error::executable missing: $BIN"
  exit 1
fi

for fw in QtCore QtGui QtWidgets QtNetwork QtSvg QtMultimedia QtQuick QtQml QtOpenGL; do
  if [[ ! -d "$APP/Contents/Frameworks/${fw}.framework" ]]; then
    echo "::error::Framework missing: ${fw}.framework — app would not open after macdeployqt"
    exit 1
  fi
  echo "OK ${fw}.framework"
done

if [[ ! -d "$APP/Contents/PlugIns/platforms" ]]; then
  echo "::error::PlugIns/platforms missing — Cocoa platform plugin absent"
  exit 1
fi

unresolved=0
while IFS= read -r line; do
  dep=$(echo "$line" | awk '{print $1}')
  [[ -z "$dep" ]] && continue
  case "$dep" in
    /usr/lib/*|/System/*|/System/Library/*) continue ;;
    @rpath/*)
      name=${dep#@rpath/}
      if [[ ! -e "$APP/Contents/Frameworks/$name" && ! -e "$APP/Contents/MacOS/$name" ]]; then
        base=${name%%/*}
        if [[ ! -d "$APP/Contents/Frameworks/$base" ]]; then
          echo "::error::unresolved @rpath dep: $dep"
          unresolved=1
        fi
      fi
      ;;
    /*)
      if [[ ! -e "$dep" ]]; then
        echo "::error::absolute dep missing on runner (and not bundled): $dep"
        unresolved=1
      fi
      ;;
  esac
done < <(otool -L "$BIN" | tail -n +2)

if [[ $unresolved -ne 0 ]]; then
  exit 1
fi
echo "OK otool dependency scan"

export QT_QPA_PLATFORM=offscreen
export BAT_QML_STRICT=warn
rm -f boot.log
"$BIN" >boot.log 2>&1 &
pid=$!
sleep 12
still=0
if kill -0 "$pid" 2>/dev/null; then
  still=1
  kill "$pid" 2>/dev/null || true
  wait "$pid" 2>/dev/null || true
else
  wait "$pid" 2>/dev/null || true
fi

echo "=== packaged .app boot log (tail) ==="
tail -40 boot.log || true

if [[ $still -ne 1 ]]; then
  echo "::error::packaged .app exited within 12s — would not open for users"
  exit 1
fi

if grep -Eq 'failed to load component|is not a type|TypeError:|ReferenceError:|Unable to assign|Invalid property assignment|Cannot specify .* anchors for items inside|is instantiated recursively|Library not loaded|image not found|dyld\[' boot.log; then
  echo "::error::QML/dyld errors on packaged .app boot"
  grep -En 'failed to load component|is not a type|TypeError:|ReferenceError:|Unable to assign|Invalid property assignment|Cannot specify|instantiated recursively|Library not loaded|image not found|dyld\[' boot.log | head -20
  exit 1
fi

echo "macOS bundle package smoke OK"
