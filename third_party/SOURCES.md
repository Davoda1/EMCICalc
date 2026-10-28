# Dependency-source inventory and retrieval

The source versions, notices and hashes define the desktop dependency graph.
Optional upstream/distribution archives are retrieved with
`python3 scripts/fetch_dependency_sources.py --all`; `retrieved-files.json`
lists those inputs. The libqalculate engine source, distribution descriptors,
recipes and patches are included in Git. Verify the complete matching source
set before distributing binary packages.

# Bundled dependency sources

EMCICalc's source ZIP and source tar.gz contain this directory. The dependency source material for the copyleft runtime libraries is indexed below and in `sources/manifest.json`: upstream archives, distribution source descriptors and patch sets, and build recipes. The inventory also covers separately redistributed GCC runtime sources. This is a source inventory, not a claim that every dependency was rebuilt or that all upstream licensing was independently certified. The application links these libraries dynamically. Replacing compatible DLLs/shared libraries is supported; the application does not impose a signature or integrity restriction on replacement libraries.

`windows-package-manifest.json` identifies downloaded MSYS2 binary packages and their SHA-256 values. `windows-runtime-package-map.json` maps the distributed runtime DLLs to those packages. The binary packages' `docs/runtime-manifest.json` lists the actual packaged libraries. This source directory deliberately excludes unused Qt SVG, Qt Quick, QML and Qt Wayland modules.

## Calculator engine, both platforms

| Component | Version | Included source | Build details |
|---|---|---|---|
| libqalculate | 5.12.0 | `libqalculate-5.12.0.tar.gz` | Original upstream source; `libqalculate-build.json` records source hash and configure options. |

Build options are `--without-libcurl --without-icu --without-gnuplot-call --disable-nls --disable-textport --disable-static --disable-insecure --enable-compiled-definitions`. The definitions are compiled into the library. Currency downloads use Qt Network separately from the engine, so the engine’s no-libcurl build option remains unchanged. No engine source changes were made. For Linux-hosted MinGW cross compilation, the generated library Makefile's `-lBcrypt` spelling is changed to `-lbcrypt` so the Linux linker finds the case-sensitive import-library filename. This adjustment does not change executable code.

## Windows libraries

| Component | Exact package version | Source archive |
|---|---|---|
| Qt base | 6.11.2-2 | `qtbase-everywhere-src-6.11.2.tar.xz` |
| GLib | 2.90.0-1 | `glib-2.90.0.tar.xz` |
| GNU libiconv | 1.19-1 | `libiconv-1.19.tar.gz` |
| GNU gettext runtime | 1.0-1 | `gettext-1.0.tar.lz` |
| GMP | 6.3.0-2 | `gmp-6.3.0.tar.xz` |
| MPFR | 4.2.2-3 | `mpfr-4.2.2.tar.xz` |
| GCC runtime libraries | 16.2.0-4 | `gcc-16.2.0.tar.xz` plus `msys2-recipes/gcc/` |

Each archive's SHA-256 matches the corresponding MSYS2 `PKGBUILD`. `sources/msys2-recipes/` includes the complete recipe directories, including all listed local patches, at repository commit `0c90635596d2c7164dce16e025c63a1a8dd003ea`. The externally referenced GLib patch `7e69f88480a4bf8d9653efd0310c4c25390a0c8b.patch` is also included. The recipe archive hashes, URLs, immutable GitHub URLs, and individual patch hashes are recorded in `sources/manifest.json`.

For rebuilding a dependency package, copy its `PKGBUILD`, matching source archive, and patches to one directory and run the normal MSYS2 `makepkg-mingw` workflow in a MINGW64 shell. The recipes name their build dependencies and exact configure/build options. The application repository's Windows bootstrap/build scripts provide the simpler application build path.

## GNU/Linux libraries

The Linux packages use Ubuntu 24.04 amd64 libraries. The following exact Ubuntu source packages are indexed as `.dsc`, original source archive(s), and `.debian.tar.xz` patch/build-recipe archives:

| Source package | Exact source version | Supplied runtime library family |
|---|---|---|
| qt6-base | 6.4.2+dfsg-21.1build5 | Qt Core, Gui, Widgets, Network, DBus, XcbQpa, platform and TLS plugins |
| glib2.0 | 2.80.0-6ubuntu3.8 | GLib |
| gmp | 2:6.3.0+dfsg-2ubuntu6.1 | GMP |
| mpfr4 | 4.2.1-1build1.1 | MPFR |
| dbus | 1.14.10-4ubuntu4.1 | D-Bus client library |
| systemd | 255.4-1ubuntu8.17 | libsystemd dependency of D-Bus |
| libgcrypt20 | 1.10.3-2build1 | libgcrypt dependency of libsystemd |
| libgpg-error | 1.47-3build2.1 | libgpg-error dependency of libgcrypt |
| gcc-14 | 14.2.0-4ubuntu2~24.04.1 | libgcc, libstdc++, and libgomp runtime libraries |

The SHA-256 of each Ubuntu original source/patch archive was checked against its corresponding `.dsc`. The older exact GLib source package was obtained from Ubuntu's retained Launchpad source archive; the others were obtained from the Ubuntu archive. Original signatures supplied by the distribution are retained where listed in the descriptor; source verification here uses SHA-256 rather than claiming a separate signature audit.

To unpack an exact Linux dependency source tree, install standard Debian/Ubuntu source tools and run `dpkg-source -x PACKAGE_VERSION.dsc` in this directory. The resulting `debian/` directory supplies the distribution patches, dependency requirements and build rules. `dpkg-buildpackage` can then rebuild the package in a matching Ubuntu 24.04 build environment. This includes full source packages even when only a single library from them is distributed.

## Networking dependencies

Qt Network and both TLS plugins are part of the Qt Base sources already included above. Windows uses the Schannel backend, which relies on the Windows TLS APIs; no separate OpenSSL Windows runtime is shipped. Linux uses the OpenSSL backend and explicitly bundles `libssl.so.3` and `libcrypto.so.3`, which Qt loads dynamically. Qt Network’s linked proxy and authentication support introduces the additional runtime dependency closure listed below.

The exact Ubuntu source descriptors, archives and distribution patches are indexed in `sources/`, with retrieval URLs and SHA-256 values in `sources/linux-source-manifest.json` and the consolidated manifest. Some older exact versions were retrieved from Launchpad’s retained primary archive. No source contents are modified. Standard `.dsc` extraction and distribution rebuild instructions above also apply to these packages.

| Source package | Exact source version |
|---|---|
| `curl` | `8.5.0-2ubuntu10.13` |
| `cyrus-sasl2` | `2.1.28+dfsg1-5ubuntu3.1` |
| `duktape` | `2.7.0+tests-0ubuntu3` |
| `e2fsprogs` | `1.47.0-2.4~exp1ubuntu4.1` |
| `gnutls28` | `3.8.3-1.1ubuntu3.4` |
| `keyutils` | `1.6.3-3build1` |
| `krb5` | `1.20.1-6ubuntu2.6` |
| `libffi` | `3.4.6-1build1` |
| `libidn2` | `2.3.7-2build1.1` |
| `libproxy` | `0.5.4-4build1` |
| `libpsl` | `0.21.2-1.1build1` |
| `libselinux` | `3.5-2ubuntu2.1` |
| `libssh` | `0.10.6-2ubuntu0.4` |
| `libtasn1-6` | `4.19.0-3ubuntu0.24.04.1` |
| `libunistring` | `1.1-2build1.1` |
| `nettle` | `3.9.1-2.2build1.1` |
| `nghttp2` | `1.59.0-1ubuntu0.2` |
| `openldap` | `2.6.7+dfsg-1~exp1ubuntu8.2` |
| `openssl` | `3.0.13-0ubuntu3.6` |
| `p11-kit` | `0.25.3-4ubuntu2.1` |
| `rtmpdump` | `2.4+20151223.gitfa8646d.1-2build7` |
| `util-linux` | `2.39.3-9ubuntu6.5` |

The ordinary calculator and physical-unit converter remain offline. Only the separate Currency converter fetches the ECB reference-rate feed. This does not change the libqalculate build configuration or its compiled mathematical definitions. Downloaded per-user rates are never included in release packages.

## Other dependencies and runtime exceptions

The runtime packages include the applicable copyright/license notices for every bundled dependency. Other libraries in the dependency closure use permissive licenses, including the selected FreeType License for FreeType and the BSD option for Zstandard. Their exact package versions, upstream project links, and binary package source information appear in the package/runtime manifests and dependency notices.

The GNU compiler runtime libraries carry GPLv3 terms with the GCC Runtime Library Exception. Its full text is included as `licenses/GCC-Runtime-Library-Exception-3.1.txt`, alongside GPLv3. The exception permits qualifying compiled combinations; it does not automatically remove source obligations when separately redistributing the runtime library binaries. Their source-delivery material includes:

- Windows: `gcc-16.2.0.tar.xz`, its original detached signature, and `msys2-recipes/gcc/` from the same immutable MSYS2 recipe commit used above. The recipe is version `16.2.0-4`. All 18 listed patch/helper files and the upstream archive match its SHA-256 values; the recipe's own hash is also recorded. Put the archive, signature, `PKGBUILD`, helper, and patches together in an MSYS2 MINGW64 build directory and use the recipe's standard `makepkg-mingw` workflow and build dependencies.
- Linux: `gcc-14_14.2.0-4ubuntu2~24.04.1.dsc`, `gcc-14_14.2.0.orig.tar.gz`, and `gcc-14_14.2.0-4ubuntu2~24.04.1.debian.tar.xz`. The two archives match the exact Ubuntu descriptor's SHA-256 values. Unpack with `dpkg-source -x gcc-14_14.2.0-4ubuntu2~24.04.1.dsc` and use the included `debian/` packaging in a matching Ubuntu 24.04 environment. The upstream 14.2.0 archive alone would omit the distribution changes.

`sources/gcc-source-manifest.json` records each added file, origin, byte count and checksum. Signatures are retained where supplied, but no independent signature-validation claim is made.

System-provided Windows APIs, the GNU/Linux C runtime and graphics driver stack are not redistributed. Development compilers, SDKs and build tools are not part of the runtime deliverables.

## Integrity and provenance

After retrieving optional archives, run `sha256sum -c SHA256SUMS.txt` from `sources/` to verify source archives and recipe files. `sources/manifest.json` is the consolidated machine-readable index; its entries include file, exact retrieval URL, SHA-256, version/platform when applicable, and byte count. The separate Windows, Linux and MSYS2 manifests retain the same information grouped by origin. The upstream archives are preserved without repacking their contents.
