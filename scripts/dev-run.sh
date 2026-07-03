#!/usr/bin/env bash
# Launch the freshly built dev app the way the project rules require:
# kill the stale instance first, surface QML errors loudly, and — if the
# locally built Qt ffmpeg media plugin exists (dev-qt-plugins/) — put it on
# QT_PLUGIN_PATH so the built-in player decodes MKV/AC3 like release builds
# (homebrew Qt ships only the AVFoundation backend; see internal/REVIEW.md).
#
# Usage: ./scripts/dev-run.sh [build-dir]   (default: build-fork)
set -euo pipefail
cd "$(dirname "$0")/.."

BUILD_DIR="${1:-build-fork}"
APP="$BUILD_DIR/BATorrent.app/Contents/MacOS/BATorrent"
[ -x "$APP" ] || { echo "no binary at $APP — build first (scripts/dev-build-fork.sh)"; exit 1; }

pkill -x BATorrent 2>/dev/null && sleep 1 || true

if [ -d dev-qt-plugins/multimedia ]; then
  export QT_PLUGIN_PATH="$PWD/dev-qt-plugins${QT_PLUGIN_PATH:+:$QT_PLUGIN_PATH}"
fi
export BAT_QML_STRICT="${BAT_QML_STRICT:-warn}"

exec "$APP"
