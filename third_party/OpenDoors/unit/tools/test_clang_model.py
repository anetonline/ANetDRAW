#!/usr/bin/env python3

import sys
import tempfile
import unittest
from pathlib import Path
from unittest import mock

sys.path.insert(0, str(Path(__file__).resolve().parent))
import clang_model  # noqa: E402
from clang_model import (Callable, Parameter, build_model, callable_from, declaration,
                         is_language_builtin, mask_turbo_asm_jumps,
                         switch_value_range)  # noqa: E402
from inventory import ROOT  # noqa: E402


class ClangModelTests(unittest.TestCase):
    def test_ast_input_preserves_source_bytes_across_platforms(self):
        completed = mock.Mock(returncode=0, stdout=b"{}", stderr=b"")
        with mock.patch.object(clang_model.subprocess, "run",
                               return_value=completed) as run:
            self.assertEqual(clang_model.run_ast(
                ROOT / "sample.c", "clang", [], "one\ntwo\n"), {})
        self.assertEqual(run.call_args.kwargs["input"], b"one\ntwo\n")
        self.assertNotIn("text", run.call_args.kwargs)

    def test_targeted_model_never_uses_checkout_newlines_for_offsets(self):
        with tempfile.TemporaryDirectory() as directory:
            source = Path(directory) / "sample.c"
            source.write_bytes(
                b"int sample(int value)\r\n{\r\n return value ? 1 : 0;\r\n}\r\n")
            with mock.patch.object(clang_model, "run_ast",
                                   wraps=clang_model.run_ast) as run:
                model = build_model(source, target_name="sample")
        self.assertEqual(len(model), 1)
        self.assertEqual(len(model[0].decisions), 1)
        self.assertIsNotNone(run.call_args.args[3])
        self.assertNotIn("\r", run.call_args.args[3])

    def test_normalizes_watcom_pack_stack_pragmas_without_moving_offsets(self):
        source = ("#pragma pack( __push, 1 )\n"
                  "typedef struct { char c; long value; } Packed;\n"
                  "#pragma pack( __pop )\n")
        result = clang_model.normalize_watcom_pack_pragmas(source)
        self.assertEqual(len(source), len(result))
        self.assertIn("#pragma pack( push  , 1 )", result)
        self.assertIn("#pragma pack( pop   )", result)

    @classmethod
    def setUpClass(cls):
        cls.model = build_model(ROOT / "src" / "ODAuto.c")

    def test_discovers_public_and_static_definitions(self):
        self.assertEqual([item.name for item in self.model],
                         ["od_autodetect", "ODWaitNoCase", "ODWaitDiscard"])

    def test_discovers_same_file_and_external_dependencies(self):
        target = self.model[0]
        names = {item.name for item in target.functions}
        self.assertIn("ODWaitNoCase", names)
        self.assertIn("od_disp", names)
        self.assertIn("strlen", names)
        self.assertEqual({item.name for item in target.variables},
                         {"bODInitialized", "eODLifecycleState", "od_control"})

    def test_pointer_returning_dependency_is_retained(self):
        block = build_model(ROOT / "src" / "ODBlock.c")
        target = next(item for item in block if item.name == "od_puttext")
        malloc = next(item for item in target.functions if item.name == "malloc")
        self.assertEqual(malloc.result, "void *")

    def test_header_inline_dependency_is_not_emitted_as_mock(self):
        definitions = build_model(ROOT / "src" / "ODCFile.c")
        target = next(item for item in definitions
                      if item.name == "ODCfgGetWordHex")
        self.assertNotIn("__sbtoupper",
                         {item.name for item in target.functions})

    def test_included_c_implementation_is_emitted_as_mock_dependency(self):
        flags = ["-std=c89", "-D__unix__", "-DHAS_INTTYPES_H",
                 "-UOPENDOORS_HAVE_VSNPRINTF"]
        definitions = build_model(ROOT / "src" / "ODFmtFB.c", flags=flags)
        target = next(item for item in definitions
                      if item.name == "ODFallbackVsnprintf")
        self.assertIn("trio_vsnprintf",
                      {item.name for item in target.functions})

    def test_included_c_is_replaced_before_ast_analysis(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "backend.c").write_text(
                "#error embedded implementation must not be analyzed\n",
                encoding="ascii")
            (root / "backend.h").write_text(
                "int backend(void);\n", encoding="ascii")
            source = root / "sample.c"
            source.write_text(
                '#include "backend.c"\nint sample(void) { return backend(); }\n',
                encoding="ascii")
            target = build_model(source)[0]
            self.assertEqual(target.name, "sample")
            self.assertIn("backend", {item.name for item in target.functions})

    def test_targeted_analysis_blanks_unselected_function_bodies(self):
        with tempfile.TemporaryDirectory() as directory:
            source = Path(directory) / "sample.c"
            source.write_text(
                "int selected(int value) { return value ? 1 : 0; }\n"
                "int borland_only(void) { asm this syntax is not clang; }\n",
                encoding="ascii")
            definitions = build_model(source, target_name="selected")
            self.assertEqual([item.name for item in definitions], ["selected"])
            selected = definitions[0]
            self.assertEqual(len(selected.decisions), 1)

    def test_definition_signature_uses_the_active_ast_source_range(self):
        with tempfile.TemporaryDirectory() as directory:
            source = Path(directory) / "sample.c"
            source.write_text(
                "#ifdef INACTIVE\n"
                "static int previous(void) { return 0; }\n"
                "#endif\n"
                "\n"
                "/* documentation */\n"
                "static\n"
                "int selected(void) { return 1; }\n",
                encoding="ascii")
            target = build_model(source, target_name="selected")[0]
            self.assertEqual(target.signature_line, 6)

    def test_runtime_library_objects_are_not_redefined(self):
        definitions = build_model(ROOT / "src" / "ODCmdLn.c")
        target = next(item for item in definitions
                      if item.name == "ODParseCommandLineArguments")
        names = {item.name for item in target.variables}
        self.assertNotIn("__stdoutp", names)
        self.assertIn("od_control", names)

    def test_variadic_language_builtins_are_not_mock_dependencies(self):
        definitions = build_model(ROOT / "src" / "ODPrntf.c")
        target = next(item for item in definitions if item.name == "od_printf")
        names = {item.name for item in target.functions}
        self.assertNotIn("__builtin_va_start", names)
        self.assertNotIn("__builtin_va_end", names)
        self.assertTrue(is_language_builtin("va_start"))
        self.assertTrue(is_language_builtin("va_end"))
        self.assertTrue(is_language_builtin("va_copy"))
        self.assertTrue(is_language_builtin("__va_copy"))
        self.assertFalse(is_language_builtin("ODVsnprintf"))

    def test_discovers_source_decisions_but_not_macro_implementations(self):
        wait = next(item for item in self.model if item.name == "ODWaitNoCase")
        self.assertGreaterEqual(len(wait.decisions), 2)
        self.assertTrue(all(item.start < item.end for item in wait.decisions))

    def test_object_like_macro_at_condition_edge_is_instrumented(self):
        source = ROOT / "src" / "ODInQue.c"
        text = source.read_text(encoding="latin-1")
        definitions = build_model(source)
        target = next(item for item in definitions
                      if item.name == "ODInQueueAlloc")
        expressions = {text[item.start:item.end] for item in target.decisions}
        self.assertIn("phInQueue == NULL", expressions)
        self.assertIn("pInputQueueInfo == NULL", expressions)
        self.assertIn("pInputQueue == NULL", expressions)
        self.assertIn("pInputQueueInfo != NULL", expressions)

    def test_leading_static_state_registration_follows_all_declarations(self):
        source = ROOT / "src" / "ODGraph.c"
        text = source.read_text(encoding="latin-1")
        target = next(item for item in build_model(source)
                      if item.name == "od_set_cursor")
        self.assertEqual(len(target.state_points), 1)
        declaration_end = text.index("   INT nWindowHeight;") + len(
            "   INT nWindowHeight;")
        self.assertEqual(target.state_points[0].position, declaration_end)
        self.assertEqual(target.state_points[0].names, ["szControlSequence"])

    def test_discovers_switch_cases_and_default(self):
        definitions = build_model(ROOT / "src" / "ODCFile.c")
        target = next(item for item in definitions if item.name == "ODCfgIsTrue")
        self.assertEqual(len(target.switches), 1)
        self.assertEqual(target.switches[0].expression_type, "int")
        self.assertEqual(len(target.switches[0].branches), 7)
        self.assertTrue(all(item.kind == "CaseStmt"
                            for item in target.switches[0].branches))

    def test_discovers_macro_valued_switch_cases(self):
        node = {"range": {
            "begin": {"spellingLoc": {"offset": 1, "tokLen": 1},
                      "expansionLoc": {"offset": 20, "tokLen": 14}},
            "end": {"spellingLoc": {"offset": 1, "tokLen": 1},
                    "expansionLoc": {"offset": 20, "tokLen": 14}},
        }}
        self.assertEqual(switch_value_range(node), (20, 34))

    def test_renders_function_pointer_parameter(self):
        item = Callable("1", "sample", "void",
                        [Parameter("callback", "int (*)(char)")], False)
        self.assertEqual(declaration(item),
                         "void sample(int (*callback)(char))")

    def test_preserves_function_calling_convention_from_qualified_type(self):
        node = {
            "id": "1",
            "name": "GetWindowLongA",
            "type": {
                "qualType":
                    "LONG (HWND, int) __attribute__((stdcall))"
            },
            "inner": [
                {"kind": "ParmVarDecl", "name": "window",
                 "type": {"qualType": "HWND"}},
                {"kind": "ParmVarDecl", "name": "index",
                 "type": {"qualType": "int"}},
            ],
        }
        item = callable_from(node)
        self.assertEqual(
            declaration(item, "utm_GetWindowLongA"),
            "LONG __attribute__((stdcall)) "
            "utm_GetWindowLongA(HWND window, int index)")

    def test_masks_turbo_asm_jumps_without_moving_source_offsets(self):
        source = "   ASM    mov ax, bx\n   ASM    jnz set_cursor\nset_cursor:\n"
        masked = mask_turbo_asm_jumps(source)
        self.assertEqual(len(masked), len(source))
        self.assertIn("ASM    mov ax, bx", masked)
        self.assertIn("ASM    nop", masked)
        self.assertNotIn("jnz set_cursor", masked)
        self.assertEqual(masked.index("set_cursor:"), source.index("set_cursor:"))

    def test_masks_commented_turbo_asm_jumps_without_moving_offsets(self):
        source = ("   ASM     jc error       /* carry means failure */\n"
                  "error:\n")
        masked = mask_turbo_asm_jumps(source)
        self.assertEqual(len(masked), len(source))
        self.assertIn("ASM     nop", masked)
        self.assertIn("/* carry means failure */", masked)
        self.assertNotIn("jc error", masked)
        self.assertEqual(masked.index("error:"), source.index("error:"))


if __name__ == "__main__":
    unittest.main()
