#!/usr/bin/env bash
# Local dev build of the FORK (vendored libtorrent in third_party/libtorrent +
# our engine patches), embedding the metadata API keys from .env — same as
# dev-build.sh but with BAT_LIBTORRENT_SOURCE=ON into build-fork/.
#
# ALWAYS use this (not a bare `cmake -B build-fork`) so the BAT_*_KEY compile
# definitions pick up .env. A bare reconfigure re-evaluates $ENV{BAT_TMDB_KEY}
# as empty and silently strips the keys → no posters / "auth required".
#
# Usage: ./scripts/dev-build-fork.sh [--bench] [--asan]
#   --bench  also build bat_bench
#   --asan   build the GUI app with AddressSanitizer + UBSan into build-fork-asan/
#            (separate dir so the fast build stays intact). Run it during your own
#            testing: a use-after-free/overflow in a real flow (IPC, stream,
#            player) stops immediately with a full stack instead of crashing
#            silently later. See internal/DEBUGGING.md.
set -euo pipefail
cd "$(dirname "$0")/.."

BUILD_DIR=build-fork
BENCH=0
SAN_FLAGS=""
for arg in "$@"; do
  case "$arg" in
    --bench) BENCH=1 ;;
    --asan)
      BUILD_DIR=build-fork-asan
      SAN_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer -g" ;;
    *) echo "unknown arg: $arg (use --bench and/or --asan)" >&2; exit 2 ;;
  esac
done

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
CMAKE_ARGS=(
  -DBAT_LIBTORRENT_SOURCE=ON
  -DBAT_BENCH=ON
  -DCMAKE_PREFIX_PATH="${QT_PREFIX};${SSL_PREFIX}"
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
)
if [[ -n "$SAN_FLAGS" ]]; then
  CMAKE_ARGS+=(
    -DCMAKE_BUILD_TYPE=RelWithDebInfo
    -DCMAKE_CXX_FLAGS="$SAN_FLAGS"
    -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined"
  )
fi

cmake -B "$BUILD_DIR" "${CMAKE_ARGS[@]}"
cmake --build "$BUILD_DIR" --target BATorrent
[[ "$BENCH" == 1 ]] && cmake --build "$BUILD_DIR" --target bat_bench

BIN="./$BUILD_DIR/BATorrent.app/Contents/MacOS/BATorrent"
if [[ -n "$SAN_FLAGS" ]]; then
  echo "done (ASan+UBSan) — run with:"
  # detect_container_overflow=0: Qt/libs aren't ASan-built, so cross-boundary
  # std::vector checks would false-positive.
  echo "  ASAN_OPTIONS=abort_on_error=1:detect_leaks=0:detect_container_overflow=0 \\"
  echo "  UBSAN_OPTIONS=print_stacktrace=1 BAT_QML_STRICT=warn $BIN"
else
  echo "done — run: $BIN"
fi
