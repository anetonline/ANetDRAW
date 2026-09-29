#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-2.0-or-later
"""Check that a built library contains every public OpenDoors symbol."""

from __future__ import annotations

import argparse
import os
import re
import shutil
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent


def nm_symbols(library: Path, nm: str) -> set[str] | None:
    result = subprocess.run(
        [nm, "-g", str(library)], text=True, stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT, check=False
    )
    if result.returncode != 0:
        return None
    symbols: set[str] = set()
    for line in result.stdout.splitlines():
        match = re.search(r"(?:^|\s)([A-Za-z?])\s+([^\s]+)\s*$", line)
        if match is not None and match.group(1).upper() != "U":
            symbols.add(match.group(2))
    return symbols


def dumpbin_symbols(library: Path, dumpbin: str) -> set[str] | None:
    option = "/exports" if library.suffix.lower() == ".dll" else "/symbols"
    result = subprocess.run(
        [dumpbin, option, str(library)], text=True, stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT, check=False
    )
    if result.returncode != 0:
        return None
    symbols: set[str] = set()
    for line in result.stdout.splitlines():
        if option == "/symbols":
            if "External" not in line or "UNDEF" in line or "|" not in line:
                continue
            symbols.add(line.rsplit("|", 1)[1].strip().split()[0])
        else:
            match = re.match(r"\s*\d+\s+[0-9A-Fa-f]+\s+[0-9A-Fa-f]+\s+(\S+)", line)
            if match is not None:
                symbols.add(match.group(1))
    return symbols


def linker_symbols(library: Path, linker: str) -> set[str] | None:
    option = "/exports" if library.suffix.lower() == ".dll" else "/symbols"
    result = subprocess.run(
        [linker, "/dump", option, str(library)], text=True,
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT, check=False
    )
    if result.returncode != 0:
        return None
    symbols: set[str] = set()
    for line in result.stdout.splitlines():
        if option == "/symbols":
            if "External" not in line or "UNDEF" in line or "|" not in line:
                continue
            symbols.add(line.rsplit("|", 1)[1].strip().split()[0])
        else:
            match = re.match(r"\s*\d+\s+[0-9A-Fa-f]+\s+[0-9A-Fa-f]+\s+(\S+)", line)
            if match is not None:
                symbols.add(match.group(1))
    return symbols


def defined_symbols(library: Path, linker: str | None = None) -> set[str]:
    for name in ("nm", "llvm-nm", "x86_64-w64-mingw32-nm",
                 "i686-w64-mingw32-nm"):
        nm = shutil.which(name)
        if nm is None:
            continue
        symbols = nm_symbols(library, nm)
        if symbols:
            return symbols
    dumpbin = shutil.which("dumpbin")
    if dumpbin is not None:
        symbols = dumpbin_symbols(library, dumpbin)
        if symbols is not None:
            return symbols
    if linker:
        symbols = linker_symbols(library, linker)
        if symbols is not None:
            return symbols
    raise RuntimeError("neither nm/llvm-nm nor dumpbin could inspect the library")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("library", type=Path)
    parser.add_argument("--linker")
    parser.add_argument("--personality", action="store_true")
    parser.add_argument("--platform", choices=("unix", "windows"))
    args = parser.parse_args()
    names_path = ROOT / "tests" / "acceptance" / (
        "dos-personality-symbols.txt" if args.personality else "public-symbols.txt"
    )
    names = []
    for line in names_path.read_text(encoding="utf-8").splitlines():
        if not line:
            continue
        parts = line.split()
        if len(parts) > 1:
            platforms = {item.strip("[],") for item in parts[1:]}
            if args.platform is not None and args.platform not in platforms:
                continue
        names.append(parts[0])
    try:
        symbols = defined_symbols(args.library, args.linker)
    except RuntimeError as error:
        print(f"error: {error}", file=sys.stderr)
        return 1
    missing = []
    for name in names:
        pattern = rf"^_?{re.escape(name)}(?:@\d+)?$"
        if not any(re.match(pattern, symbol) for symbol in symbols):
            missing.append(name)
    if missing:
        for name in missing:
            print(f"error: {args.library} has no public symbol {name}", file=sys.stderr)
        return 1
    print(f"{args.library} contains all {len(names)} expected public symbols.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
