import sys
import json
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import generate as generate_module  # noqa: E402
from clang_model import Callable, Variable  # noqa: E402
from generate import (default_mock, early_alias_macros,
                      early_mock_declarations, embedded_case,
                      explicit_mock_names, generate, HEADER_MACRO_APIS,
                      HEADER_MOCK_DECLARATIONS, insert_late_defines,
                      late_mock_names, mock_declarations, mock_definitions,
                      replace_c_includes,
                      variable_definition, variable_definitions)  # noqa: E402
from inventory import ROOT  # noqa: E402


class IncludedImplementationTests(unittest.TestCase):
    def test_repository_source_identity_preserves_its_directory(self):
        self.assertEqual(
            generate_module.manifest_source(ROOT / "src" / "ODAuto.c"),
            "src/ODAuto.c")

    def test_repository_source_line_marker_preserves_its_directory(self):
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / "generated.c"
            generate(
                ROOT / "src" / "ODAuto.c", "ODWaitDiscard",
                ROOT / "unit" / "cases" / "ODAuto" / "ODWaitDiscard.c",
                output, "clang", ["-std=c99", "-D__unix__",
                                   "-D_POSIX_C_SOURCE=200809L"])
            self.assertIn('#line 1 "src/ODAuto.c"',
                          output.read_text(encoding="latin-1"))

    def test_emits_an_unmodified_boolean_tree_for_llvm_coverage(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source = root / "sample.c"
            case = root / "case.c"
            portable = root / "portable.c"
            llvm = root / "llvm.c"
            source.write_text(
                "static int target(int left, int right)\n"
                "{\n   static int calls;\n   ++calls;\n"
                "   if(left && right) return calls;\n   return 0;\n}\n",
                encoding="latin-1")
            case.write_text(
                "static void test_target(void) { UT_ASSERT(utt_target(1, 1)); }\n"
                "static const UTTestCase ut_cases[] = {{\"target\", test_target}};\n",
                encoding="latin-1")
            generate(source, "target", case, portable, "clang", ["-std=c89"],
                     uninstrumented_output=llvm)
            self.assertIn("ut_cov_condition", portable.read_text("latin-1"))
            self.assertNotIn("ut_cov_condition", llvm.read_text("latin-1"))
            self.assertIn("ut_state_register", llvm.read_text("latin-1"))
            self.assertEqual(
                json.loads(portable.with_suffix(".model.json").read_text()),
                json.loads(llvm.with_suffix(".model.json").read_text()))

    def test_replaces_quoted_c_implementation_with_matching_header(self):
        source = """\
#ifdef OLD
#include "trio/trio.c"
#else
#include "third_party/trio/trio.c"
#endif
"""
        self.assertEqual(replace_c_includes(source), """\
#ifdef OLD
#include "trio/trio.h"
#else
#include "third_party/trio/trio.h"
#endif
""")

    def test_leaves_headers_and_system_includes_unchanged(self):
        source = '#include "local.h"\n#include <runtime.c>\n'
        self.assertEqual(replace_c_includes(source), source)

    def test_embeds_case_local_headers_without_absolute_includes(self):
        case = ROOT / "unit" / "cases" / "ODGetIn" / "ODShiftSeq.c"
        result = embedded_case(case)
        self.assertNotIn('#include "common.h"', result)
        self.assertNotIn(str(case.resolve()), result)
        self.assertIn("ut_set_sequence", result)


class VariableDefinitionTests(unittest.TestCase):
    def test_preserves_function_pointer_calling_convention(self):
        variable = Variable("id", "hook", "BOOL (*)(INT)", True, "ODCALL")
        self.assertEqual(variable_definition(variable),
                         "BOOL (ODCALL *hook)(INT);")

    def test_omits_runtime_provided_external_objects(self):
        variables = [Variable("id1", "environ", "char **", False, ""),
                     Variable("id2", "od_control", "tODControl", False, "")]
        self.assertEqual(variable_definitions(variables, {"environ"}),
                         ["tODControl od_control;"])


class LateDefineTests(unittest.TestCase):
    def test_reasserts_configuration_at_the_target_signature(self):
        source = "#define outp real_outp\n\nstatic void target(void)\n{\n}\n"
        self.assertEqual(insert_late_defines(
            source, 3, ["outp=UTOutp", "FEATURE"], ["DISABLED"]),
            "#define outp real_outp\n\n"
            "#undef DISABLED\n"
            "#undef outp\n#define outp UTOutp\n"
            "#undef FEATURE\n#define FEATURE 1\n"
            "#line 3\n"
            "static void target(void)\n{\n}\n")

    def test_inserts_turbo_mock_prototypes_after_headers(self):
        source = "#include <conio.h>\n\nstatic void target(void)\n{\n}\n"
        result = insert_late_defines(
            source, 3, [], declarations=["int utm_inp(unsigned);"])
        self.assertIn("int utm_inp(unsigned);\n#line 3\nstatic", result)


class MockDeclarationTests(unittest.TestCase):
    def test_every_deferred_header_api_has_a_source_visible_prototype(self):
        self.assertEqual(HEADER_MACRO_APIS,
                         set(HEADER_MOCK_DECLARATIONS))

    def test_explicit_case_mocks_force_header_macro_interception(self):
        case = ("#define UT_CUSTOM_MOCK_memcpy\n"
                "#define UT_CUSTOM_MOCK_vfprintf\n"
                "#define UT_CUSTOM_MOCK_sscanf\n"
                "#define UT_CUSTOM_MOCK_sigemptyset\n"
                "#define UT_CUSTOM_MOCK_mktime\n"
                "#define UT_CUSTOM_MOCK_printf\n"
                "#define UT_CUSTOM_MOCK_strcat\n"
                "#define UT_CUSTOM_MOCK_strcpy\n"
                "#define UT_CUSTOM_MOCK_strncat\n"
                "#define UT_CUSTOM_MOCK_strncpy\n"
                "#define UT_CUSTOM_MOCK_time\n")
        self.assertEqual(explicit_mock_names(case, ["-D__unix__"]),
                         {"memcpy", "mktime", "printf", "sigemptyset",
                          "sscanf", "vfprintf",
                          "strcat", "strcpy", "strncat", "strncpy",
                          "time"})

    def test_unix_ignores_inactive_odstr_compatibility_mocks(self):
        case = "\n".join(
            f"#define UT_CUSTOM_MOCK_{name}"
            for name in (
                "stricmp", "strlwr", "strnicmp", "strupr", "toupper"))
        self.assertEqual(explicit_mock_names(case, ["-D__unix__"]),
                         {"toupper"})

    def test_windows_ignores_inactive_posix_mocks_but_keeps_localtime(self):
        case = ("#define UT_CUSTOM_MOCK_localtime\n"
                "#define UT_CUSTOM_MOCK_sigemptyset\n"
                "#define UT_CUSTOM_MOCK_sigprocmask\n")
        flags = ["-target", "x86_64-pc-windows-msvc"]
        self.assertEqual(explicit_mock_names(case, flags), {"localtime"})

    def test_dos_does_not_force_modern_header_macro_interception(self):
        case = ("#define UT_CUSTOM_MOCK_isspace\n"
                "#define UT_CUSTOM_MOCK_strcpy\n")
        self.assertEqual(explicit_mock_names(
            case, ["-D__WATCOMC__=1300", "-D__386__"]), set())
        self.assertEqual(explicit_mock_names(
            case, ["-D__TURBOC__=0x0201", "-D__LARGE__"]), set())

    def test_declares_errno_accessors_before_runtime_headers_use_them(self):
        self.assertEqual(early_mock_declarations(
            {"__error", "__errno_location", "__get_errno_ptr", "_errno",
             "fopen"}), [
                "int *__error(void);",
                "int *utm___error(void);",
                "int *__errno_location(void);",
                "int *utm___errno_location(void);",
                "int *__get_errno_ptr(void);",
                "int *utm___get_errno_ptr(void);",
                "int *_errno(void);",
                "int *utm__errno(void);",
            ])

    def test_dos_keeps_mock_aliases_before_its_non_inline_headers(self):
        names = {"exit", "toupper"}
        self.assertEqual(late_mock_names(names, ["-D__WATCOMC__=1300"]),
                         set())
        self.assertEqual(late_mock_names(names, ["-D__TURBOC__=0x0201"]),
                         set())
        self.assertEqual(late_mock_names(names, ["-D__unix__"]),
                         {"toupper"})

    def test_unix_keeps_opendoors_compatibility_aliases_early(self):
        names = {"stricmp", "strlwr", "strnicmp", "strupr", "toupper"}
        self.assertEqual(late_mock_names(names, ["-D__unix__"]),
                         {"toupper"})

    def test_windows_defers_localtime_but_not_inactive_posix_apis(self):
        names = {"localtime", "sigemptyset", "sigprocmask"}
        flags = ["-target", "x86_64-pc-windows-msvc"]
        self.assertEqual(late_mock_names(names, flags), {"localtime"})

    def test_declares_header_macro_mocks_missing_from_the_ast(self):
        self.assertEqual(mock_declarations(
            {"sscanf", "strncat", "toupper", "vfprintf"}), [
            "int utm_sscanf(const char *, const char *, ...);",
            "char *utm_strncat(char *, const char *, size_t);",
            "int utm_toupper(int);",
            "int utm_vfprintf(FILE *, const char *, va_list);",
        ])

    def test_does_not_rename_dependencies_while_platform_headers_are_read(self):
        self.assertEqual(early_alias_macros([
            ("target", "utt_target"),
            ("strcpy", "utm_strcpy"),
            ("toupper", "utm_toupper"),
        ], {"strcpy", "toupper"}), [
            ("target", "utt_target"),
        ])

    def test_reasserts_dependency_aliases_after_runtime_headers(self):
        self.assertEqual(mock_definitions("target", {"__error": object()}),
                         ["target=utt_target", "__error=utm___error"])

class DefaultMockTests(unittest.TestCase):
    def test_zeroes_results_without_a_mockable_libc_dependency(self):
        function = Callable("id", "dependency", "RECT", [], False)
        result = default_mock(function, 1)
        self.assertNotIn("memset", result)
        self.assertIn("ut_result_bytes", result)

    def test_uses_the_source_visible_windows_boolean_type(self):
        function = Callable("id", "dependency", "WINBOOL", [], False)
        result = default_mock(function, 1)
        self.assertIn("BOOL utm_dependency(void)", result)
        self.assertIn("BOOL ut_result;", result)
        self.assertNotIn("WINBOOL", result)


if __name__ == "__main__":
    unittest.main()
