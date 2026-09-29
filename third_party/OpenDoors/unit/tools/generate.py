#!/usr/bin/env python3
"""Generate an isolated, single-function OpenDoors unit translation unit."""

from __future__ import annotations

import argparse
import json
import re
from pathlib import Path

from clang_model import (Callable, POSIX_API_FLAG, Variable, build_model,
                         declaration, insert_name, replace_c_includes)
from inventory import ROOT, scan_c

CASE_ROOT = (ROOT / "unit" / "cases").resolve()
LOCAL_CASE_INCLUDE = re.compile(
    r'^[ \t]*#[ \t]*include[ \t]*"([^"\r\n]+)"[ \t]*(?:\r?\n)?$')
CUSTOM_MOCK = re.compile(
    r'^[ \t]*#[ \t]*define[ \t]+UT_CUSTOM_MOCK_([A-Za-z_][A-Za-z0-9_]*)\b',
    re.MULTILINE)
CTYPE_APIS = {
    "isalnum", "isalpha", "isblank", "iscntrl", "isdigit", "isgraph",
    "islower", "isprint", "ispunct", "isspace", "isupper", "isxdigit",
    "tolower", "toupper",
}
HEADER_MACRO_APIS = CTYPE_APIS | {
    "fprintf",
    "memchr", "memcmp", "memcpy", "memmove", "memset",
    "localtime", "mktime", "printf",
    "sigaddset", "sigemptyset", "sigismember", "sigpending", "sigprocmask",
    "snprintf", "sprintf", "sscanf", "strcat", "strcpy", "strdup", "stricmp",
    "strlwr", "strncat", "strncpy", "strnicmp", "strupr",
    "time",
    "vfprintf", "vsnprintf", "vsprintf",
}
ODSTR_COMPATIBILITY_APIS = {"stricmp", "strlwr", "strnicmp", "strupr"}
POSIX_HEADER_APIS = {
    "sigaddset", "sigemptyset", "sigismember", "sigpending", "sigprocmask",
}

HEADER_MOCK_DECLARATIONS = {
    "isalnum": "int utm_isalnum(int);",
    "isalpha": "int utm_isalpha(int);",
    "isblank": "int utm_isblank(int);",
    "iscntrl": "int utm_iscntrl(int);",
    "isdigit": "int utm_isdigit(int);",
    "isgraph": "int utm_isgraph(int);",
    "islower": "int utm_islower(int);",
    "isprint": "int utm_isprint(int);",
    "ispunct": "int utm_ispunct(int);",
    "isspace": "int utm_isspace(int);",
    "isupper": "int utm_isupper(int);",
    "isxdigit": "int utm_isxdigit(int);",
    "fprintf": "int utm_fprintf(FILE *, const char *, ...);",
    "memchr": "void *utm_memchr(const void *, int, size_t);",
    "memcmp": "int utm_memcmp(const void *, const void *, size_t);",
    "memcpy": "void *utm_memcpy(void *, const void *, size_t);",
    "memmove": "void *utm_memmove(void *, const void *, size_t);",
    "memset": "void *utm_memset(void *, int, size_t);",
    "localtime": "struct tm *utm_localtime(const time_t *);",
    "mktime": "time_t utm_mktime(struct tm *);",
    "printf": "int utm_printf(const char *, ...);",
    "sigaddset": "int utm_sigaddset(sigset_t *, int);",
    "sigemptyset": "int utm_sigemptyset(sigset_t *);",
    "sigismember": "int utm_sigismember(const sigset_t *, int);",
    "sigpending": "int utm_sigpending(sigset_t *);",
    "sigprocmask": "int utm_sigprocmask(int, const sigset_t *, sigset_t *);",
    "snprintf": "int utm_snprintf(char *, size_t, const char *, ...);",
    "sprintf": "int utm_sprintf(char *, const char *, ...);",
    "sscanf": "int utm_sscanf(const char *, const char *, ...);",
    "strcat": "char *utm_strcat(char *, const char *);",
    "strcpy": "char *utm_strcpy(char *, const char *);",
    "strdup": "char *utm_strdup(const char *);",
    "stricmp": "int utm_stricmp(const char *, const char *);",
    "strlwr": "char *utm_strlwr(char *);",
    "strncat": "char *utm_strncat(char *, const char *, size_t);",
    "strncpy": "char *utm_strncpy(char *, const char *, size_t);",
    "strnicmp": "int utm_strnicmp(const char *, const char *, size_t);",
    "strupr": "char *utm_strupr(char *);",
    "time": "time_t utm_time(time_t *);",
    "tolower": "int utm_tolower(int);",
    "toupper": "int utm_toupper(int);",
    "vfprintf": "int utm_vfprintf(FILE *, const char *, va_list);",
    "vsnprintf": "int utm_vsnprintf(char *, size_t, const char *, va_list);",
    "vsprintf": "int utm_vsprintf(char *, const char *, va_list);",
}


def manifest_source(source: Path) -> str:
    """Return a stable repository-relative source identity when possible."""
    try:
        return source.resolve().relative_to(ROOT.resolve()).as_posix()
    except ValueError:
        return source.name


def embedded_case(case: Path, stack: tuple[Path, ...] = ()) -> str:
    """Embed a case and its case-local quoted includes for DOS staging."""
    case = case.resolve()
    if case in stack:
        raise RuntimeError(f"recursive unit case include: {case}")
    result = []
    for line in case.read_text(encoding="latin-1").splitlines(keepends=True):
        match = LOCAL_CASE_INCLUDE.match(line)
        candidate = ((case.parent / match.group(1)).resolve()
                     if match else None)
        if (candidate is not None and candidate.is_file() and
                candidate.is_relative_to(CASE_ROOT)):
            result.append(embedded_case(candidate, stack + (case,)))
        else:
            result.append(line)
    return "".join(result)


def macro_name(name: str) -> str:
    return re.sub(r"[^A-Za-z0-9_]", "_", name)


def explicit_mock_names(case_text: str,
                        flags: list[str] | None = None) -> set[str]:
    """Return modern-header APIs a case explicitly requires intercepted."""
    flags = flags or []
    if any(flag.startswith(("-D__WATCOMC__", "-D__TURBOC__"))
           for flag in flags):
        return set()
    result = set(CUSTOM_MOCK.findall(case_text)) & HEADER_MACRO_APIS
    if any(flag.startswith("-D__unix__") for flag in flags):
        result -= ODSTR_COMPATIBILITY_APIS
    else:
        result -= POSIX_HEADER_APIS
    return result


def late_mock_names(names: set[str], flags: list[str]) -> set[str]:
    """Return dependencies whose aliases must follow modern CRT headers."""
    if any(flag.startswith(("-D__WATCOMC__", "-D__TURBOC__"))
           for flag in flags):
        return set()
    result = names & HEADER_MACRO_APIS
    if any(flag.startswith("-D__unix__") for flag in flags):
        result -= ODSTR_COMPATIBILITY_APIS
    else:
        result -= POSIX_HEADER_APIS
    return result


def generated_type_spelling(text: str) -> str:
    """Use source-visible names for Clang's internal Windows typedefs."""
    return re.sub(r"\bWINBOOL\b", "BOOL", text)


def early_mock_declarations(names: set[str]) -> list[str]:
    """Declare CRT errno accessors before later runtime headers use them."""
    declarations = []
    if "__error" in names:
        declarations.extend(("int *__error(void);",
                             "int *utm___error(void);"))
    if "__errno_location" in names:
        declarations.extend(("int *__errno_location(void);",
                             "int *utm___errno_location(void);"))
    if "__get_errno_ptr" in names:
        declarations.extend(("int *__get_errno_ptr(void);",
                             "int *utm___get_errno_ptr(void);"))
    if "_errno" in names:
        declarations.extend(("int *_errno(void);",
                             "int *utm__errno(void);"))
    return declarations


def mock_declarations(names: set[str]) -> list[str]:
    """Declare deferred CRT mocks after the source's platform headers."""
    return [HEADER_MOCK_DECLARATIONS[name] for name in sorted(names)]


def early_alias_macros(macros: list[tuple[str, str]],
                       mock_names: set[str]) -> list[tuple[str, str]]:
    """Keep dependency aliases out of platform-header declarations."""
    return [macro for macro in macros if macro[0] not in mock_names]


def blank_body(text: str) -> str:
    return ";" + "".join("\n" if char == "\n" else " " for char in text[1:])


def insert_late_defines(text: str, line: int,
                        definitions: list[str],
                        undefinitions: list[str] | None = None,
                        declarations: list[str] | None = None) -> str:
    """Reassert command-line definitions after source headers redefine them."""
    if not definitions and not undefinitions and not declarations:
        return text
    lines = []
    for name in undefinitions or []:
        lines.append(f"#undef {name}")
    for definition in definitions:
        name, separator, value = definition.partition("=")
        lines.extend((f"#undef {name}",
                      f"#define {name} {value if separator else '1'}"))
    lines.extend(declarations or [])
    lines.append(f"#line {line}")
    offset = 0
    for unused in range(1, line):
        offset = text.index("\n", offset) + 1
    return text[:offset] + "\n".join(lines) + "\n" + text[offset:]


def mock_definitions(target_name: str, mocks: dict[str, object]) -> list[str]:
    """Return aliases that must survive headers which undefine CRT macros."""
    definitions = [f"{target_name}=utt_{macro_name(target_name)}"]
    definitions.extend(
        f"{name}=utm_{macro_name(name)}" for name in sorted(mocks))
    return definitions


def instrument(text: str, decisions, switches, state_points) -> tuple[str, list[dict[str, object]], list[dict[str, object]]]:
    starts: dict[int, list[tuple[int, int, str]]] = {}
    ends: dict[int, list[tuple[int, int, str]]] = {}
    metadata = []
    seen = set()
    decision_id = 0
    for decision in decisions:
        key = (decision.start, decision.end)
        if key in seen:
            continue
        seen.add(key)
        decision_id += 1
        count = len(decision.conditions)
        starts.setdefault(decision.start, []).append((
            decision.end, 0,
            f"(ut_cov_begin({decision_id}), ut_cov_result({decision_id}, {count}, ("))
        ends.setdefault(decision.end, []).append((decision.start, 1, ")))"))
        for condition_id, (start, end) in enumerate(decision.conditions):
            starts.setdefault(start, []).append((
                end, 1,
                f"ut_cov_condition({decision_id}, {condition_id}, !!("))
            ends.setdefault(end, []).append((start, 0, "))"))
        metadata.append({
            "id": decision_id,
            "kind": decision.kind,
            "line": text.count("\n", 0, decision.start) + 1,
            "conditions": count,
        })
    branch_metadata = []
    branch_id = 0
    for switch_id, switch in enumerate(switches, 1):
        case_calls = []
        default_id = 0
        for branch in switch.branches:
            branch_id += 1
            branch_metadata.append({
                "id": branch_id,
                "kind": branch.kind,
                "line": branch.line,
                "switch": switch_id,
            })
            if branch.kind == "DefaultStmt":
                default_id = branch_id
            else:
                value = text[branch.value_start:branch.value_end]
                case_calls.append(
                    f", ut_cov_switch_case_value({switch_id}, {branch_id}, "
                    f"(unsigned long)({value}))")
        if default_id == 0:
            branch_id += 1
            default_id = branch_id
            branch_metadata.append({
                "id": branch_id,
                "kind": "ImplicitDefault",
                "line": switch.line,
                "switch": switch_id,
            })
        starts.setdefault(switch.expression_start, []).append((
            switch.expression_end, -1,
            f"(ut_cov_switch_begin({switch_id}, (unsigned long)("))
        ends.setdefault(switch.expression_end, []).append((
            switch.expression_start, 2,
            "))" + "".join(case_calls) +
            f", ({switch.expression_type})ut_cov_switch_result("
            f"{switch_id}, {default_id}))"))

    insertions = {}
    for position, values in starts.items():
        insertions[position] = insertions.get(position, "") + "".join(
            value[2] for value in sorted(values, key=lambda value: (-value[0], value[1])))
    for position, values in ends.items():
        ending = "".join(
            value[2] for value in sorted(values, key=lambda value: (-value[0], value[1])))
        insertions[position] = ending + insertions.get(position, "")
    for point in state_points:
        registrations = "".join(
            f" ut_state_register((void *)&{name}, sizeof({name}));"
            for name in point.names)
        insertions[point.position] = (insertions.get(point.position, "") +
                                      registrations)
    result = text
    for position in sorted(insertions, reverse=True):
        result = result[:position] + insertions[position] + result[position:]
    return result, metadata, branch_metadata


def variable_definition(item: Variable) -> str:
    if item.calling_macro and "(*)" in item.type:
        return generated_type_spelling(item.type).replace(
            "(*)", f"({item.calling_macro} *{item.name})", 1) + ";"
    return insert_name(generated_type_spelling(item.type), item.name) + ";"


def variable_definitions(variables: list[Variable],
                         provided_variables: set[str]) -> list[str]:
    """Define external objects except those supplied by the runtime."""
    return [variable_definition(item) for item in variables
            if item.name not in provided_variables]


def default_mock(item: Callable, mock_id: int) -> str:
    name = "utm_" + macro_name(item.name)
    result_type = generated_type_spelling(item.result)
    lines = [generated_type_spelling(
        declaration(item, name, include_storage=True)), "{"]
    if item.result != "void":
        lines.extend([
            f"   {result_type} ut_result;",
            "   unsigned char *ut_result_bytes = (unsigned char *)&ut_result;",
            "   size_t ut_result_index;",
        ])
    for parameter in item.parameters:
        lines.append(f"   (void){parameter.name};")
    lines.append(f"   ut_unexpected_mock({mock_id}, \"{item.name}\");")
    if item.result != "void":
        lines.extend([
            "   for(ut_result_index = 0; ut_result_index < sizeof(ut_result);",
            "      ++ut_result_index)",
            "      ut_result_bytes[ut_result_index] = 0;",
            "   return ut_result;",
        ])
    lines.append("}")
    return "\n".join(lines)


def generate(source: Path, target_name: str, case: Path, output: Path,
             clang: str, flags: list[str],
             late_defines: list[str] | None = None,
             late_undefines: list[str] | None = None,
             late_declarations: list[str] | None = None,
             provided_variables: set[str] | None = None,
             uninstrumented_output: Path | None = None) -> None:
    source = source.resolve()
    text = source.read_text(encoding="latin-1")
    model = build_model(source, clang, flags, target_name=target_name)
    targets = [item for item in model if item.name == target_name]
    if len(targets) != 1:
        raise RuntimeError(
            f"{source.name}:{target_name} has {len(targets)} active definitions")
    target = targets[0]
    mocks = {}
    for item in target.functions:
        mocks[item.name] = item
    case_text = embedded_case(case).rstrip("\r\n")
    explicit_mocks = explicit_mock_names(case_text, flags)
    alias_names = set(mocks) | explicit_mocks
    deferred_alias_names = late_mock_names(alias_names, flags)
    alias_definitions = mock_definitions(target_name, alias_names)

    replacements = []
    for item in scan_c(text):
        if item.name != target_name:
            replacements.append((item.body_offset, item.end_offset,
                                 blank_body(text[item.body_offset:item.end_offset])))
    transformed = text
    for start, end, replacement in sorted(replacements, reverse=True):
        transformed = transformed[:start] + replacement + transformed[end:]
    uninstrumented = transformed
    transformed, decision_metadata, branch_metadata = instrument(
        transformed, target.decisions, target.switches, target.state_points)
    uninstrumented, _, _ = instrument(
        uninstrumented, [], [], target.state_points)

    def finalize_body(value: str) -> str:
        value = replace_c_includes(value)
        return insert_late_defines(
            value, target.signature_line,
            (late_defines or []) + alias_definitions,
            late_undefines or [],
            (late_declarations or []) +
            mock_declarations(deferred_alias_names))

    transformed = finalize_body(transformed)
    uninstrumented = finalize_body(uninstrumented)

    macros = [tuple(definition.split("=", 1))
              for definition in alias_definitions]
    lines = [
        "/* Generated file: do not edit. */",
        '#include "ut.h"',
        "",
    ]
    lines.extend(early_mock_declarations(alias_names))
    if early_mock_declarations(alias_names):
        lines.append("")
    for original, replacement in early_alias_macros(
            macros, deferred_alias_names):
        lines.append(f"#define {original} {replacement}")
    lines.append(f'#line 1 "{manifest_source(source)}"')
    body_index = len(lines)
    lines.append(transformed)
    for original, unused in macros:
        lines.append(f"#undef {original}")
    lines.extend(["", "/* Controllable external object dependencies. */"])
    lines.extend(variable_definitions(
        target.variables, provided_variables or set()))
    lines.extend([
        "",
        "static void ut_register_file_state(void)",
        "{",
    ])
    for variable in target.state_variables:
        lines.append(
            f"   ut_state_register((void *)&{variable.name}, sizeof({variable.name}));")
    lines.extend([
        "}",
        "",
        "/* Embedded unit case and case-local support. */",
        case_text,
        "",
        "/* Default mocks fail immediately; a case may provide an override. */",
    ])
    for mock_id, item in enumerate(sorted(mocks.values(), key=lambda value: value.name), 1):
        guard = "UT_CUSTOM_MOCK_" + macro_name(item.name)
        lines.extend([f"#ifndef {guard}", default_mock(item, mock_id), "#endif", ""])
    lines.extend([
        "int main(int argc, char **argv)",
        "{",
        "   const char *report = argc > 1 ? argv[1] : NULL;",
        "   ut_register_file_state();",
        "   return ut_run(ut_cases,",
        "      (unsigned short)(sizeof(ut_cases) / sizeof(ut_cases[0])),",
        "      report);",
        "}",
    ])
    metadata = json.dumps({
        "version": 1,
        "source": manifest_source(source),
        "function": target_name,
        "decisions": decision_metadata,
        "branches": branch_metadata,
    }, indent=2) + "\n"

    def write_unit(destination: Path, body: str) -> None:
        rendered = list(lines)
        rendered[body_index] = body
        destination.parent.mkdir(parents=True, exist_ok=True)
        destination.write_text("\n".join(rendered) + "\n", encoding="latin-1")
        destination.with_suffix(".model.json").write_text(
            metadata, encoding="utf-8")

    write_unit(output, transformed)
    if uninstrumented_output is not None:
        write_unit(uninstrumented_output, uninstrumented)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("source", type=Path)
    parser.add_argument("function")
    parser.add_argument("case", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--clang", default="clang")
    parser.add_argument("--flag", action="append", default=[])
    parser.add_argument("--provided-variable", action="append", default=[])
    parser.add_argument("--uninstrumented-output", type=Path)
    args = parser.parse_args()
    flags = args.flag or ["-std=c89", "-D__unix__", POSIX_API_FLAG,
                          "-DHAS_INTTYPES_H",
                          "-DOPENDOORS_HAVE_VSNPRINTF=1"]
    generate(args.source, args.function, args.case, args.output,
             args.clang, flags, provided_variables=set(args.provided_variable),
             uninstrumented_output=args.uninstrumented_output)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
