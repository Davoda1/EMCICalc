# EMCICalc Desktop Edition 1.0.0 — desktop distribution licensing

This document describes the Windows and GNU/Linux desktop packages. It does not describe the Android Edition. The full license texts and the individual files' notices govern; this summary does not replace them.

## Application and component terms

Original EMCICalc source and original artwork retain the MIT license and the copyright notice in `LICENSE`. libqalculate 5.12.0 is **GPL-2.0-or-later, not LGPL**. Distributing the combined application with libqalculate therefore requires GPL compliance even when the engine is dynamically linked. GPL version 3 is the compatible version used for this release alongside Qt 6's LGPL version 3 terms.

That distribution requirement does not turn every dependency into GPL-only code and does not grant permission to use later license versions for components that do not offer that choice. Qt, compiler runtimes, fonts, permissive libraries, and other third-party material retain their own notices and terms. Consult `THIRD-PARTY.md`, each runtime package's `docs/dependency-notices/`, and `docs/licenses/`.

The package uses Qt Core, Gui, Widgets, Network, and the platform/TLS support described in its runtime manifest. Qt Base 6.4.2 is used on Linux and 6.11.2 on Windows. Qt Base's applicable LGPL-3.0 terms and its bundled third-party notices must be respected; not every Qt module has the same licensing choices.

## Charging for downloads

GPL-compliant paid downloads, free downloads, and voluntary payments are possible. Charging does not remove recipients' rights to inspect, modify, rebuild, or redistribute the GPL-covered work under its license. A proprietary-only end-user license, prohibition on redistribution, or source access surcharge would not substitute for those terms.

For the download distribution used here, provide the matching Desktop 1.0.0 source ZIP or tar.gz alongside the executable packages, with equivalent access at no further charge for the source. Keep the matching source available for as long as required by the applicable license and distribution method. A public source page can provide that access, but an upstream homepage or link alone is not the matching source package. This release does not rely on a written offer whose fulfilment has not been arranged.

## Qt replacement and relinking

These release packages use shared Qt and engine libraries. In a separate extracted copy, compatible Qt DLLs/shared objects and plugins can be replaced; the application does not add a signature or integrity check that prevents that replacement. Preserve the ABI, architecture, plugin layout, and dependencies when substituting a build. The source and build scripts are also supplied.

LGPLv3 section 4 includes both a suitable shared-library mechanism and a route using the library's minimal corresponding source plus corresponding application code that permits recombination/relinking. Static Qt linking therefore does **not inherently require a commercial Qt license**. A static distribution would need its own applicable source, relinking, notice, and installation-information provisions; the shared release's instructions alone do not establish compliance for a future static build. This release does not use a commercial Qt license.

## Compiler runtimes and source contents

GCC's Runtime Library Exception permits qualifying compiled combinations; it is not a blanket removal of the runtime libraries' own GPL obligations. A binary distribution must include matching source material for separately redistributed GCC runtime libraries; unmodified status or upstream links alone do not replace that source-delivery route. `third_party/SOURCES.md` records the exact GCC/MSYS2 and Ubuntu versions, patches, recipes, hashes, and build instructions.

`docs/CORRESPONDING-SOURCE.md` identifies the companion archives. The source manifests describe the included files and their provenance. SHA-256 checking verifies those materials against recorded checksums; it is not a guarantee of bit-for-bit reproducibility or a legal certification. Runtime manifests identify the binaries actually shipped by each package. Operating-system libraries not redistributed with EMCICalc are distinguished from the bundled runtime libraries.

## Primary references

- [GNU GPL version 3](https://www.gnu.org/licenses/gpl-3.0.html): sections 4–6 on fees, conveying modified source/object code, and corresponding source.
- [GNU LGPL version 3](https://www.gnu.org/licenses/lgpl-3.0.html): section 4 on shared-library and relinking routes; also reproduced in [Qt's official LGPL page](https://doc.qt.io/qt-6/lgpl.html).
- [Qt licensing](https://doc.qt.io/qt-6/licensing.html): module-dependent open-source and commercial options.
- [GCC runtime license and exception](https://gcc.gnu.org/onlinedocs/libstdc++/manual/license.html) and [FSF rationale/FAQ](https://www.gnu.org/licenses/gcc-exception-3.1-faq.html).
- libqalculate's included `COPYING` and source-file headers in `third_party/sources/libqalculate-5.12.0.tar.gz`.

The license copies in the release remain available offline. Website references are explanatory pointers, not replacements for those notices or the supplied source material.
