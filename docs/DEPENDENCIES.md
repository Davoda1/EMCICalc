# Desktop dependency pins

Build with a coherent compiler, Qt and native-library ABI. Input URLs, hashes,
source recipes and notices are recorded in `third_party/`. Changes to the graph
require a matched rebuild and focused calculation, import and platform checks.

| Component | Configuration |
| --- | --- |
| Language | C++17 |
| Linux | Ubuntu 24.04 amd64 packages; Qt 6.4.2 |
| Windows Qt | MSYS2 MINGW64 Qt 6.11.2-2 |
| libqalculate | 5.12.0 |
| Windows native libraries | GMP 6.3.0, MPFR 4.2.2, GLib 2.90.0, libiconv 1.19, gettext 1.0 |
| Windows compiler/runtime | GCC 16.2.0-4 headers/runtime, MSVCRT, SEH and POSIX winpthreads |
| Linux-hosted Windows build | Clang/LLD 21.1.8 |

Linux distribution patches and Windows import/runtime libraries are part of
the dependency contract. An upstream version alone does not establish ABI or
security compatibility. Runtime package manifests must record the libraries
actually bundled by each build.

## Currency transport

`ExchangeRateService::startRequest` disables HTTP/2 for the bounded ECB request
path, including redirects, using `QNetworkRequest::Http2AllowedAttribute`.
HTTPS, host restrictions, timeouts, response-size limits and cache validation
remain active. This implements the mitigation described in
[Qt's CVE-2026-95620 advisory](https://www.qt.io/blog/security-advisory-cve-2026-95620).
Review Qt updates as part of dependency maintenance and validate the complete
build before changing Qt or its runtime libraries.

## Upstream references

- [Qt releases](https://www.qt.io/blog) and [supported platforms](https://doc.qt.io/qt-6.11/supported-platforms.html)
- [libqalculate releases](https://github.com/Qalculate/libqalculate/releases)
- [GMP](https://gmplib.org/) and [MPFR](https://www.mpfr.org/)

See [corresponding source](CORRESPONDING-SOURCE.md) for the dependency retrieval
and distribution requirements. [CI](CI.md) documents configured checks; an
individual run provides results only for its exact commit and environment.
