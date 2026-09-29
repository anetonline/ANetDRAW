#!/usr/bin/env python3
"""Inventory first-party C functions and assembler procedures.

The scanner deliberately sees inactive preprocessor branches.  Platform
compilation later narrows this conservative union to the definitions which
exist in each configuration.
"""

from __future__ import annotations

import argparse
import json
import re
import sys
from dataclasses import dataclass
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SOURCES_FILE = ROOT / "unit" / "sources.json"
INVENTORY_FILE = ROOT / "unit" / "inventory.json"
IDENTIFIER = re.compile(r"[A-Za-z_][A-Za-z0-9_]*")
CONTROL_WORDS = {"if", "for", "while", "switch", "sizeof"}
DECLARATION_WORDS = {
    "auto", "char", "const", "double", "enum", "extern", "float", "int",
    "long", "register", "short", "signed", "static", "struct", "typedef",
    "union", "unsigned", "void", "volatile",
}


@dataclass(frozen=True)
class Function:
    name: str
    signature_line: int
    body_line: int
    end_line: int
    body_offset: int = 0
    end_offset: int = 0


def _masked_c(text: str) -> str:
    """Replace comments and literals with spaces while retaining newlines."""
    result = list(text)
    index = 0
    state = "code"
    while index < len(text):
        char = text[index]
        following = text[index + 1] if index + 1 < len(text) else ""
        if state == "code":
            if char == "/" and following == "*":
                result[index] = result[index + 1] = " "
                state = "comment"
                index += 2
                continue
            if char == "/" and following == "/":
                result[index] = result[index + 1] = " "
                state = "line-comment"
                index += 2
                continue
            if char == '"':
                result[index] = " "
                state = "string"
            elif char == "'":
                result[index] = " "
                state = "character"
        elif state == "comment":
            if char == "*" and following == "/":
                result[index] = result[index + 1] = " "
                state = "code"
                index += 2
                continue
            if char != "\n":
                result[index] = " "
        elif state == "line-comment":
            if char == "\n":
                state = "code"
            else:
                result[index] = " "
        else:
            if char == "\\" and following:
                result[index] = " "
                if following != "\n":
                    result[index + 1] = " "
                index += 2
                continue
            if (state == "string" and char == '"') or (
                    state == "character" and char == "'"):
                result[index] = " "
                state = "code"
            elif char != "\n":
                result[index] = " "
        index += 1
    masked = "".join(result)
    # Directives are not C tokens at file scope.  Leaving an #endif between a
    # conditionally selected signature and its body hides the closing ')';
    # braces and parentheses in macro definitions can also look like source
    # structure.  Preserve inactive ordinary lines, but blank directive lines.
    lines = []
    conditional_stack: list[dict[str, bool]] = []
    original_lines = text.splitlines(keepends=True)
    masked_lines = masked.splitlines(keepends=True)
    for original, line in zip(original_lines, masked_lines):
        directive = original.lstrip()
        if directive.startswith("#"):
            directive_text = directive[1:].lstrip()
            if re.match(r"if\s+0(?:\s|$)", directive_text):
                conditional_stack.append({"literal_zero": True,
                                          "disabled": True})
            elif re.match(r"(?:if|ifdef|ifndef)\b", directive_text):
                conditional_stack.append({"literal_zero": False,
                                          "disabled": False})
            elif re.match(r"(?:else|elif)\b", directive_text):
                if (conditional_stack and
                        conditional_stack[-1]["literal_zero"]):
                    conditional_stack[-1]["disabled"] = False
            elif re.match(r"endif\b", directive_text):
                if conditional_stack:
                    conditional_stack.pop()
            lines.append("".join("\n" if char == "\n" else
                                 "\r" if char == "\r" else " "
                                 for char in line))
        elif any(frame["disabled"] for frame in conditional_stack):
            lines.append("".join("\n" if char == "\n" else
                                 "\r" if char == "\r" else " "
                                 for char in line))
        else:
            lines.append(line)
    return "".join(lines)


def _line(text: str, offset: int) -> int:
    return text.count("\n", 0, offset) + 1


def scan_c(text: str) -> list[Function]:
    masked = _masked_c(text)
    functions: list[Function] = []
    braces: list[int] = []
    index = 0
    while index < len(masked):
        char = masked[index]
        if char == "{":
            if not braces:
                before = index - 1
                while before >= 0 and masked[before].isspace():
                    before -= 1
                if before >= 0 and masked[before] == ")":
                    depth = 1
                    opening = before - 1
                    while opening >= 0 and depth:
                        if masked[opening] == ")":
                            depth += 1
                        elif masked[opening] == "(":
                            depth -= 1
                        opening -= 1
                    if depth == 0:
                        name_end = opening + 1
                        name_pos = name_end - 1
                        while name_pos >= 0 and masked[name_pos].isspace():
                            name_pos -= 1
                        match_end = name_pos + 1
                        while name_pos >= 0 and (
                                masked[name_pos].isalnum() or
                                masked[name_pos] == "_"):
                            name_pos -= 1
                        name = masked[name_pos + 1:match_end]
                        if (not IDENTIFIER.fullmatch(name) or
                                name in CONTROL_WORDS):
                            declaration_start = name_pos
                            while (declaration_start >= 0 and
                                   masked[declaration_start] not in ";}"):
                                declaration_start -= 1
                            candidates = [
                                match.group(1) for match in re.finditer(
                                    r"([A-Za-z_][A-Za-z0-9_]*)\s*\(",
                                    masked[declaration_start + 1:index])
                                if match.group(1) not in CONTROL_WORDS and
                                match.group(1) not in DECLARATION_WORDS
                            ]
                            if candidates:
                                name = candidates[0]
                        if IDENTIFIER.fullmatch(name) and name not in CONTROL_WORDS:
                            signature = name_pos
                            while signature >= 0 and masked[signature] not in ";}":
                                signature -= 1
                            functions.append(Function(
                                name=name,
                                signature_line=_line(masked, signature + 1),
                                body_line=_line(masked, index),
                                end_line=0,
                                body_offset=index,
                            ))
            braces.append(index)
        elif char == "}" and braces:
            braces.pop()
            if not braces and functions and functions[-1].end_line == 0:
                current = functions[-1]
                functions[-1] = Function(
                    current.name,
                    current.signature_line,
                    current.body_line,
                    _line(masked, index),
                    current.body_offset,
                    index + 1,
                )
        index += 1
    return [function for function in functions if function.end_line]


def scan_asm(text: str) -> list[Function]:
    starts: dict[str, tuple[int, int]] = {}
    functions: list[Function] = []
    for line_number, line in enumerate(text.splitlines(), 1):
        proc = re.match(
            r"^\s*([A-Za-z_][A-Za-z0-9_@$?]*)\s+PROC\b", line,
            re.IGNORECASE)
        if proc:
            starts.setdefault(proc.group(1), (line_number, line_number))
            continue
        endp = re.match(
            r"^\s*([A-Za-z_][A-Za-z0-9_@$?]*)\s+ENDP\b", line,
            re.IGNORECASE)
        if endp and endp.group(1) in starts:
            start, body = starts.pop(endp.group(1))
            functions.append(Function(endp.group(1), start, body, line_number))
    return functions


def build_inventory(root: Path = ROOT) -> dict[str, object]:
    source_config = json.loads(SOURCES_FILE.read_text(encoding="utf-8"))
    entries = []
    for source in source_config["sources"]:
        path = root / source["path"]
        text = path.read_text(encoding="latin-1")
        scanner = scan_asm if path.suffix.lower() == ".asm" else scan_c
        functions = scanner(text)
        function_platforms = source.get("function_platforms", {})
        entries.append({
            "path": source["path"],
            "platforms": source["platforms"],
            "functions": [
                {
                    "name": function.name,
                    "signature_line": function.signature_line,
                    "body_line": function.body_line,
                    "end_line": function.end_line,
                    "platforms": function_platforms.get(
                        function.name, source["platforms"]),
                    "test": None,
                }
                for function in functions
            ],
        })
    return {"version": 1, "sources": entries}


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--write", action="store_true")
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    inventory = build_inventory()
    rendered = json.dumps(inventory, indent=2) + "\n"
    if args.write:
        INVENTORY_FILE.write_text(rendered, encoding="utf-8")
    elif args.check:
        if not INVENTORY_FILE.exists() or INVENTORY_FILE.read_text(
                encoding="utf-8") != rendered:
            print("unit/inventory.json is stale; run inventory.py --write",
                  file=sys.stderr)
            return 1
    else:
        sys.stdout.write(rendered)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
