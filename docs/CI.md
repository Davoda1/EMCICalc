# Continuous integration

The `CI` workflow runs for pushes and pull requests targeting `master`, and can
also be started manually. The workflow runs in public and private repositories. All
jobs have read-only repository permissions, external actions are pinned to
reviewed commit SHAs, and checkout does not persist Git credentials. There is
no release, signing, tag creation, deployment or repository-setting mutation in
this workflow. Superseded runs for the same branch or pull request are cancelled.

## Source job

`python3 scripts/ci_checks.py` checks actual Git inputs for credentials, private
key/build outputs, external source symlinks, first-party machine/contact fields,
version consistency, pinned action references and the included engine source
hash. It preserves required third-party names and notices. It then runs syntax
checks and the existing PE-audit, package-layout, source-release and localization fixtures, and
verifies available corresponding-source inputs against the recorded hashes.

This is a targeted guard, not a proof that arbitrary future secrets can never
be introduced. Review diffs before committing, especially generated metadata.
The job has a 10-minute limit and its helper is bounded to 9 minutes. Individual
checks also have finite limits. The JSON result is retained for seven
days; no source archive or runtime package is uploaded by this job.

## Native GNU/Linux job

After source checks pass, an Ubuntu 24.04 amd64 runner installs its supported Qt
6.4 development packages and builds the included, checksum-pinned libqalculate
5.12.0 source. It compiles the actual application plus ten focused existing
test executables. The common desktop shell is compiled once by the existing
CMake object-library arrangement.

The selected suites cover engine arithmetic/formatting, currency cache and URL
policy, history, keyboard dispatch, archive validation, layout, settings
persistence, integral/S⇔D captions, shared button text, and keypad legends.
Tests use Qt's
offscreen platform at 100% scale and isolated temporary configuration/data
directories. Necessary DejaVu and Liberation fonts are installed explicitly.

The native job has a 30-minute total limit, including package installation.
Its build command is capped at 22 minutes and its focused test batch at eight
minutes; these are nested limits, not an additive budget extension. It builds
only the selected targets rather than every test target in the repository.
There are no automatic retries or repeated full suites. Test XML and the CTest
log are retained for seven days, including failure results when
available.

For a prepared Linux development machine, reproduce the native jobs with:

```bash
timeout --kill-after=10s 1320s bash scripts/ci_linux.sh build
timeout --kill-after=10s 480s bash scripts/ci_linux.sh test
```

Manual runs offer a `native_scope` choice. `full` is the default and is always
used for pushes and pull requests. `captions` builds and runs only the existing
`desktop_caption` fixture, for bounded investigation of a caption failure.
`keypad` builds and runs only `panelradix` to diagnose a keypad failure.
`caption_keypad` compiles the application and runs only `desktop_caption` and
`panelradix`, whose actual key-legend checks cover keypad layouts, window sizes,
themes and modifier layers. Source checks still
run. Both the run title and retained scope file identify these partial runs.
Locally, set `CI_NATIVE_SCOPE` to the corresponding scope name for
both helper commands. A partial run does not establish that the omitted suites
passed at that commit. The caption fixture emits exact font and legend-fit measurements
only when its Pocket SHIFT assertion fails.

Host package security revisions may advance within Ubuntu 24.04; this job is a
native compatibility/regression check, not a byte-identical build of
every distribution input. libqalculate's input hash is fixed. The
workflow uses no untrusted build cache and does not change dependency pins.

## Evidence boundaries

A successful run establishes the checks shown in that run for its exact commit.
Adding the workflow alone does not establish a pass. Check the Actions run and
its commit SHA before citing results. Failures and timeouts remain failures;
the workflow never converts them into release-readiness claims.

Linux offscreen execution does not establish native Windows behavior, physical
display/DPI/accessibility behavior, actual package installation, or clean-machine
runtime compatibility. The PE and packaging fixture tests are synthetic audits,
not Windows execution or real ELF package qualification. Release workflows and
their separate validation requirements are documented in `RELEASING.md`.
