# Corresponding source — EMCICalc Desktop Edition 1.0.0

This repository contains application source, artwork, tests, build and packaging
scripts, dependency recipes, patches, notices and input manifests.

The engine archive `libqalculate-5.12.0.tar.gz` is included. Optional upstream and
distribution archives are indexed by URL, byte size and SHA-256 in
`third_party/sources/manifest.json`. Retrieve and verify those inputs with:

```bash
python3 scripts/fetch_dependency_sources.py --all
python3 scripts/fetch_dependency_sources.py --verify-local
```

`third_party/SOURCES.md` documents Ubuntu source-package extraction and MSYS2
rebuild recipes. Keep downloaded SDKs, installed toolchains and build caches
outside source archives.

Repository source exports contain only committed Git files. Downloaded optional
archives are not automatically added by `scripts/release_source.py`. Before
binary distribution, assemble the complete matching corresponding-source set,
including any required dependency archives, changes and patches, and verify it
against the actual runtime package. The repository snapshot alone does not
include every optional dependency archive.

Provide corresponding source with equivalent access under the applicable
licenses; an upstream homepage alone is not a replacement for the matching
source set. See [licensing](LICENSING.md), [release instructions](RELEASING.md)
and [third-party notices](../THIRD-PARTY.md).

EMCICalc code and artwork are MIT licensed. Distribution with libqalculate follows
the GPLv3 option; dependencies retain their respective ownership and license terms.
User settings, history, currency caches, credentials and signing material must
not be included in source or runtime packages.
