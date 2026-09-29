#!/usr/bin/env python3
"""Build a test-generation model from Clang's JSON AST."""

from __future__ import annotations

import json
import re
import subprocess
from dataclasses import dataclass, field
from pathlib import Path
from typing import Iterator

from inventory import ROOT, scan_c


POSIX_API_FLAG = "-D_POSIX_C_SOURCE=200809L"

LANGUAGE_BUILTINS = {"va_start", "va_end", "va_copy", "__va_copy"}


def is_language_builtin(name: object) -> bool:
    text = str(name or "")
    return text.startswith("__builtin_") or text in LANGUAGE_BUILTINS


def replace_c_includes(text: str) -> str:
    """Use declarations, not embedded implementation bodies, in test copies."""
    result = re.sub(
        r'(^[ \t]*#[ \t]*include[ \t]*"[^"\r\n]+)\.c("[^\r\n]*$)',
        r"\1.h\2", text, flags=re.MULTILINE)
    if len(result) != len(text):
        raise RuntimeError("implementation include replacement changed offsets")
    return result


def mask_turbo_asm_jumps(text: str) -> str:
    """Hide Turbo inline-assembly jumps from Clang without moving offsets."""
    pattern = re.compile(
        r"^([ \t]*ASM[ \t]+)(j[a-z]+[ \t]+[A-Za-z_][A-Za-z0-9_]*)"
        r"([ \t]*(?:/\*.*\*/[ \t]*)?)$",
        re.MULTILINE | re.IGNORECASE)

    def replace(match: re.Match[str]) -> str:
        instruction = match.group(2)
        return (match.group(1) + "nop" + " " * (len(instruction) - 3)
                + match.group(3))

    result = pattern.sub(replace, text)
    if len(result) != len(text):
        raise RuntimeError("Turbo assembly normalization changed offsets")
    return result


def normalize_watcom_pack_pragmas(text: str) -> str:
    """Translate Watcom's pack-stack names to Clang without moving offsets."""
    prefix = r"(^[ \t]*#[ \t]*pragma[ \t]+pack[ \t]*\([ \t]*)"
    result = re.sub(prefix + r"__push\b", r"\1push  ", text,
                    flags=re.MULTILINE)
    result = re.sub(prefix + r"__pop\b", r"\1pop  ", result,
                    flags=re.MULTILINE)
    if len(result) != len(text):
        raise RuntimeError("Watcom pack normalization changed offsets")
    return result


def blank_unselected_bodies(text: str, target_name: str) -> str:
    """Hide non-target bodies from Clang without changing source offsets."""
    replacements = []
    for function in scan_c(text):
        if function.name == target_name:
            continue
        replacements.append((function.body_offset, function.end_offset))
    result = text
    for start, end in sorted(replacements, reverse=True):
        result = result[:start] + ";" + "".join(
            "\n" if char == "\n" else " " for char in result[start + 1:end]
        ) + result[end:]
    return result


@dataclass
class Parameter:
    name: str
    type: str


@dataclass
class Callable:
    id: str
    name: str
    result: str
    parameters: list[Parameter]
    variadic: bool
    calling_macro: str = ""
    storage: str = ""
    type_suffix: str = ""


@dataclass
class Variable:
    id: str
    name: str
    type: str
    writable: bool
    calling_macro: str = ""


@dataclass
class Decision:
    kind: str
    start: int
    end: int
    conditions: list[tuple[int, int]]


@dataclass
class SwitchBranch:
    kind: str
    value_start: int | None
    value_end: int | None
    line: int


@dataclass
class Switch:
    expression_start: int
    expression_end: int
    expression_type: str
    line: int
    branches: list[SwitchBranch]


@dataclass
class StatePoint:
    position: int
    names: list[str]


@dataclass
class Definition(Callable):
    signature_line: int = 0
    body_line: int = 0
    end_line: int = 0
    body_start: int = 0
    body_end: int = 0
    functions: list[Callable] = field(default_factory=list)
    variables: list[Variable] = field(default_factory=list)
    state_variables: list[Variable] = field(default_factory=list)
    state_points: list[StatePoint] = field(default_factory=list)
    decisions: list[Decision] = field(default_factory=list)
    switches: list[Switch] = field(default_factory=list)


def walk(node: dict[str, object]) -> Iterator[dict[str, object]]:
    yield node
    for child in node.get("inner", []):
        yield from walk(child)


def location(value: dict[str, object], key: str = "") -> dict[str, object]:
    item = value.get(key, value) if key else value
    if not isinstance(item, dict):
        return {}
    if "expansionLoc" in item:
        return item["expansionLoc"]
    if "spellingLoc" in item:
        return item["spellingLoc"]
    return item


def function_result(qualified_type: str) -> str:
    marker = qualified_type.find("(")
    if marker < 0:
        raise ValueError(f"cannot separate function result type: {qualified_type}")
    return qualified_type[:marker].rstrip()


def function_type_suffix(qualified_type: str) -> str:
    """Return qualifiers which follow a function's parameter type list."""
    opening = qualified_type.find("(")
    if opening < 0:
        return ""
    depth = 0
    for position in range(opening, len(qualified_type)):
        if qualified_type[position] == "(":
            depth += 1
        elif qualified_type[position] == ")":
            depth -= 1
            if depth == 0:
                return qualified_type[position + 1:].strip()
    return ""


def callable_from(node: dict[str, object], source_text: str = "") -> Callable:
    parameters = []
    for index, child in enumerate(node.get("inner", [])):
        if child.get("kind") != "ParmVarDecl":
            continue
        parameters.append(Parameter(
            child.get("name") or f"ut_arg_{index}",
            child["type"]["qualType"],
        ))
    signature = ""
    node_range = node.get("range", {})
    begin = location(node_range, "begin").get("offset")
    end = location(node_range, "end").get("offset")
    if source_text and isinstance(begin, int) and isinstance(end, int):
        signature = source_text[begin:end + 1]
    calling_macro = "ODCALL" if "ODCALL" in signature else ""
    type_suffix = function_type_suffix(node["type"]["qualType"])
    if calling_macro:
        type_suffix = ""
    storage = "static" if node.get("storageClass") == "static" else ""
    return Callable(
        id=node["id"],
        name=node["name"],
        result=function_result(node["type"]["qualType"]),
        parameters=parameters,
        variadic=bool(node.get("variadic")),
        calling_macro=calling_macro,
        storage=storage,
        type_suffix=type_suffix,
    )


def source_range(node: dict[str, object]) -> tuple[int, int] | None:
    node_range = node.get("range", {})
    raw_begin = node_range.get("begin", {})
    raw_end = node_range.get("end", {})
    begin = location(raw_begin)
    end = location(raw_end)
    if not isinstance(begin.get("offset"), int) or not isinstance(end.get("offset"), int):
        return None
    if (("expansionLoc" in raw_begin or "spellingLoc" in raw_begin) and
            ("expansionLoc" in raw_end or "spellingLoc" in raw_end) and
            begin["offset"] == end["offset"]):
        return None
    return begin["offset"], end["offset"] + end.get("tokLen", 1)


def switch_value_range(node: dict[str, object]) -> tuple[int, int] | None:
    """Return a reusable case value, including one macro invocation token."""
    result = source_range(node)
    if result is not None:
        return result
    node_range = node.get("range", {})
    raw_begin = node_range.get("begin", {})
    raw_end = node_range.get("end", {})
    if ("expansionLoc" not in raw_begin or "expansionLoc" not in raw_end):
        return None
    begin = location(raw_begin)
    end = location(raw_end)
    if (not isinstance(begin.get("offset"), int) or
            begin.get("offset") != end.get("offset")):
        return None
    return begin["offset"], begin["offset"] + begin.get("tokLen", 1)


def macro_invocation_follows(text: str, raw_location: dict[str, object]) -> bool:
    if ("expansionLoc" not in raw_location and
            "spellingLoc" not in raw_location):
        return False
    expanded = location(raw_location)
    offset = expanded.get("offset")
    length = expanded.get("tokLen", 1)
    if not isinstance(offset, int):
        return True
    position = offset + length
    while position < len(text) and text[position].isspace():
        position += 1
    return position < len(text) and text[position] == "("


def expression_source_range(node: dict[str, object],
                            text: str) -> tuple[int, int] | None:
    node_range = node.get("range", {})
    raw_begin = node_range.get("begin", {})
    raw_end = node_range.get("end", {})
    if (macro_invocation_follows(text, raw_begin) or
            macro_invocation_follows(text, raw_end)):
        # A function-like expansion location normally covers only the macro
        # name, not its invocation arguments. Wrapping that partial spelling
        # changes the C parse (for example, !isspace followed by an orphaned
        # argument list). Object-like tokens such as NULL are complete source
        # expressions and are safe to instrument at their expansion range.
        return None
    begin = location(raw_begin)
    end = location(raw_end)
    if (not isinstance(begin.get("offset"), int) or
            not isinstance(end.get("offset"), int)):
        return None
    if (("expansionLoc" in raw_begin or "spellingLoc" in raw_begin) and
            ("expansionLoc" in raw_end or "spellingLoc" in raw_end) and
            begin["offset"] == end["offset"]):
        return begin["offset"], end["offset"] + end.get("tokLen", 1)
    return source_range(node)


def unwrap_expression(node: dict[str, object]) -> dict[str, object]:
    transparent = {"ImplicitCastExpr", "ExprWithCleanups", "ConstantExpr"}
    while node.get("kind") in transparent and len(node.get("inner", [])) == 1:
        node = node["inner"][0]
    return node


def logical_conditions(node: dict[str, object]) -> list[dict[str, object]]:
    node = unwrap_expression(node)
    if node.get("kind") == "ParenExpr" and len(node.get("inner", [])) == 1:
        inner = unwrap_expression(node["inner"][0])
        if inner.get("kind") == "BinaryOperator" and inner.get("opcode") in {"&&", "||"}:
            return logical_conditions(inner)
    if node.get("kind") == "BinaryOperator" and node.get("opcode") in {"&&", "||"}:
        result = []
        for child in node.get("inner", []):
            result.extend(logical_conditions(child))
        return result
    if node.get("kind") == "UnaryOperator" and node.get("opcode") == "!":
        child = unwrap_expression(node.get("inner", [{}])[0])
        if child.get("kind") in {"ParenExpr", "BinaryOperator"}:
            nested = logical_conditions(child)
            if len(nested) > 1:
                return nested
    return [node]


def statement_condition(node: dict[str, object]) -> dict[str, object] | None:
    children = node.get("inner", [])
    kind = node.get("kind")
    if kind in {"IfStmt", "WhileStmt", "SwitchStmt"} and children:
        return children[0]
    if kind == "DoStmt" and len(children) >= 2:
        return children[-1]
    if kind == "ForStmt" and len(children) >= 3:
        return children[2] or None
    if kind in {"ConditionalOperator", "BinaryConditionalOperator"} and children:
        return children[0]
    return None


def decisions_in(body: dict[str, object], text: str, body_start: int,
                 body_end: int) -> list[Decision]:
    result = []
    for node in walk(body):
        if node.get("kind") not in {"IfStmt", "WhileStmt", "DoStmt", "ForStmt",
                                    "ConditionalOperator", "BinaryConditionalOperator"}:
            continue
        condition = statement_condition(node)
        if condition is None:
            continue
        whole = expression_source_range(condition, text)
        if whole is None or whole[0] < body_start or whole[1] > body_end:
            continue
        atoms = []
        for atom in logical_conditions(condition):
            atom_range = expression_source_range(atom, text)
            if atom_range is not None and atom_range not in atoms:
                atoms.append(atom_range)
        if not atoms:
            atoms = [whole]
        result.append(Decision(node["kind"], whole[0], whole[1], atoms))
    return sorted(result, key=lambda item: (item.start, -item.end, item.kind))


def switch_label_nodes(node: dict[str, object]) -> Iterator[dict[str, object]]:
    for child in node.get("inner", []):
        if child.get("kind") == "SwitchStmt":
            continue
        if child.get("kind") in {"CaseStmt", "DefaultStmt"}:
            yield child
        yield from switch_label_nodes(child)


def switches_in(body: dict[str, object], text: str, body_start: int,
                body_end: int) -> list[Switch]:
    result = []
    for node in walk(body):
        if node.get("kind") != "SwitchStmt":
            continue
        condition = statement_condition(node)
        condition_range = source_range(condition) if condition else None
        children = node.get("inner", [])
        switch_body = children[-1] if children else None
        if (condition_range is None or switch_body is None or
                condition_range[0] < body_start or condition_range[1] > body_end):
            continue
        branches = []
        for label in switch_label_nodes(switch_body):
            label_range = source_range(label)
            if label_range is None:
                continue
            if label.get("kind") == "DefaultStmt":
                value_start = value_end = None
            else:
                label_children = label.get("inner", [])
                value_range = (switch_value_range(label_children[0])
                               if label_children else None)
                if value_range is None:
                    continue
                value_start, value_end = value_range
            branches.append(SwitchBranch(
                kind=label["kind"],
                value_start=value_start,
                value_end=value_end,
                line=text.count("\n", 0, label_range[0]) + 1,
            ))
        result.append(Switch(
            expression_start=condition_range[0],
            expression_end=condition_range[1],
            expression_type=condition.get("type", {}).get("qualType", "int"),
            line=text.count("\n", 0, condition_range[0]) + 1,
            branches=branches,
        ))
    return sorted(result, key=lambda item: (item.expression_start,
                                             -item.expression_end))


def run_ast(source: Path, clang: str, flags: list[str],
            text: str | None = None) -> dict[str, object]:
    command = [clang, *flags, "-iquote", str(source.parent),
               "-I", str(ROOT / "include"), "-I", str(ROOT / "src"),
               "-I", str(ROOT),
               "-Xclang", "-ast-dump=json", "-fsyntax-only"]
    if text is None:
        command.append(str(source))
    else:
        command.extend(["-x", "c", "-"])
    # A Windows text-mode pipe expands LF to CRLF and invalidates AST offsets.
    source_bytes = text.encode("latin-1") if text is not None else None
    process = subprocess.run(command, cwd=ROOT, stdout=subprocess.PIPE,
                             stderr=subprocess.PIPE, input=source_bytes)
    if process.returncode:
        raise RuntimeError(
            f"Clang AST failed for {source.name}:\n"
            f"{process.stderr.decode('utf-8', errors='replace')}")
    return json.loads(process.stdout.decode("utf-8"))


def build_model(source: Path, clang: str = "clang",
                flags: list[str] | None = None,
                target_name: str | None = None) -> list[Definition]:
    flags = flags or ["-std=c89", "-D__unix__", POSIX_API_FLAG,
                      "-DHAS_INTTYPES_H",
                      "-DOPENDOORS_HAVE_VSNPRINTF=1"]
    text = source.read_text(encoding="latin-1")
    scanned = scan_c(text)
    analysis_text = replace_c_includes(text)
    if target_name is not None:
        analysis_text = blank_unselected_bodies(analysis_text, target_name)
    if any(flag.startswith("-D__WATCOMC__") for flag in flags):
        analysis_text = normalize_watcom_pack_pragmas(analysis_text)
    if any(flag.startswith("-D__TURBOC__") for flag in flags):
        analysis_text = mask_turbo_asm_jumps(analysis_text)
    ast = run_ast(source, clang, flags,
                  analysis_text
                  if target_name is not None or analysis_text != text else None)
    nodes = list(walk(ast))
    declaration_origins = {}
    current_origin = None
    for top_level in ast.get("inner", []):
        top_location = top_level.get("loc", {})
        if top_location.get("file"):
            current_origin = top_location["file"]
        if top_level.get("id") and current_origin:
            declaration_origins.setdefault(top_level["id"], current_origin)
    declarations = {}
    for node in nodes:
        if node.get("kind") not in {"FunctionDecl", "VarDecl"} or "id" not in node:
            continue
        previous = declarations.get(node["id"])
        if (previous is None or
                len(node.get("inner", [])) > len(previous.get("inner", []))):
            declarations[node["id"]] = node
    source_variable_nodes = {
        node.get("name"): node for node in ast.get("inner", [])
        if node.get("kind") == "VarDecl" and
        node.get("storageClass") != "extern" and
        not node.get("loc", {}).get("includedFrom")
    }
    source_variables = set(source_variable_nodes)
    definitions: list[Definition] = []
    for expected in scanned:
        candidates = []
        for node in ast.get("inner", []):
            if node.get("kind") != "FunctionDecl" or node.get("name") != expected.name:
                continue
            body = next((child for child in node.get("inner", [])
                         if child.get("kind") == "CompoundStmt"), None)
            if body is None:
                continue
            begin = location(body.get("range", {}), "begin")
            begin_line = begin.get("line")
            if begin_line is None and "offset" in begin:
                begin_line = text.count("\n", 0, begin["offset"]) + 1
            if begin_line == expected.body_line:
                candidates.append((node, body))
        if not candidates:
            continue
        node, body = candidates[-1]
        base = callable_from(node, text)
        body_begin = location(body["range"], "begin")["offset"]
        body_end_location = location(body["range"], "end")
        body_end = body_end_location["offset"] + body_end_location.get("tokLen", 1)
        local_ids = {child["id"] for child in walk(body)
                     if child.get("kind") in {"ParmVarDecl", "VarDecl"}}
        function_refs: dict[str, Callable] = {}
        variable_refs: dict[str, Variable] = {}
        state_refs: dict[str, Variable] = {}
        for child in walk(body):
            if child.get("kind") != "DeclRefExpr":
                continue
            reference = child.get("referencedDecl", {})
            reference_id = reference.get("id")
            full = declarations.get(reference_id, reference)
            if (reference.get("kind") == "FunctionDecl" and
                    reference.get("name") != base.name):
                if is_language_builtin(reference.get("name")):
                    continue
                definition_begin = location(full.get("range", {}), "begin")
                included_c_body = str(
                    definition_begin.get("file", "")).lower().endswith(".c")
                if (any(item.get("kind") == "CompoundStmt"
                        for item in full.get("inner", [])) and
                        full.get("loc", {}).get("includedFrom") and
                        not included_c_body):
                    # A header-defined inline is already part of this
                    # translation unit. Renaming it would rename both its
                    # declaration and definition, so a mock would be a second
                    # conflicting definition rather than an isolated call.
                    continue
                try:
                    item = callable_from(full, text)
                except (KeyError, ValueError):
                    continue
                function_refs[item.name] = item
            elif (reference.get("kind") == "VarDecl" and
                  reference_id not in local_ids):
                item_type = reference.get("type", {}).get("qualType")
                if not item_type or item_type.startswith("const "):
                    continue
                origin = declaration_origins.get(reference_id)
                if origin:
                    origin_path = Path(origin)
                    if not origin_path.is_absolute():
                        origin_path = (ROOT / origin_path).resolve()
                    try:
                        origin_path.relative_to(ROOT.resolve())
                    except ValueError:
                        # Runtime-library objects must retain the actual CRT
                        # definition; a zero-filled test definition can even
                        # replace process-wide state such as FreeBSD stdout.
                        continue
                if reference.get("name") in source_variables:
                    state_refs[reference["name"]] = Variable(
                        id=reference_id or reference["name"],
                        name=reference["name"], type=item_type, writable=True)
                else:
                    declaration_text = ""
                    if origin:
                        origin_path = Path(origin)
                        if not origin_path.is_absolute():
                            origin_path = (ROOT / origin_path).resolve()
                        declaration_range = source_range(full)
                        if declaration_range and origin_path.is_file():
                            origin_text = origin_path.read_text(
                                encoding="latin-1")
                            declaration_text = origin_text[
                                declaration_range[0]:declaration_range[1]]
                    variable_refs[reference["name"]] = Variable(
                        id=reference_id or reference["name"],
                        name=reference["name"],
                        type=item_type,
                        writable=not item_type.startswith("const "),
                        calling_macro=("ODCALL" if
                                       "ODCALL" in declaration_text else ""),
                    )
        leading_declarations = []
        for body_child in body.get("inner", []):
            if body_child.get("kind") != "DeclStmt":
                break
            leading_declarations.append(body_child)
        leading_ids = {item.get("id") for item in leading_declarations}
        leading_end = None
        if leading_declarations:
            leading_range = source_range(leading_declarations[-1])
            if leading_range is not None:
                leading_end = leading_range[1]
        state_points = []
        leading_state_names = []
        for child in walk(body):
            if child.get("kind") != "DeclStmt":
                continue
            names = []
            for declaration_node in child.get("inner", []):
                item_type = declaration_node.get("type", {}).get("qualType", "")
                if (declaration_node.get("kind") == "VarDecl" and
                        declaration_node.get("storageClass") == "static" and
                        not item_type.startswith("const ") and
                        declaration_node.get("name")):
                    names.append(declaration_node["name"])
            child_range = source_range(child)
            if not names or child_range is None:
                continue
            # Clang's DeclStmt range includes the terminating semicolon.
            if child.get("id") in leading_ids and leading_end is not None:
                leading_state_names.extend(names)
            else:
                state_points.append(StatePoint(child_range[1], names))
        if leading_state_names and leading_end is not None:
            state_points.append(StatePoint(leading_end, leading_state_names))
        definitions.append(Definition(
            **base.__dict__,
            signature_line=text.count("\n", 0, source_range(node)[0]) + 1
            if source_range(node) is not None else expected.signature_line,
            body_line=expected.body_line,
            end_line=expected.end_line,
            body_start=body_begin,
            body_end=body_end,
            functions=sorted(function_refs.values(), key=lambda item: item.name),
            variables=sorted(variable_refs.values(), key=lambda item: item.name),
            state_variables=sorted(state_refs.values(), key=lambda item: item.name),
            state_points=sorted(state_points, key=lambda item: item.position),
            decisions=decisions_in(body, text, body_begin, body_end),
            switches=switches_in(body, text, body_begin, body_end),
        ))
    return definitions


def insert_name(type_name: str, name: str) -> str:
    if "(*)" in type_name:
        return type_name.replace("(*)", f"(*{name})", 1)
    bracket = type_name.find("[")
    if bracket >= 0:
        return f"{type_name[:bracket].rstrip()} {name}{type_name[bracket:]}"
    return f"{type_name} {name}"


def declaration(item: Callable, name: str | None = None,
                include_storage: bool = False) -> str:
    pieces = []
    if include_storage and item.storage:
        pieces.append(item.storage)
    pieces.append(item.result)
    if item.type_suffix:
        pieces.append(item.type_suffix)
    if item.calling_macro:
        pieces.append(item.calling_macro)
    pieces.append(name or item.name)
    parameters = [insert_name(param.type, param.name)
                  for param in item.parameters]
    if item.variadic:
        parameters.append("...")
    elif not parameters:
        parameters.append("void")
    return " ".join(pieces) + "(" + ", ".join(parameters) + ")"
