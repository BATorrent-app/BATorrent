#!/usr/bin/env bash
# Local dev build of the FORK (vendored libtorrent in third_party/libtorrent +
# our engine patches), embedding the metadata API keys from .env — same as
# dev-build.sh but with BAT_LIBTORRENT_SOURCE=ON into build-fork/.
#
# ALWAYS use this (not a bare `cmake -B build-fork`) so the BAT_*_KEY compile
# definitions pick up .env. A bare reconfigure re-evaluates $ENV{BAT_TMDB_KEY}
# as empty and silently strips the keys → no posters / "auth required".
#
# Usage: ./scripts/dev-build-fork.sh [--bench]   (--bench also builds bat_bench)
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
SSL_PREFIX="$(brew --prefix openssl@3)"

# libtorrent is built from source (the fork), so no LT prefix here.
cmake -B build-fork \
  -DBAT_LIBTORRENT_SOURCE=ON \
  -DBAT_BENCH=ON \
  -DCMAKE_PREFIX_PATH="${QT_PREFIX};${SSL_PREFIX}" \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build-fork --target BATorrent
[[ "${1:-}" == "--bench" ]] && cmake --build build-fork --target bat_bench

echo "done — run: ./build-fork/BATorrent.app/Contents/MacOS/BATorrent"
