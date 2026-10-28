# Releasing EMCICalc Desktop

## Source files

The **Source release files** Actions workflow is manual (`workflow_dispatch`).
Run it from `master` with the committed version, currently `1.0.0`. It supports
both public and private repositories. Leave **create_draft** disabled to produce an Actions
artifact. Ordinary pushes and pull requests cannot publish a release.

The job has a 10-minute packaging limit and retains its artifact for 30 days.
Download the files before retention expires. The set contains:

- `EMCICalc-1.0.0-Source.zip`
- `EMCICalc-1.0.0-Source.tar.gz`
- `RELEASE-NOTES.md`
- one `SHA256SUM.txt`, covering the other three files

The archives have an `EMCICalc-1.0.0/` root and equivalent source bytes
and executable modes. `SOURCE-MANIFEST.json` records the exact Git commit and
per-file hashes. Export uses committed Git blobs, excludes untracked files and
Git history, and refuses tracked changes, unsafe paths, symlinks/submodules,
credential markers and compiled outputs. Required dependency hashes are checked.
Upstream attribution and notices remain in the source set.

From a clean committed checkout, the equivalent local command is:

```bash
timeout 540 python3 scripts/release_source.py --version 1.0.0 \
  --expected-commit "$(git rev-parse HEAD)" --output ../emcicalc-release-files
cd ../emcicalc-release-files
sha256sum --check SHA256SUM.txt
```

The output directory must be outside the repository and empty. This command
creates source files only; it does not contact GitHub, create a tag or publish.

## Optional draft and publication

Selecting **create_draft** adds a separate job with `contents: write`. It requires
a successful `CI (full)` run of `ci.yml` for the same commit,
verifies checksums and refuses an existing release or tag for the version.
It creates a draft targeting the exported commit and attaches the four files.
The packaging job has only `contents: read`; authentication uses the scoped
GitHub Actions token.

Partial diagnostic runs and results from other commits do not satisfy the draft
gate. Source files can still be generated with **create_draft** disabled.

Publication is a separate manual decision after reviewing the files and the
exact-commit validation evidence. The workflow does not publish the draft,
overwrite assets, delete tags or rewrite history. A source archive does not
establish native runtime or device validation.

## Application packages

Windows portable ZIPs place the real `EMCICalc.exe` at archive root. Linux
tarballs place the real `EMCICalc` at archive root. Debian installs a real
`/usr/bin/emcicalc`, application libraries under `/usr/lib/emcicalc`, and shared
data under `/usr/share`. Runtime packages contain no launcher scripts.

Use matching native dependencies and toolchains, audit actual dependency closure
and resources, and check launch, input, display/DPI, saved data and installation
on supported systems. See the [Linux](BUILD-LINUX.md) and
[Windows](BUILD-WINDOWS.md) build guides.

Provide matching corresponding source and required notices for each binary
release; see [CORRESPONDING-SOURCE.md](CORRESPONDING-SOURCE.md). Recalculate one
shared `SHA256SUM.txt` for a mixed source/binary set; do not add per-file checksum
sidecars. Exclude SDKs, toolchains, credentials, user profiles and identifying
logs from release assets. [CI.md](CI.md) describes configured automated coverage.
