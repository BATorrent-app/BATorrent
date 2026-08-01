# AGENTS.md

## Cursor Cloud specific instructions

BATorrent is a single C++17 / Qt 6 desktop torrent client (built on libtorrent).
The one executable also hosts the built-in WebUI and streaming server. Standard
build/run/test commands live in `README.md` ("Build from source"),
`docs/TESTING.md`, `docs/CI.md`, and `scripts/` — prefer those. The notes below
are only the non-obvious, environment-specific gotchas.

### Toolchain already provisioned (baked into the VM snapshot)

- **Qt 6.7.3** is installed via `aqt` at `~/Qt/6.7.3/gcc_64` (Ubuntu's apt Qt is
  too old — the QML needs `QtQuick.Effects`/`MultiEffect`, 6.5+). Always point
  CMake at it: `-DCMAKE_PREFIX_PATH=$HOME/Qt/6.7.3/gcc_64`.
- apt build deps are installed: `libtorrent-rasterbar-dev` (2.0.10), `libboost-dev`,
  `libssl-dev`, GL/EGL/xcb runtime libs, `imagemagick`. QtKeychain is NOT installed,
  so secrets fall back to plaintext QSettings — expected/fine for dev.

### Gotcha: must build with gcc/g++, not the default `c++`

The default `c++`/`cc` alternative is `clang++`, which auto-selects an incomplete
gcc-14 toolchain and fails linking with `cannot find -lstdc++`. Always configure
CMake with the GNU compilers:

```
-DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++
```

(equivalently `export CC=gcc CXX=g++` before running any `scripts/dev-build*.sh`,
which otherwise inherit the broken `c++`).

### Build & test (dev)

- Tests (fast, uses system libtorrent):
  `cmake -B build-tests -DBAT_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=$HOME/Qt/6.7.3/gcc_64 -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++`
  then `cmake --build build-tests -j$(nproc)`.
- App (dev, links system libtorrent — much faster than the vendored fork):
  `cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=$HOME/Qt/6.7.3/gcc_64 -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++`
  then `cmake --build build -j$(nproc)`. The vendored engine fork
  (`-DBAT_LIBTORRENT_SOURCE=ON`, per `scripts/dev-build-fork-linux.sh`) is only
  needed for release/engine work and requires `git submodule update --init
  --recursive` + `scripts/apply-fork-patches.sh` first.

### Running the tests

- Run Catch2 binaries directly and gate on the string `All tests passed`: the
  harness can segfault in Qt-singleton static teardown *after* a passing run (no
  `QCoreApplication`), so a nonzero exit code alone is not a real failure. Use
  `LC_ALL=C QT_QPA_PLATFORM=offscreen`.
- The QML suite is `./build-tests/tests/test_qml` (run with
  `QT_QPA_PLATFORM=offscreen`); `ctest -R test_qml` reports "No tests were found"
  — run the binary directly.
- WebUI JS tests: `node --test tests/webui/test_webui.mjs` (Node is preinstalled).

### Lint

- QML: `QMLLINT=$HOME/Qt/6.7.3/gcc_64/bin/qmllint bash scripts/qml-lint.sh`.
  Note: this newer qmllint prints `Unknown option 'comma'` but the script still
  reports `qml-lint: clean` and exits 0 — that is the intended pass state.
- i18n parity: `python3 scripts/check-i18n-parity.py`.

### Running the GUI

A virtual X display is available on `:1`. Launch the app with:

```
DISPLAY=:1 LD_LIBRARY_PATH=$HOME/Qt/6.7.3/gcc_64/lib QT_QPA_PLATFORM=xcb ./build/BATorrent
```

Look for `[boot] first frame presented — boot healthy` in stdout. For a headless
sanity boot use `QT_QPA_PLATFORM=offscreen`. Adding a `.torrent`/magnet and
watching it download works end-to-end (egress is open).
