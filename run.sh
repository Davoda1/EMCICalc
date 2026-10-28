#!/usr/bin/env bash
# Source-development helper only. Distributed applications use native entry points.
set -euo pipefail
ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
ACTION="${1:-help}"
BUILD="${EMCICALC_BUILD_DIR:-$ROOT/build}"
PREFIX="${EMCICALC_PREFIX:-}"
usage() {
  cat <<'EOF'
EMCICalc Desktop Edition 1.0.0 development helper
  ./run.sh help       Show this help
  ./run.sh build      Configure and build release binaries
  ./run.sh test       Build and run CTest
  ./run.sh package    Build and create Linux tar.gz and .deb in dist/
  ./run.sh clean      Remove only the configured build directory
Environment: EMCICALC_BUILD_DIR, EMCICALC_PREFIX (optional CMake prefix),
CMAKE_BUILD_PARALLEL_LEVEL (optional). Requires CMake, Ninja, C++17, Qt 6,
libqalculate and development headers. Packaging also needs Python 3,
patchelf and dpkg-deb. There is no application-launch subcommand.
EOF
}
build() {
  args=(-S "$ROOT" -B "$BUILD" -G Ninja -DCMAKE_BUILD_TYPE=Release)
  if [[ -n "$PREFIX" ]]; then args+=("-DCMAKE_PREFIX_PATH=$PREFIX"); fi
  cmake "${args[@]}"
  cmake --build "$BUILD" --parallel "${CMAKE_BUILD_PARALLEL_LEVEL:-2}"
}
case "$ACTION" in
  help|-h|--help) usage ;;
  build) build ;;
  test) build; ctest --test-dir "$BUILD" --output-on-failure ;;
  package)
    build
    package_args=(--platform linux --build-dir "$BUILD" --output "$ROOT/dist")
    if [[ "${EMCICALC_EMBEDDED_DEFINITIONS:-0}" == 1 ]]; then package_args+=(--compiled-definitions); fi
    python3 "$ROOT/scripts/package.py" "${package_args[@]}"
    ;;
  clean)
    if [[ ! -d "$BUILD" ]]; then echo 'Build directory does not exist.'; exit 0; fi
    BUILD_REAL="$(cd -- "$BUILD" && pwd -P)"
    [[ "$BUILD_REAL" == "$ROOT" || "$BUILD_REAL" == / ]] && { echo 'Refusing unsafe build directory' >&2; exit 2; }
    if [[ ! -f "$BUILD_REAL/CMakeCache.txt" ]] || ! grep -Fqx "CMAKE_HOME_DIRECTORY:INTERNAL=$ROOT" "$BUILD_REAL/CMakeCache.txt"; then
      echo 'Refusing to remove a directory that is not a configured build of this source tree.' >&2
      exit 2
    fi
    cmake -E rm -rf "$BUILD_REAL"
    ;;
  *) usage; exit 2 ;;
esac
