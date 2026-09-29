#!/usr/bin/env python3

import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from asm_generate import (_dependency_defines, instrument_procedure,
                          render_assembly)  # noqa: E402


class AssemblyInstrumentationTests(unittest.TestCase):
    def test_dependency_indices_have_stable_case_macros(self):
        self.assertEqual(
            _dependency_defines(["__xspawn", "setvectsub"]),
            "#define UT_ASM_DEPENDENCY_XSPAWN 0U\n"
            "#define UT_ASM_DEPENDENCY_SETVECTSUB 1U\n")

    def test_records_both_conditional_jump_outcomes(self):
        source = """\
target PROC near
        cmp ax,bx
        je equal
        mov ax,1
equal:  ret
target ENDP
"""
        result, branches = instrument_procedure(source, "target")
        self.assertEqual(branches, [{"id": 1, "line": 3,
                                     "instruction": "je"}])
        self.assertIn("jne UTB0001N", result)
        self.assertIn("UTCOV 1, 1", result)
        self.assertIn("jmp     equal", result)
        self.assertIn("UTB0001N:", result)
        self.assertIn("UTCOV 1, 0", result)

    def test_records_loop_outcomes_without_inverting_the_instruction(self):
        source = """\
target PROC near
again: loop again
        ret
target ENDP
"""
        result, branches = instrument_procedure(source, "target")
        self.assertEqual(branches[0]["instruction"], "loop")
        self.assertIn("loop UTB0001T", result)
        self.assertIn("UTCOV 1, 0", result)
        self.assertIn("UTB0001T:", result)
        self.assertIn("UTCOV 1, 1", result)
        self.assertIn("jmp     again", result)

    def test_widens_explicit_short_jumps_in_the_instrumented_procedure(self):
        source = """\
target PROC near
        jmp short done
done:   ret
target ENDP
"""
        result, unused = instrument_procedure(source, "target")
        self.assertIn("jmp     done", result)
        self.assertNotIn("jmp short done", result)

    def test_does_not_count_branches_in_a_constant_false_assembler_if(self):
        source = """\
FEATURE EQU 20
target PROC near
IF FEATURE - 20
        je unavailable
ENDIF
        jne available
available:
unavailable:
        ret
target ENDP
"""
        result, branches = instrument_procedure(source, "target")
        self.assertEqual(len(branches), 1)
        self.assertEqual(branches[0]["line"], 6)
        self.assertIn("        je unavailable", result)
        self.assertIn("UTCOV 1, 1", result)

    def test_rewrites_interrupts_and_procedure_dependencies_only_in_target(self):
        source = """\
first PROC near
        int 21h
        call second
        ret
first ENDP
second PROC near
        int 67h
        ret
second ENDP
"""
        result, unused = instrument_procedure(source, "first")
        self.assertIn("call    ut_int21", result)
        self.assertIn("call    utm_second", result)
        self.assertIn("second PROC near\n        int 67h", result)

    def test_rewrites_interrupts_in_an_explicit_shared_label_range(self):
        source = """\
shared_start:
        int 21h
        jne shared_end
shared_end:
        ret
target PROC near
        ret
target ENDP
"""
        result, branches = instrument_procedure(
            source, "target", external_ranges=[("shared_start", "shared_end")])
        self.assertIn("shared_start:\n        call    ut_int21", result)
        self.assertIn("jne shared_end", result)
        self.assertEqual(branches, [])

    def test_replaces_the_relocated_kernel_jump_when_requested(self):
        source = """\
_TEXT SEGMENT
target PROC near
        jmp dword ptr cs:[s1add]
resume: ret
target ENDP
_TEXT ENDS
"""
        result, unused = instrument_procedure(
            source, "target", mock_kernel_jump=True)
        self.assertIn("call    utm_kernel", result)
        self.assertIn("jmp     fnish1", result)
        self.assertNotIn("jmp dword ptr cs:[s1add]", result)

    def test_dependency_stub_dispatches_a_complete_register_frame(self):
        source = """\
_TEXT SEGMENT
target PROC near
        call dependency
        ret
target ENDP
dependency PROC near
        ret
dependency ENDP
_TEXT ENDS
"""
        result, unused = render_assembly(source, "target")
        stub = result.split("utm_dependency", 1)[1]
        self.assertIn("call    _ut_asm_dependency", stub)
        self.assertIn("pushf", stub)
        self.assertIn("push    es", stub)
        self.assertIn("push    ax", stub)
        self.assertIn("pop     ax", stub)
        self.assertIn("popf", stub)

    def test_support_data_offsets_are_relative_to_dgroup(self):
        source = """\
_TEXT SEGMENT
target PROC near
        call dependency
        je done
done:   ret
target ENDP
dependency PROC near
        ret
dependency ENDP
_TEXT ENDS
"""
        result, unused = render_assembly(source, "target")
        self.assertIn("ASSUME  es:DGROUP", result)
        self.assertIn("es:[_ut_asm_coverage", result)
        self.assertIn("es:[_ut_asm_mock_calls", result)
        self.assertIn("es:[_ut_asm_target_registers", result)
        self.assertIn("ASSUME  es:NOTHING", result)
        self.assertNotIn("es:[DGROUP:_ut_asm_", result)

    def test_target_wrapper_preserves_c_callee_saved_registers(self):
        source = """\
_TEXT SEGMENT
target PROC near
        ret
target ENDP
_TEXT ENDS
"""
        result, unused = render_assembly(source, "target")
        wrapper = result.split("_ut_asm_call_target PROC far", 1)[1]
        wrapper = wrapper.split("_ut_asm_call_target ENDP", 1)[0]
        self.assertIn(
            "push    bp\n"
            "                push    bx\n"
            "                push    si\n"
            "                push    di\n"
            "                push    ds\n"
            "                push    es", wrapper)
        self.assertIn(
            "pop     es\n"
            "                pop     ds\n"
            "                pop     di\n"
            "                pop     si\n"
            "                pop     bx\n"
            "                pop     bp\n"
            "                retf", wrapper)

    def test_generates_requested_code_word_accessors(self):
        source = """\
_TEXT SEGMENT
state_word dw 0
target PROC near
        ret
target ENDP
_TEXT ENDS
"""
        result, unused = render_assembly(
            source, "target", code_words=["state_word"])
        self.assertIn("PUBLIC  _ut_asm_set_word_state_word", result)
        self.assertIn("mov     cs:[state_word],ax", result)
        self.assertIn("PUBLIC  _ut_asm_get_word_state_word", result)
        self.assertIn("mov     ax,cs:[state_word]", result)


if __name__ == "__main__":
    unittest.main()
