#!/usr/bin/env bash
# Development helper: build the bundled engine using native MSYS2 GCC under Wine.
# Usage: scripts/build-qalculate-wine.sh /absolute/path/to/mingw64
set -euo pipefail
if [[ $# -ne 1 || ! -f "$1/bin/g++.exe" ]]; then
  echo "Usage: $0 /absolute/path/to/complete/mingw64 (including GCC)" >&2
  exit 2
fi
PREFIX=$(cd "$1" && pwd)
ROOT=$(cd "$(dirname "$0")/.." && pwd)
RUNNER="$ROOT/scripts/wine_mingw.py"
ARCHIVE="$ROOT/third_party/sources/libqalculate-5.12.0.tar.gz"
printf '%s  %s\n' 'f3dfdcf97d5a9e3a5bc0ebe66afd184721b606276eee3ba0ada8c92a5e71d44c' "$ARCHIVE" | sha256sum --check -
mkdir -p "$ROOT/.dependencies"
SOURCE="$ROOT/.dependencies/libqalculate-5.12.0"
if [[ ! -f "$SOURCE/configure" ]]; then
  tar --no-same-owner -xzf "$ARCHIVE" -C "$ROOT/.dependencies"
fi
# Separate cached objects whenever either the compiler or runtime changes.
ABI_KEY=$(sha256sum "$PREFIX/bin/g++.exe" "$PREFIX/bin/libstdc++-6.dll" | sha256sum | cut -c1-16)
BUILD="$ROOT/.dependencies/build-qalculate-wine-$ABI_KEY"
mkdir -p "$BUILD"
cd "$BUILD"
export PKG_CONFIG_LIBDIR="$PREFIX/lib/pkgconfig:$PREFIX/share/pkgconfig"
export PKG_CONFIG_SYSROOT_DIR="$(dirname "$PREFIX")"
export LIBXML_CFLAGS="-I$PREFIX/include/libxml2 -I$PREFIX/include"
export LIBXML_LIBS="-L$PREFIX/lib -lxml2"
export CPPFLAGS="-I$PREFIX/include"
export LDFLAGS="-L$PREFIX/lib"
export CFLAGS='-O2'
export CXXFLAGS='-O2'
export CC="$RUNNER --prefix=$PREFIX --tool=gcc"
export CXX="$RUNNER --prefix=$PREFIX --tool=g++"
export LD=x86_64-w64-mingw32-ld
export AR=x86_64-w64-mingw32-ar
export RANLIB=x86_64-w64-mingw32-ranlib
"$SOURCE/configure" --host=x86_64-w64-mingw32 --build=x86_64-pc-linux-gnu --prefix="$PREFIX" --without-libcurl --without-icu --without-gnuplot-call --disable-nls --disable-textport --disable-static --disable-dependency-tracking --disable-insecure --enable-compiled-definitions > configure.log 2>&1
sed -i 's/-lBcrypt/-lbcrypt/g' libqalculate/Makefile
make -j4 > build.log 2>&1
# Libtool's Linux-hosted wrapper detection cannot emit the native Windows DLL
# and import archive reliably. Link the compiled objects with GCC16 directly.
mkdir -p stage
"$RUNNER" --prefix="$PREFIX" --tool=g++ -shared -O2 \
  -o "$BUILD/stage/libqalculate-23.dll" \
  "-Wl,--out-implib,$BUILD/stage/libqalculate.dll.a" \
  libqalculate/.libs/*.o -L"$PREFIX/lib" \
  -lxml2 -liconv -lbcrypt -lmpfr -lgmp -lwinpthread > install.log 2>&1
mkdir -p "$PREFIX/include/libqalculate" "$PREFIX/lib/pkgconfig"
for header in Function.h Calculator.h DataSet.h Variable.h ExpressionItem.h Number.h MathStructure.h Prefix.h util.h includes.h Unit.h BuiltinFunctions.h QalculateDateTime.h qalculate.h; do
  cp "$SOURCE/libqalculate/$header" "$PREFIX/include/libqalculate/"
done
cp libqalculate.pc "$PREFIX/lib/pkgconfig/"

install -m 755 stage/libqalculate-23.dll "$PREFIX/bin/libqalculate-23.dll"
install -m 644 stage/libqalculate.dll.a "$PREFIX/lib/libqalculate.dll.a"
printf "Built libqalculate 5.12.0 using the matching native MSYS2 GCC toolchain.\n"
