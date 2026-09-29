import hashlib
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from check import (cmake_first_party_sources, validate_configurations,
                   validate_proposal,
                   validate_release_coverage_policy,
                   validate_source_native_flags,
                   validate_turbo_shards,
                   validate_unit_workflow_policy)  # noqa: E402


class ConfigurationValidationTests(unittest.TestCase):
    def test_extracts_first_party_sources_from_both_build_manifests(self):
        modern = """\
set(OPENDOORS_SOURCES
    src/ODAuto.c
)
list(APPEND OPENDOORS_SOURCES src/ODFrame.c src/ODRes.rc)
"""
        dos = """\
set(OPENDOORS_DOS_SOURCES
    ${OPENDOORS_ROOT}/src/ODAuto.c
    ${OPENDOORS_ROOT}/src/ODDos.c
)
list(APPEND OPENDOORS_DOS_SOURCES
    \"${OPENDOORS_SOURCE_DIR}/OD32Foss.c\")
DEPENDS \"${OPENDOORS_SOURCE_DIR}/ODSwap.asm\"
add_executable(smoke \"${OPENDOORS_ROOT}/tests/link_smoke.c\")
"""
        self.assertEqual(cmake_first_party_sources(modern, dos), {
            "src/ODAuto.c", "src/ODFrame.c", "src/ODDos.c",
            "src/OD32Foss.c", "src/ODSwap.asm",
        })

    def test_validates_source_platform_native_flags(self):
        self.assertEqual(validate_source_native_flags({
            "sample.c": {"windows": ["-Wno-example"]}
        }), [])
        problems = validate_source_native_flags({
            "sample.c": {"unknown": ["-Wno-example"],
                         "windows": [""]}
        })
        self.assertIn("unknown platform", "\n".join(problems))
        self.assertIn("nonempty strings", "\n".join(problems))

    def test_accepts_named_configuration_options(self):
        test = {"platforms": ["windows"], "configurations": [{
            "name": "fallback-c89",
            "platforms": ["windows"],
            "defines": ["FEATURE=1", "SECOND"],
            "undefines": ["OTHER"],
            "native_flags": ["-Wno-example"],
            "turbo_late_defines": ["atol=utm_atoi"],
            "windows_fixture": "unit/fixtures/door32/door32.c",
            "turbo_tsr_fixture": "tests/fossil_tsr.asm",
            "dos_tsr_fixture": "tests/fossil_tsr.asm",
            "turbo_late_declarations": ["int utm_inp(unsigned);"],
            "provided_variables": ["environ"],
        }]}
        self.assertEqual(validate_configurations(1, test), [])

    def test_rejects_configuration_platform_outside_its_owner(self):
        test = {"platforms": ["unix"], "configurations": [{
            "name": "windows-only", "platforms": ["windows"]
        }]}
        self.assertIn("platforms exceed", "\n".join(
            validate_configurations(1, test)))

    def test_rejects_an_owner_platform_with_no_configuration(self):
        test = {"platforms": ["unix", "windows"], "configurations": [{
            "name": "windows-only", "platforms": ["windows"]
        }]}
        self.assertIn("no configuration for: unix", "\n".join(
            validate_configurations(1, test)))

    def test_rejects_an_unsafe_windows_fixture_path(self):
        test = {"configurations": [{
            "name": "fixture",
            "windows_fixture": "../door32.c",
        }]}
        self.assertIn("invalid windows fixture", "\n".join(
            validate_configurations(1, test)))

    def test_rejects_an_unsafe_turbo_tsr_fixture_path(self):
        test = {"configurations": [{
            "name": "fixture",
            "turbo_tsr_fixture": "/tmp/fossil.asm",
        }]}
        self.assertIn("invalid Turbo TSR fixture", "\n".join(
            validate_configurations(1, test)))

    def test_rejects_empty_configuration_list(self):
        self.assertIn("must be a nonempty list", "\n".join(
            validate_configurations(1, {"configurations": []})))

    def test_rejects_invalid_provided_variables(self):
        test = {"configurations": [{
            "name": "runtime",
            "provided_variables": ["not-an-identifier"],
        }]}
        self.assertIn("invalid provided variable", "\n".join(
            validate_configurations(1, test)))

    def test_rejects_duplicate_and_unsafe_names(self):
        test = {"configurations": [
            {"name": "same name"},
            {"name": "same name"},
        ]}
        result = "\n".join(validate_configurations(1, test))
        self.assertIn("invalid name", result)
        self.assertIn("duplicate name", result)

    def test_rejects_conflicting_macro_operations(self):
        test = {"configurations": [{
            "name": "conflict",
            "defines": ["FEATURE=1"],
            "undefines": ["FEATURE"],
        }]}
        self.assertIn("both defines and undefines FEATURE", "\n".join(
            validate_configurations(1, test)))

    def test_rejects_invalid_option_shapes_and_unknown_keys(self):
        test = {"configurations": [{
            "name": "bad",
            "defines": "FEATURE=1",
            "undefines": ["NOT-A-MACRO"],
            "native_flags": [1],
            "mystery": [],
        }]}
        result = "\n".join(validate_configurations(1, test))
        self.assertIn("defines must be a list", result)
        self.assertIn("invalid undefine", result)
        self.assertIn("native_flags must contain nonempty strings", result)
        self.assertIn("unknown keys: mystery", result)


class WaiverProposalTests(unittest.TestCase):
    def setUp(self):
        self.temporary_directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary_directory.cleanup)
        self.root = Path(self.temporary_directory.name)
        source = b"sample\n"
        (self.root / "ODBlock.c").write_bytes(source)
        self.expected = {("ODBlock.c", "od_scroll"): {
            "unix", "windows", "dos16", "dos32"
        }}
        self.proposal = {
            "id": "sample",
            "status": "proposed",
            "source": "ODBlock.c",
            "function": "od_scroll",
            "platforms": ["unix"],
            "start_line": 1,
            "end_line": 1,
            "kinds": ["branch"],
            "reason": "reason",
            "evidence": "evidence",
            "source_sha256": hashlib.sha256(source).hexdigest(),
        }

    def test_accepts_well_formed_proposal(self):
        self.assertEqual(validate_proposal(
            1, self.proposal, self.expected, self.root), [])

    def test_accepts_a_line_coverage_proposal(self):
        self.proposal["kinds"] = ["line"]
        self.assertEqual(validate_proposal(
            1, self.proposal, self.expected, self.root), [])

    def test_rejects_approved_status_in_proposal_file(self):
        self.proposal["status"] = "approved"
        self.assertIn("status must be proposed", "\n".join(
            validate_proposal(1, self.proposal, self.expected, self.root)))


class ReleaseCoveragePolicyTests(unittest.TestCase):
    def test_rejects_proposed_waiver_flag_from_release_workflow(self):
        self.assertEqual(validate_release_coverage_policy(
            "python unit/tools/run.py --coverage"), [])
        self.assertIn("must not allow proposed coverage waivers", "\n".join(
            validate_release_coverage_policy(
                "python unit/tools/run.py "
                "--allow-proposed-coverage-waivers")))

    def test_requires_strict_full_reusable_release_gate(self):
        unit = ("on:\n  push:\n  pull_request:\n  workflow_call:\ncheck.py\n"
                "unix_matrix:\n"
                "matrix: ${{ fromJSON(needs.select.outputs.unix_matrix) }}\n"
                "watcom_matrix:\n"
                "matrix: ${{ fromJSON(needs.select.outputs.watcom_matrix) }}\n"
                "runs-on: macos-15\n"
                "unit-macos-${{ matrix.platform }}-coverage\n")
        release = ("uses: ./.github/workflows/unit-tests.yml\n"
                   "with:\n  full: true\n"
                   "  collect:\n    needs:\n      - unit-tests\n")
        self.assertEqual(validate_unit_workflow_policy(unit, release), [])
        self.assertTrue(validate_unit_workflow_policy(
            unit.replace("check.py", "check.py --allow-incomplete"), release))
        self.assertTrue(validate_unit_workflow_policy(
            unit, release.replace("full: true", "full: false")))
        self.assertTrue(validate_unit_workflow_policy(
            unit, release.replace("  - unit-tests\n", "")))

    def test_requires_an_exact_dynamic_unix_variant_matrix(self):
        unit = ("on:\n  push:\n  pull_request:\n  workflow_call:\ncheck.py\n"
                "unix_matrix: selector-output\n"
                "matrix: ${{ fromJSON(needs.select.outputs.unix_matrix) }}\n"
                "watcom_matrix:\n"
                "matrix: ${{ fromJSON(needs.select.outputs.watcom_matrix) }}\n"
                "runs-on: macos-15\n"
                "unit-macos-${{ matrix.platform }}-coverage\n")
        release = ("uses: ./.github/workflows/unit-tests.yml\n"
                   "with:\n  full: true\n"
                   "  collect:\n    needs:\n      - unit-tests\n")
        self.assertEqual(validate_unit_workflow_policy(unit, release), [])
        self.assertIn("exact Unix variant matrix", "\n".join(
            validate_unit_workflow_policy(
                unit.replace(
                    "matrix: ${{ fromJSON(needs.select.outputs.unix_matrix) }}",
                    "matrix:\n  platform: [unix, windows]"),
                release)))

    def test_requires_an_exact_dynamic_watcom_variant_matrix(self):
        unit = ("on:\n  push:\n  pull_request:\n  workflow_call:\ncheck.py\n"
                "unix_matrix:\n"
                "matrix: ${{ fromJSON(needs.select.outputs.unix_matrix) }}\n"
                "watcom_matrix: selector-output\n"
                "matrix: ${{ fromJSON(needs.select.outputs.watcom_matrix) }}\n"
                "runs-on: macos-15\n"
                "unit-macos-${{ matrix.platform }}-coverage\n")
        release = ("uses: ./.github/workflows/unit-tests.yml\n"
                   "with:\n  full: true\n"
                   "  collect:\n    needs:\n      - unit-tests\n")
        self.assertEqual(validate_unit_workflow_policy(unit, release), [])
        self.assertIn("exact Watcom variant matrix", "\n".join(
            validate_unit_workflow_policy(
                unit.replace(
                    "matrix: ${{ fromJSON(needs.select.outputs.watcom_matrix) }}",
                    "matrix:\n  include: [dos16, dos32]"),
                release)))

    def test_requires_native_and_llvm_coverage_on_macos(self):
        unit = ("on:\n  push:\n  pull_request:\n  workflow_call:\ncheck.py\n"
                "unix_matrix:\n"
                "matrix: ${{ fromJSON(needs.select.outputs.unix_matrix) }}\n"
                "watcom_matrix:\n"
                "matrix: ${{ fromJSON(needs.select.outputs.watcom_matrix) }}\n"
                "runs-on: macos-15\n"
                "unit-macos-${{ matrix.platform }}-coverage\n")
        release = ("uses: ./.github/workflows/unit-tests.yml\n"
                   "with:\n  full: true\n"
                   "  collect:\n    needs:\n      - unit-tests\n")
        self.assertEqual(validate_unit_workflow_policy(unit, release), [])
        self.assertIn("macOS unit coverage", "\n".join(
            validate_unit_workflow_policy(
                unit.replace("runs-on: macos-15\n", ""), release)))


class TurboShardValidationTests(unittest.TestCase):
    def test_accepts_an_explicit_multi_shard_count(self):
        self.assertEqual(validate_turbo_shards(2, {"turbo_shards": 4}), [])

    def test_rejects_non_integer_and_single_shard_values(self):
        self.assertTrue(validate_turbo_shards(2, {"turbo_shards": "4"}))
        self.assertTrue(validate_turbo_shards(2, {"turbo_shards": 1}))


if __name__ == "__main__":
    unittest.main()
