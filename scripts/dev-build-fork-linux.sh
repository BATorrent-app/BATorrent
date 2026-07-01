#!/usr/bin/env bash
# Local dev build of the FORK on LINUX — the Homebrew-free mirror of
# dev-build-fork.sh. Qt comes from aqt (~/Qt/6.7.*/gcc_64), OpenSSL/boost from
# the distro. Run it from bash (NOT fish): the `source .env` step needs bash.
#
# ALWAYS use this (not a bare `cmake -B build-fork`) so the BAT_*_KEY compile
# definitions pick up .env — a bare reconfigure re-evaluates $ENV{BAT_TMDB_KEY}
# as empty and silently strips the keys → no posters / "auth required".
#
# Usage: bash scripts/dev-build-fork-linux.sh [--bench] [--asan]
#   --bench  also build bat_bench
#   --asan   build with AddressSanitizer + UBSan into build-fork-asan/
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

# Official Qt 6.7 (aqt) ships the FFmpeg media backend → built-in player decodes
# MKV/AVI/WebM, matching the release. BAT_QT_PREFIX overrides; else newest
# ~/Qt/6.7.*/gcc_64 kit.
QT_PREFIX="${BAT_QT_PREFIX:-}"
if [[ -z "$QT_PREFIX" ]]; then
  QT_PREFIX="$(ls -d "$HOME"/Qt/6.7.*/gcc_64 2>/dev/null | sort -V | tail -1 || true)"
fi
if [[ -z "$QT_PREFIX" || ! -d "$QT_PREFIX" ]]; then
  echo "error: no Qt 6.7 (gcc_64) found. Install it with:" >&2
  echo "  pip install aqtinstall && aqt install-qt linux desktop 6.7.3 gcc_64 -m qtmultimedia -O ~/Qt" >&2
  echo "or export BAT_QT_PREFIX=/path/to/your/Qt/6.7.x/gcc_64" >&2
  exit 1
fi
echo "Qt prefix: $QT_PREFIX"

# libtorrent is built from source (the fork). OpenSSL/boost are found via the
# system (apt: libssl-dev libboost-all-dev), so no prefix needed here.
CMAKE_ARGS=(
  -DBAT_LIBTORRENT_SOURCE=ON
  -DBAT_BENCH=ON
  -DCMAKE_PREFIX_PATH="$QT_PREFIX"
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
cmake --build "$BUILD_DIR" --target BATorrent -j"$(nproc)"
[[ "$BENCH" == 1 ]] && cmake --build "$BUILD_DIR" --target bat_bench -j"$(nproc)"

BIN="./$BUILD_DIR/BATorrent"
echo "done — run: $BIN"
