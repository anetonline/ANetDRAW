#!/usr/bin/env python3
"""Generate isolated units from the exact Turbo C 2.01 source path."""

from __future__ import annotations

import argparse
import json
import os
from pathlib import Path

from asm_generate import generate as generate_assembly
from generate import generate
from inventory import ROOT
from run import (TESTS_FILE, dos_short_stem, expand_configurations,
                 preprocessor_flags, selection_owners)


def turbo_ast_flags(watcom: Path) -> list[str]:
    """Clang compatibility flags for Turbo's 16-bit large-model source path."""
    return [
        "-std=c89",
        "-U__unix__", "-U__unix", "-Uunix", "-U__FreeBSD__",
        "-D__TURBOC__=0x0201", "-D__LARGE__", "-D_M_IX86=100",
        "-D_M_I86=1",
        "-DOPENDOORS_HAVE_VSNPRINTF=1",
        "-Dinterrupt=", "-Dfar=", "-Dasm=__asm", "-fasm-blocks",
        "-fdeclspec", "-Wno-ignored-attributes", "-Wno-ignored-pragmas",
        "-D__near=", "-D__far=", "-D__huge=", "-D__interrupt=",
        "-D__based(x)=", "-D__watcall=", "-D__int64=long long",
        "-D__builtin_alloca=watcom_alloca",
        "-include", str(ROOT / "unit" / "framework" / "turbo_ast.h"),
        "-I", str(ROOT / "include"),
        "-I", str(ROOT / "src"),
        "-I", str(ROOT),
        "-I", str(ROOT / "third_party"),
        "-I", str(ROOT / "unit" / "framework" / "turbo_include"),
        "-isystem", str(watcom / "h"),
    ]


def selected_tests(sources: set[str], functions: set[str],
                   owners: set[tuple[str, str]] | None = None):
    document = json.loads(TESTS_FILE.read_text(encoding="utf-8"))
    for test in document["tests"]:
        if "dos16" not in test["platforms"]:
            continue
        if owners is not None and (test["source"], test["function"]) not in owners:
            continue
        if sources and test["source"] not in sources:
            continue
        if functions and test["function"] not in functions:
            continue
        yield from expand_configurations(test, "dos16")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--build", type=Path, required=True)
    parser.add_argument("--clang", default="clang")
    parser.add_argument("--watcom", type=Path)
    parser.add_argument("--source", action="append", default=[])
    parser.add_argument("--function", action="append", default=[])
    parser.add_argument(
        "--selection", type=Path,
        help="selector JSON containing exact source/function ownership pairs")
    args = parser.parse_args()

    watcom = args.watcom
    if watcom is None:
        value = os.environ.get("WATCOM")
        if not value:
            raise RuntimeError("WATCOM must name the Open Watcom installation")
        watcom = Path(value)
    if not (watcom / "h").is_dir():
        raise RuntimeError(f"Open Watcom headers are missing: {watcom / 'h'}")

    owners = selection_owners(args.selection) if args.selection else None
    tests = list(selected_tests(
        set(args.source), set(args.function), owners))
    if not tests:
        print("no selected Turbo C unit tests")
        return 0
    args.build.mkdir(parents=True, exist_ok=True)
    for pattern in ("U???????.c", "U???????.model.json",
                    "U???????.unit.asm"):
        for stale in args.build.glob(pattern):
            stale.unlink()

    used = set()
    base_flags = turbo_ast_flags(watcom)
    for test in tests:
        configuration = test["configuration"]
        stem = dos_short_stem(test["source"], test["function"],
                              configuration["name"])
        if stem in used:
            raise RuntimeError(f"Turbo C 8.3 unit-test name collision: {stem}")
        used.add(stem)
        output = args.build / (stem + ".c")
        late_defines = (list(configuration.get("defines", [])) +
                        list(configuration.get("turbo_late_defines", [])))
        if Path(test["source"]).suffix.lower() == ".asm":
            generate_assembly(
                ROOT / test["source"], test["function"],
                ROOT / test["case"], output,
                args.build / (stem + ".unit.asm"))
        else:
            generate(ROOT / test["source"], test["function"],
                     ROOT / test["case"], output, args.clang,
                     base_flags + preprocessor_flags(configuration),
                     late_defines=late_defines,
                     late_undefines=list(configuration.get("undefines", [])),
                     late_declarations=list(configuration.get(
                         "turbo_late_declarations", [])),
                     provided_variables=set(configuration.get(
                         "provided_variables", [])))
        model_path = output.with_suffix(".model.json")
        model = json.loads(model_path.read_text(encoding="utf-8"))
        model["compiler"] = "Turbo C 2.01"
        model["turbo_defines"] = late_defines
        model["turbo_undefines"] = list(configuration.get("undefines", []))
        model_path.write_text(json.dumps(model, indent=2) + "\n",
                              encoding="utf-8")
    print(f"generated {len(tests)} Turbo C unit translation(s)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
