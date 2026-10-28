# EMCICalc Desktop Edition 1.0.0 — storage and workspace exchange


## Saved data and backups

Close EMCICalc before copying its saved-data directory for backup or replacing application files. Copy unsaved expressions and important draft text separately. The installation directory and the user profile have different purposes; portable extraction does not create a portable-data directory beside the executable.

The authoritative path is the full selectable path displayed by the running application. Default paths are `%AppData%/davoda1/EMCICalc` on Windows and `~/.config/davoda1/EMCICalc` on GNU/Linux. The `--config-root` option selects an explicit configuration directory. Do not assume that an installation directory contains the profile.

| Data | Handling |
| --- | --- |
| Desktop / Calculator geometry and preferences | Kept separately, including presentation mode, themes/skins, layouts and fullscreen behavior |
| Natural / Linear, angle, domain, base, precision, separators | Retained with canonical punctuation independent of display punctuation |
| Per-base expression drafts | Retained; a missing new draft starts empty |
| History | Compressed history retains up to 10,000 successful entries; saved results are not reevaluated on recall |
| M, Ans and R0–R9 | Preserve canonical values and approximate/exact flags; empty differs from stored zero |
| Workbenches, definitions and custom layouts | Versioned structured state; drafts restore without calculation |
| Currency | Last-good compressed rates and date/source survive; failure to refresh does not destroy the cache |
| Malformed or conflicting files | Validation and recovery paths preserve useful evidence; check the visible restore/save message |

## Settings writes

Settings use atomic file replacement. The read handle is closed before replacement, including during application shutdown.

## Workspace files

A `.emci` file is a versioned exchange document. It can contain selected history, canonical values, registers, definitions, matrices, datasets, graph configuration and custom layouts. It excludes machine-specific paths, window geometry, permissions, secrets and signing material. Export offers section choices; import previews selected content and uses explicit conflict choices. Import validates data before applying it and uses rollback if a multi-file save fails.

Importing a formula registers/loads mathematical data; it does not execute arbitrary saved expressions on open. A graph draft can be restored without plotting. Recalled history retains the result’s frozen context; a deliberate recalculation is a separate action. Exchange compatibility is governed by the schema and supported content.

## Multiple processes and interrupted writes

Settings, history and registers use atomic saves and process-safe locks. The workspace journal coordinates the complete import transaction and is validated before restoration. A settings conflict preserves this instance’s draft as `settings-conflict-UTC-PID.json` while retaining the other process’s primary settings. Register saves reject stale revisions and reload; history merges under its archive lock. A contended successful-result transaction is blocked instead of partially committing Ans/history. A save conflict is reported rather than silently overwriting another process’s work. Do not manually remove a live lock or pending journal to bypass a reported conflict. Close the owning instance and retry the operation.

A backup taken while the application is closed is the simplest complete recovery point. Keep exported workspaces separately from the live profile. AC and a failed calculation clear active output but do not delete independent history or registers.
