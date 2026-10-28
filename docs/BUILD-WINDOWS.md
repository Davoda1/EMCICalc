# Windows x64 build guide — EMCICalc Desktop Edition 1.0.0

Target Windows 10/11 x64. The build configuration uses a coherent MSYS2 MINGW64 ABI: Qt Base 6.11.2-2, GCC 16.2.0-4 C++ headers/runtime, MSVCRT, SEH exceptions and POSIX winpthreads. libqalculate is 5.12.0. The native Linux cross compiler/linker are Clang/LLD 21.1.8. Android Qt versions, icons, deployment tools and signing are unrelated to this desktop build.

The exact downloaded packages and SHA-256 values appear in `third_party/desktop-toolchain-inputs.json`, `third_party/windows-package-manifest.json`, `third_party/windows-build-toolchain-manifest.json` and `third_party/windows-clang-host-packages.json`. Build libqalculate with the matching headers and recipe. An import archive can be recreated from the matching DLL with the included PE helper; it does not repair an ABI mismatch.

Extract either `EMCICalc-1.0.0-Source.zip` or `EMCICalc-1.0.0-Source.tar.gz`, then run the commands below from its `EMCICalc-1.0.0` directory. The product/release version is exactly 1.0.0; native numeric Windows version fields are 1.0.0.0.

The commands below describe the build procedure. Validate each package on Windows before distribution. See [DEPENDENCIES.md](DEPENDENCIES.md) for dependency maintenance and [CI.md](CI.md) for automated coverage.

## Native Windows development

Install MSYS2 and open its MINGW64 shell. The helper builds the checked, included libqalculate 5.12.0 source with the same compiler/runtime prefix as Qt:

```bash
bash scripts/bootstrap-msys2.sh
```

MSYS2's repositories roll forward. For an build with exact dependency inputs, use the recorded package versions and hashes, not an unpinned updated repository. In PowerShell with CMake, Ninja and Python 3 available:

```powershell
.\run.ps1 build -Prefix C:\msys64\mingw64
.\run.ps1 test -Prefix C:\msys64\mingw64
.\run.ps1 package -Prefix C:\msys64\mingw64
```

CMD uses the equivalent `run.cmd build`, `run.cmd test` and `run.cmd package` commands with the same `-Prefix`. A distinct `-BuildDir` isolates another compiler/dependency configuration. The helper rejects an existing build cache using a different compiler. Build/development PowerShell, CMD and Bash helpers belong only in source archives.

The portable package places the native `EMCICalc.exe` at the ZIP root, with the application icon and all recursively required non-system DLLs. It uses `%AppData%\davoda1\EMCICalc` for per-user saved data. Extraction does not silently enable a different data policy. Keep the DLL and plugin directories alongside the executable.

## Linux-hosted cross build

Install native CMake, Ninja, Python 3, Clang/LLD 21.1.8 and MinGW binutils. Assemble one prefix from the recorded MSYS2 MINGW64 package set. Clang 18 is insufficient for the GCC 16 headers; do not mix a host compiler's C++ headers, libc++, UCRT64 libraries or an unrelated engine DLL.

```bash
cmake -S . -B build-windows -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE="$PWD/scripts/cmake/mingw64-clang.cmake" \
  -DEMCI_WINDOWS_SYSROOT="$PWD/scripts/cmake" \
  -DEMCICALC_MINGW_PREFIX=/absolute/path/to/mingw64 \
  -DEMCICALC_CLANG_CXX=/usr/bin/clang++-21 \
  -DEMCICALC_LLD=/usr/bin/ld.lld-21 \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build-windows --parallel 2
python3 scripts/package.py --platform windows --compiled-definitions \
  --build-dir build-windows --sysroot /absolute/path/to/mingw64 --output dist
```

Unset any native-Linux `CPATH`, `LIBRARY_PATH` and `PKG_CONFIG_PATH` before a cross build; the adapter supplies the Windows paths explicitly. It invokes no Windows program. Native MinGW binutils process COFF archives and resources; they do not replace the selected C++ runtime.

If the engine import library is absent but the matching DLL and headers are available:

```bash
python3 scripts/rebuild-import-library.py \
  /absolute/path/to/mingw64/bin/libqalculate-23.dll \
  /absolute/path/to/mingw64/lib/libqalculate.dll.a
```

For a fresh engine build on a Linux host where Wine is available and permitted, `scripts/build-qalculate-wine.sh /absolute/path/to/mingw64` runs that prefix's native GCC compiler under Wine. This is optional development infrastructure; Wine is not a runtime dependency of the Windows package.

## Runtime verification and license records

The packaging helper includes `platforms/qwindows.dll`, the Windows style plugin and `tls/qschannelbackend.dll`. Currency HTTPS uses Windows Schannel and the Windows certificate store. Before writing the ZIP it audits every named, ordinal, forwarded and delay-load non-system import against the actual bundled DLL exports, rejecting missing symbols and architecture mismatches.

```bash
python3 scripts/audit_pe.py dist/staging/EMCICalc-1.0.0-Windows-x64 \
  --json dist/windows-pe-audit.json
```

Use packaging option `--stage-only` to prepare and audit the runtime directory without producing a ZIP. With `--output dist`, the runtime directory is `dist/staging/EMCICalc-1.0.0-Windows-x64` and the final archive is `EMCICalc-1.0.0-Windows-x64.zip`. The resource check additionally verifies x64 GUI identity, version 1.0.0.0 and every embedded icon image against the application ICO:

```bash
python3 scripts/audit_windows_resources.py \
  dist/staging/EMCICalc-1.0.0-Windows-x64/EMCICalc.exe assets/icon.ico \
  --json dist/windows-resources-audit.json
```

A successful cross build and PE audit demonstrate compilation and import compatibility. They do not prove a clean Windows launch, native dialogs, clipboard, DPI behavior or operating-system integration; execute the program and desktop tests on Windows for those gates. Record the checks performed and identify untested platforms in the release notes.

The EXE's product/file version is 1.0.0 and its visible title is `EMCICalc`. Package/runtime manifests identify Desktop Edition. See `docs/LICENSING.md`, `docs/CORRESPONDING-SOURCE.md`, `THIRD-PARTY.md` and included source recipes for distribution obligations. libqalculate is GPL-2.0-or-later, not LGPL.

## Bundled CJK fallback fonts

The runtime packages include unmodified Noto Sans CJK JP and SC Regular 2.004 under the SIL Open Font License 1.1. These fonts supply Japanese and Simplified Chinese glyphs where the system fonts are incomplete. Registration is local to the application process; the system primary UI font is retained. Windows stores them in `fonts/`; Linux stores them in `share/emcicalc/fonts/`. Keep these folders with the application. Source packages contain the identical OTF files, copyright/OFL notices and tagged upstream URL/SHA-256 manifest in `assets/fonts/`.
