#!/usr/bin/env bash
# Native Linux CI helper. Development source only; never a runtime launcher.
set -euo pipefail
ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"
BUILD="$ROOT/build-ci-linux"
export CMAKE_BUILD_PARALLEL_LEVEL="${CMAKE_BUILD_PARALLEL_LEVEL:-2}"
export EMCICALC_PREFIX="$ROOT/.dependencies/prefix"
export PKG_CONFIG_PATH="$EMCICALC_PREFIX/lib/pkgconfig${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}"
export LD_LIBRARY_PATH="$EMCICALC_PREFIX/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
mkdir -p "$ROOT/.ci-results"
SCOPE="${CI_NATIVE_SCOPE:-full}"
case "$SCOPE" in
  full)
    TARGETS=(EMCICalc engine_tests exchangerates_tests historyarchive_tests keyboard_tests
      desktop_archive_tests desktop_layout_tests desktop_settings_persistence_tests
      desktop_caption_tests desktop_button_text_tests panelradix_tests)
    TESTS='^(engine|exchangerates|historyarchive|keyboard|desktop_archive|desktop_layout|desktop_settings_persistence|desktop_caption|desktop_button_text|panelradix)$'
    ;;
  captions)
    TARGETS=(desktop_caption_tests)
    TESTS='^desktop_caption$'
    ;;
  keypad)
    TARGETS=(panelradix_tests)
    TESTS='^panelradix$'
    ;;
  caption_keypad)
    TARGETS=(EMCICalc desktop_caption_tests panelradix_tests)
    TESTS='^(desktop_caption|panelradix)$'
    ;;
  *) echo 'CI_NATIVE_SCOPE must be full, captions, keypad or caption_keypad' >&2; exit 2 ;;
esac
printf '%s\n' "$SCOPE" > "$ROOT/.ci-results/native-scope.txt"

case "${1:-}" in
  build)
    bash scripts/bootstrap-linux.sh
    [[ "$(pkg-config --modversion libqalculate)" == 5.12.0 ]] || { echo 'Unexpected libqalculate version' >&2; exit 1; }
    cmake -S . -B "$BUILD" -G Ninja -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_TESTING=ON -DCMAKE_PREFIX_PATH="$EMCICALC_PREFIX"
    cmake --build "$BUILD" --parallel "$CMAKE_BUILD_PARALLEL_LEVEL" --target "${TARGETS[@]}"
    ;;
  test)
    TEST_ROOT="$(mktemp -d)"
    trap 'rm -rf -- "$TEST_ROOT"' EXIT
    export XDG_CONFIG_HOME="$TEST_ROOT/config"
    export XDG_CACHE_HOME="$TEST_ROOT/cache"
    export XDG_DATA_HOME="$TEST_ROOT/data"
    export XDG_RUNTIME_DIR="$TEST_ROOT/runtime"
    mkdir -p "$XDG_CONFIG_HOME" "$XDG_CACHE_HOME" "$XDG_DATA_HOME" "$XDG_RUNTIME_DIR"
    chmod 700 "$XDG_RUNTIME_DIR"
    export QT_QPA_PLATFORM=offscreen
    export QT_SCALE_FACTOR=1
    export QT_AUTO_SCREEN_SCALE_FACTOR=0
    export LC_ALL=C.UTF-8
    ctest --test-dir "$BUILD" --output-on-failure --no-tests=error \
      --parallel 2 --timeout 180 --output-junit "$ROOT/.ci-results/linux-tests.xml" \
      -R "$TESTS"
    ;;
  *)
    echo 'Usage: bash scripts/ci_linux.sh {build|test}' >&2
    exit 2
    ;;
esac
