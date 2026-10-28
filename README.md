# EMCICalc Desktop Edition 1.0.0

EMCICalc is a native C++17 / Qt 6 Widgets calculator and mathematical workspace
by **davoda1** for Windows 10/11 x64 and GNU/Linux x86_64.

Desktop and Calculator modes have independent themes, layouts, settings and
window geometry. Features include exact and approximate calculations, symbolic
operations, natural and linear input, units and currency, graphs, matrices,
statistics, Finance worksheets, bounded Steps explanations, history, registers
and workspace exchange. The interface supports nine languages, keyboard and
mouse input, and accessibility labels.

## Build and use

- [GNU/Linux build and packaging](docs/BUILD-LINUX.md)
- [Windows build and packaging](docs/BUILD-WINDOWS.md)
- [Features](docs/guides/Feature-Coverage.md), [keyboard and mouse](docs/guides/Keyboard-Mouse-Guide.md), [saved data](docs/guides/Storage-Guide.md)
- [Steps](docs/guides/Steps-Coverage.md), [Statistics](docs/guides/Statistics-Guide.md), [matrix and graph limits](docs/guides/Matrix-Graph-Limits.md), [Finance](docs/FINANCE-GUIDE.md)

Per-user data is stored separately from the application at the path shown in
Settings. Windows portable packages place `EMCICalc.exe` at ZIP root; Linux
portable packages place `EMCICalc` at tar root. Debian installs
`/usr/bin/emcicalc`, application libraries under `/usr/lib/emcicalc`, and shared
data under `/usr/share`. Runtime packages contain no launcher scripts.

## Development and releases

[CI](docs/CI.md) configures bounded source checks and focused Linux regression
tests. An individual Actions run records the checks performed for its commit;
these checks do not establish Windows or physical-display compatibility.
[Release instructions](docs/RELEASING.md) cover source archives, checksums and
platform validation. Source archives are not executable application packages.

See the [changelog](CHANGELOG.md), [contribution guide](CONTRIBUTING.md),
[security policy](SECURITY.md), and [dependency pins](docs/DEPENDENCIES.md).

## Licensing and source

Application code and artwork are MIT licensed. Distribution with libqalculate
follows the GPLv3 option described in [licensing](docs/LICENSING.md).
[Third-party notices](THIRD-PARTY.md) and [corresponding-source instructions](docs/CORRESPONDING-SOURCE.md)
identify dependency licenses, source archives, patches and retrieval hashes.
