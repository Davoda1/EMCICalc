# Third-party software and notices

EMCICalc is a native C++17 application using Qt 6 Widgets, Qt Network, and libqalculate. Original application code and original icon artwork are identified by the top-level license; dependencies remain separately licensed. Distribution of the combined application with libqalculate must comply with GNU GPL version 3, using the GPLv3 option in libqalculate's GPL-2.0-or-later license alongside Qt 6's LGPL-3.0 terms. This does not relicense any dependency's source files, make every bundled component GPL-only, or grant later-version permissions absent from a component's own terms. The original MIT notice remains in `LICENSE`. See `docs/LICENSING.md`.

| Component | Versions | License | Upstream |
| --- | --- | --- | --- |
| Source Sans 3 (documentation) | Supplied font files and original notices | SIL Open Font License 1.1 | https://github.com/adobe-fonts/source-sans |
| Noto Sans CJK JP + SC Regular | 2.004 | SIL Open Font License 1.1 | https://github.com/notofonts/noto-cjk/tree/Sans2.004 |
| libqalculate | 5.12.0 | GPL-2.0-or-later | https://github.com/Qalculate/libqalculate |
| Qt Base | 6.4.2 on Linux; 6.11.2 on Windows | LGPL-3.0; component-specific alternatives and bundled notices apply | https://www.qt.io/qt-licensing |
| OpenSSL (Linux TLS backend) | See runtime manifest | Apache-2.0 | https://www.openssl.org/ |
| GMP | Distribution version / 6.3.0 on Windows | LGPL-3.0-or-later or GPL-2.0-or-later | https://gmplib.org/ |
| MPFR | Distribution version / 4.2.2 on Windows | LGPL-3.0-or-later | https://www.mpfr.org/ |
| libxml2 | Distribution version | MIT | https://gitlab.gnome.org/GNOME/libxml2 |
| GCC runtime libraries | 14.2.0-4ubuntu2~24.04.1 on Linux; 16.2.0-4 on Windows | GPL-3.0-or-later with GCC Runtime Library Exception 3.1 | https://gcc.gnu.org/ |
| ICU, PCRE2, zlib, libpng, FreeType, HarfBuzz, and other transitive libraries | See runtime manifest | Respective upstream licenses | See bundled notices and source manifest |

The portable distributions dynamically link the included libraries. Compatible replacement builds can be substituted, and the complete EMCICalc application source and build instructions are provided. Use a separate copy when experimenting with replacement libraries so the original release remains available.

`docs/runtime-manifest.json` in each runtime package lists the runtime libraries actually bundled. `docs/dependency-notices/` contains available distribution/upstream copyright and license notices. `docs/licenses/` contains the GPL and LGPL texts. Dependency source archives, distribution patches, recipes and hashes are indexed under `third_party/sources/`, including GCC compiler runtime sources. This Git tree includes engine source and recipes; retrieve optional archives with `python3 scripts/fetch_dependency_sources.py --all` before preparing a complete corresponding-source distribution. Permissively licensed dependencies are identified in notices and runtime manifests; the source inventory does not claim to include the source of every permissive dependency or operating-system component. See `docs/CORRESPONDING-SOURCE.md` and the source package’s `third_party/SOURCES.md`.

Mathematical definitions compiled into the release engine originate from libqalculate and are subject to its license. The source packaging helper can alternatively bundle external definitions under `share/qalculate` for distribution-library builds. Local user Qalculate definitions, exchange-rate caches, and downloaded resources are not packaged.

Microsoft Windows system libraries and GNU/Linux glibc/graphics-driver libraries are operating-system dependencies, not bundled EMCICalc components. Separately shipped GCC runtime DLLs/shared libraries are bundled components, with source material and the Runtime Library Exception included; their unmodified status is not used as a substitute for a source-delivery route. Qt, Qalculate, Casio, and HiPER are the names or marks of their respective owners. EMCICalc is independently developed and is not endorsed by those projects or manufacturers.

Currency-rate downloads use Qt Network with the operating system’s trusted certificate authorities. Windows uses Qt’s Schannel backend; Linux uses Qt’s OpenSSL backend and bundled OpenSSL libraries. European Central Bank daily reference-rate data is fetched from its public HTTPS service only on currency refresh, and is not bundled as a current-rate claim.

The unmodified Noto Sans CJK JP and SC fallback fonts are included locally for Japanese and Simplified Chinese UI glyphs when system fonts lack them. Their original OFL notice, tagged upstream download URLs, and SHA-256 hashes are in `assets/fonts/` and `third_party/noto-cjk-fonts.json`. They do not replace the user’s system primary font.
