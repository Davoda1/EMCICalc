# EMCICalc Desktop Edition 1.0.0 — keyboard and mouse guide

Application navigation remains available throughout the window:

| Shortcut | Action |
| --- | --- |
| `Ctrl+L` | Open the calculator and select its expression |
| `Ctrl+G` | Open and focus the graph workspace |
| `Ctrl+H` | Open history |
| `Ctrl+,` | Open settings |
| `F1` | Function-argument/catalog help or focused-key tooltip in those controls; otherwise the built-in guide |
| `F11` | Toggle fullscreen |
| `Escape` | Leave fullscreen first; otherwise step back through Calculator pages, then cancel REG/SHIFT/ALPHA or clear the calculator expression/result when already in Calculate |
| `Tab` / `Shift+Tab` outside the expression | Move keyboard focus |

The following calculator shortcuts apply while the calculator has focus. They leave graph, history-search, and settings input fields available for their own editing. Calculator buttons show brief press feedback when activated by mouse or a mapped keyboard action.

| Shortcut | Calculator action |
| --- | --- |
| `Enter`, numpad `Enter`, `Ctrl+Enter` | Evaluate once |
| `Shift+Enter` | Add a visual line break within the same expression |
| `Tab` / `Shift+Tab` in the expression | Cycle through empty input slots; resume normal focus navigation when none remain |
| Tap and release `Shift`, or `F2` | Toggle the one-shot SHIFT keypad layer |
| Tap and release `Alt`, or `F3` | Toggle the one-shot ALPHA keypad layer |
| Hold `Shift` and press a mapped digit | Use its SHIFT action; top-row and numpad digits both work |
| Hold `Alt` and press a mapped digit | Use its ALPHA action; top-row and numpad digits both work |
| `Shift+.` | Activate the latched REG keypad layer |
| Top-row or numpad `0`–`9` while REG is active | Insert one atomic R0–R9 token; keep REG active |
| `.`, numpad decimal, deliberate `Shift`, or `Escape` while REG is active | Exit REG without editing or clearing the expression |
| `F4` | Cycle RAD, DEG, and GRAD angle mode |
| `F5` | Switch Exact / Decimal result mode |
| `Alt+S`, `Alt+C`, `Alt+T` | Insert sine, cosine, tangent |
| `Alt+R` | Insert square root |
| `Alt+L`, `Alt+N` | Insert base-10 logarithm / natural logarithm |
| `Alt+P`, `Alt+I` | Insert pi / imaginary unit |
| `Alt+A`, `Alt+X` | Insert previous answer / variable x |
| `Ctrl+M` | Store result in memory (MS) |
| `Ctrl+R` | Recall memory (MR) |
| `Ctrl+Shift+M` | Add result to memory (M+) |
| `Ctrl+Shift+Delete` | Clear memory (MC) |
| `Ctrl+Shift+C` | Copy result |
| `Alt+Up` / `Alt+Down` in the expression | Browse expression history |
| `Up` / `Down` in the expression | Move between visual lines |
| `Left` / `Right` | Move through the expression and its mathematical structures |
| `Home` / `End` | Move the expression cursor |
| `Backspace` / `Delete` | Delete content; remove a complete function wrapper at its token boundary |

Digits, operators, parentheses, and function names can be typed normally. A literal `=` stays in the expression so equations can be entered; use Enter to evaluate. With comma decimals selected, physical decimal-comma input maps to a canonical dot, and a semicolon within function arguments inserts a canonical argument comma. The dedicated comma key inserts an argument comma explicitly; Pocket uses ALPHA then + because it has no dedicated comma key. Numeric-keypad decimal input follows the configured decimal presentation. Copy/paste and explicit argument insertion retain canonical punctuation. Standard `Ctrl+C/V/X/A/Z/Y` editing shortcuts are preserved.

A bare Shift or Alt tap changes a keypad layer only when the calculator expression, result, or keypad has focus. Holding Shift activates SHIFT; holding Alt activates ALPHA until release. Top-row and numeric-keypad digits activate their corresponding alternate actions, and the layer remains active for further key presses while held. For example, hold Shift and press either 3 key to activate the SHIFT action displayed above 3; in Calculator mode, holding Shift with either 4 key activates factorial in both REAL and CPLX. Shifted top-row digits select calculator functions rather than typing their punctuation symbols. Mapped combinations are consumed through key press and release. Numeric-keypad keys are recognized by their physical position even when Num Lock or Shift makes the operating system report a navigation key: Shift+numpad 4 uses the function on 4, while the separate Shift+Left arrow still selects text. Alt+numpad calculator actions do not also insert an Alt-code character when Alt is released. A held modifier also applies when clicking a virtual key. A quick unused tap toggles the one-shot layer like clicking its on-screen key. Holding for at least 350 ms counts as a hold even without another key, so releasing it returns to normal instead of leaving the layer armed. Using a mapped key while held also makes release return to normal, without a minimum duration. The on-screen SHIFT or ALPHA key visibly depresses while held. Text selection with Shift+arrows, Shift+Enter, existing Alt letter shortcuts, Ctrl/Meta shortcuts, and AltGr text input remain available. While a calculator layer is held, unassigned printable combinations are ignored instead of entering punctuation or activating a menu. Release the modifier to resume ordinary typing; an unmodified numpad + or the virtual + key enters addition. Focus changes cancel held state so no modifier remains stuck.

SHIFT and ALPHA select distinct alternate layers; a tap or click selects a one-shot layer, while a physical hold keeps its layer active until release. A mapped digit or operator then activates the selected alternate action. After arming a layer, unmodified physical Left/Right also activate its keypad actions: Home/End for SHIFT, or `<`/`>` for ALPHA. An armed physical `=` uses the matching alternate action; without an armed layer, typed `=` stays equation text and Enter evaluates. Gold legends show SHIFT functions and blue legends show ALPHA functions; the active function appears enlarged on the key. The backspace icon deletes to the left normally, deletes to the right after SHIFT, and undoes after ALPHA. Physical Delete continues to delete right. Activating SHIFT before an applicable function shortcut uses its corresponding shifted keypad action, including pi → tau and x → y. Dedicated memory and mode shortcuts retain their stated action with SHIFT armed; `Alt+I` and `Alt+A` always insert `i` and `Ans`.

In DEC/HEX, ALPHA places A–F on 7/8/9/1/2/3 and comparisons on the arrow/power/root/equal keys. In Compact/Expanded, x has its own key, SHIFT then x inserts y, and ALPHA then 6 inserts z; MR handles memory recall. Pocket uses ALPHA then ×/÷ for x/y, ALPHA then + for argument comma, and ALPHA then 6/0 for z/M. Desktop keeps x, SHIFT then x for y, and ALPHA then minus for z, alongside its memory strip. SHIFT then 8 inserts infinity; SHIFT then the decimal key activates REG, and ALPHA then the decimal key inserts ran# (`rand()`). ALPHA then 4/5 inserts the binary/hexadecimal prefix in every layout. Each visible keypad keeps only one assignment for an identical action. The complete reviewed mapping for Desktop, Pocket, Compact, and Expanded is included in [docs/KEYPAD-MAP.json](../KEYPAD-MAP.json), with normal, SHIFT, ALPHA and REG behavior, canonical actions, base/domain restrictions, and explicit-literal context overrides.

The equals key and Enter evaluate the input; ALPHA then equals inserts the equation/comparison symbol `=`. SHIFT then equals (`≈`) evaluates again in Decimal mode, whereas S⇔D reformats the stored answer without reevaluating it. Calculator’s lower Ans key recalls the previous answer. ALPHA then ) inserts assignment `:=` in all layouts. The divide key no longer carries assignment. SHIFT then 9 enters recurring digits in every layout; Median remains available in Statistics and on SHIFT then to in Desktop. Pocket keeps absolute value on ALPHA then ln. The minus key covers both subtraction and a negative sign. Division enters an inline operator. SHIFT then Ans inserts a fraction template in Natural input and an ordinary slash in Linear input. ALPHA then Ans inserts a mixed fraction; these are distinct editing actions. Fixed square/cube and square/cube-root shortcuts remain separate from variable-exponent and nth-root templates.

When the graph canvas has focus:

| Shortcut | Graph action |
| --- | --- |
| Arrow keys | Pan the view |
| `+` / `-` | Zoom in / out |
| `Home` | Reset the view |
| `Enter` | Plot the expressions |
| `Shift+Left` / `Shift+Right` | Trace adjacent sampled values |


## New workspaces and local focus

Ctrl+Shift+T opens Transform selection for the selected structured expression. F1 inside a function call or on a selected catalog entry opens argument guidance. F1 on a focused keypad key shows normal/SHIFT/ALPHA help when Key tooltips are enabled; outside these controls the global F1 guide remains available. Mouse selection and Shift+arrows respect atomic token boundaries; right-click offers native context actions. Read-only results permit copy but omit cut/paste and other mutation actions. Tool fields keep ordinary keyboard editing and do not arm calculator modifiers.

The Graph / Table workbench accepts mouse dragging and wheel zoom in the focused canvas. In the 3D view dragging rotates the genuine sampled surface. The View controls expose reset, fit, zoom, rotation and image export. Native focus traversal reaches selectors and actions; editable table cells open on double-click or the normal keyboard editing route.

Desktop Settings stages changes until Apply; Cancel discards the staged set. Calculator Settings retains immediate accepted changes. A save or import file uses the operating system’s native file picker; mathematical tools stay inside the application window.

Screen and Key tooltips remain separately configurable. Help includes normal, SHIFT and ALPHA meanings with plain localized accessible descriptions, including when the visible mathematics uses vector artwork. Increasing Font size enlarges nominal interface text and preserves Qt/OS DPI behavior; long expressions/results pan locally.

The configured host checks are documented in [CI](../CI.md). Native operating-system key delivery and physical display behavior require platform testing.

## Calculator menu navigation

Calculator MENU uses compact category cards and submenu destination rows. Arrow and Enter handling is scoped to those menu controls; Tab and Space retain ordinary focus/activation behavior. Back restores the previous menu focus. Narrow Quick access uses two columns and menus scroll locally. These presentation changes do not remap the mathematical keypad or capture operating-system shortcuts.

In Calculator Settings, Tab/Backtab follow the Appearance, Input, Number notation and Help groups. Numeric minus/plus actions supplement ordinary typing and arrow-key entry. Desktop Settings retains its original order and native spinner behavior.

## Key captions

The integral key includes its dx differential again; selecting it retains the existing integration action and parser argument order. S⇔D has balanced visual spacing and continues to reformat the committed canonical result. Neither caption correction changes physical keyboard shortcuts or modifier behavior. Shared workbench button captions keep their normal mouse/keyboard actions.
