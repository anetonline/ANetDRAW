#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-2.0-or-later
"""Verify exhaustive acceptance-test coverage of the installed public API."""

from __future__ import annotations

import json
import re
import sys
from pathlib import Path

import check_api_docs


ROOT = Path(__file__).resolve().parent.parent
MANIFEST = ROOT / "tests" / "acceptance" / "public-contract.json"

PUBLIC_TYPES = {
    "BYTE", "INT8", "WORD", "INT16", "DWORD", "INT32", "DWORD_PTR",
    "INT", "UINT", "BOOL", "tODMilliSec", "tODReserveResult",
    "tODEditTextFormat",
    "tODEditMenuResult", "tODEditOptions", "tODInputEventType",
    "tODInputEvent", "OD_COMPONENT", "OD_PERSONALITY_PROC", "tODControl",
}


def listed_names(path: Path, pattern: str) -> set[str]:
    text = path.read_text(encoding="utf-8")
    return set(re.findall(pattern, text, flags=re.MULTILINE))


def compare_list(failures: list[str], label: str, actual: set[str],
                 expected: set[str]) -> None:
    for name in sorted(expected - actual):
        failures.append(f"{label} omits {name}")
    for name in sorted(actual - expected):
        failures.append(f"{label} contains stale name {name}")


def exported_functions() -> set[str]:
    text = check_api_docs.HEADER.read_text(encoding="utf-8")
    text = re.sub(r"(?m)^\s*#.*$", "", text)
    return set(re.findall(
        r"ODAPIDEF\b(?:(?!;).)*?\b([A-Za-z_][A-Za-z0-9_]*)\s*\(",
        text,
        flags=re.DOTALL,
    ))


def inventory() -> set[tuple[str, str]]:
    items: set[tuple[str, str]] = set()
    items.update(("function", name) for name in exported_functions())
    items.update(
        ("personality-function", name)
        for name in check_api_docs.personality_functions()
    )
    items.update(("control-field", name) for name in check_api_docs.control_fields())
    items.update(("constant", name) for name in check_api_docs.public_constants())
    items.update(("compatibility-alias", name)
                 for name in check_api_docs.compatibility_aliases())
    items.update(("type", name) for name in PUBLIC_TYPES)
    items.update({("global", "od_control"), ("personality-global", "szStatusText")})
    return items


def main() -> int:
    failures: list[str] = []
    try:
        data = json.loads(MANIFEST.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        print(f"error: unable to read {MANIFEST.relative_to(ROOT)}: {error}",
              file=sys.stderr)
        return 1

    tests = data.get("tests", {})
    coverage = data.get("coverage", [])
    if data.get("version") != 1:
        failures.append("public-contract manifest version must be 1")
    if not isinstance(tests, dict) or not tests:
        failures.append("public-contract manifest has no test registry")

    covered: dict[tuple[str, str], dict[str, object]] = {}
    for entry in coverage:
        if not isinstance(entry, dict):
            failures.append("coverage entry is not an object")
            continue
        key = (entry.get("kind"), entry.get("name"))
        if not all(isinstance(value, str) and value for value in key):
            failures.append(f"invalid coverage identity: {key!r}")
            continue
        if key in covered:
            failures.append(f"duplicate coverage entry: {key[0]} {key[1]}")
        covered[key] = entry
        evidence = entry.get("evidence")
        platforms = entry.get("platforms")
        if not isinstance(evidence, list) or not evidence:
            failures.append(f"{key[0]} {key[1]} has no evidence")
        else:
            for test_id in evidence:
                if test_id not in tests:
                    failures.append(
                        f"{key[0]} {key[1]} names unknown test {test_id!r}")
        if not isinstance(platforms, list) or not platforms:
            failures.append(f"{key[0]} {key[1]} has no platform applicability")

    declared = inventory()
    for kind, name in sorted(declared - set(covered)):
        failures.append(f"public {kind} lacks acceptance coverage: {name}")
    for kind, name in sorted(set(covered) - declared):
        failures.append(f"stale acceptance entry has no public declaration: {kind} {name}")

    acceptance = ROOT / "tests" / "acceptance"
    compare_list(failures, "control-fields.inc",
        listed_names(acceptance / "control-fields.inc",
                     r"^FIELD\(([A-Za-z_][A-Za-z0-9_]*)\)"),
        set(check_api_docs.control_fields()))
    compare_list(failures, "public-constants.inc",
        listed_names(acceptance / "public-constants.inc",
                     r"^REQUIRE_MACRO\(([A-Za-z_][A-Za-z0-9_]*)\)"),
        set(check_api_docs.public_constants()))
    compare_list(failures, "compatibility-aliases.inc",
        listed_names(acceptance / "compatibility-aliases.inc",
                     r"^REQUIRE_MACRO\(([A-Za-z_][A-Za-z0-9_]*)\)"),
        set(check_api_docs.compatibility_aliases()))
    compare_list(failures, "public-symbols.txt",
        listed_names(acceptance / "public-symbols.txt",
                     r"^([A-Za-z_][A-Za-z0-9_]*)(?:\s|$)"),
        exported_functions() | {"od_control"})
    compare_list(failures, "dos-personality-symbols.txt",
        listed_names(acceptance / "dos-personality-symbols.txt",
                     r"^([A-Za-z_][A-Za-z0-9_]*)$"),
        set(check_api_docs.personality_functions()) | {"szStatusText"})

    if failures:
        for failure in failures:
            print(f"error: {failure}", file=sys.stderr)
        return 1

    by_kind: dict[str, int] = {}
    for kind, _name in declared:
        by_kind[kind] = by_kind.get(kind, 0) + 1
    summary = ", ".join(f"{count} {kind}" for kind, count in sorted(by_kind.items()))
    print(f"Public acceptance contract covers {len(declared)} items: {summary}.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
