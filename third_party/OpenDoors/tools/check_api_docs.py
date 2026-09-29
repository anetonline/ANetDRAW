#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-2.0-or-later
"""Check that the high-level OpenDoors API has Markdown reference coverage."""

from __future__ import annotations

import re
import sys
from collections import defaultdict
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
HEADER = ROOT / "include" / "OpenDoor.h"
PERSONALITY_HEADER = ROOT / "include" / "ODStat.h"
API_DIR = ROOT / "docs" / "reference" / "api"
PERSONALITY_DIR = ROOT / "docs" / "reference" / "personality"
CONTROL_DIR = ROOT / "docs" / "reference" / "control"
START_MARKER = "OpenDoors API function prototypes."
END_MARKER = "Definitions for compatibility with previous versions."
CONTROL_MARKER = "The OpenDoors control structure"
DOCS_DIR = ROOT / "docs"
CONSTANTS_DIR = DOCS_DIR / "reference" / "constants"
TYPES_PAGE = DOCS_DIR / "reference" / "types.md"
COMPATIBILITY_PAGE = DOCS_DIR / "reference" / "compatibility.md"
CONTROL_INDEX = CONTROL_DIR / "index.md"


def prose_code_spans(text: str):
    """Yield unlinked inline-code spans outside headings and fenced examples."""
    in_fence = False
    for line_number, line in enumerate(text.splitlines(), 1):
        if line.lstrip().startswith("```"):
            in_fence = not in_fence
            continue
        if in_fence or line.startswith("#"):
            continue

        linked_ranges = [
            (match.start(), match.end())
            for match in re.finditer(r"\[[^\]]*`[^`]+`[^\]]*\]\([^)]+\)", line)
        ]
        for match in re.finditer(r"`([^`]+)`", line):
            if any(start <= match.start() < end for start, end in linked_ranges):
                continue
            prefix = line[: match.start()]
            if prefix.rfind("[") > prefix.rfind("]"):
                # The link label may continue onto a later Markdown line.
                continue
            yield line_number, match.group(1)


def inline_names(path: Path) -> set[str]:
    return {
        name
        for _line_number, name in prose_code_spans(path.read_text(encoding="utf-8"))
    }


def declared_functions() -> set[str]:
    text = HEADER.read_text(encoding="utf-8")
    try:
        start = text.index(START_MARKER)
    except ValueError as error:
        raise RuntimeError("OpenDoor.h API declaration markers were not found") from error

    # Compatibility entry points declared after the main prototype block are
    # still exported public functions and require their own reference pages.
    declaration_block = text[start:]
    return set(
        re.findall(
            r"ODAPIDEF\b(?:(?!;).)*?\b(od_[a-z0-9_]+)\s*\(",
            declaration_block,
            flags=re.DOTALL,
        )
    )


def documented_functions() -> dict[str, list[Path]]:
    pages: dict[str, list[Path]] = defaultdict(list)
    for path in API_DIR.rglob("od_*.md"):
        pages[path.stem].append(path)
    return pages


def personality_functions() -> set[str]:
    text = PERSONALITY_HEADER.read_text(encoding="utf-8")
    return set(
        re.findall(
            r"^\s*(?:void|INT|BOOL)\s+(?:(?:ODCALL|ODVCALL)\s+)?"
            r"(OD(?:Stat|Scrn)[A-Za-z0-9_]+)\s*\(",
            text,
            flags=re.MULTILINE,
        )
    )


def documented_personality_functions() -> dict[str, list[Path]]:
    pages: dict[str, list[Path]] = defaultdict(list)
    for path in PERSONALITY_DIR.glob("OD*.md"):
        pages[path.stem].append(path)
    return pages


def control_fields() -> set[str]:
    text = HEADER.read_text(encoding="utf-8")
    marker = text.index(CONTROL_MARKER)
    start = text.index("typedef struct\n{", marker)
    end = text.index("} tODControl;", start)
    fields: set[str] = set()

    block = re.sub(r"/\*.*?\*/", "", text[start:end], flags=re.DOTALL)
    block = re.sub(r"(?m)^\s*#.*$", "", block)
    for declaration in block.split(";"):
        declaration = declaration.strip()
        if not declaration:
            continue
        match = re.search(
            r"\(\s*(?:OD_DOS32_CALLBACK\s*)?\*\s*(\w+)\s*\)",
            declaration,
        )
        if match is None:
            match = re.search(
                r"\b([A-Za-z_][A-Za-z0-9_]*)\s*"
                r"(?:\[[^\]]+\]\s*)*$",
                declaration,
            )
        if match is not None:
            fields.add(match.group(1))

    return fields


def public_constants() -> set[str]:
    """Return application-facing constants and declaration macros."""
    text = HEADER.read_text(encoding="utf-8")
    start = text.index("/* TRUE and FALSE manifest constants")
    end = text.index(CONTROL_MARKER, start)
    public_block = text[start:end]
    names = set(
        re.findall(
            r"^\s*#\s*define\s+([A-Za-z_][A-Za-z0-9_]*)\b",
            public_block,
            flags=re.MULTILINE,
        )
    )
    names.update(
        re.findall(
            r"\b(?:FORMAT_|EDIT_MENU_|EVENT_)[A-Z0-9_]+\b", public_block
        )
    )
    names.update(
        {
            "OD_VERSION",
            "DIRSEP",
            "DIRSEP_STR",
            "ODPLAT_WIN32",
            "ODPLAT_NIX",
            "ODPLAT_DOS",
            "ODPLAT_DOS32",
            "OD_WIN32_STATIC",
            "OD_DLL",
            "ODCALL",
            "ODVCALL",
            "OD_GLOBAL_CONV",
            "ODAPIDEF",
            "OD_API_VAR_DEFN",
            "OD_API_VAR_DECL",
            "OD_EXPORT",
            "OD_IMPORT",
            "OD_NAMING_CONVENTION",
            "ODFAR",
            "B_YELLOW",
            "B_WHITE",
        }
    )
    return names


def compatibility_aliases() -> set[str]:
    """Return source-compatibility macros retained by OpenDoor.h."""
    text = HEADER.read_text(encoding="utf-8")
    start = text.index(END_MARKER)
    end = text.index("/* Obsolete functions. */", start)
    names = set(
        re.findall(
            r"^\s*#\s*define\s+([A-Za-z_][A-Za-z0-9_]*)\b",
            text[start:end],
            flags=re.MULTILINE,
        )
    )
    names.add("od_init_with_config")
    return names


def reference_symbols(
    declared: set[str], fields: set[str], personality_declared: set[str]
) -> tuple[dict[str, Path], dict[str, set[Path]]]:
    """Return documented prose names and the pages which define each name."""
    targets: dict[str, Path] = {}
    definitions: dict[str, set[Path]] = defaultdict(set)

    for name in declared:
        page = API_DIR / f"{name}.md"
        for spelling in (name, f"{name}()"):
            targets[spelling] = page
            definitions[spelling].add(page)

    for name in personality_declared:
        page = PERSONALITY_DIR / f"{name}.md"
        for spelling in (name, f"{name}()"):
            targets[spelling] = page
            definitions[spelling].add(page)
    targets["szStatusText"] = PERSONALITY_DIR / "szStatusText.md"
    definitions["szStatusText"].add(PERSONALITY_DIR / "szStatusText.md")

    control_pages = [
        CONTROL_DIR / "connection.md",
        CONTROL_DIR / "caller.md",
        CONTROL_DIR / "runtime.md",
        CONTROL_DIR / "customization.md",
    ]
    for field in fields:
        pages = {
            page
            for page in control_pages
            if re.search(
                rf"(?m)^#{{2,6}} `{re.escape(field)}`\s*$",
                page.read_text(encoding="utf-8"),
            )
        }
        if not pages:
            continue
        target = next(page for page in control_pages if page in pages)
        for spelling in (field, f"od_control.{field}"):
            targets[spelling] = target
            definitions[spelling].update(pages)

    constant_pages = [
        CONSTANTS_DIR / "general.md",
        CONSTANTS_DIR / "errors.md",
        CONSTANTS_DIR / "colors.md",
        CONSTANTS_DIR / "input.md",
        CONSTANTS_DIR / "display.md",
        CONSTANTS_DIR / "session.md",
        CONSTANTS_DIR / "components.md",
    ]
    for name in public_constants():
        pages = {
            page
            for page in constant_pages
            if re.search(rf"\b{re.escape(name)}\b", page.read_text(encoding="utf-8"))
        }
        if not pages:
            continue
        targets[name] = next(page for page in constant_pages if page in pages)
        definitions[name].update(pages)

    type_names = {
        "BYTE",
        "INT8",
        "WORD",
        "INT16",
        "DWORD",
        "INT32",
        "DWORD_PTR",
        "INT",
        "UINT",
        "BOOL",
        "TRUE",
        "FALSE",
        "tODMilliSec",
        "tODReserveResult",
        "tODInputEventType",
        "tODInputEvent",
        "tODEditOptions",
        "OD_PERSONALITY_PROC",
        "OD_COMPONENT",
    }
    for name in type_names:
        targets.setdefault(name, TYPES_PAGE)
        definitions[name].add(TYPES_PAGE)

    constant_groups = {
        "ERR_*": "errors.md",
        "ERRORLEVEL_*": "errors.md",
        "GETIN_*": "input.md",
        "EDIT_*": "input.md",
        "OD_KEY_*": "input.md",
        "MENU_*": "display.md",
        "POPUP_*": "display.md",
        "SCROLL_*": "display.md",
        "STATUS_*": "display.md",
        "BOX_*": "display.md",
        "COM_*": "session.md",
        "DIS_*": "session.md",
        "INCLUDE_*": "components.md",
        "PER_*": "components.md",
        "PEROP_*": "components.md",
        "D_*": "colors.md",
        "L_*": "colors.md",
        "B_*": "colors.md",
    }
    for name, filename in constant_groups.items():
        page = CONSTANTS_DIR / filename
        targets[name] = page
        definitions[name].add(page)

    compatibility_names = {
        "od_log_open()",
        "od_emulate()",
        "ODConfigInit",
        "ODLogEnable",
        "ODMPSEnable",
    }
    for name in compatibility_names:
        targets[name] = COMPATIBILITY_PAGE
        definitions[name].add(COMPATIBILITY_PAGE)

    for name in compatibility_aliases():
        targets.setdefault(name, COMPATIBILITY_PAGE)
        definitions[name].add(COMPATIBILITY_PAGE)
    for name in {"od_colour_config", "od_set_colour", "od_kernal"}:
        targets[f"{name}()"] = COMPATIBILITY_PAGE
        definitions[f"{name}()"].add(COMPATIBILITY_PAGE)

    targets["od_control"] = CONTROL_INDEX
    definitions["od_control"] = {CONTROL_INDEX}
    targets["od_*"] = API_DIR / "index.md"
    definitions["od_*"] = {API_DIR / "index.md"}
    targets["OpenDoor.h"] = API_DIR / "index.md"
    definitions["OpenDoor.h"] = {API_DIR / "index.md"}
    targets["ODStat.h"] = PERSONALITY_DIR / "index.md"
    definitions["ODStat.h"] = {PERSONALITY_DIR / "index.md"}
    targets["OD_WIN32_STATIC"] = DOCS_DIR / "guides" / "building.md"
    definitions["OD_WIN32_STATIC"].add(DOCS_DIR / "guides" / "building.md")
    return targets, definitions


def main() -> int:
    declared = declared_functions()
    documented = documented_functions()
    personality_declared = personality_functions()
    personality_documented = documented_personality_functions()
    fields = control_fields()
    failures: list[str] = []

    for name in sorted(declared):
        if name not in documented:
            failures.append(f"missing API page for {name}()")

    for name, paths in sorted(documented.items()):
        if name not in declared:
            relative = ", ".join(str(path.relative_to(ROOT)) for path in paths)
            failures.append(f"API page has no current declaration: {name} ({relative})")
            continue

        for path in paths:
            text = path.read_text(encoding="utf-8")
            if f"# `{name}()`" not in text:
                failures.append(f"{path.relative_to(ROOT)} has no canonical heading")
            if "## Synopsis" not in text:
                failures.append(f"{path.relative_to(ROOT)} has no Synopsis section")
            if not re.search(rf"\b{re.escape(name)}\s*\(", text):
                failures.append(f"{path.relative_to(ROOT)} does not name its declaration")

    for name in sorted(personality_declared):
        if name not in personality_documented:
            failures.append(f"missing personality SDK page for {name}()")

    for name, paths in sorted(personality_documented.items()):
        if name not in personality_declared:
            relative = ", ".join(str(path.relative_to(ROOT)) for path in paths)
            failures.append(
                f"personality SDK page has no current declaration: {name} ({relative})"
            )
            continue
        for path in paths:
            text = path.read_text(encoding="utf-8")
            if f"# `{name}()`" not in text:
                failures.append(f"{path.relative_to(ROOT)} has no canonical heading")
            if "## Synopsis" not in text:
                failures.append(f"{path.relative_to(ROOT)} has no Synopsis section")
            if not re.search(rf"\b{re.escape(name)}\s*\(", text):
                failures.append(f"{path.relative_to(ROOT)} does not name its declaration")

    if not (PERSONALITY_DIR / "szStatusText.md").is_file():
        failures.append("missing personality SDK page for szStatusText")

    for name in sorted(fields):
        heading = re.compile(rf"(?m)^#{{2,6}} `{re.escape(name)}`\s*$")
        matching_pages = [
            path
            for path in CONTROL_DIR.glob("*.md")
            if heading.search(path.read_text(encoding="utf-8"))
        ]
        if not matching_pages:
            failures.append(f"od_control field is not documented: {name}")
        elif len(matching_pages) > 1:
            relative = ", ".join(
                str(path.relative_to(ROOT)) for path in matching_pages
            )
            failures.append(
                f"od_control field has multiple canonical entries: {name} ({relative})"
            )

    constant_text = "\n".join(
        path.read_text(encoding="utf-8") for path in CONSTANTS_DIR.glob("*.md")
    )
    constants = public_constants()
    for name in sorted(constants):
        if not re.search(rf"\b{re.escape(name)}\b", constant_text):
            failures.append(f"public constant or macro is not documented: {name}")

    compatibility_text = COMPATIBILITY_PAGE.read_text(encoding="utf-8")
    aliases = compatibility_aliases()
    for name in sorted(aliases):
        if not re.search(rf"\b{re.escape(name)}\b", compatibility_text):
            failures.append(f"compatibility alias is not documented: {name}")

    targets, definitions = reference_symbols(declared, fields, personality_declared)
    for path in sorted(DOCS_DIR.rglob("*.md")):
        for line_number, name in prose_code_spans(path.read_text(encoding="utf-8")):
            if name not in targets or path in definitions[name]:
                continue
            failures.append(
                f"{path.relative_to(ROOT)}:{line_number} has an unlinked "
                f"API reference: `{name}`"
            )

    if failures:
        for failure in failures:
            print(f"error: {failure}", file=sys.stderr)
        return 1

    print(
        f"API documentation covers all {len(declared)} high-level functions "
        f"with {sum(len(paths) for paths in documented.values())} page(s); "
        f"all {len(fields)} od_control fields, {len(constants)} public "
        f"constants/macros, and {len(aliases)} compatibility aliases are named; "
        f"all {len(personality_declared)} personality SDK functions are covered; "
        "prose API references are linked."
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
