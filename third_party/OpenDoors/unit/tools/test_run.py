import json
import subprocess
import sys
import tempfile
import unittest
from contextlib import redirect_stderr
from io import StringIO
from pathlib import Path
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parent))

import run as run_module  # noqa: E402
from run import (analyzer_platform_flags, coverage_disposition,
                 DEFAULT_DOS_TIMEOUT,
                 coverage_waived, default_build_path, expand_configurations,
                 dos_aux_stem, dos_batch, dos_short_stem, dosbox_arguments,
                 effective_coverage_waivers,
                 executable_suffix, native_test_arguments,
                 llvm_branch_waived, llvm_coverage_supported, native_coverage,
                 map_llvm_record_lines,
                 missing_llvm_mcdc_records,
                 original_source_line,
                 platform_defines,
                 run_step, selected_tests, selection_owners,
                 watcom_ast_compatibility_flags,
                 watcom_environment, watcom_target_flags,
                 windows_batch, windows_batch_arguments, windows_llvm_batch,
                 windows_fixture_arguments)  # noqa: E402


class DosObjectNamingTests(unittest.TestCase):
    def test_native_label_drops_source_directory(self):
        self.assertEqual(
            run_module.unit_test_label("src/ODAuto.c", "od_autodetect"),
            "ODAuto-od_autodetect")

    def test_default_runtime_timeout_accommodates_the_full_dos_suite(self):
        self.assertEqual(DEFAULT_DOS_TIMEOUT, 900)

    def test_auxiliary_stem_never_matches_the_unit_stem(self):
        self.assertEqual(dos_aux_stem("U1234567"), "U123456A")
        self.assertEqual(dos_aux_stem("UFC1BBAA"), "UFC1BBAB")

    def test_default_build_paths_separate_cross_target_configurations(self):
        paths = {
            default_build_path("unix", "native", None),
            default_build_path("windows", "native", "x86"),
            default_build_path("windows", "native", "x64"),
            default_build_path("dos16", "watcom16", None),
            default_build_path("dos32", "watcom32r", None),
            default_build_path("dos32", "watcom32s", None),
        }
        self.assertEqual(len(paths), 6)

    def test_runner_accepts_the_documented_omitted_build_argument(self):
        completed = subprocess.run(
            [sys.executable, str(Path(run_module.__file__)),
             "--platform", "unix", "--function", "ODThreadSleep"],
            cwd=Path(run_module.__file__).resolve().parents[2],
            stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        self.assertEqual(completed.returncode, 0, completed.stderr)


class PlatformDefinitionTests(unittest.TestCase):
    def test_posix_interfaces_are_requested_for_unix_targets(self):
        feature = "-D_POSIX_C_SOURCE=200809L"
        self.assertIn(feature, platform_defines("unix"))
        self.assertNotIn(feature, platform_defines("windows"))
        self.assertNotIn(feature, platform_defines("dos16"))
        self.assertNotIn(feature, platform_defines("dos32"))

    def test_libc_fortification_does_not_bypass_unix_mocks(self):
        flag = "-U_FORTIFY_SOURCE"
        self.assertIn(flag, platform_defines("unix"))
        self.assertNotIn(flag, platform_defines("windows"))

    def test_windows_runtime_deprecation_aliases_do_not_fail_the_harness(self):
        self.assertIn("-D_CRT_SECURE_NO_WARNINGS", platform_defines("windows"))


class TestSelectionTests(unittest.TestCase):
    def test_loads_exact_owners_from_selector_output(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "selection.json"
            path.write_text(json.dumps({
                "sources": {"first.c": ["one"], "second.c": ["two"]}
            }), encoding="utf-8")
            self.assertEqual(selection_owners(path), {
                ("first.c", "one"), ("second.c", "two")
            })

    def test_rejects_malformed_selector_ownership(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "selection.json"
            path.write_text('{"sources":{"first.c":"one"}}',
                            encoding="utf-8")
            with self.assertRaisesRegex(ValueError, "function lists"):
                selection_owners(path)

    def test_exact_owner_selection_does_not_form_a_cross_product(self):
        document = {"tests": [
            {"source": source, "function": function,
             "platforms": ["unix"]}
            for source in ("first.c", "second.c")
            for function in ("one", "two")
        ]}
        selected = list(selected_tests(
            "unix", set(), set(), None, document,
            {("first.c", "one"), ("second.c", "two")}))
        self.assertEqual(
            [(item["source"], item["function"]) for item in selected],
            [("first.c", "one"), ("second.c", "two")])

    def test_start_at_retains_boundary_and_following_platform_tests(self):
        document = {"tests": [
            {"source": "first.c", "function": "one",
             "platforms": ["windows"]},
            {"source": "second.c", "function": "two",
             "platforms": ["unix"]},
            {"source": "third.c", "function": "three",
             "platforms": ["windows"]},
            {"source": "fourth.c", "function": "four",
             "platforms": ["windows"]},
        ]}
        selected = list(selected_tests(
            "windows", set(), set(), ("third.c", "three"), document))
        self.assertEqual(
            [(item["source"], item["function"]) for item in selected],
            [("third.c", "three"), ("fourth.c", "four")])

    def test_source_platform_flags_are_merged_with_case_flags(self):
        document = {
            "source_native_flags": {
                "sample.c": {"windows": ["-Wno-source-warning"]}
            },
            "tests": [{
                "source": "sample.c", "function": "sample",
                "platforms": ["windows"],
                "configurations": [{
                    "name": "default",
                    "native_flags": ["-Wno-case-warning"]
                }]
            }]
        }
        selected = list(selected_tests(
            "windows", set(), set(), None, document))
        self.assertEqual(selected[0]["configuration"]["native_flags"],
                         ["-Wno-source-warning", "-Wno-case-warning"])


class CoverageWaiverTests(unittest.TestCase):
    def test_detects_compound_decisions_missing_from_llvm_mcdc_records(self):
        model = {"decisions": [
            {"id": 1, "line": 10, "conditions": 2},
            {"id": 2, "line": 20, "conditions": 1},
            {"id": 3, "line": 30, "conditions": 3},
        ]}
        generated = [
            '#line 1 "sample.c"',
            *['line'] * 9,
            # LLVM is authoritative about the four macro-expanded conditions.
            'compound decision',
        ]
        records = [[11, 0, 0, [True, True, True, True]]]
        self.assertEqual(missing_llvm_mcdc_records(
            model, records, generated, "sample.c"),
            [{"id": 3, "line": 30, "conditions": 3}])

    def test_ignores_llvm_mcdc_records_from_other_files(self):
        model = {"decisions": [
            {"id": 1, "line": 10, "conditions": 2},
        ]}
        generated = [
            '#line 1 "sample.c"',
            *['line'] * 9,
            'compound decision',
        ]
        records = [[11, 0, 11, 20, 0, 1, 2, 0, [True, True]]]
        self.assertEqual(missing_llvm_mcdc_records(
            model, records, generated, "sample.c"),
            [{"id": 1, "line": 10, "conditions": 2}])

    def test_maps_generated_lines_through_repeated_line_directives(self):
        generated = [
            '/* support */', '#line 1 "sample.c"', 'first', 'second',
            '#line 100', 'target',
        ]
        self.assertEqual(original_source_line(generated, 6, "sample.c"), 100)
        self.assertIsNone(original_source_line(generated, 1, "sample.c"))

    def test_normalizes_llvm_records_to_original_source_lines(self):
        generated = ['#line 40 "sample.c"', 'decision']
        records = [[2, 3, 2, 20, 1, 1, [True, True]]]
        self.assertEqual(map_llvm_record_lines(
            records, generated, "sample.c"),
            [[40, 3, 2, 20, 1, 1, [True, True]]])

    def test_maps_llvm_line_counts_and_identifies_uncovered_source_lines(self):
        generated = [
            '/* support */', '#line 40 "sample.c"',
            'covered();', 'missed();', 'break;', '/* comment */',
            '#line 90 "sample.c"', 'also_covered();', '#ifdef FEATURE', '}',
        ]
        show = """\
utt_sample:
    3|      2|covered();
    4|      0|missed();
    5|      0|break;
    6|       |/* comment */
    8|   1.00k|also_covered();
    9|      0|#ifdef FEATURE
   10|      0|}
"""
        source_lines = ([""] * 39 + ["covered();", "missed();",
                        "break;", "/* comment */"] + [""] * 46 +
                        ["also_covered();", "#ifdef FEATURE", "}"])
        self.assertEqual(
            run_module.llvm_line_coverage(
                show, generated, "sample.c", source_lines),
            ([40, 90], [41]))

    def test_llvm_coverage_reports_and_rejects_an_uncovered_line(self):
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            executable = directory / "sample-coverage"
            profile = directory / "sample.profraw"
            generated = directory / "sample-llvm.c"
            output = directory / "sample.coverage.json"
            generated.write_text(
                '/* support */\n#line 40 "sample.c"\n'
                'covered();\nmissed();\n', encoding="latin-1")
            generated.with_suffix(".model.json").write_text(json.dumps({
                "source": "sample.c", "function": "sample",
                "decisions": [], "branches": [],
            }), encoding="utf-8")
            document = {"data": [{"functions": [{
                "name": "utt_sample", "count": 1,
                "branches": [], "mcdc_records": [],
            }]}]}

            def completed(arguments, **unused):
                stdout = (json.dumps(document) if "export" in arguments
                          else "utt_sample:\n  3|  1|covered();\n"
                               "  4|  0|missed();\n")
                return subprocess.CompletedProcess(arguments, 0, stdout)

            with patch.object(run_module, "command"), patch.object(
                    run_module.subprocess, "run", side_effect=completed):
                with self.assertRaisesRegex(RuntimeError,
                                             "missed 1 lines: 41"):
                    run_module.llvm_coverage(
                        executable, profile, "sample.c", "sample", "unix",
                        generated, output)

            result = json.loads(output.read_text(encoding="utf-8"))
            self.assertEqual(result["lines"], 2)
            self.assertEqual(result["covered_lines"], [40])
            self.assertEqual(result["missed_lines"], [41])

    def test_discards_llvm_records_from_other_files(self):
        generated = [
            '#line 70 "support.h"', 'header decision',
            '#line 40 "sample.c"', 'source decision',
        ]
        records = [
            [2, 3, 2, 20, 0, 1, 1, 0, 4],
            [4, 3, 4, 20, 1, 1, 0, 0, 4],
        ]
        self.assertEqual(map_llvm_record_lines(
            records, generated, "sample.c"),
            [[40, 3, 4, 20, 1, 1, 0, 0, 4]])

    def test_discards_unreachable_do_while_zero_branch_records(self):
        generated = [
            '#line 40 "sample.c"', 'source decision', '} while(0)',
        ]
        records = [
            [2, 3, 2, 20, 1, 1, 0, 0, 4],
            [3, 1, 3, 11, 0, 2, 0, 0, 4],
        ]
        self.assertEqual(map_llvm_record_lines(
            records, generated, "sample.c"),
            [[40, 3, 2, 20, 1, 1, 0, 0, 4]])

    def test_discards_zero_width_macro_branch_records(self):
        generated = ['#line 40 "sample.c"', 'FD_ZERO(&set);']
        records = [[2, 4, 2, 4, 0, 1, 0, 0, 4]]
        self.assertEqual(map_llvm_record_lines(
            records, generated, "sample.c"), [])

    def test_approved_and_proposed_waivers_have_distinct_dispositions(self):
        entries = [{
            "source": "sample.c", "function": "sample",
            "platform": "unix", "kind": "branch",
            "start_line": 10, "end_line": 10,
        }, {
            "source": "sample.c", "function": "sample",
            "platform": "unix", "kind": "mcdc",
            "start_line": 20, "end_line": 20,
            "proposed": True,
        }]
        self.assertEqual(coverage_disposition(
            entries, "sample.c", "sample", "unix", "branch", 10),
            (True, False))
        self.assertEqual(coverage_disposition(
            entries, "sample.c", "sample", "unix", "mcdc", 20),
            (False, True))

    def test_proposals_are_used_only_when_explicitly_enabled(self):
        approved = [{"source": "approved.c", "function": "approved"}]
        proposals = {"proposals": [{
            "id": "sample-proposal",
            "status": "proposed",
            "source": "sample.c",
            "function": "sample",
            "platforms": ["unix", "dos16"],
            "kinds": ["branch", "mcdc"],
            "start_line": 10,
            "end_line": 11,
        }]}

        strict = effective_coverage_waivers(approved, proposals, False)
        permissive = effective_coverage_waivers(approved, proposals, True)

        self.assertEqual(strict, approved)
        self.assertEqual(len(permissive), 5)
        self.assertFalse(permissive[0].get("proposed", False))
        self.assertTrue(all(item["proposed"] for item in permissive[1:]))
        self.assertEqual(
            {(item["platform"], item["kind"])
             for item in permissive[1:]},
            {("unix", "branch"), ("unix", "mcdc"),
             ("dos16", "branch"), ("dos16", "mcdc")})

    def test_single_condition_mcdc_inherits_branch_waiver(self):
        waivers = [{
            "source": "sample.c",
            "function": "sample",
            "platform": "dos16",
            "kind": "branch",
            "start_line": 10,
            "end_line": 10,
        }]
        self.assertTrue(coverage_waived(
            waivers, "sample.c", "sample", "dos16", "mcdc", 10,
            conditions=1))
        self.assertFalse(coverage_waived(
            waivers, "sample.c", "sample", "dos16", "mcdc", 10,
            conditions=2))
        self.assertFalse(coverage_waived(
            waivers, "sample.c", "sample", "unix", "mcdc", 10,
            conditions=1))

    def test_llvm_condition_branch_accepts_mcdc_waiver(self):
        waivers = [{
            "source": "sample.c",
            "function": "sample",
            "platform": "unix",
            "kind": "mcdc",
            "start_line": 20,
            "end_line": 20,
        }]
        self.assertTrue(llvm_branch_waived(
            waivers, "sample.c", "sample", "unix", 20))
        self.assertFalse(coverage_waived(
            waivers, "sample.c", "sample", "unix", "branch", 20))

    def test_native_coverage_discards_duplicate_observations(self):
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            model = directory / "sample.model.json"
            report = directory / "sample.cov"
            output = directory / "sample.coverage.json"
            model.write_text(json.dumps({
                "source": "sample.c",
                "function": "sample",
                "branches": [],
                "decisions": [{
                    "id": 1,
                    "line": 1,
                    "conditions": 1,
                }],
            }), encoding="utf-8")
            report.write_text(
                "D 1 1 1 0 0\n"
                "D 1 1 1 0 0\n"
                "D 1 1 1 1 1\n"
                "D 1 1 1 1 1\n",
                encoding="ascii")

            native_coverage(report, model, "unix", output)

            result = json.loads(output.read_text(encoding="utf-8"))
            self.assertEqual(len(result["decisions"][0]["vectors"]), 2)
            self.assertEqual(
                result["decisions"][0]["mcc_observed_combinations"], [0, 1])
            self.assertTrue(result["decisions"][0]["mcc_complete"])

    def test_native_coverage_requires_both_assembly_branch_outcomes(self):
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            model = directory / "sample.model.json"
            report = directory / "sample.cov"
            output = directory / "sample.coverage.json"
            model.write_text(json.dumps({
                "source": "sample.asm",
                "function": "sample",
                "branches": [],
                "decisions": [],
                "assembly_branches": [{
                    "id": 1, "line": 7, "instruction": "jne"
                }],
            }), encoding="utf-8")
            report.write_text("A 1 0\n", encoding="ascii")
            with self.assertRaisesRegex(RuntimeError,
                                        "assembly branch 1 line 7"):
                native_coverage(report, model, "dos16", output)

            report.write_text("A 1 0\nA 1 1\n", encoding="ascii")
            native_coverage(report, model, "dos16", output)
            result = json.loads(output.read_text(encoding="utf-8"))
            self.assertEqual(result["assembly_branches"][0]["outcomes"],
                             [0, 1])


class WatcomASTCompatibilityTests(unittest.TestCase):
    def test_dos32_analysis_uses_a_32_bit_c_data_model(self):
        self.assertIn("-m32",
                      run_module.analyzer_platform_flags("dos32", []))

    def test_compiler_driver_selects_dos_for_compilation_and_linking(self):
        self.assertEqual(watcom_target_flags("watcom16"), ["-bcl=dos"])
        self.assertEqual(watcom_target_flags("watcom32r"),
                         ["-bt=dos", "-l=dos4g"])
        self.assertEqual(watcom_target_flags("watcom32s"),
                         ["-bt=dos", "-l=dos4g"])

    def test_uses_watcom_header_va_list_definition(self):
        flags = watcom_ast_compatibility_flags("/opt/watcom")
        self.assertFalse(any("__va_list" in flag for flag in flags))
        self.assertIn("-D__watcall=", flags)
        self.assertIn("-Dinterrupt=", flags)
        self.assertIn("-Dfar=", flags)
        self.assertIn("-fasm-blocks", flags)
        self.assertNotIn("-D_WCCALLBACK=", flags)
        self.assertEqual(flags[-2:], ["-isystem", "/opt/watcom/h"])


class LanguageStandardTests(unittest.TestCase):
    def test_modern_targets_use_the_cmake_c99_contract(self):
        self.assertEqual(run_module.language_standard_flag("unix"),
                         "-std=c99")
        self.assertEqual(run_module.language_standard_flag("windows"),
                         "-std=c99")
        for platform in ("unix", "windows"):
            self.assertNotIn("-pedantic",
                             run_module.native_compile_flags(platform))

    def test_permits_intentionally_partial_aggregate_initializers(self):
        for platform in ("unix", "windows"):
            self.assertIn("-Wno-missing-field-initializers",
                          run_module.native_compile_flags(platform))

    def test_ignores_parameters_unused_by_the_isolated_body(self):
        for platform in ("unix", "windows"):
            self.assertIn("-Wno-unused-parameter",
                          run_module.native_compile_flags(platform))

    def test_dos_analysis_remains_c89_compatible(self):
        self.assertEqual(run_module.language_standard_flag("dos16"),
                         "-std=c89")
        self.assertEqual(run_module.language_standard_flag("dos32"),
                         "-std=c89")


class WindowsASTCompatibilityTests(unittest.TestCase):
    def test_builds_a_fixture_dll_beside_the_test_executable(self):
        self.assertEqual(windows_fixture_arguments(
            "i686-w64-mingw32-gcc", Path("unit/fixtures/door32/door32.c"),
            Path("build/case/DOOR32.DLL")), [
                "i686-w64-mingw32-gcc", "-std=c99", "-Wall", "-Wextra",
                "-Werror", "-shared", "-Wl,--kill-at",
                "unit/fixtures/door32/door32.c", "-o",
                "build/case/DOOR32.DLL"])

    def test_fixture_receives_the_executable_architecture_flags(self):
        arguments = windows_fixture_arguments(
            "clang", Path("door32.c"), Path("DOOR32.DLL"), ["-m32"])
        self.assertIn("-m32", arguments)

    def test_msvc_style_clang_fixture_omits_the_gnu_export_option(self):
        arguments = windows_fixture_arguments(
            "clang", Path("door32.c"), Path("DOOR32.DLL"))
        self.assertNotIn("-Wl,--kill-at", arguments)

    def test_llvm_execution_includes_native_modern_platforms(self):
        self.assertTrue(llvm_coverage_supported("unix", False))
        self.assertFalse(llvm_coverage_supported("unix", True))
        self.assertTrue(llvm_coverage_supported("windows", True))
        self.assertFalse(llvm_coverage_supported("windows", False))
        self.assertFalse(llvm_coverage_supported("dos16"))
        self.assertFalse(llvm_coverage_supported("dos32"))

    def test_uses_a_windows_target_for_predefined_platform_macros(self):
        flags = analyzer_platform_flags(
            "windows", ["-D_CRTIMP=", "-DDECLSPEC_IMPORT="])
        self.assertIn("-target", flags)
        target = flags[flags.index("-target") + 1]
        self.assertEqual(target, "x86_64-w64-windows-gnu")

    def test_uses_a_32_bit_windows_target_with_an_i686_compiler(self):
        flags = analyzer_platform_flags(
            "windows", ["-D_CRTIMP="], "i686-w64-mingw32-gcc")
        target = flags[flags.index("-target") + 1]
        self.assertEqual(target, "i686-w64-windows-gnu")

    def test_explicit_architecture_overrides_a_generic_compiler_name(self):
        flags = analyzer_platform_flags(
            "windows", ["-D_CRTIMP="], "clang", "x86")
        target = flags[flags.index("-target") + 1]
        self.assertEqual(target, "i686-w64-windows-gnu")

    def test_native_windows_clang_can_model_the_msvc_abi(self):
        flags = analyzer_platform_flags(
            "windows", ["-D_CRTIMP="], "clang", "x86", "msvc")
        target = flags[flags.index("-target") + 1]
        self.assertEqual(target, "i686-pc-windows-msvc")

    def test_windows_executables_retain_the_mingw_suffix(self):
        self.assertEqual(executable_suffix("windows"), ".exe")
        self.assertEqual(executable_suffix("unix"), "")
        self.assertEqual(executable_suffix("dos16"), ".EXE")

    def test_windows_native_test_can_be_launched_through_wine(self):
        arguments = native_test_arguments(
            Path("build/test.exe"), Path("build/test.cov"),
            Path("/usr/local/bin/wine64"))
        self.assertEqual(arguments, [
            "/usr/local/bin/wine64", "build/test.exe", "build/test.cov"])

    def test_batch_records_each_failure_and_continues_to_completion(self):
        text = windows_batch([
            ("ODCom-ODComGetByte.exe", "ODCom-ODComGetByte.native.cov"),
            ("ODCom-ODComSendByte.exe", "ODCom-ODComSendByte.native.cov"),
        ])
        self.assertTrue(text.startswith("@ECHO OFF\r\n"))
        self.assertIn(
            '"%~dp0ODCom-ODComGetByte.exe" '
            '"%~dp0ODCom-ODComGetByte.native.cov" '
            '>"%~dp0ODCom-ODComGetByte.OUT" 2>&1\r\n', text)
        self.assertIn("IF NOT ERRORLEVEL 1 GOTO OK0\r\n", text)
        self.assertIn(
            'ECHO FAIL>"%~dp0ODCom-ODComGetByte.BAD"\r\n:OK0\r\n',
            text)
        self.assertIn(
            '"%~dp0ODCom-ODComSendByte.exe" '
            '"%~dp0ODCom-ODComSendByte.native.cov" '
            '>"%~dp0ODCom-ODComSendByte.OUT" 2>&1\r\n', text)
        self.assertIn('ECHO DONE>"%~dp0UTDONE.OK"\r\n', text)
        self.assertTrue(text.endswith(
            'ECHO DONE>"%~dp0UTDONE.OK"\r\nEXIT /B 0\r\n'))

    def test_llvm_batch_assigns_a_distinct_profile_to_each_executable(self):
        text = windows_llvm_batch([
            ("ODAuto-od_autodetect-coverage.exe",
             "ODAuto-od_autodetect.profraw"),
            ("ODCom-ODComGetByte-coverage.exe",
             "ODCom-ODComGetByte.profraw"),
        ])
        self.assertIn(
            'SET "LLVM_PROFILE_FILE=%~dp0ODAuto-od_autodetect.profraw"\r\n',
            text)
        self.assertIn(
            '"%~dp0ODAuto-od_autodetect-coverage.exe" '
            '>"%~dp0ODAuto-od_autodetect-coverage.OUT" 2>&1\r\n', text)
        self.assertIn(
            'SET "LLVM_PROFILE_FILE=%~dp0ODCom-ODComGetByte.profraw"\r\n',
            text)
        self.assertTrue(text.endswith(
            'ECHO DONE>"%~dp0UTLLVM.OK"\r\nEXIT /B 0\r\n'))

    def test_wine_runs_one_cmd_batch_through_the_default_z_drive(self):
        batch = Path("/tmp/unit windows/UTRUN.CMD")
        self.assertEqual(windows_batch_arguments(
            batch, Path("/usr/local/bin/wine64")), [
                "/usr/local/bin/wine64", "cmd.exe", "/d", "/c",
                r"Z:\tmp\unit windows\UTRUN.CMD"])


class FailureAggregationTests(unittest.TestCase):
    def test_records_a_failure_and_allows_later_steps(self):
        failures = []
        calls = []

        def fail():
            calls.append("failed")
            raise RuntimeError("expected red gate")

        def pass_later():
            calls.append("continued")

        with redirect_stderr(StringIO()):
            self.assertFalse(run_step(failures, "native coverage", fail))
        self.assertTrue(run_step(failures, "LLVM coverage", pass_later))
        self.assertEqual(calls, ["failed", "continued"])
        self.assertEqual(len(failures), 1)
        self.assertIn("native coverage", failures[0])
        self.assertIn("expected red gate", failures[0])


class ConfigurationTests(unittest.TestCase):
    def test_expands_named_preprocessor_configurations(self):
        test = {
            "source": "sample.c",
            "function": "sample",
            "configurations": [
                {"name": "runtime", "defines": ["HAVE_FORMAT=1"]},
                {"name": "fallback", "undefines": ["HAVE_FORMAT"]},
            ],
        }
        expanded = list(expand_configurations(test))
        self.assertEqual([item["configuration"]["name"]
                          for item in expanded], ["runtime", "fallback"])
        self.assertEqual(expanded[0]["source"], "sample.c")
        self.assertNotIn("configuration", test)

    def test_implicit_configuration_preserves_existing_tests(self):
        expanded = list(expand_configurations({"source": "sample.c"}))
        self.assertEqual(expanded[0]["configuration"], {"name": "default"})

    def test_filters_configuration_to_its_applicable_platforms(self):
        test = {
            "source": "sample.c",
            "configurations": [
                {"name": "default"},
                {"name": "diagnostics", "defines": ["DIAGNOSTICS=1"],
                 "platforms": ["windows"]},
            ],
        }
        self.assertEqual(
            [item["configuration"]["name"]
             for item in expand_configurations(test, "unix")],
            ["default"])
        self.assertEqual(
            [item["configuration"]["name"]
             for item in expand_configurations(test, "windows")],
            ["default", "diagnostics"])


class DOSRuntimeTests(unittest.TestCase):
    def test_dosbox_uses_private_configuration_and_exits(self):
        arguments = dosbox_arguments(
            Path("/usr/bin/dosbox"), Path("/tmp/unit-build"),
            Path("/tmp/unit-build/UTRUN.BAT"),
            Path("/tmp/unit-build/dosbox.conf"))
        self.assertEqual(arguments[:6], [
            "/usr/bin/dosbox", "-noconsole", "-exit", "-conf",
            "/tmp/unit-build/dosbox.conf", "-c"])
        self.assertIn('mount c "/tmp/unit-build"', arguments)
        self.assertEqual(arguments[-4:], ["-c", "c:", "-c", "UTRUN.BAT"])

    def test_short_stems_are_deterministic_unique_83_names(self):
        first = dos_short_stem("ODPrntf.c", "od_printf", "default")
        self.assertEqual(first,
                         dos_short_stem("ODPrntf.c", "od_printf", "default"))
        self.assertNotEqual(first,
                            dos_short_stem("ODPrntf.c", "od_printf", "fallback"))
        self.assertLessEqual(len(first), 8)
        self.assertTrue(first.startswith("U"))
        self.assertTrue(first.isalnum())

    def test_batch_records_each_failure_and_continues_to_completion(self):
        text = dos_batch([("U1234567.EXE", "U1234567.COV"),
                          ("U7654321.EXE", "U7654321.COV")],
                         ["FOSSILTS.COM"])
        self.assertTrue(text.startswith("@ECHO OFF\r\nFOSSILTS.COM\r\n"))
        self.assertIn(
            "U1234567.EXE U1234567.COV >U1234567.OUT\r\n", text)
        self.assertIn("IF NOT ERRORLEVEL 1 GOTO OK0\r\n", text)
        self.assertIn("ECHO FAIL>U1234567.BAD\r\n:OK0\r\n", text)
        self.assertIn(
            "U7654321.EXE U7654321.COV >U7654321.OUT\r\n", text)
        self.assertIn("IF NOT ERRORLEVEL 1 GOTO OK1\r\n", text)
        self.assertIn("ECHO FAIL>U7654321.BAD\r\n:OK1\r\n", text)
        self.assertIn("ECHO DONE>UTDONE.OK\r\n", text)
        self.assertTrue(text.endswith("ECHO DONE>UTDONE.OK\r\nEXIT\r\n"))
        self.assertNotIn("GOTO FAILED", text)

    def test_watcom_compiler_does_not_inherit_x_display(self):
        original = {"DISPLAY": ":91", "PATH": "/bin"}
        result = watcom_environment(original, "/opt/watcom")
        self.assertNotIn("DISPLAY", result)
        self.assertEqual(result["INCLUDE"], "/opt/watcom/h")
        self.assertIn("DISPLAY", original)


if __name__ == "__main__":
    unittest.main()
