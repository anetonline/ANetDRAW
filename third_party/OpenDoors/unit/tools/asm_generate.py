#!/usr/bin/env python3
"""Generate isolated, instrumented 16-bit assembly unit-test modules."""

from __future__ import annotations

import argparse
import json
import re
from pathlib import Path

from generate import embedded_case, manifest_source
from inventory import scan_asm


CONDITIONAL_INVERSE = {
    "ja": "jbe", "jae": "jb", "jb": "jae", "jbe": "ja",
    "jc": "jnc", "jcxz": None, "je": "jne", "jg": "jle",
    "jge": "jl", "jl": "jge", "jle": "jg", "jna": "ja",
    "jnae": "jae", "jnb": "jb", "jnbe": "jbe", "jnc": "jc",
    "jne": "je", "jng": "jg", "jnge": "jge", "jnl": "jl",
    "jnle": "jle", "jno": "jo", "jnp": "jp", "jns": "js",
    "jnz": "jz", "jo": "jno", "jp": "jnp", "jpe": "jpo",
    "jpo": "jpe", "js": "jns", "jz": "jnz",
}
LOOP_INSTRUCTIONS = {"loop", "loope", "loopne", "loopnz", "loopz"}
LINE = re.compile(
    r"^(?P<prefix>\s*(?:[A-Za-z_][A-Za-z0-9_@$?]*:\s*)?)"
    r"(?P<instruction>[A-Za-z]+)\s+"
    r"(?P<operand>[^;\s]+)(?P<suffix>\s*(?:;.*)?)$",
    re.IGNORECASE)
SHORT_JUMP = re.compile(
    r"^(?P<prefix>\s*(?:[A-Za-z_][A-Za-z0-9_@$?]*:\s*)?)"
    r"jmp\s+short\s+(?P<operand>[^;\s]+)"
    r"(?P<suffix>\s*(?:;.*)?)$", re.IGNORECASE)


def _procedure_lines(text: str, target: str) -> tuple[int, int]:
    functions = [item for item in scan_asm(text)
                 if item.name.lower() == target.lower()]
    if len(functions) != 1:
        raise RuntimeError(
            f"assembly target {target} has {len(functions)} procedures")
    function = functions[0]
    return function.signature_line, function.end_line


def _coverage_lines(branch: int, outcome: int,
                    indent: str = "        ") -> list[str]:
    return [f"{indent}UTCOV {branch}, {outcome}"]


def _constant_false_lines(lines: list[str]) -> set[int]:
    """Find lines disabled by numeric MASM IF expressions we can prove."""
    constants = {}
    for line in lines:
        match = re.match(
            r"^\s*([A-Za-z_][A-Za-z0-9_@$?]*)\s+EQU\s+"
            r"([0-9]+|[0-9A-F]+h)\s*(?:;.*)?$", line, re.IGNORECASE)
        if match is not None:
            value = match.group(2)
            constants[match.group(1).lower()] = int(
                value[:-1], 16) if value.lower().endswith("h") else int(value)
    disabled = set()
    states = []
    for line_number, line in enumerate(lines, 1):
        if any(state is False for state in states):
            disabled.add(line_number)
        conditional = re.match(
            r"^\s*IF\s+([A-Za-z_][A-Za-z0-9_@$?]*)\s*-\s*"
            r"([0-9]+|[0-9A-F]+h)\s*(?:;.*)?$", line, re.IGNORECASE)
        if conditional is not None:
            name = conditional.group(1).lower()
            value_text = conditional.group(2)
            value = (int(value_text[:-1], 16)
                     if value_text.lower().endswith("h") else int(value_text))
            states.append(None if name not in constants
                          else constants[name] - value != 0)
        elif re.match(r"^\s*ELSE\b", line, re.IGNORECASE) and states:
            if states[-1] is not None:
                states[-1] = not states[-1]
        elif re.match(r"^\s*ENDIF\b", line, re.IGNORECASE) and states:
            states.pop()
    return disabled


def instrument_procedure(text: str, target: str,
                         external_ranges: list[tuple[str, str]] | None = None,
                         mock_kernel_jump: bool = False
                         ) -> tuple[str, list[dict[str, object]]]:
    """Instrument one procedure and redirect its external effects to mocks."""
    start, end = _procedure_lines(text, target)
    procedures = {item.name.lower(): item.name for item in scan_asm(text)}
    procedures.pop(target.lower(), None)
    lines = text.splitlines()
    constant_false_lines = _constant_false_lines(lines)
    label_lines = {}
    for line_number, line in enumerate(lines, 1):
        label = re.match(r"^([A-Za-z_][A-Za-z0-9_@$?]*):", line)
        if label is not None:
            label_lines[label.group(1).lower()] = line_number
    external_spans = []
    for first, last in external_ranges or []:
        try:
            first_line = label_lines[first.lower()]
            last_line = label_lines[last.lower()]
        except KeyError as error:
            raise RuntimeError(
                f"assembly shared label not found: {error.args[0]}") from error
        if first_line > last_line:
            raise RuntimeError(
                f"assembly shared range is reversed: {first} {last}")
        external_spans.append((first_line, last_line))
    output = []
    branches = []
    branch_id = 0
    for line_number, line in enumerate(lines, 1):
        in_target = start < line_number < end
        in_external = any(first <= line_number <= last
                          for first, last in external_spans)
        if not in_target and not in_external:
            output.append(line)
            continue
        if line_number in constant_false_lines:
            output.append(line)
            continue
        if (in_target and mock_kernel_jump and re.match(
                r"^\s*jmp\s+dword\s+ptr\s+cs:\[s1add\]", line,
                re.IGNORECASE)):
            indent = re.match(r"^\s*", line).group(0)
            output.append(f"{indent}call    utm_kernel")
            output.append(f"{indent}jmp     fnish1")
            continue
        short_jump = SHORT_JUMP.match(line)
        if short_jump is not None:
            output.append(
                f"{short_jump.group('prefix')}jmp     "
                f"{short_jump.group('operand')}{short_jump.group('suffix')}")
            continue
        match = LINE.match(line)
        if match is None:
            output.append(line)
            continue
        instruction = match.group("instruction").lower()
        operand = match.group("operand")
        prefix = match.group("prefix")
        indent_match = re.match(r"^\s*", prefix)
        indent = indent_match.group(0) if indent_match else "        "
        if not indent:
            indent = "        "
        if instruction == "int" and operand.lower() in {"21h", "67h"}:
            output.append(
                f"{prefix}call    ut_int{operand[:-1]}{match.group('suffix')}")
            continue
        if in_external:
            output.append(line)
            continue
        if instruction == "call" and operand.lower() in procedures:
            output.append(
                f"{prefix}call    utm_{procedures[operand.lower()]}"
                f"{match.group('suffix')}")
            continue
        if instruction not in CONDITIONAL_INVERSE and instruction not in LOOP_INSTRUCTIONS:
            output.append(line)
            continue
        branch_id += 1
        branch = {"id": branch_id, "line": line_number,
                  "instruction": instruction}
        branches.append(branch)
        not_taken = f"UTB{branch_id:04d}N"
        taken = f"UTB{branch_id:04d}T"
        after = f"UTB{branch_id:04d}A"
        if instruction in LOOP_INSTRUCTIONS or instruction == "jcxz":
            output.append(f"{prefix}{instruction} {taken}{match.group('suffix')}")
            output.extend(_coverage_lines(branch_id, 0, indent))
            output.append(f"{indent}jmp     {after}")
            output.append(f"{taken}:")
            output.extend(_coverage_lines(branch_id, 1, indent))
            output.append(f"{indent}jmp     {operand}")
            output.append(f"{after}:")
        else:
            inverse = CONDITIONAL_INVERSE[instruction]
            output.append(f"{prefix}{inverse} {not_taken}{match.group('suffix')}")
            output.extend(_coverage_lines(branch_id, 1, indent))
            output.append(f"{indent}jmp     {operand}")
            output.append(f"{not_taken}:")
            output.extend(_coverage_lines(branch_id, 0, indent))
    trailing_newline = "\n" if text.endswith("\n") else ""
    return "\n".join(output) + trailing_newline, branches


def _assembly_support(branch_count: int, procedure_count: int) -> str:
    coverage_size = max(branch_count * 2, 1)
    mock_size = max(procedure_count * 2, 2)
    return f"""
EXTRN _ut_asm_interrupt:FAR
EXTRN _ut_asm_dependency:FAR
EXTRN _ut_asm_data_anchor:BYTE

_DATA           SEGMENT word public 'DATA'
                PUBLIC  _ut_asm_coverage
                PUBLIC  _ut_asm_mock_calls
                PUBLIC  _ut_asm_mock_ax
                PUBLIC  _ut_asm_target_registers
_ut_asm_coverage db     {coverage_size} dup (0)
_ut_asm_mock_calls dw   {mock_size // 2} dup (0)
_ut_asm_mock_ax  dw     {mock_size // 2} dup (0)
_ut_asm_target_registers dw 10 dup (0)
_DATA           ENDS
DGROUP          GROUP   _DATA

UTCOV           MACRO   branch_id, outcome
                pushf
                push    es
                push    ax
                mov     ax,SEG _ut_asm_coverage
                mov     es,ax
                ASSUME  es:DGROUP
                mov     byte ptr es:[_ut_asm_coverage + ((branch_id-1)*2) + outcome],1
                ASSUME  es:NOTHING
                pop     ax
                pop     es
                popf
                ENDM
"""


def _interrupt_wrapper(number: int) -> str:
    return f"""
ut_int{number}    PROC    near
                pushf
                push    es
                push    ds
                push    di
                push    si
                push    bp
                push    bx
                push    dx
                push    cx
                push    ax
                mov     bp,sp
                mov     ax,SEG _ut_asm_data_anchor
                mov     ds,ax
                push    ss
                push    bp
                mov     ax,{number}h
                push    ax
                call    _ut_asm_interrupt
                add     sp,6
                pop     ax
                pop     cx
                pop     dx
                pop     bx
                pop     bp
                pop     si
                pop     di
                pop     ds
                pop     es
                popf
                ret
ut_int{number}    ENDP
"""


def _mock_stub(name: str, index: int) -> str:
    offset = index * 2
    return f"""
utm_{name}       PROC    near
                pushf
                push    es
                push    ds
                push    di
                push    si
                push    bp
                push    bx
                push    dx
                push    cx
                push    ax
                mov     bp,sp
                mov     ax,SEG _ut_asm_mock_calls
                mov     es,ax
                mov     bx,{offset}
                ASSUME  es:DGROUP
                inc     word ptr es:[_ut_asm_mock_calls+bx]
                ASSUME  es:NOTHING
                mov     ax,SEG _ut_asm_data_anchor
                mov     ds,ax
                push    ss
                push    bp
                mov     ax,{index}
                push    ax
                call    _ut_asm_dependency
                add     sp,6
                pop     ax
                pop     cx
                pop     dx
                pop     bx
                pop     bp
                pop     si
                pop     di
                pop     ds
                pop     es
                popf
                ret
utm_{name}       ENDP
"""


def _target_wrapper(target: str) -> str:
    return f"""
                PUBLIC  _ut_asm_call_target
_ut_asm_call_target PROC far
                push    bp
                push    bx
                push    si
                push    di
                push    ds
                push    es
                mov     ax,SEG _ut_asm_target_registers
                mov     es,ax
                ASSUME  es:DGROUP
                mov     ax,es:[_ut_asm_target_registers+0]
                mov     cx,es:[_ut_asm_target_registers+2]
                mov     dx,es:[_ut_asm_target_registers+4]
                mov     bx,es:[_ut_asm_target_registers+6]
                mov     bp,es:[_ut_asm_target_registers+8]
                mov     si,es:[_ut_asm_target_registers+10]
                mov     di,es:[_ut_asm_target_registers+12]
                mov     ds,es:[_ut_asm_target_registers+14]
                mov     es,es:[_ut_asm_target_registers+16]
                ASSUME  es:NOTHING
                call    {target}
                pushf
                push    es
                push    ds
                push    di
                push    si
                push    bp
                push    bx
                push    dx
                push    cx
                push    ax
                mov     bp,sp
                mov     ax,SEG _ut_asm_target_registers
                mov     es,ax
                ASSUME  es:DGROUP
                mov     ax,ss:[bp+0]
                mov     es:[_ut_asm_target_registers+0],ax
                mov     ax,ss:[bp+2]
                mov     es:[_ut_asm_target_registers+2],ax
                mov     ax,ss:[bp+4]
                mov     es:[_ut_asm_target_registers+4],ax
                mov     ax,ss:[bp+6]
                mov     es:[_ut_asm_target_registers+6],ax
                mov     ax,ss:[bp+8]
                mov     es:[_ut_asm_target_registers+8],ax
                mov     ax,ss:[bp+10]
                mov     es:[_ut_asm_target_registers+10],ax
                mov     ax,ss:[bp+12]
                mov     es:[_ut_asm_target_registers+12],ax
                mov     ax,ss:[bp+14]
                mov     es:[_ut_asm_target_registers+14],ax
                mov     ax,ss:[bp+16]
                mov     es:[_ut_asm_target_registers+16],ax
                mov     ax,ss:[bp+18]
                mov     es:[_ut_asm_target_registers+18],ax
                ASSUME  es:NOTHING
                add     sp,20
                pop     es
                pop     ds
                pop     di
                pop     si
                pop     bx
                pop     bp
                retf
_ut_asm_call_target ENDP
"""


def _code_word_accessors(name: str) -> str:
    return f"""
                PUBLIC  _ut_asm_set_word_{name}
_ut_asm_set_word_{name} PROC far
                push    bp
                mov     bp,sp
                mov     ax,[bp+ARG_1]
                mov     cs:[{name}],ax
                pop     bp
                retf
_ut_asm_set_word_{name} ENDP

                PUBLIC  _ut_asm_get_word_{name}
_ut_asm_get_word_{name} PROC far
                mov     ax,cs:[{name}]
                retf
_ut_asm_get_word_{name} ENDP
"""


def render_assembly(text: str, target: str,
                    code_words: list[str] | None = None,
                    external_ranges: list[tuple[str, str]] | None = None,
                    mock_kernel_jump: bool = False
                    ) -> tuple[str, dict[str, object]]:
    transformed, branches = instrument_procedure(
        text, target, external_ranges=external_ranges,
        mock_kernel_jump=mock_kernel_jump)
    code_words = code_words or []
    for name in code_words:
        if re.search(rf"^\s*{re.escape(name)}\s+dw\b", text,
                     re.MULTILINE | re.IGNORECASE) is None:
            raise RuntimeError(f"assembly code word not found: {name}")
    procedures = [item.name for item in scan_asm(text)]
    dependencies = []
    for name in procedures:
        if re.search(rf"\bcall\s+utm_{re.escape(name)}\b", transformed,
                     re.IGNORECASE):
            dependencies.append(name)
    support = _assembly_support(len(branches), len(procedures))
    segment = re.search(r"^_TEXT\s+SEGMENT\b", transformed,
                        re.MULTILINE | re.IGNORECASE)
    ending = re.search(r"^_TEXT\s+ENDS\b", transformed,
                       re.MULTILINE | re.IGNORECASE)
    if segment is None or ending is None:
        raise RuntimeError("assembly source lacks _TEXT segment")
    transformed = transformed[:segment.start()] + support + transformed[segment.start():]
    ending = re.search(r"^_TEXT\s+ENDS\b", transformed,
                       re.MULTILINE | re.IGNORECASE)
    helpers = (_interrupt_wrapper(21) + _interrupt_wrapper(67) +
               _target_wrapper(target))
    for name in code_words:
        helpers += _code_word_accessors(name)
    for name in dependencies:
        helpers += _mock_stub(name, procedures.index(name))
    if mock_kernel_jump:
        helpers += _mock_stub("kernel", procedures.index(target))
    transformed = transformed[:ending.start()] + helpers + transformed[ending.start():]
    return transformed, {
        "version": 1,
        "source": "",
        "function": target,
        "branches": [],
        "decisions": [],
        "assembly_branches": branches,
        "assembly_dependencies": dependencies +
            (["kernel"] if mock_kernel_jump else []),
        "assembly_procedures": procedures,
    }


def _dependency_defines(procedures: list[str]) -> str:
    lines = []
    for index, name in enumerate(procedures):
        macro = re.sub(r"[^A-Za-z0-9]+", "_", name).strip("_").upper()
        lines.append(f"#define UT_ASM_DEPENDENCY_{macro} {index}U")
    return "\n".join(lines) + "\n"


def render_case(case: Path, branch_count: int,
                procedures: list[str]) -> str:
    return """/* Generated file: do not edit. */
#include "ut.h"
#include "utasm.h"
#include "OpenDoor.h"
#include "ODSwap.h"

""" + _dependency_defines(procedures) + """
unsigned char ut_asm_data_anchor;
extern unsigned char ut_asm_coverage[];
extern unsigned short ut_asm_mock_calls[];
extern unsigned short ut_asm_mock_ax[];
extern UTAsmRegisters ut_asm_target_registers;
void ODSWAPCALL ut_asm_call_target(void);

""" + embedded_case(case) + """

#ifndef UT_CUSTOM_ASM_INTERRUPT
void UTASMCALL ut_asm_interrupt(unsigned interrupt_number,
   UTAsmRegisters far *registers)
{
   (void)interrupt_number;
   (void)registers;
   ut_unexpected_mock(1, "software interrupt");
}
#endif

#ifndef UT_CUSTOM_ASM_DEPENDENCY
void UTASMCALL ut_asm_dependency(unsigned dependency_index,
   UTAsmRegisters far *registers)
{
   (void)dependency_index;
   (void)registers;
   ut_unexpected_mock(1, "assembly dependency");
}
#endif

""" + "int main(int argc, char **argv)\n{\n" + \
        "   ut_cov_asm_register(ut_asm_coverage, " + \
        str(branch_count) + ");\n" + """\
   return ut_run(ut_cases, (unsigned short)(sizeof(ut_cases) /
      sizeof(ut_cases[0])), argc > 1 ? argv[1] : NULL);
}
"""


def generate(source: Path, target: str, case: Path, c_output: Path,
             asm_output: Path) -> None:
    text = source.read_text(encoding="latin-1")
    case_text = case.read_text(encoding="latin-1")
    code_words = sorted(set(re.findall(
        r"\but_asm_(?:set|get)_word_([A-Za-z_][A-Za-z0-9_]*)\b",
        case_text)))
    external_ranges = re.findall(
        r"UT_ASM_EXTERNAL_RANGE\s+([A-Za-z_][A-Za-z0-9_]*)\s+"
        r"([A-Za-z_][A-Za-z0-9_]*)", case_text)
    mock_kernel_jump = "UT_ASM_MOCK_KERNEL_JUMP" in case_text
    assembly, model = render_assembly(
        text, target, code_words, external_ranges, mock_kernel_jump)
    model["source"] = manifest_source(source)
    c_output.write_text(render_case(
        case, len(model["assembly_branches"]),
        model["assembly_procedures"]), encoding="latin-1")
    asm_output.write_text(assembly, encoding="latin-1")
    c_output.with_suffix(".model.json").write_text(
        json.dumps(model, indent=2) + "\n", encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("source", type=Path)
    parser.add_argument("function")
    parser.add_argument("case", type=Path)
    parser.add_argument("c_output", type=Path)
    parser.add_argument("asm_output", type=Path)
    args = parser.parse_args()
    generate(args.source, args.function, args.case,
             args.c_output, args.asm_output)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
