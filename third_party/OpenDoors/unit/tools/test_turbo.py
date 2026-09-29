#!/usr/bin/env python3
"""Regression tests for Turbo C unit-suite sharding."""

import json
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from turbo import (execution_units, merge_coverage_reports,
                   turbo_compile_batch, turbo_fixture_sources,
                   turbo_link_batch)  # noqa: E402
from turbo_generate import turbo_ast_flags  # noqa: E402


class TurboShardTests(unittest.TestCase):
    def test_tsr_fixtures_follow_dos16_configuration_scope(self):
        manifest = {"tests": [{
            "source": "sample.c", "function": "sample",
            "configurations": [
                {"name": "windows", "platforms": ["windows"],
                 "turbo_tsr_fixture": "tests/windows.asm"},
                {"name": "dos", "platforms": ["dos16"],
                 "turbo_tsr_fixture": "tests/fossil_tsr.asm"},
            ],
        }]}
        self.assertEqual(
            turbo_fixture_sources(manifest, {("sample.c", "sample")}),
            [Path(__file__).resolve().parents[2] / "tests/fossil_tsr.asm"])

    def test_turbo_analysis_selects_borland_not_watcom_paths(self):
        flags = turbo_ast_flags(Path("/watcom"))
        self.assertIn("-D__TURBOC__=0x0201", flags)
        self.assertIn("-D_M_IX86=100", flags)
        self.assertIn("-Dasm=__asm", flags)
        self.assertNotIn("-D__va_list=char *", flags)
        include_paths = [Path(flags[index + 1])
                         for index, flag in enumerate(flags[:-1])
                         if flag == "-I"]
        self.assertIn(Path(__file__).resolve().parents[1]
                      / "framework" / "turbo_include", include_paths)
        self.assertFalse(any(flag.startswith("-DMK_FP(") for flag in flags))
        self.assertFalse(any("__WATCOMC__" in flag for flag in flags))

    def test_turbo_analysis_forces_its_borland_only_type_declarations(self):
        flags = turbo_ast_flags(Path("/watcom"))
        index = flags.index("-include")
        self.assertEqual(Path(flags[index + 1]).name, "turbo_ast.h")

    def test_turbo_analysis_resolves_the_staged_trio_include_spelling(self):
        flags = turbo_ast_flags(Path("/watcom"))
        include_paths = [Path(flags[index + 1])
                         for index, flag in enumerate(flags[:-1])
                         if flag == "-I"]
        self.assertIn(Path(__file__).resolve().parents[2] / "third_party",
                      include_paths)

    def test_batch_assembles_exact_model_and_applies_configuration(self):
        with tempfile.TemporaryDirectory() as directory:
            model = Path(directory) / "model.json"
            model.write_text(json.dumps({
                "source": "sample.c", "function": "sample",
                "turbo_defines": ["FEATURE=1"],
                "turbo_undefines": ["DISABLED"]
            }), encoding="ascii")
            runs = [("U1234567", "U1234567", 0, model)]
            compile_batch = turbo_compile_batch(runs)
            link_batch = turbo_link_batch(runs, ["FOSSILTS.COM"])
            self.assertIn("TCC -ml -S", compile_batch)
            self.assertIn("C:\\SRC\\TRIO", compile_batch)
            self.assertNotIn("-DFEATURE=1", compile_batch)
            self.assertNotIn("-UDISABLED", compile_batch)
            self.assertIn("ASMFIX", compile_batch)
            self.assertIn(
                "COPY U1234567.ASM C:\\UNITASM\\U1234567.ASM",
                compile_batch)
            self.assertNotIn("JWASM", compile_batch + link_batch)
            self.assertIn("ECHO DONE>C:\\UNITCC.OK", compile_batch)
            self.assertIn("UT.OBJ U1234567.OBJ", link_batch)
            self.assertLess(link_batch.index("C:\\FOSSILTS.COM"),
                            link_batch.index("U1234567.EXE"))
            self.assertIn("ECHO DONE>C:\\UNITDONE.OK", link_batch)

    def test_expands_only_a_manifested_unit_into_unique_dos_stems(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            units = []
            for stem, source, function in (
                    ("U1234567", "large.c", "large"),
                    ("U7654321", "small.c", "small")):
                unit = root / (stem + ".c")
                model = root / (stem + ".model.json")
                unit.write_text("", encoding="ascii")
                model.write_text(json.dumps({
                    "source": source, "function": function
                }), encoding="ascii")
                units.append((stem, unit, model))
            manifest = {"tests": [{
                "source": "large.c", "function": "large",
                "turbo_shards": 3
            }]}

            runs = execution_units(units, manifest)

            self.assertEqual([run[2] for run in runs], [1, 2, 3, 0])
            self.assertEqual(len({run[0] for run in runs}), 4)
            self.assertTrue(all(len(run[0]) == 8 for run in runs))
            self.assertEqual([run[1] for run in runs[:3]], ["U1234567"] * 3)

    def test_link_batch_includes_the_isolated_assembly_object(self):
        with tempfile.TemporaryDirectory() as directory:
            model = Path(directory) / "model.json"
            model.write_text(json.dumps({
                "source": "ODSwap.asm", "function": "__getdrv"
            }), encoding="ascii")
            batch = turbo_link_batch([
                ("U1234567", "U1234567", 0, model)
            ])
            self.assertIn(
                "UT.OBJ U1234567.OBJ U123456A.OBJ", batch)

    def test_link_batch_uses_distinct_auxiliary_name_when_stem_ends_in_a(self):
        with tempfile.TemporaryDirectory() as directory:
            model = Path(directory) / "model.json"
            model.write_text(json.dumps({
                "source": "ODSwap.asm", "function": "__getems"
            }), encoding="ascii")
            batch = turbo_link_batch([
                ("UFC1BBAA", "UFC1BBAA", 0, model)
            ])
            self.assertIn(
                "UT.OBJ UFC1BBAA.OBJ UFC1BBAB.OBJ", batch)

    def test_merges_one_header_and_every_observation(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            first = root / "first.cov"
            second = root / "second.cov"
            output = root / "merged.cov"
            first.write_text(
                "OPENDOORS-UNIT-COVERAGE 1\nD 1 1 1 0 0\n", encoding="ascii")
            second.write_text(
                "OPENDOORS-UNIT-COVERAGE 1\nD 1 1 1 1 1\n", encoding="ascii")

            merge_coverage_reports([first, second], output)

            self.assertEqual(output.read_text(encoding="ascii"),
                "OPENDOORS-UNIT-COVERAGE 1\n"
                "D 1 1 1 0 0\nD 1 1 1 1 1\n")


if __name__ == "__main__":
    unittest.main()
