# GNU/Linux build guide — EMCICalc Desktop Edition 1.0.0

The build configuration uses C++17, GCC 13.3.0, Qt 6.4.2 (Ubuntu package `6.4.2+dfsg-21.1build5`), and libqalculate 5.12.0. The Linux GCC runtime input is Ubuntu 14.2.0; this is distinct from the compiler version. A runtime package's exact ELF/glibc requirement is measured from the application and every bundled shared library during packaging and recorded in `docs/runtime-manifest.json`. A distribution with an older glibc must build from source rather than relying on an unsupported binary-compatibility claim.

Extract either `EMCICalc-1.0.0-Source.zip` or `EMCICalc-1.0.0-Source.tar.gz`, then run the commands below from its `EMCICalc-1.0.0` directory.

The commands below describe the build procedure. Validate each package on its target system before distribution; [CI.md](CI.md) describes automated coverage.

## Dependencies and native build

Use Ubuntu 24.04 amd64, or a compatible development environment. Install the host packages with:

```bash
sudo apt update
sudo apt install build-essential cmake ninja-build pkg-config qt6-base-dev \
  libgmp-dev libmpfr-dev libxml2-dev zlib1g-dev patchelf python3 dpkg-dev
```

The bootstrap builds the pinned, SHA-256 checked engine from the source archive included in `third_party/sources`. It disables engine networking, NLS and external plotting, and embeds mathematical definitions. Currency downloads remain a separate Qt Network operation.

```bash
bash scripts/bootstrap-linux.sh
export EMCICALC_PREFIX="$PWD/.dependencies/prefix"
export PKG_CONFIG_PATH="$EMCICALC_PREFIX/lib/pkgconfig${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}"
export LD_LIBRARY_PATH="$EMCICALC_PREFIX/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export EMCICALC_EMBEDDED_DEFINITIONS=1
./run.sh build
./run.sh test
./run.sh package
```

`run.sh` is a development helper and has no application launch action. The native tar entry point is `EMCICalc` at archive root. Debian installs a real executable at `/usr/bin/emcicalc`, private libraries/plugins under `/usr/lib/emcicalc`, fonts/icon under `/usr/share/emcicalc` and documentation under `/usr/share/doc/emcicalc`. Its ELF RPATH is `$ORIGIN/../lib/emcicalc`; startup locates private plugins before QApplication is constructed. Portable Qt configuration and RPATH remain relative to the extracted root. Release and Debian version are `1.0.0`. Per-user data is stored separately from the executable at the location shown in Settings.

`EMCICALC_BUILD_DIR` selects a separate build directory. `CMAKE_BUILD_PARALLEL_LEVEL` limits compilation concurrency. Use a fresh directory when changing compilers or dependency prefixes. Qt, engine headers and libraries must be compatible; do not point desktop builds at an Android Qt installation.

## Packaging and audits

The packaging helper resolves the actual ELF dependency closure, bundles the Qt XCB/offscreen and OpenSSL plugins, embeds the application icon, sets relative RPATHs, and calculates the maximum required GLIBC symbol version. Operating-system glibc and graphics driver libraries remain OS dependencies. The Qt OpenSSL plugin loads `libssl.so.3` and `libcrypto.so.3` dynamically; both are included explicitly. System CA certificates are still required for HTTPS.

```bash
python3 scripts/package.py --platform linux --compiled-definitions \
  --build-dir build --output dist
python3 scripts/audit_linux_package.py \
  dist/staging/EMCICalc-1.0.0-Linux-x86_64 \
  --launch --json dist/linux-package-audit.json
```

Add `--stage-only` to prepare the runtime and Debian filesystem trees for review without producing release archives. With `--output dist`, the runtime is staged in `dist/staging/EMCICalc-1.0.0-Linux-x86_64` and the Debian tree in `dist/staging/deb-root`. The artifact names are `EMCICalc-1.0.0-Linux-x86_64.tar.gz` and `emcicalc_1.0.0_amd64.deb`.

For extracted development packages, `--qt-plugins /absolute/path/to/qt6/plugins` selects the matching Qt plugins. `--dependency-notices third_party/licenses/linux-staged` includes notices from extracted packages not known to the host dpkg database. These switches do not change how a user starts the application.

The audit's launch option runs the native application with a new temporary profile, cleared build-library search paths and Qt's offscreen backend. It observes normal startup for three seconds, then terminates the process deliberately. This is not a claim of a physical desktop session or a clean-machine launch. X11/Wayland interaction must be checked separately where available.

To exercise Debian file installation and removal without modifying the host installation:

```bash
python3 scripts/audit_linux_package.py \
  dist/staging/EMCICalc-1.0.0-Linux-x86_64 --launch \
  --deb dist/emcicalc_1.0.0_amd64.deb \
  --install-root "$PWD/debian-install-check" \
  --json dist/linux-debian-audit.json
```

The isolated root must be unused. This lifecycle check verifies native files, executable permission, real executable, RPATH and removal, but deliberately bypasses dpkg dependency checks because that root contains no operating-system package database. It does not replace installation on a clean supported system.

## Source and licenses

`third_party/desktop-toolchain-inputs.json` pins downloaded package inputs and hashes. `third_party/sources/SHA256SUMS.txt`, checksum-verified upstream archive retrieval, Ubuntu descriptors/patches and MSYS2 recipes preserve the dependency-source route. Qt 6 uses LGPLv3 terms with component-specific notices; libqalculate is GPL-2.0-or-later, not LGPL. Combined distribution follows the GPLv3 option documented in `docs/LICENSING.md`. Host-only build tools are not application runtime components.

## Bundled CJK fallback fonts

The runtime packages include unmodified Noto Sans CJK JP and SC Regular 2.004 under the SIL Open Font License 1.1. These fonts supply Japanese and Simplified Chinese glyphs where the system fonts are incomplete. Registration is local to the application process; the system primary UI font is retained. Windows stores them in `fonts/`; Linux stores them in `share/emcicalc/fonts/`. Keep these folders with the application. Source packages contain the identical OTF files, copyright/OFL notices and tagged upstream URL/SHA-256 manifest in `assets/fonts/`.
