#!/usr/bin/env python3
"""Select the smallest safe OpenDoors unit-test set for a Git diff."""

from __future__ import annotations

import argparse
import json
import re
import subprocess
import sys
from collections import defaultdict
from pathlib import Path, PurePosixPath

from inventory import ROOT, build_inventory, scan_c


LOCAL_INCLUDE = re.compile(r'^\s*#\s*include\s*"([^"]+)"', re.MULTILINE)
HUNK = re.compile(r"^@@ -(\d+)(?:,(\d+))? \+(\d+)(?:,(\d+))? @@")
FULL_PREFIXES = (
    "unit/framework/",
    "unit/tools/",
)
FULL_FILES = {
    ".github/workflows/unit-tests.yml",
    "CMakeLists.txt",
    "dos/CMakeLists.txt",
}


def registered_tests() -> list[dict[str, object]]:
    """Return the registered owners used to choose applicable CI platforms."""
    document = json.loads(
        (ROOT / "unit" / "tests.json").read_text(encoding="utf-8"))
    return document["tests"]


def unix_variants(platforms: dict[str, object]) -> dict[str, list[str]]:
    """Build the Unix job matrix only when Unix owners are selected."""
    return {"platform": ["unix"] if "unix" in platforms else []}


def watcom_variants(platforms: dict[str, object]) -> dict[str, list[dict[str, str]]]:
    """Build the Watcom matrix for only the selected DOS data models."""
    variants = []
    if "dos16" in platforms:
        variants.append({"target": "dos16-large", "platform": "dos16",
                         "toolchain": "watcom16"})
    if "dos32" in platforms:
        variants.extend([
            {"target": "dos32-register", "platform": "dos32",
             "toolchain": "watcom32r"},
            {"target": "dos32-stack", "platform": "dos32",
             "toolchain": "watcom32s"},
        ])
    return {"include": variants}


def git(*arguments: str, check: bool = True) -> str:
    process = subprocess.run(
        ["git", *arguments], cwd=ROOT, check=check,
        stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    return process.stdout


def changed_files(base: str, head: str) -> list[str]:
    arguments = ["diff", "--name-only", "--diff-filter=ACDMRTUXB", base]
    if head != "WORKTREE":
        arguments.append(head)
    output = git(*arguments)
    return [line for line in output.splitlines() if line]


def file_at(revision: str, path: str) -> str | None:
    process = subprocess.run(
        ["git", "show", f"{revision}:{path}"], cwd=ROOT,
        stdout=subprocess.PIPE, stderr=subprocess.DEVNULL)
    if process.returncode:
        return None
    return process.stdout.decode("latin-1")


def working_file(path: str) -> str | None:
    candidate = ROOT / path
    if not candidate.is_file():
        return None
    return candidate.read_text(encoding="latin-1")


def includes(text: str | None, owner: str) -> set[str]:
    if text is None:
        return set()
    parent = PurePosixPath(owner).parent
    result = set()
    for match in LOCAL_INCLUDE.finditer(text):
        path = str(parent / match.group(1))
        result.add(str(PurePosixPath(path)))
    return result


def include_closure(path: str, revision: str | None) -> set[str]:
    pending = [path]
    seen: set[str] = set()
    while pending:
        current = pending.pop()
        if current in seen:
            continue
        seen.add(current)
        text = file_at(revision, current) if revision else working_file(current)
        pending.extend(includes(text, current) - seen)
    seen.discard(path)
    return seen


def changed_ranges(base: str, head: str, path: str) -> tuple[list[tuple[int, int]], list[tuple[int, int]]]:
    arguments = ["diff", "--unified=0", "--no-ext-diff", base]
    if head != "WORKTREE":
        arguments.append(head)
    arguments.extend(["--", path])
    patch = git(*arguments)
    old: list[tuple[int, int]] = []
    new: list[tuple[int, int]] = []
    for line in patch.splitlines():
        match = HUNK.match(line)
        if not match:
            continue
        old_start = int(match.group(1))
        old_count = int(match.group(2) or "1")
        new_start = int(match.group(3))
        new_count = int(match.group(4) or "1")
        old.append((old_start, old_start + max(old_count, 1) - 1))
        new.append((new_start, new_start + max(new_count, 1) - 1))
    return old, new


def containing_functions(text: str | None, ranges: list[tuple[int, int]]) -> set[str] | None:
    if text is None:
        return None
    lines = text.splitlines()
    functions = scan_c(text)
    selected: set[str] = set()
    for start, end in ranges:
        if any(lines[number - 1].lstrip().startswith("#")
               for number in range(start, min(end, len(lines)) + 1)):
            return None
        owners = [function for function in functions
                  if start >= function.body_line and end <= function.end_line]
        if len(owners) != 1:
            return None
        selected.add(owners[0].name)
    return selected


def source_functions(base: str, head: str, path: str) -> set[str] | None:
    old_ranges, new_ranges = changed_ranges(base, head, path)
    old = containing_functions(file_at(base, path), old_ranges)
    new_text = file_at(head, path) if head != "WORKTREE" else working_file(path)
    new = containing_functions(new_text, new_ranges)
    if old is None or new is None or old != new:
        return None
    return old


def test_owner(path: str, tests: list[dict[str, object]] | None = None) \
        -> tuple[str, str] | None:
    """Return the registered owner of an individual case source."""
    registered = tests if tests is not None else registered_tests()
    matches = [
        (str(test["source"]), str(test["function"]))
        for test in registered if test.get("case") == path
    ]
    return matches[0] if len(matches) == 1 else None


def case_source(path: str) -> str | None:
    """Return the production source owning a case-support path."""
    parts = PurePosixPath(path).parts
    if len(parts) < 4 or parts[:2] != ("unit", "cases"):
        return None
    source_document = json.loads(
        (ROOT / "unit" / "sources.json").read_text(encoding="utf-8"))
    by_stem = {Path(item["path"]).stem: item["path"]
               for item in source_document["sources"]}
    return by_stem.get(parts[2])


def fixture_owners(path: str, tests: list[dict[str, object]]) \
        -> set[tuple[str, str]]:
    """Return owners whose isolated runtime depends on a changed fixture."""
    keys = ("windows_fixture", "dos_tsr_fixture", "turbo_tsr_fixture")
    return {
        (str(test["source"]), str(test["function"]))
        for test in tests
        if any(configuration.get(key) == path
               for configuration in test.get("configurations", [])
               for key in keys)
    }


def all_selection(inventory: dict[str, object]) -> dict[str, list[str]]:
    return {
        source["path"]: sorted({item["name"] for item in source["functions"]})
        for source in inventory["sources"]
    }


def manifest_document(revision: str, path: str) -> dict[str, object]:
    """Load a JSON manifest from a Git revision or the working tree."""
    text = (working_file(path) if revision == "WORKTREE"
            else file_at(revision, path))
    return json.loads(text) if text is not None else {}


def manifest_metadata_changed(base: str, head: str, path: str,
                              collection: str) -> bool:
    """Report a top-level manifest/schema change outside its records."""
    old = manifest_document(base, path).copy()
    new = manifest_document(head, path).copy()
    old.pop(collection, None)
    new.pop(collection, None)
    return old != new


def changed_manifest_keys(base: str, head: str, path: str,
                          collection: str, key_fields: tuple[str, ...]) \
        -> set[tuple[str, ...]]:
    """Return the union of records whose manifest value changed."""
    def records(revision: str) -> dict[tuple[str, ...], dict[str, object]]:
        document = manifest_document(revision, path)
        return {
            tuple(str(item[field]) for field in key_fields): item
            for item in document.get(collection, [])
        }

    old = records(base)
    new = records(head)
    return {key for key in old.keys() | new.keys()
            if old.get(key) != new.get(key)}


def select(base: str, head: str, force_full: bool) -> dict[str, object]:
    inventory = build_inventory()
    tests = registered_tests()
    configured = {source["path"]: source for source in inventory["sources"]}
    if force_full:
        selected = all_selection(inventory)
        reason = "full"
    else:
        files = changed_files(base, head)
        manifest_collections = {
            "unit/inventory.json": "sources",
            "unit/sources.json": "sources",
            "unit/tests.json": "tests",
        }
        infrastructure_changed = any(
            path in FULL_FILES or path.startswith(FULL_PREFIXES)
            for path in files)
        schema_changed = any(
            path in files and manifest_metadata_changed(
                base, head, path, collection)
            for path, collection in manifest_collections.items())
        if infrastructure_changed or schema_changed:
            selected = all_selection(inventory)
            reason = "unit-infrastructure"
        else:
            by_source: dict[str, set[str] | None] = {}
            function_names = {
                path: {item["name"] for item in source["functions"]}
                for path, source in configured.items()
            }
            if "unit/tests.json" in files:
                changed_owners = changed_manifest_keys(
                    base, head, "unit/tests.json", "tests",
                    ("source", "function"))
                for source, function in changed_owners:
                    if (source not in configured or
                            function not in function_names[source]):
                        continue
                    current = by_source.get(source, set())
                    if current is not None:
                        current.add(function)
                        by_source[source] = current
            if "unit/sources.json" in files:
                changed_sources = changed_manifest_keys(
                    base, head, "unit/sources.json", "sources", ("path",))
                for (source,) in changed_sources:
                    if source in configured:
                        by_source[source] = None
            changed_headers = {path for path in files
                               if Path(path).suffix.lower() in {".h", ".inc"}}
            for path in files:
                if path in configured:
                    if Path(path).suffix.lower() == ".c":
                        names = source_functions(base, head, path)
                        by_source[path] = names
                    else:
                        by_source[path] = None
                    continue
                owner = test_owner(path, tests)
                if owner and owner[0] in configured:
                    current = by_source.get(owner[0], set())
                    if current is not None:
                        current.add(owner[1])
                        by_source[owner[0]] = current
                    continue
                case_owner = case_source(path)
                if case_owner in configured:
                    by_source[case_owner] = None
                    continue
                for source, function in fixture_owners(path, tests):
                    if source not in configured:
                        continue
                    current = by_source.get(source, set())
                    if current is not None:
                        current.add(function)
                        by_source[source] = current
            if changed_headers:
                for source in configured:
                    closure = (include_closure(source, None if head == "WORKTREE" else head) |
                               include_closure(source, base))
                    if closure & changed_headers:
                        by_source[source] = None
            selected = {}
            for source, names in by_source.items():
                if names is None:
                    names = {item["name"] for item in configured[source]["functions"]}
                selected[source] = sorted(names)
            reason = "affected"
    owner_platforms = {
        (str(test["source"]), str(test["function"])):
            [str(platform) for platform in test["platforms"]]
        for test in tests
    }
    platforms: dict[str, set[str]] = defaultdict(set)
    for path, functions in selected.items():
        for function in functions:
            for platform in owner_platforms.get((path, function), []):
                platforms[platform].add(path)
    return {
        "version": 1,
        "mode": reason,
        "run": bool(selected),
        "sources": selected,
        "platforms": {key: sorted(value) for key, value in sorted(platforms.items())},
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--base")
    parser.add_argument("--head", default="HEAD")
    parser.add_argument("--working-tree", action="store_true")
    parser.add_argument("--full", action="store_true")
    parser.add_argument("--github-output", type=Path)
    args = parser.parse_args()
    base = args.base
    if args.working_tree:
        args.head = "WORKTREE"
        if not base:
            base = "HEAD"
    if not args.full and not base:
        parser.error("--base is required unless --full or --working-tree is used")
    result = select(base or "HEAD", args.head, args.full)
    rendered = json.dumps(result, separators=(",", ":"))
    print(json.dumps(result, indent=2))
    if args.github_output:
        with args.github_output.open("a", encoding="utf-8") as output:
            output.write(f"selection={rendered}\n")
            output.write(f"run={'true' if result['run'] else 'false'}\n")
            for platform in ("unix", "windows", "dos16", "dos32"):
                output.write(
                    f"platform_{platform}="
                    f"{'true' if platform in result['platforms'] else 'false'}\n")
            output.write(
                "unix_matrix=" +
                json.dumps(unix_variants(result["platforms"]),
                           separators=(",", ":")) + "\n")
            output.write(
                "watcom_matrix=" +
                json.dumps(watcom_variants(result["platforms"]),
                           separators=(",", ":")) + "\n")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
