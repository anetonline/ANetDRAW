#!/usr/bin/env python3
"""Validate unit-test ownership and reviewed coverage waivers."""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import sys
from pathlib import Path

from inventory import INVENTORY_FILE, ROOT, SOURCES_FILE, build_inventory


TESTS_FILE = ROOT / "unit" / "tests.json"
WAIVERS_FILE = ROOT / "unit" / "coverage-waivers.json"
PROPOSALS_FILE = ROOT / "unit" / "coverage-waiver-proposals.json"
RELEASE_WORKFLOW = ROOT / ".github" / "workflows" / "release.yml"
UNIT_WORKFLOW = ROOT / ".github" / "workflows" / "unit-tests.yml"
MODERN_CMAKE = ROOT / "src" / "CMakeLists.txt"
DOS_CMAKE = ROOT / "dos" / "CMakeLists.txt"
PROPOSED_WAIVER_FLAG = "--allow-proposed-coverage-waivers"

CONFIGURATION_NAME = re.compile(r"^[A-Za-z0-9_-]+$")
MACRO_NAME = re.compile(r"^[A-Za-z_][A-Za-z0-9_]*$")
DEFINE = re.compile(r"^[A-Za-z_][A-Za-z0-9_]*(?:=.*)?$")
UNIT_PLATFORMS = {"unix", "windows", "dos16", "dos32"}


def cmake_first_party_sources(modern: str, dos: str) -> set[str]:
    """Extract repository-relative C/assembly library build inputs."""
    sources = set()
    for text, variable in ((modern, "OPENDOORS_SOURCES"),
                           (dos, "OPENDOORS_DOS_SOURCES")):
        blocks = re.findall(
            rf"set\s*\(\s*{variable}\s+(.*?)\)", text, re.DOTALL)
        blocks += re.findall(
            rf"list\s*\(\s*APPEND\s+{variable}\s+(.*?)\)",
            text, re.DOTALL)
        for block in blocks:
            found = re.findall(
                r"(?:\$\{(?:OPENDOORS_ROOT|OPENDOORS_SOURCE_DIR|"
                r"PROJECT_SOURCE_DIR)\}/)?"
                r"([A-Za-z0-9_./-]+\.(?:c|asm))\b", block)
            sources.update(path if "/" in path else f"src/{path}"
                           for path in found)
    sources.update(
        f"src/{path}" for path in re.findall(
            r"\$\{OPENDOORS_SOURCE_DIR\}/"
            r"([A-Za-z0-9_.-]+\.(?:c|asm))\b", dos))
    sources.update(re.findall(
        r"\$\{OPENDOORS_ROOT\}/(src/[A-Za-z0-9_.-]+\.(?:c|asm))\b",
        dos))
    return sources


def validate_first_party_sources() -> list[str]:
    """Require unit ownership for exactly the library build inputs."""
    if not (MODERN_CMAKE.is_file() and DOS_CMAKE.is_file() and
            SOURCES_FILE.is_file()):
        return ["cannot validate first-party source manifests"]
    expected = cmake_first_party_sources(
        MODERN_CMAKE.read_text(encoding="utf-8"),
        DOS_CMAKE.read_text(encoding="utf-8"))
    document = json.loads(SOURCES_FILE.read_text(encoding="utf-8"))
    actual = {str(source.get("path")) for source in document.get("sources", [])}
    problems = []
    missing = expected - actual
    extra = actual - expected
    if missing:
        problems.append(
            "unit source manifest lacks build inputs: " +
            ", ".join(sorted(missing)))
    if extra:
        problems.append(
            "unit source manifest names non-build inputs: " +
            ", ".join(sorted(extra)))
    return problems


def validate_source_native_flags(value: object) -> list[str]:
    """Validate warning flags that apply to every case from one source."""
    if not isinstance(value, dict):
        return ["source_native_flags must be an object"]
    problems = []
    for source, platforms in value.items():
        prefix = f"source native flags {source}"
        if not isinstance(source, str) or not source:
            problems.append("source native flags has an invalid source")
            continue
        if not isinstance(platforms, dict):
            problems.append(f"{prefix} must be an object")
            continue
        for platform, flags in platforms.items():
            if platform not in UNIT_PLATFORMS:
                problems.append(f"{prefix} has unknown platform: {platform}")
            if (not isinstance(flags, list) or
                    any(not isinstance(flag, str) or not flag
                        for flag in flags)):
                problems.append(
                    f"{prefix} {platform} flags must contain nonempty strings")
    return problems


def validate_configurations(index: int, test: dict[str, object]) -> list[str]:
    """Validate optional, explicitly named preprocessor test variants."""
    if "configurations" not in test:
        return []
    prefix = f"unit test {index} configurations"
    configurations = test["configurations"]
    if not isinstance(configurations, list) or not configurations:
        return [f"{prefix} must be a nonempty list"]
    problems = []
    names = set()
    covered_platforms = set()
    allowed = {"name", "defines", "undefines", "native_flags",
               "turbo_late_defines", "windows_fixture",
               "dos_tsr_fixture", "turbo_tsr_fixture",
               "turbo_late_declarations", "provided_variables",
               "platforms"}
    for number, configuration in enumerate(configurations, 1):
        item_prefix = f"{prefix} item {number}"
        if not isinstance(configuration, dict):
            problems.append(f"{item_prefix} must be an object")
            continue
        unknown = set(configuration) - allowed
        if unknown:
            problems.append(
                f"{item_prefix} has unknown keys: {', '.join(sorted(unknown))}")
        name = configuration.get("name")
        if not isinstance(name, str) or not CONFIGURATION_NAME.fullmatch(name):
            problems.append(f"{item_prefix} has invalid name")
        if isinstance(name, str) and name in names:
            problems.append(f"{item_prefix} has duplicate name: {name}")
        elif isinstance(name, str):
            names.add(name)
        platforms = configuration.get("platforms")
        if platforms is not None:
            if (not isinstance(platforms, list) or not platforms or
                    any(not isinstance(platform, str) or
                        platform not in UNIT_PLATFORMS
                        for platform in platforms)):
                problems.append(f"{item_prefix} has invalid platforms")
            elif not set(platforms) <= set(test.get("platforms", [])):
                problems.append(
                    f"{item_prefix} platforms exceed the unit test platforms")
            else:
                covered_platforms.update(platforms)
        else:
            covered_platforms.update(test.get("platforms", []))

        option_values = {}
        patterns = {"defines": DEFINE, "undefines": MACRO_NAME}
        for option, pattern in patterns.items():
            values = configuration.get(option, [])
            if not isinstance(values, list):
                problems.append(f"{item_prefix} {option} must be a list")
                continue
            if (any(not isinstance(value, str) or
                    not pattern.fullmatch(value) for value in values)):
                singular = "define" if option == "defines" else "undefine"
                problems.append(f"{item_prefix} has invalid {singular}")
                continue
            option_values[option] = values
        flags = configuration.get("native_flags", [])
        if (not isinstance(flags, list) or
                any(not isinstance(flag, str) or not flag
                    for flag in flags)):
            problems.append(
                f"{item_prefix} native_flags must contain nonempty strings")
        turbo_late_defines = configuration.get("turbo_late_defines", [])
        if (not isinstance(turbo_late_defines, list) or
                any(not isinstance(value, str) or not DEFINE.fullmatch(value)
                    for value in turbo_late_defines)):
            problems.append(f"{item_prefix} has invalid turbo late define")
        turbo_late_declarations = configuration.get(
            "turbo_late_declarations", [])
        if (not isinstance(turbo_late_declarations, list) or
                any(not isinstance(value, str) or "\n" in value or
                    "\r" in value or not value.rstrip().endswith(";")
                    for value in turbo_late_declarations)):
            problems.append(
                f"{item_prefix} has invalid Turbo late declaration")
        provided_variables = configuration.get("provided_variables", [])
        if (not isinstance(provided_variables, list) or
                any(not isinstance(value, str) or
                    not MACRO_NAME.fullmatch(value)
                    for value in provided_variables)):
            problems.append(f"{item_prefix} has invalid provided variable")
        windows_fixture = configuration.get("windows_fixture")
        if windows_fixture is not None:
            fixture_path = Path(windows_fixture) \
                if isinstance(windows_fixture, str) else None
            if (fixture_path is None or fixture_path.is_absolute() or
                    fixture_path.suffix.lower() != ".c" or
                    ".." in fixture_path.parts or
                    fixture_path.parts[:2] != ("unit", "fixtures")):
                problems.append(f"{item_prefix} has invalid windows fixture")
        turbo_tsr_fixture = configuration.get("turbo_tsr_fixture")
        if turbo_tsr_fixture is not None:
            fixture_path = Path(turbo_tsr_fixture) \
                if isinstance(turbo_tsr_fixture, str) else None
            if (fixture_path is None or fixture_path.is_absolute() or
                    fixture_path.suffix.lower() != ".asm" or
                    ".." in fixture_path.parts):
                problems.append(
                    f"{item_prefix} has invalid Turbo TSR fixture")
        dos_tsr_fixture = configuration.get("dos_tsr_fixture")
        if dos_tsr_fixture is not None:
            fixture_path = Path(dos_tsr_fixture) \
                if isinstance(dos_tsr_fixture, str) else None
            if (fixture_path is None or fixture_path.is_absolute() or
                    fixture_path.suffix.lower() != ".asm" or
                    ".." in fixture_path.parts):
                problems.append(f"{item_prefix} has invalid DOS TSR fixture")
        defined = {value.split("=", 1)[0]
                   for value in option_values.get("defines", [])}
        undefined = set(option_values.get("undefines", []))
        for macro in sorted(defined & undefined):
            problems.append(
                f"{item_prefix} both defines and undefines {macro}")
    uncovered = set(test.get("platforms", [])) - covered_platforms
    if uncovered:
        problems.append(
            f"{prefix} has no configuration for: "
            f"{', '.join(sorted(uncovered))}")
    return problems


def validate_turbo_shards(index: int, test: dict[str, object]) -> list[str]:
    """A Turbo-only split is meaningful only when it has multiple shards."""
    if "turbo_shards" not in test:
        return []
    count = test["turbo_shards"]
    if (not isinstance(count, int) or isinstance(count, bool) or
            count < 2 or count > 15):
        return [f"unit test {index} turbo_shards must be an integer from 2 to 15"]
    return []


def expected_tests(inventory: dict[str, object]) -> dict[tuple[str, str], set[str]]:
    expected = {}
    for source in inventory["sources"]:
        for function in source["functions"]:
            key = (source["path"], function["name"])
            expected.setdefault(key, set()).update(function["platforms"])
    return expected


def validate_proposal(index: int, proposal: dict[str, object],
                      expected: dict[tuple[str, str], set[str]],
                      root: Path = ROOT) -> list[str]:
    prefix = f"coverage waiver proposal {index}"
    problems = []
    required = {"id", "status", "source", "function", "platforms",
                "start_line", "end_line", "kinds", "reason", "evidence",
                "source_sha256"}
    missing = required - proposal.keys()
    if missing:
        return [f"{prefix} lacks: {', '.join(sorted(missing))}"]
    if proposal["status"] != "proposed":
        problems.append(f"{prefix} status must be proposed")
    key = (proposal["source"], proposal["function"])
    if key not in expected:
        problems.append(f"{prefix} names unknown function")
    platforms = proposal["platforms"]
    if not isinstance(platforms, list) or not platforms:
        problems.append(f"{prefix} must name at least one platform")
    elif key in expected:
        invalid = set(platforms) - expected[key]
        if invalid:
            problems.append(
                f"{prefix} names invalid platforms: {', '.join(sorted(invalid))}")
    kinds = proposal["kinds"]
    valid_kinds = {"line", "branch", "mcdc", "assembly-branch"}
    if not isinstance(kinds, list) or not kinds or set(kinds) - valid_kinds:
        problems.append(f"{prefix} has invalid kinds")
    start = proposal["start_line"]
    end = proposal["end_line"]
    if not isinstance(start, int) or not isinstance(end, int) or start > end:
        problems.append(f"{prefix} has invalid line range")
    else:
        source_path = root / str(proposal["source"])
        if source_path.is_file():
            lines = source_path.read_bytes().splitlines(keepends=True)
            selected = b"".join(lines[start - 1:end])
            digest = hashlib.sha256(selected).hexdigest()
            if digest != proposal["source_sha256"]:
                problems.append(f"{prefix} is stale")
    return problems


def validate_release_coverage_policy(workflow: str) -> list[str]:
    """Proposed coverage waivers must never relax a release gate."""
    if (PROPOSED_WAIVER_FLAG in workflow or
            "allow_proposed_coverage_waivers:" in workflow):
        return ["release workflow must not allow proposed coverage waivers"]
    return []


def validate_unit_workflow_policy(unit_workflow: str,
                                  release_workflow: str) -> list[str]:
    """Require selective CI and an uncompromised full release invocation."""
    problems = []
    for trigger in ("push:", "pull_request:", "workflow_call:"):
        if trigger not in unit_workflow:
            problems.append(f"unit workflow must support {trigger[:-1]}")
    if "check.py --allow-incomplete" in unit_workflow:
        problems.append("unit workflow ownership validation must be strict")
    if ("unix_matrix:" not in unit_workflow or
            "matrix: ${{ fromJSON(needs.select.outputs.unix_matrix) }}"
            not in unit_workflow):
        problems.append("unit workflow must use the exact Unix variant matrix")
    if ("watcom_matrix:" not in unit_workflow or
            "matrix: ${{ fromJSON(needs.select.outputs.watcom_matrix) }}"
            not in unit_workflow):
        problems.append("unit workflow must use the exact Watcom variant matrix")
    if ("runs-on: macos-15" not in unit_workflow or
            "unit-macos-${{ matrix.platform }}-coverage" not in unit_workflow):
        problems.append("unit workflow must run macOS unit coverage")
    if "uses: ./.github/workflows/unit-tests.yml" not in release_workflow:
        problems.append("release workflow must invoke the full unit workflow")
    if not re.search(
            r"uses:\s*\./\.github/workflows/unit-tests\.yml\s+with:\s+full:\s*true",
            release_workflow):
        problems.append("release unit workflow invocation must set full: true")
    collect = re.search(
        r"(?ms)^  collect:\s*\n(.*?)(?=^  [A-Za-z0-9_-]+:\s*$|\Z)",
        release_workflow)
    if (collect is None or "    needs:" not in collect.group(1) or
            not re.search(r"(?m)^      - unit-tests\s*$", collect.group(1))):
        problems.append("release publishing must depend on the strict unit suite")
    return problems


def validate(allow_incomplete: bool) -> list[str]:
    problems = []
    problems.extend(validate_first_party_sources())
    release_text = ""
    if RELEASE_WORKFLOW.is_file():
        release_text = RELEASE_WORKFLOW.read_text(encoding="utf-8")
        problems.extend(validate_release_coverage_policy(
            release_text))
    if UNIT_WORKFLOW.is_file() and release_text:
        problems.extend(validate_unit_workflow_policy(
            UNIT_WORKFLOW.read_text(encoding="utf-8"), release_text))
    generated = build_inventory()
    if not INVENTORY_FILE.exists() or json.loads(
            INVENTORY_FILE.read_text(encoding="utf-8")) != generated:
        problems.append("unit/inventory.json is stale; run inventory.py --write")
    expected = expected_tests(generated)
    document = json.loads(TESTS_FILE.read_text(encoding="utf-8"))
    problems.extend(validate_source_native_flags(
        document.get("source_native_flags", {})))
    actual = {}
    for index, test in enumerate(document.get("tests", []), 1):
        problems.extend(validate_configurations(index, test))
        problems.extend(validate_turbo_shards(index, test))
        key = (test.get("source"), test.get("function"))
        if key in actual:
            problems.append(f"duplicate test ownership: {key[0]}:{key[1]}")
            continue
        actual[key] = set(test.get("platforms", []))
        case = ROOT / test.get("case", "")
        if not case.is_file():
            problems.append(f"missing case file for {key[0]}:{key[1]}: {case}")
    for key in sorted(actual.keys() - expected.keys()):
        problems.append(f"test names unknown function: {key[0]}:{key[1]}")
    if not allow_incomplete:
        for key in sorted(expected.keys() - actual.keys()):
            problems.append(f"missing unit test: {key[0]}:{key[1]}")
    for key in sorted(expected.keys() & actual.keys()):
        missing = expected[key] - actual[key]
        extra = actual[key] - expected[key]
        if missing and not allow_incomplete:
            problems.append(
                f"{key[0]}:{key[1]} lacks platforms: {', '.join(sorted(missing))}")
        if extra:
            problems.append(
                f"{key[0]}:{key[1]} has invalid platforms: {', '.join(sorted(extra))}")

    waiver_document = json.loads(WAIVERS_FILE.read_text(encoding="utf-8"))
    required = {"source", "function", "platform", "start_line", "end_line",
                "kind", "reason", "evidence", "source_sha256"}
    for index, waiver in enumerate(waiver_document.get("waivers", []), 1):
        missing_fields = required - waiver.keys()
        if missing_fields:
            problems.append(
                f"coverage waiver {index} lacks: {', '.join(sorted(missing_fields))}")
            continue
        key = (waiver["source"], waiver["function"])
        if key not in expected:
            problems.append(f"coverage waiver {index} names unknown function")
        elif waiver["platform"] not in expected[key]:
            problems.append(f"coverage waiver {index} names invalid platform")
        if waiver["kind"] not in {
                "line", "branch", "mcdc", "assembly-branch"}:
            problems.append(f"coverage waiver {index} has invalid kind")
        if waiver["start_line"] > waiver["end_line"]:
            problems.append(f"coverage waiver {index} has reversed line range")
        source_path = ROOT / waiver["source"]
        if source_path.is_file() and waiver["start_line"] <= waiver["end_line"]:
            lines = source_path.read_bytes().splitlines(keepends=True)
            selected = b"".join(lines[waiver["start_line"] - 1:waiver["end_line"]])
            digest = hashlib.sha256(selected).hexdigest()
            if digest != waiver["source_sha256"]:
                problems.append(f"coverage waiver {index} is stale")

    proposal_document = json.loads(
        PROPOSALS_FILE.read_text(encoding="utf-8"))
    approved_sites = {
        (item["source"], item["function"], item["platform"], item["kind"],
         item["start_line"], item["end_line"])
        for item in waiver_document.get("waivers", [])
        if required <= item.keys()
    }
    for index, proposal in enumerate(
            proposal_document.get("proposals", []), 1):
        problems.extend(validate_proposal(index, proposal, expected))
        if {"source", "function", "platforms", "kinds", "start_line",
                "end_line"} <= proposal.keys():
            for platform in proposal["platforms"]:
                for kind in proposal["kinds"]:
                    site = (proposal["source"], proposal["function"],
                            platform, kind, proposal["start_line"],
                            proposal["end_line"])
                    if site in approved_sites:
                        problems.append(
                            f"coverage waiver proposal {index} duplicates an approved waiver")
    return problems


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--allow-incomplete", action="store_true")
    args = parser.parse_args()
    problems = validate(args.allow_incomplete)
    for problem in problems:
        print(problem, file=sys.stderr)
    if problems:
        return 1
    tests = json.loads(TESTS_FILE.read_text(encoding="utf-8"))["tests"]
    inventory = build_inventory()
    total = len(expected_tests(inventory))
    print(f"unit ownership: {len(tests)}/{total} functions")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
