#!/usr/bin/env bash
# Local dev build that embeds the metadata API keys from .env (same keys the
# GitHub Actions build uses). Reconfigures cmake so the BAT_*_KEY compile
# definitions pick up the env vars, then builds.
#
# Usage: ./scripts/dev-build.sh
set -euo pipefail
cd "$(dirname "$0")/.."

if [[ -f .env ]]; then
  set -a; source .env; set +a
else
  echo "warning: no .env found — building without metadata API keys (no posters)."
fi

[[ -n "${BAT_TMDB_KEY:-}" ]] && echo "TMDB key:  present" || echo "TMDB key:  (empty — posters will fail)"
[[ -n "${BAT_IGDB_ID:-}" ]]  && echo "IGDB id:   present" || echo "IGDB id:   (empty — game art disabled)"

QT_PREFIX="$(brew --prefix qt@6 2>/dev/null || brew --prefix qt)"
LT_PREFIX="$(brew --prefix libtorrent-rasterbar)"
SSL_PREFIX="$(brew --prefix openssl@3)"

# Reconfigure so the compile-time key definitions are re-evaluated, then build.
cmake -B build \
  -DCMAKE_PREFIX_PATH="${QT_PREFIX};${LT_PREFIX};${SSL_PREFIX}" \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build

echo "done — run: ./build/BATorrent.app/Contents/MacOS/BATorrent"
