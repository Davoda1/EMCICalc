# EMCICalc Desktop Edition 1.0.0 — feature index

EMCICalc combines a scientific calculator with connected mathematical workbenches. This index describes the source contracts; configured automated checks and platform limits are documented in [CI](../CI.md).

| Area | Behavior | Documentation |
| --- | --- | --- |
| Native calculator | C++17, Qt Widgets and a serialized libqalculate worker; exact and approximate results, symbolic operations and units | [Keyboard and mouse](Keyboard-Mouse-Guide.md) |
| Desktop and Calculator modes | Independent themes, layouts, preferences and geometry; adaptive in-window tools and local scrolling | [Keyboard and mouse](Keyboard-Mouse-Guide.md) |
| Mathematical editor | Natural and Linear input, atomic symbols, structured slots, selection and undo; recurring decimals and DMS | [Keyboard and mouse](Keyboard-Mouse-Guide.md) |
| Results and memory | Canonical copy, frozen calculation contexts, explicit reuse, M, Ans and R0–R9; empty registers remain distinct from zero | [Storage](Storage-Guide.md) |
| Algebra and Calculus | Guided forms for equations, derivatives, limits, integrals, sums and products | [Steps boundaries](Steps-Coverage.md) |
| Matrices and vectors | Resizable expression grids, operations, named storage, CSV and bounded numerical decompositions | [Matrix and graph limits](Matrix-Graph-Limits.md) |
| Data and Statistics | Frequency datasets, summaries, fits, predictions, scatter and residual plots, CSV | [Statistics](Statistics-Guide.md) |
| Finance | 29 worksheets and 26 callable helpers with explicit timing, rounding and validation rules | [Finance guide](../FINANCE-GUIDE.md) |
| Units and Currency | Separate converters, exact/decimal unit results and a dated last-good currency cache | Built-in Guide |
| Graph and Table | Up to 20 explicit, polar, parametric, implicit, inequality and surface series; interactive 3D, tables and exports | [Matrix and graph limits](Matrix-Graph-Limits.md) |
| Definitions | Evaluated values, unknowns and pure parameterized functions; validated restore without running saved formulas | Built-in Guide |
| Input workflows | Separate Expression, Classic and RPN state; transactional accumulator and stack operations | Built-in Guide |
| Custom keypad and Programmer | Semantic action layouts and fixed-width checked or wrapping integer operations | Built-in Guide |
| Workspace exchange | Selective versioned sections, conflict handling, validation and atomic recovery | [Storage](Storage-Guide.md) |
| Offline Steps | Bounded exact rational, algebra, calculus and matrix rules with expandable explanations and PDF/PNG export | [Steps coverage](Steps-Coverage.md) |
| Platform integration | Native Windows and GNU/Linux executables, dependency notices and package audits | [Windows](../BUILD-WINDOWS.md), [GNU/Linux](../BUILD-LINUX.md) |

Desktop uses a 680×540 minimum window and Calculator uses 320×540. Desktop Settings applies staged changes through Apply/Cancel; Calculator Settings accepts changes immediately. Eight Desktop themes, eight Calculator skins, three stock keypads and nine interface languages provide independent appearance choices.

Calculation contracts include DEG/RAD/GRAD, REAL/CPLX, fractional BIN/OCT/DEC/HEX, canonical punctuation independent of display separators, and a 10,000-entry compressed history. Startup and AC do not create an automatic zero result or erase saved registers/history.

Steps has explicit supported families, graph annotations are numerical estimates, and Statistics/decomposition procedures have finite numerical ranges. Automated host coverage does not by itself establish native Windows, display, accessibility or package-installation behavior.
