#!/usr/bin/env python3
"""Check UI translation coverage and preserve Qt substitution markers.

Run from any directory with Python 3; no third-party dependencies are needed.
Function identifiers, mathematical tokens, and engine-generated diagnostics use
libqalculate's portable syntax and are intentionally outside this UI catalog.
"""
from __future__ import annotations

import ast
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
cpp = (ROOT / "src" / "i18n.cpp").read_text(encoding="utf-8")
match = re.search(r'R"EMCITR\((.*?)\)EMCITR"', cpp, re.S)
assert match, "Embedded translation catalog missing"

def unique_messages(pairs: list[tuple[str, object]]) -> dict[str, object]:
    result: dict[str, object] = {}
    for key, value in pairs:
        if key in result:
            raise SystemExit(f"Duplicate translation message: {key!r}")
        result[key] = value
    return result

catalog = json.loads(match.group(1), object_pairs_hook=unique_messages)
errors: list[str] = []
placeholders = re.compile(r"%(?:L?\d+|n)")
for key, translations in catalog.items():
    if len(translations) != 8:
        errors.append(f"{key!r}: expected eight translated locales")
    for column, translated in enumerate(translations):
        if not isinstance(translated, str) or not translated.strip():
            errors.append(f"{key!r}, column {column}: empty translation")
        elif sorted(placeholders.findall(key)) != sorted(placeholders.findall(translated)):
            errors.append(f"{key!r}, column {column}: substitution markers differ")

literal = r'"(?:\\.|[^"\\])*"'
call = re.compile(r"\b(?:trc|tr_|translate_?|text)\(\s*((?:" + literal + r"\s*)+)")
used: set[str] = set()

def add_literals(fragment: str) -> None:
    used.update(ast.literal_eval(part) for part in re.findall(literal, fragment))

for path in sorted((ROOT / "src").glob("*.cpp")):
    if path.name == "i18n.cpp":
        continue
    text = path.read_text(encoding="utf-8")
    for found in call.finditer(text):
        used.add("".join(ast.literal_eval(part) for part in re.findall(literal, found.group(1))))
    # Status indicators may choose their translated description conditionally.
    for found in re.finditer(r"\b(?:trc|tr_|translate_?|text)\(\s*[^()?;]*\?\s*("
                             + literal + r")\s*:\s*(" + literal + r")\s*\)", text):
        add_literals(found.group(1))
        add_literals(found.group(2))
    # Labels registered for retranslation and form-row captions.
    for pattern in [r"\baddText\(\s*\w+\s*,\s*(" + literal + r")\s*\)",
                    r"\baddRow\(\s*(" + literal + r")\s*,",
                    r"\brestoreNotice_\s*=\s*(" + literal + r")"]:
        for found in re.finditer(pattern, text):
            add_literals(found.group(1))
    # Find translated label arrays by their use, rather than a fixed list of
    # names. This includes page titles and memory tooltips while excluding
    # arrays of invariant units or mathematical tokens.
    for found in re.finditer(r"\bconst\s+char\s*\*\s*(?:const\s+)?(\w+)"
                             r"\s*\[\s*\]\s*(?:=\s*)?\{([^}]+)\}", text):
        name = re.escape(found.group(1))
        translated = re.search(r"\b(?:trc|tr_|translate_?|text)\(\s*" + name + r"\s*\[", text)
        registered = re.search(r"\baddText\(\s*\w+\s*,\s*" + name + r"\s*\[", text)
        if translated or registered:
            add_literals(found.group(2))
    # Settings choices use stable English IDs, translated when displayed.
    for found in re.finditer(r"\bchoice\(\s*\{([^}]+)\}\s*,", text):
        add_literals(found.group(1))
    # Calculator theme names are the first field in each palette record.
    for themes in re.finditer(r"QList<CalculatorThemeDefinition>\s+\w+\s*\{(.*?)\};", text, re.S):
        for found in re.finditer(r"\{\s*(" + literal + r")\s*,", themes.group(1)):
            add_literals(found.group(1))
    # Toolbox pairs contain a translated title and invariant math expression.
    toolbox = re.search(r"\btools\s*=\s*\{(.*?)\};", text, re.S)
    if toolbox:
        for found in re.finditer(r"\{\s*(" + literal + r")\s*,", toolbox.group(1)):
            add_literals(found.group(1))
    # Keypad action maps contain an internal token and a translated tooltip.
    for actions in re.finditer(r"\b(?:actions|descriptions)\s*\{(.*?)\};", text, re.S):
        for found in re.finditer(r"\{\s*" + literal + r"\s*,\s*(" + literal + r")\s*\}", actions.group(1)):
            add_literals(found.group(1))
    # The calculator's mode grid translates captions from MenuItem records.
    for items in re.finditer(r"\bMenuItem\s+\w+\s*\[\s*\]\s*=\s*\{(.*?)\};", text, re.S):
        for found in re.finditer(r"\{\s*" + literal + r"\s*,\s*(" + literal + r")\s*\}", items.group(1)):
            add_literals(found.group(1))
    # Statistics records translate both the descriptive name and category.
    for items in re.finditer(r"\bFunction\s+functions\s*\[\s*\]\s*\{(.*?)\};", text, re.S):
        for found in re.finditer(r"\{\s*" + literal + r"\s*,\s*(" + literal + r")\s*,\s*(" + literal + r")\s*\}", items.group(1)):
            add_literals(found.group(1))
            add_literals(found.group(2))
    # Matrix actions store translated captions in a button property.
    for found in re.finditer(r'QList<QPair<const char\*,QString>>\{(.*?)\}\)', text, re.S):
        if 'setProperty("translationKey"' in text[found.end():found.end()+300]:
            for caption in re.finditer(r"\{\s*(" + literal + r")\s*,", found.group(1)):
                add_literals(caption.group(1))
    # Finance result codes have explicit, user-visible localized status labels.
    if path.name == "financepanel.cpp":
        for statuses in re.finditer(r"statusLabels\{(.*?)\};", text, re.S):
            for found in re.finditer(r"\{\s*" + literal + r"\s*,\s*(" + literal + r")", statuses.group(1)):
                add_literals(found.group(1))
    # Currency service diagnostics are stable English keys translated by the
    # MainWindow callback before being shown in the currency panel.
    if path.name == "exchangerates.cpp":
        for found in re.finditer(r'QStringLiteral\(\s*(' + literal + r')\s*\)', text):
            value = ast.literal_eval(found.group(1))
            if value.startswith(("Exchange-rate ", "The downloaded exchange rates ")):
                used.add(value)
    # Register archive diagnostics are translated by the MainWindow owner after
    # read/write/recovery failures, including conditional preservation messages.
    if path.name == "registerarchive.cpp":
        for found in re.finditer(literal, text):
            value = ast.literal_eval(found.group(0))
            if value.startswith(("Register data ", "Invalid register data ", "Could not save registers")):
                used.add(value)
    # Both titles and complete paragraphs in the Guide are translated.
    guide = re.search(r"QList<QPair<const char\*,const char\*>>\{(.*?)\}\)\{html", text, re.S)
    if guide:
        add_literals(guide.group(1))

# Finance worksheets are schema-driven rather than literal widget captions.
# Scan balanced argument lists so nested and conditional labels are covered,
# including helper wrappers, nested table columns and stable choice IDs.
def arguments_after(text: str, start: int) -> list[str]:
    args: list[str] = []
    begin = start
    depth = 0
    quoted = False
    escaped = False
    for position in range(start, len(text)):
        char = text[position]
        if quoted:
            if escaped:
                escaped = False
            elif char == "\\":
                escaped = True
            elif char == '"':
                quoted = False
            continue
        if char == '"':
            quoted = True
        elif char in "([{":
            depth += 1
        elif char in ")]}" and depth:
            depth -= 1
        elif char == ")":
            args.append(text[begin:position])
            return args
        elif char == "," and not depth:
            args.append(text[begin:position])
            begin = position + 1
    return []

finance_used: set[str] = set()
for path in sorted((ROOT / "src").glob("finance*.cpp")):
    if path.name == "financepanel.cpp":
        continue
    source = path.read_text(encoding="utf-8")
    for match in re.finditer(r"\b(field|select|table|amount|rateField|tool|makeTool)\(", source):
        arguments = arguments_after(source, match.end())
        if len(arguments) < 2:
            continue
        # A conditional label can compare an invariant identifier. Remove the
        # comparison's string operand while retaining both visible captions.
        label = re.sub(r"(?:==|!=)\s*" + literal, "", arguments[1])
        finance_used.update(ast.literal_eval(part) for part in re.findall(literal, label))
        if match.group(1) == "select" and len(arguments) > 2:
            finance_used.update(ast.literal_eval(part) for part in re.findall(literal, arguments[2]))
    for match in re.finditer(r"\.((?:title)|(?:category))\s*=([^;]+);", source):
        label = re.sub(r"(?:==|!=)\s*" + literal, "", match.group(2))
        finance_used.update(ast.literal_eval(part) for part in re.findall(literal, label))
    for match in re.finditer(r"return\s*\{\s*id\s*,\s*title\s*,\s*(" + literal + r")", source):
        finance_used.add(ast.literal_eval(match.group(1)))
used.update(finance_used)

for key in sorted(used):
    if key not in catalog:
        errors.append(f"Untranslated UI message: {key!r}")
if errors:
    raise SystemExit("\n".join(errors))
print(f"Localization PASS: {len(used)} used messages; {len(catalog)} catalog entries × 8 translated locales; {len(finance_used)} finance schema keys.")
