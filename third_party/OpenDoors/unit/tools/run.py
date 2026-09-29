#!/usr/bin/env python3
"""Generate, build, run, and measure isolated OpenDoors unit tests."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
import shutil
import subprocess
import sys
from pathlib import Path

from clang_model import POSIX_API_FLAG
from inventory import ROOT


TESTS_FILE = ROOT / "unit" / "tests.json"
FRAMEWORK = ROOT / "unit" / "framework" / "ut.c"
WAIVERS_FILE = ROOT / "unit" / "coverage-waivers.json"
PROPOSALS_FILE = ROOT / "unit" / "coverage-waiver-proposals.json"
DEFAULT_DOS_TIMEOUT = 900


def default_build_path(platform: str, toolchain: str,
                       windows_architecture: str | None) -> Path:
    """Return a collision-free default for one target configuration."""
    suffix = platform
    if platform == "windows":
        suffix += "-" + (windows_architecture or "x64")
    elif platform.startswith("dos"):
        suffix += "-" + toolchain
    return ROOT / "build" / ("unit-" + suffix)


def platform_defines(platform: str) -> list[str]:
    definitions = {
        "unix": ["-D__unix__", POSIX_API_FLAG, "-U_FORTIFY_SOURCE"],
        # A renamed CRT declaration must not retain dllimport when its test
        # supplies the corresponding mock definition.
        "windows": ["-D_CRTIMP=", "-DDECLSPEC_IMPORT=",
                    "-D_CRT_SECURE_NO_WARNINGS"],
        "dos16": ["-U__unix__", "-U__unix", "-Uunix", "-U__FreeBSD__",
                  "-Dfar=", "-D__LARGE__", "-D__WATCOMC__=1300",
                  "-D__I86__", "-D_M_IX86=100", "-D_M_I86=1",
                  "-DOPENDOORS_HAVE_VSNPRINTF=1"],
        "dos32": ["-U__unix__", "-U__unix", "-Uunix", "-U__FreeBSD__",
                  "-D__WATCOMC__=1300", "-D__386__", "-D__FLAT__",
                  "-D_M_IX86=386", "-D__watcall=", "-D__cdecl=",
                  "-DOPENDOORS_HAVE_VSNPRINTF=1"],
    }
    return list(definitions[platform])


def analyzer_platform_flags(platform: str,
                            native_flags: list[str],
                            compiler: str = "",
                            windows_architecture: str | None = None,
                            windows_abi: str = "gnu") -> list[str]:
    flags = list(native_flags)
    if platform == "windows":
        compiler_name = Path(compiler).name.lower()
        architecture = "i686" if (windows_architecture == "x86" or
            windows_architecture is None and compiler_name.startswith(
                ("i386", "i486", "i586", "i686"))) else "x86_64"
        target = (f"{architecture}-pc-windows-msvc" if windows_abi == "msvc"
                  else f"{architecture}-w64-windows-gnu")
        flags.extend(["-target", target])
    elif platform == "dos32":
        flags.append("-m32")
    return flags


def language_standard_flag(platform: str) -> str:
    """Return the production language contract for a modeled platform."""
    if platform in {"unix", "windows"}:
        return "-std=c99"
    return "-std=c89"


def native_compile_flags(platform: str) -> list[str]:
    """Return warnings and language mode for a native modern target."""
    if platform not in {"unix", "windows"}:
        return []
    return [language_standard_flag(platform), "-Wall", "-Wextra", "-Werror",
            "-Wno-unused-function", "-Wno-unused-variable",
            "-Wno-unused-parameter",
            "-Wno-overlength-strings",
            "-Wno-missing-field-initializers"]


def executable_suffix(platform: str) -> str:
    if platform.startswith("dos"):
        return ".EXE"
    if platform == "windows":
        return ".exe"
    return ""


def unit_test_label(source: str, function: str) -> str:
    """Return a filesystem-safe descriptive stem for a non-DOS unit."""
    return Path(source).stem + "-" + function


def llvm_coverage_supported(platform: str,
                            native_windows: bool = os.name == "nt") -> bool:
    """Return whether this host can execute the target's LLVM coverage run."""
    return ((platform == "unix" and not native_windows) or
            (platform == "windows" and native_windows))


def native_test_arguments(executable: Path, report: Path,
                          wine: Path | None = None) -> list[str]:
    arguments = [str(executable), str(report)]
    if wine is not None:
        arguments.insert(0, str(wine))
    return arguments


def windows_batch(executables: list[tuple[str, str]]) -> str:
    """Run isolated Windows units synchronously in one CMD process."""
    lines = ["@ECHO OFF"]
    for index, (executable, report) in enumerate(executables):
        stem = Path(executable).stem
        prefix = "%~dp0"
        lines.extend([
            f'"{prefix}{executable}" "{prefix}{report}" '
            f'>"{prefix}{stem}.OUT" 2>&1',
            f"IF NOT ERRORLEVEL 1 GOTO OK{index}",
            f'ECHO FAIL>"{prefix}{stem}.BAD"',
            f":OK{index}",
        ])
    lines.extend([
        'ECHO DONE>"%~dp0UTDONE.OK"',
        "EXIT /B 0",
    ])
    return "\r\n".join(lines) + "\r\n"


def windows_llvm_batch(executables: list[tuple[str, str]]) -> str:
    """Run instrumented Windows units with one profile file per executable."""
    lines = ["@ECHO OFF"]
    for index, (executable, profile) in enumerate(executables):
        stem = Path(executable).stem
        prefix = "%~dp0"
        lines.extend([
            f'SET "LLVM_PROFILE_FILE={prefix}{profile}"',
            f'"{prefix}{executable}" >"{prefix}{stem}.OUT" 2>&1',
            f"IF NOT ERRORLEVEL 1 GOTO OK{index}",
            f'ECHO FAIL>"{prefix}{stem}.BAD"',
            f":OK{index}",
        ])
    lines.extend([
        'ECHO DONE>"%~dp0UTLLVM.OK"',
        "EXIT /B 0",
    ])
    return "\r\n".join(lines) + "\r\n"


def windows_batch_arguments(batch: Path,
                            wine: Path | None = None) -> list[str]:
    """Return one native or Wine CMD invocation for a Windows unit batch."""
    resolved = str(batch.resolve())
    if wine is not None:
        resolved = "Z:" + resolved.replace("/", "\\")
        return [str(wine), "cmd.exe", "/d", "/c", resolved]
    return ["cmd.exe", "/d", "/c", resolved]


def windows_fixture_arguments(compiler: str, source: Path,
                              output: Path,
                              architecture_flags: list[str] | None = None
                              ) -> list[str]:
    """Build a test-only Windows DLL beside its isolated executable."""
    compiler_name = Path(compiler).name.lower()
    export_flags = (["-Wl,--kill-at"] if
        "mingw" in compiler_name or compiler_name.startswith("gcc") else [])
    return [compiler, "-std=c99", "-Wall", "-Wextra", "-Werror",
            *(architecture_flags or []), "-shared", *export_flags,
            str(source), "-o", str(output)]


def dos_short_stem(source: str, function: str, configuration: str) -> str:
    identity = "\0".join((source, function, configuration)).encode("utf-8")
    return "U" + hashlib.sha256(identity).hexdigest()[:7].upper()


def dos_aux_stem(stem: str) -> str:
    """Return a distinct 8.3 stem for a unit's auxiliary assembly object."""
    return stem[:7] + ("B" if stem[-1:].upper() == "A" else "A")


def dos_batch(executables: list[tuple[str, str]],
              tsr_fixtures: list[str] | None = None) -> str:
    lines = ["@ECHO OFF"]
    lines.extend(tsr_fixtures or [])
    for index, (executable, report) in enumerate(executables):
        stem = Path(executable).stem
        failure = stem + ".BAD"
        output = stem + ".OUT"
        lines.extend([f"{executable} {report} >{output}",
                      f"IF NOT ERRORLEVEL 1 GOTO OK{index}",
                      f"ECHO FAIL>{failure}", f":OK{index}"])
    lines.extend(["ECHO DONE>UTDONE.OK", "EXIT"])
    return "\r\n".join(lines) + "\r\n"


def dosbox_arguments(dosbox: Path, build: Path, batch: Path,
                     configuration: Path) -> list[str]:
    """Build a hermetic DOSBox command which terminates after the batch."""
    return [str(dosbox.resolve()), "-noconsole", "-exit", "-conf",
            str(configuration.resolve()), "-c",
            f'mount c "{build.resolve()}"', "-c", "c:", "-c", batch.name]


def watcom_environment(base: dict[str, str], watcom: str) -> dict[str, str]:
    environment = base.copy()
    # Open Watcom predates X11 conventions and interprets DISPLAY as one of
    # its own tool settings.  DOSBox, but not the compiler, should inherit it.
    environment.pop("DISPLAY", None)
    environment.setdefault("INCLUDE", str(Path(watcom) / "h"))
    return environment


def watcom_target_flags(toolchain: str) -> list[str]:
    """Select the DOS compiler target and matching linker system."""
    if toolchain == "watcom16":
        return ["-bcl=dos"]
    return ["-bt=dos", "-l=dos4g"]


def assemble_dos_tsr_fixtures(sources: list[Path], build: Path,
                              watcom: str) -> list[str]:
    """Assemble test-only real-mode TSRs to install before DOS unit cases."""
    assembler = Path(watcom) / "binl" / "wasm"
    linker = Path(watcom) / "binl" / "wlink"
    if not assembler.is_file() or not linker.is_file():
        raise RuntimeError("Open Watcom assembler or linker is missing")
    names = []
    for source in sources:
        stem = "".join(character for character in source.stem.upper()
                       if character.isalnum())[:8]
        object_file = build / (stem + ".OBJ")
        executable = build / (stem + ".COM")
        command([str(assembler), "-q", "-0", f"-fo={object_file}",
                 str(source)])
        command([str(linker), "format", "dos", "com", "option", "quiet",
                 "option", "nodefaultlibs", "name", str(executable),
                 "file", str(object_file)])
        names.append(executable.name)
    return names


def watcom_ast_compatibility_flags(watcom: str) -> list[str]:
    """Return Clang shims for parsing Watcom headers without changing ABI types."""
    return [
        "-fdeclspec", "-fasm-blocks", "-Wno-ignored-attributes",
        "-Wno-ignored-pragmas", "-Dinterrupt=", "-Dfar=",
        "-D__near=", "-D__far=", "-D__huge=", "-D__interrupt=",
        "-D__based(x)=", "-D__watcall=", "-D__int64=long long",
        "-D__builtin_alloca=watcom_alloca",
        "-isystem", str(Path(watcom) / "h"),
    ]


def coverage_waived(waivers: list[dict[str, object]], source: str,
                     function: str, platform: str, kind: str, line: int,
                     conditions: int | None = None) -> bool:
    kinds = {kind}
    if kind == "mcdc" and conditions == 1:
        kinds.add("branch")
    return any(item["source"] == source and item["function"] == function and
               item["platform"] == platform and item["kind"] in kinds and
               item["start_line"] <= line <= item["end_line"]
               for item in waivers)


def coverage_disposition(
        waivers: list[dict[str, object]], source: str, function: str,
        platform: str, kind: str, line: int,
        conditions: int | None = None) -> tuple[bool, bool]:
    """Return whether a site has an approved and/or proposed waiver."""
    matching = [item for item in waivers
                if coverage_waived([item], source, function, platform, kind,
                                    line, conditions)]
    return (any(not item.get("proposed", False) for item in matching),
            any(item.get("proposed", False) for item in matching))


def effective_coverage_waivers(
        approved: list[dict[str, object]],
        proposal_document: dict[str, object],
        allow_proposed: bool) -> list[dict[str, object]]:
    """Return approved waivers plus optionally expanded proposal records."""
    result = list(approved)
    if not allow_proposed:
        return result
    for proposal in proposal_document.get("proposals", []):
        if proposal.get("status") != "proposed":
            continue
        for platform in proposal["platforms"]:
            for kind in proposal["kinds"]:
                result.append({
                    "source": proposal["source"],
                    "function": proposal["function"],
                    "platform": platform,
                    "kind": kind,
                    "start_line": proposal["start_line"],
                    "end_line": proposal["end_line"],
                    "proposed": True,
                    "proposal_id": proposal["id"],
                })
    return result


def load_coverage_waivers(allow_proposed: bool) -> list[dict[str, object]]:
    approved = json.loads(WAIVERS_FILE.read_text(
        encoding="utf-8"))["waivers"]
    proposals = json.loads(PROPOSALS_FILE.read_text(encoding="utf-8"))
    return effective_coverage_waivers(approved, proposals, allow_proposed)


def llvm_branch_waived(waivers: list[dict[str, object]], source: str,
                        function: str, platform: str, line: int) -> bool:
    return (coverage_waived(waivers, source, function, platform, "branch",
                             line) or
            coverage_waived(waivers, source, function, platform, "mcdc",
                             line))


def original_source_line(generated_lines: list[str], generated_line: int,
                         source: str) -> int | None:
    """Map a physical generated line through the active C #line directives."""
    current_file = None
    next_line = 1
    directive = re.compile(r'^\s*#line\s+(\d+)(?:\s+"([^"]+)")?')
    for physical, text in enumerate(generated_lines, 1):
        match = directive.match(text)
        if match is not None:
            next_line = int(match.group(1))
            if match.group(2) is not None:
                current_file = match.group(2)
            if physical == generated_line:
                return None
            continue
        if physical == generated_line:
            return next_line if current_file == source else None
        next_line += 1
    return None


def map_llvm_record_lines(records: list, generated_lines: list[str],
                          source: str) -> list:
    """Return relevant LLVM records with physical lines mapped to the source."""
    result = []
    for record in records:
        # File zero is the generated translation itself.  Other file IDs are
        # virtual files used for macro expansions (or included headers), whose
        # decisions are not part of the isolated source function's model.
        if (len(record) > 6 and isinstance(record[6], int) and
                record[6] != 0):
            continue
        # A zero-width region has no source expression.  LLVM 22 produces
        # these for branches internal to macros such as FD_ZERO().
        if record[0] == record[2] and record[1] == record[3]:
            continue
        mapped = original_source_line(generated_lines, record[0], source)
        if mapped is None:
            continue
        # LLVM 22 emits an unreachable branch for the loop backedge in the
        # conventional statement-macro wrapper.  It cannot execute and is not
        # a source decision, so do not require tests to cover it.
        generated_text = generated_lines[record[0] - 1]
        if re.fullmatch(r'\s*}\s*while\s*\(\s*0\s*\)\s*;?\s*',
                        generated_text):
            continue
        copy = list(record)
        copy[0] = mapped
        result.append(copy)
    return result


def llvm_line_coverage(show: str, generated_lines: list[str],
                       source: str,
                       source_lines: list[str] | None = None
                       ) -> tuple[list[int], list[int]]:
    """Map LLVM's executable line counts back to the production source."""
    coverage: dict[int, bool] = {}
    for text in show.splitlines():
        match = re.match(r"^\s*([0-9]+)\|([^|]*)\|", text)
        if match is None:
            continue
        count = match.group(2).strip()
        if not count:
            continue
        mapped = original_source_line(
            generated_lines, int(match.group(1)), source)
        if mapped is None:
            continue
        if source_lines is not None and mapped <= len(source_lines):
            source_text = source_lines[mapped - 1].strip()
            if (source_text.startswith("#") or
                    re.fullmatch(r"[{}]+", source_text) or
                    source_text == "break;"):
                continue
        coverage[mapped] = coverage.get(mapped, False) or count != "0"
    covered = sorted(line for line, executed in coverage.items() if executed)
    missed = sorted(line for line, executed in coverage.items() if not executed)
    return covered, missed


def command(arguments: list[str], environment: dict[str, str] | None = None,
            cwd: Path = ROOT) -> None:
    print("+", " ".join(arguments), flush=True)
    subprocess.run(arguments, cwd=cwd, check=True, env=environment)


def run_step(failures: list[str], label: str, action) -> bool:
    try:
        action()
        return True
    except (subprocess.CalledProcessError, subprocess.TimeoutExpired,
            RuntimeError, OSError,
            ValueError) as error:
        message = f"{label}: {error}"
        failures.append(message)
        print("FAIL", message, file=sys.stderr, flush=True)
        return False


def expand_configurations(test: dict[str, object],
                          platform: str | None = None):
    configurations = test.get("configurations") or [{"name": "default"}]
    for configuration in configurations:
        if (platform is not None and configuration.get("platforms") is not None
                and platform not in configuration["platforms"]):
            continue
        expanded = dict(test)
        expanded["configuration"] = dict(configuration)
        yield expanded


def preprocessor_flags(configuration: dict[str, object],
                       watcom: bool = False) -> list[str]:
    define_prefix = "-d" if watcom else "-D"
    undefine_prefix = "-u" if watcom else "-U"
    return ([undefine_prefix + name
             for name in configuration.get("undefines", [])] +
            [define_prefix + value
             for value in configuration.get("defines", [])])


def native_coverage(report: Path, model_path: Path, platform: str,
                    output: Path, allow_proposed: bool = False) -> None:
    model = json.loads(model_path.read_text(encoding="utf-8"))
    observations: dict[int, list[tuple[int, int, int]]] = {}
    observed_branches: set[int] = set()
    assembly_observations: dict[int, set[int]] = {}
    for line in report.read_text(encoding="ascii").splitlines():
        fields = line.split()
        if (fields[:1] == ["A"] and len(fields) == 3 and
                int(fields[2]) in {0, 1}):
            assembly_observations.setdefault(int(fields[1]), set()).add(
                int(fields[2]))
            continue
        if fields[:1] == ["B"] and len(fields) == 3 and int(fields[2]) != 0:
            observed_branches.add(int(fields[1]))
            continue
        if fields[:1] != ["D"] or len(fields) != 6:
            continue
        decision = int(fields[1])
        observations.setdefault(decision, []).append(
            (int(fields[3]), int(fields[4]), int(fields[5])))
    waivers = load_coverage_waivers(allow_proposed)

    def disposition(kind: str, line: int,
                    conditions: int | None = None) -> tuple[bool, bool]:
        return coverage_disposition(
            waivers, model["source"], model["function"], platform, kind,
            line, conditions)

    failures = []
    decisions = []
    branches = []
    assembly_branches = []
    for item in model.get("branches", []):
        item_waived, item_proposed = disposition("branch", item["line"])
        observed = item["id"] in observed_branches
        if not observed and not (item_waived or item_proposed):
            failures.append(
                f"switch branch {item['id']} line {item['line']} was not executed")
        branches.append({**item, "observed": observed, "waived": item_waived,
                         "proposed_waiver": item_proposed})
    for item in model.get("assembly_branches", []):
        item_waived, item_proposed = disposition(
            "assembly-branch", item["line"])
        outcomes = sorted(assembly_observations.get(item["id"], set()))
        if outcomes != [0, 1] and not (item_waived or item_proposed):
            failures.append(
                f"assembly branch {item['id']} line {item['line']} "
                "lacks both outcomes")
        assembly_branches.append({
            **item,
            "outcomes": outcomes,
            "waived": item_waived,
            "proposed_waiver": item_proposed,
        })
    for item in model["decisions"]:
        vectors = list(dict.fromkeys(observations.get(item["id"], [])))
        outcomes = sorted({vector[2] for vector in vectors})
        condition_mask = (1 << item["conditions"]) - 1
        mcc_combinations = sorted({
            vector[1] & condition_mask for vector in vectors
            if vector[0] & condition_mask == condition_mask
        })
        pairs = []
        for condition in range(item["conditions"]):
            bit = 1 << condition
            covered = False
            for left_index, left in enumerate(vectors):
                for right in vectors[left_index + 1:]:
                    if not (left[0] & bit and right[0] & bit):
                        continue
                    if bool(left[1] & bit) == bool(right[1] & bit):
                        continue
                    if left[2] == right[2]:
                        continue
                    other = (left[0] & right[0]) & ~bit
                    if (left[1] & other) == (right[1] & other):
                        covered = True
                        break
                if covered:
                    break
            pairs.append(covered)
        branch_waived, branch_proposed = disposition(
            "branch", item["line"])
        mcdc_waived, mcdc_proposed = disposition(
            "mcdc", item["line"], item["conditions"])
        if outcomes != [0, 1] and not (branch_waived or branch_proposed):
            failures.append(f"decision {item['id']} line {item['line']} lacks both outcomes")
        if not all(pairs) and not (mcdc_waived or mcdc_proposed):
            failures.append(f"decision {item['id']} line {item['line']} lacks MC/DC")
        decisions.append({
            **item,
            "vectors": [{"evaluated": value[0], "values": value[1],
                         "result": value[2]} for value in vectors],
            "outcomes": outcomes,
            "mcdc_pairs": pairs,
            "branch_waived": branch_waived,
            "branch_proposed_waiver": branch_proposed,
            "mcdc_waived": mcdc_waived,
            "mcdc_proposed_waiver": mcdc_proposed,
            "theoretical_combinations": 1 << item["conditions"],
            "mcc_observed_combinations": mcc_combinations,
            "mcc_observed_count": len(mcc_combinations),
            "mcc_complete": len(mcc_combinations) == 1 << item["conditions"],
        })
    output.write_text(json.dumps({
        "version": 1,
        "source": model["source"],
        "function": model["function"],
        "platform": platform,
        "decisions": decisions,
        "branches": branches,
        "assembly_branches": assembly_branches,
    }, indent=2) + "\n", encoding="utf-8")
    if failures:
        raise RuntimeError("native coverage failed: " + "; ".join(failures))


def llvm_coverage(executable: Path, profile: Path, source: str, function: str,
                  platform: str, generated: Path, output: Path,
                  allow_proposed: bool = False) -> None:
    merged = profile.with_suffix(".profdata")
    command(["llvm-profdata", "merge", "-sparse", str(profile),
             "-o", str(merged)])
    process = subprocess.run(
        ["llvm-cov", "export", str(executable),
         f"-instr-profile={merged}", "-skip-expansions"],
        cwd=ROOT, check=True, stdout=subprocess.PIPE, text=True)
    document = json.loads(process.stdout)
    expected_name = "utt_" + function
    matches = [item for item in document["data"][0]["functions"]
               if item["name"] == expected_name or
               item["name"].endswith(":" + expected_name)]
    if len(matches) != 1:
        raise RuntimeError(
            f"coverage contains {len(matches)} definitions for utt_{function}")
    record = matches[0]
    generated_lines = generated.read_text(encoding="latin-1").splitlines()
    source_path = ROOT / source
    source_lines = (source_path.read_text(encoding="latin-1").splitlines()
                    if source_path.is_file() else None)
    show = subprocess.run(
        ["llvm-cov", "show", str(executable), f"-instr-profile={merged}",
         f"-name=utt_{function}", "-show-line-counts", "-show-mcdc",
         "-use-color=0"], cwd=ROOT, check=True,
        stdout=subprocess.PIPE, text=True).stdout
    covered_lines, all_missed_lines = llvm_line_coverage(
        show, generated_lines, source, source_lines)
    waivers = load_coverage_waivers(allow_proposed)

    def disposition(kind: str, line: int,
                    conditions: int | None = None) -> tuple[bool, bool]:
        return coverage_disposition(
            waivers, source, function, platform, kind, line,
            conditions)

    def branch_disposition(line: int) -> tuple[bool, bool]:
        branch = disposition("branch", line)
        mcdc = disposition("mcdc", line)
        return (branch[0] or mcdc[0], branch[1] or mcdc[1])

    branches = map_llvm_record_lines(
        record["branches"], generated_lines, source)
    all_missed = [branch for branch in branches
                  if branch[4] == 0 or branch[5] == 0]
    missed = [branch for branch in all_missed
              if not any(branch_disposition(branch[0]))]
    waived_branches = [branch for branch in all_missed
                       if branch_disposition(branch[0])[0]]
    proposed_branches = [branch for branch in all_missed
                         if branch_disposition(branch[0])[1]]
    raw_mcdc = record.get("mcdc_records", [])
    mcdc = map_llvm_record_lines(raw_mcdc, generated_lines, source)
    model = json.loads(
        generated.with_suffix(".model.json").read_text(encoding="utf-8"))
    absent_mcdc = missing_llvm_mcdc_records(
        model, raw_mcdc, generated_lines, source)
    absent_mcdc_unwaived = []
    absent_mcdc_waived = []
    absent_mcdc_proposed = []
    for decision in absent_mcdc:
        approved, proposed = coverage_disposition(
            waivers, source, function, platform, "mcdc",
            decision["line"], decision["conditions"])
        if approved:
            absent_mcdc_waived.append(decision)
        elif proposed:
            absent_mcdc_proposed.append(decision)
        else:
            absent_mcdc_unwaived.append(decision)
    all_missed_mcdc = [decision for decision in mcdc
                       if not all(decision[-1])]
    missed_mcdc = [decision for decision in all_missed_mcdc
                   if not any(disposition(
                       "mcdc", decision[0], len(decision[-1])))]
    waived_mcdc = [decision for decision in all_missed_mcdc
                   if disposition("mcdc", decision[0],
                                  len(decision[-1]))[0]]
    proposed_mcdc = [decision for decision in all_missed_mcdc
                     if disposition("mcdc", decision[0],
                                    len(decision[-1]))[1]]
    missed_lines = [line for line in all_missed_lines
                    if not any(disposition("line", line))]
    waived_lines = [line for line in all_missed_lines
                     if disposition("line", line)[0]]
    proposed_lines = [line for line in all_missed_lines
                       if disposition("line", line)[1]]
    result = {
        "version": 1,
        "function": function,
        "executions": record["count"],
        "lines": len(covered_lines) + len(all_missed_lines),
        "covered_lines": covered_lines,
        "missed_lines": missed_lines,
        "waived_lines": waived_lines,
        "proposed_waiver_lines": proposed_lines,
        "branches": len(record["branches"]) * 2,
        "missed_branches": missed,
        "waived_branches": waived_branches,
        "proposed_waiver_branches": proposed_branches,
        "mcdc": mcdc,
        "missed_mcdc": missed_mcdc,
        "waived_mcdc": waived_mcdc,
        "proposed_waiver_mcdc": proposed_mcdc,
        "missing_mcdc_records": absent_mcdc_unwaived,
        "waived_missing_mcdc_records": absent_mcdc_waived,
        "proposed_waiver_missing_mcdc_records": absent_mcdc_proposed,
    }
    output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    mcc_path = output.with_suffix(".mcc.txt")
    summary = [
        "OpenDoors multiple-condition coverage report",
        f"Function: {function}",
        "",
        "Theoretical combinations count every Boolean assignment (2^N).",
        "LLVM marks short-circuited conditions '-' rather than inventing a value.",
        "MC/DC is gated; complete multiple-condition coverage is reported only.",
        "",
    ]
    for index, decision in enumerate(mcdc, 1):
        condition_count = len(decision[-1])
        covered = sum(1 for value in decision[-1] if value)
        summary.append(
            f"Decision {index}: {condition_count} conditions, "
            f"{1 << condition_count} theoretical combinations, "
            f"{covered}/{condition_count} MC/DC independence pairs")
    if not mcdc:
        summary.append("No compound decisions.")
    summary.extend(["", "LLVM observed vectors", "", show])
    mcc_path.write_text("\n".join(summary), encoding="utf-8")
    if record["count"] == 0:
        raise RuntimeError(f"utt_{function} did not execute")
    if missed_lines:
        raise RuntimeError(
            f"utt_{function} missed {len(missed_lines)} lines: " +
            ", ".join(str(line) for line in missed_lines))
    if missed:
        raise RuntimeError(
            f"utt_{function} missed {len(missed)} branch decisions")
    if missed_mcdc:
        raise RuntimeError(
            f"utt_{function} missed MC/DC in {len(missed_mcdc)} decisions")
    if absent_mcdc_unwaived:
        raise RuntimeError(
            f"utt_{function} lacks LLVM MC/DC records for "
            f"{len(absent_mcdc_unwaived)} compound decisions")


def missing_llvm_mcdc_records(model: dict[str, object],
                              records: list,
                              generated_lines: list[str],
                              source: str) -> list[dict[str, object]]:
    """Return modeled compound decisions absent from LLVM's MC/DC map."""
    available = []
    for record in records:
        if (len(record) > 6 and isinstance(record[6], int) and
                record[6] != 0):
            continue
        line = original_source_line(generated_lines, record[0], source)
        if line is not None:
            available.append(line)
    missing = []
    for decision in model.get("decisions", []):
        if decision["conditions"] <= 1:
            continue
        if decision["line"] in available:
            available.remove(decision["line"])
        else:
            missing.append({key: decision[key]
                            for key in ("id", "line", "conditions")})
    return missing


def selection_owners(path: Path) -> set[tuple[str, str]]:
    """Load the selector's exact source/function ownership pairs."""
    document = json.loads(path.read_text(encoding="utf-8"))
    sources = document.get("sources")
    if not isinstance(sources, dict):
        raise ValueError("unit selection must contain a sources object")
    owners = set()
    for source, functions in sources.items():
        if (not isinstance(source, str) or
                not isinstance(functions, list) or
                not all(isinstance(function, str) for function in functions)):
            raise ValueError("unit selection sources must map to function lists")
        owners.update((source, function) for function in functions)
    return owners


def selected_tests(platform: str, sources: set[str], functions: set[str],
                   start_at: tuple[str, str] | None = None,
                   document: dict[str, object] | None = None,
                   owners: set[tuple[str, str]] | None = None):
    if document is None:
        document = json.loads(TESTS_FILE.read_text(encoding="utf-8"))
    started = start_at is None
    for test in document["tests"]:
        if not started:
            if (test["source"], test["function"]) != start_at:
                continue
            started = True
        if platform not in test["platforms"]:
            continue
        if owners is not None and (test["source"], test["function"]) not in owners:
            continue
        if sources and test["source"] not in sources:
            continue
        if functions and test["function"] not in functions:
            continue
        source_flags = document.get("source_native_flags", {}).get(
            test["source"], {}).get(platform, [])
        for expanded in expand_configurations(test, platform):
            if source_flags:
                expanded = dict(expanded)
                configuration = dict(expanded["configuration"])
                configuration["native_flags"] = [
                    *source_flags,
                    *configuration.get("native_flags", []),
                ]
                expanded["configuration"] = configuration
            yield expanded
    if not started:
        raise ValueError(
            f"unit-test resume boundary is not in the manifest: "
            f"{start_at[0]}:{start_at[1]}")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--platform", default="unix")
    parser.add_argument("--source", action="append", default=[])
    parser.add_argument("--function", action="append", default=[])
    parser.add_argument(
        "--selection", type=Path,
        help="selector JSON containing exact source/function ownership pairs")
    parser.add_argument(
        "--start-at", metavar="SOURCE:FUNCTION",
        help="start at this manifest entry and run every following selection")
    parser.add_argument("--build", type=Path)
    parser.add_argument("--cc", default="clang")
    parser.add_argument("--clang", default="clang")
    parser.add_argument("--toolchain", choices=["native", "watcom16",
                                                 "watcom32r", "watcom32s"],
                        default="native")
    parser.add_argument("--native-flag", action="append", default=[])
    parser.add_argument("--ast-flag", action="append", default=[])
    parser.add_argument("--compile-only", action="store_true")
    parser.add_argument("--coverage", action="store_true")
    parser.add_argument(
        "--allow-proposed-coverage-waivers", action="store_true",
        help="continue past coverage gaps recorded as proposed waivers")
    parser.add_argument("--dosbox", type=Path,
                        help="DOSBox executable for batched DOS execution")
    parser.add_argument("--wine", type=Path,
                        help="Wine executable for Windows test execution")
    parser.add_argument("--windows-architecture", choices=["x86", "x64"],
                        help="architecture shared by Windows tests and fixtures")
    parser.add_argument("--windows-abi", choices=["gnu", "msvc"],
                        default="gnu",
                        help="ABI modeled by the Windows compiler")
    parser.add_argument("--dos-timeout", type=int, default=DEFAULT_DOS_TIMEOUT)
    args = parser.parse_args()
    start_at = None
    if args.start_at is not None:
        source, separator, function = args.start_at.partition(":")
        if not separator or not source or not function:
            parser.error("--start-at requires SOURCE:FUNCTION")
        start_at = (source, function)
    try:
        owners = selection_owners(args.selection) if args.selection else None
        tests = list(selected_tests(
            args.platform, set(args.source), set(args.function), start_at,
            owners=owners))
    except ValueError as error:
        parser.error(str(error))
    if not tests:
        print("no selected unit tests")
        return 0
    if args.build is None:
        args.build = default_build_path(
            args.platform, args.toolchain, args.windows_architecture)
    args.build.mkdir(parents=True, exist_ok=True)
    supported_platforms = {"unix", "windows", "dos16", "dos32"}
    if args.platform not in supported_platforms:
        parser.error(f"native runner does not yet support {args.platform}")
    if args.toolchain == "watcom16" and args.platform != "dos16":
        parser.error("watcom16 requires --platform dos16")
    if args.toolchain.startswith("watcom32") and args.platform != "dos32":
        parser.error("watcom32 requires --platform dos32")
    if args.platform.startswith("dos") and args.toolchain == "native":
        parser.error("DOS platforms require a Watcom toolchain")
    if args.windows_architecture and args.platform != "windows":
        parser.error("--windows-architecture requires --platform windows")
    watcom = None
    if args.toolchain != "native":
        watcom = os.environ.get("WATCOM")
        if not watcom:
            raise RuntimeError("WATCOM must name the Open Watcom installation")
    native_platform_flags = platform_defines(args.platform)
    windows_architecture_flags = (
        ["-m32"] if args.windows_architecture == "x86" else [])
    ast_platform_flags = analyzer_platform_flags(
        args.platform, native_platform_flags, args.cc,
        args.windows_architecture, args.windows_abi)
    if args.platform in {"unix", "windows"}:
        common_flags = [*native_compile_flags(args.platform),
                        "-I", str(ROOT / "include"),
                        "-I", str(ROOT / "src"),
                        "-I", str(ROOT),
                        "-I", str(ROOT / "unit" / "framework"),
                        *native_platform_flags, *windows_architecture_flags,
                        "-DHAS_INTTYPES_H",
                        "-DOPENDOORS_HAVE_VSNPRINTF=1", *args.native_flag]
        ast_platform_flags.extend(["-DHAS_INTTYPES_H",
                                   "-DOPENDOORS_HAVE_VSNPRINTF=1"])
    else:
        common_flags = []
    if args.toolchain == "watcom32s":
        ast_platform_flags.append("-D__SW_3S")
    if args.toolchain != "native":
        ast_platform_flags.extend(watcom_ast_compatibility_flags(watcom))
    ast_flags = [language_standard_flag(args.platform), *ast_platform_flags,
                 *args.ast_flag]
    failures = []
    dos_runs = []
    windows_runs = []
    windows_llvm_runs = []
    dos_stems = set()
    for test in tests:
        configuration = test["configuration"]
        configuration_name = configuration["name"]
        label = unit_test_label(test["source"], test["function"])
        if configuration_name != "default":
            label += "-" + configuration_name
        stem = label
        if args.platform.startswith("dos"):
            stem = dos_short_stem(test["source"], test["function"],
                                  configuration_name)
            if stem in dos_stems:
                raise RuntimeError(f"DOS 8.3 unit-test name collision: {stem}")
            dos_stems.add(stem)
        test_ast_flags = ast_flags + preprocessor_flags(configuration)
        test_common_flags = (common_flags + preprocessor_flags(configuration) +
                             list(configuration.get("native_flags", [])))
        generated = args.build / (stem + ".c")
        generated_assembly = None
        llvm_generated = args.build / (stem + "-llvm.c")
        executable = args.build / (stem + executable_suffix(args.platform))
        if Path(test["source"]).suffix.lower() == ".asm":
            generated_assembly = args.build / (stem + ".ASM")
            generate_command = [
                sys.executable,
                str(ROOT / "unit" / "tools" / "asm_generate.py"),
                test["source"], test["function"], test["case"],
                str(generated), str(generated_assembly),
            ]
        else:
            generate_command = [
                sys.executable, str(ROOT / "unit" / "tools" / "generate.py"),
                test["source"], test["function"], test["case"], str(generated),
                "--clang", args.clang,
            ]
            generate_command.extend("--flag=" + flag for flag in test_ast_flags)
            generate_command.extend(
                "--provided-variable=" + variable
                for variable in configuration.get("provided_variables", []))
            if llvm_coverage_supported(args.platform):
                generate_command.extend([
                    "--uninstrumented-output", str(llvm_generated)])
        if not run_step(failures, label + " generation",
                        lambda: command(generate_command)):
            continue
        if args.toolchain == "native":
            fixture = configuration.get("windows_fixture")
            if args.platform == "windows" and fixture is not None:
                fixture_source = ROOT / str(fixture)
                fixture_output = args.build / (
                    fixture_source.stem.upper() + ".DLL")
                fixture_compiled = run_step(
                    failures, label + " Windows fixture compile",
                    lambda: command(windows_fixture_arguments(
                        args.cc, fixture_source, fixture_output,
                        windows_architecture_flags)))
                if not fixture_compiled:
                    continue
            compiled = run_step(
                failures, label + " native compile",
                lambda: command([args.cc, *test_common_flags, str(FRAMEWORK),
                                 str(generated), "-o", str(executable)]))
        else:
            environment = watcom_environment(os.environ, watcom)
            compiler = args.cc
            if args.cc == "clang":
                compiler = "wcl" if args.toolchain == "watcom16" else "wcl386"
            model_flag = "-ml" if args.toolchain == "watcom16" else (
                "-3s" if args.toolchain == "watcom32s" else "-3r")
            assembly_object = None
            if generated_assembly is not None:
                assembly_object = args.build / (dos_aux_stem(stem) + ".obj")
                assembler = Path(watcom) / "binl" / "wasm"
                assembled = run_step(
                    failures, label + " assembly compile",
                    lambda: command([
                        str(assembler), "-q", "-0", "-dLCODE", "-dLDATA",
                        "-fo=" + assembly_object.name,
                        generated_assembly.name,
                    ], environment=environment, cwd=args.build))
                if not assembled:
                    continue
            compile_command = [
                compiler, *watcom_target_flags(args.toolchain), model_flag,
                "-zq", "-za99", "-we",
                "-dOPENDOORS_HAVE_VSNPRINTF=1",
                *preprocessor_flags(configuration, watcom=True),
                "-i=" + str(ROOT / "include"),
                "-i=" + str(ROOT / "src"),
                "-i=" + str(ROOT),
                "-i=" + str(ROOT / "unit" / "framework"),
                "-fe=" + executable.name, str(FRAMEWORK), generated.name,
                *args.native_flag,
            ]
            if assembly_object is not None:
                compile_command.append(assembly_object.name)
            compiled = run_step(
                failures, label + " Watcom compile",
                lambda: command(compile_command, environment=environment,
                                cwd=args.build))
        if not compiled:
            continue
        if args.compile_only:
            continue
        if args.platform.startswith("dos"):
            dos_runs.append({
                "label": label,
                "test": test,
                "generated": generated,
                "executable": executable,
                "report": args.build / (stem + ".COV"),
                "failure": args.build / (stem + ".BAD"),
                "output": args.build / (stem + ".OUT"),
            })
            continue
        native_report = args.build / (stem + ".native.cov")
        if args.platform == "windows":
            windows_runs.append({
                "label": label,
                "generated": generated,
                "executable": executable,
                "report": native_report,
                "failure": args.build / (stem + ".BAD"),
                "output": args.build / (stem + ".OUT"),
            })
        else:
            native_ran = run_step(
                failures, stem + " native test",
                lambda: command(native_test_arguments(
                    executable, native_report, args.wine)))
            if native_ran or native_report.is_file():
                run_step(
                    failures, stem + " portable coverage",
                    lambda: native_coverage(
                        native_report, generated.with_suffix(".model.json"),
                        args.platform,
                        args.build / (stem + ".native-coverage.json"),
                        args.allow_proposed_coverage_waivers))
        if args.coverage and llvm_coverage_supported(args.platform):
            coverage_executable = args.build / (
                stem + "-coverage" + executable_suffix(args.platform))
            raw = args.build / (stem + ".profraw")
            coverage_compiled = run_step(
                failures, stem + " LLVM compile",
                lambda: command([
                    args.clang, *test_common_flags, "-fprofile-instr-generate",
                    "-fcoverage-mapping", "-fcoverage-mcdc", str(FRAMEWORK),
                    str(llvm_generated), "-o", str(coverage_executable)]))
            if not coverage_compiled:
                continue
            if args.platform == "windows":
                windows_llvm_runs.append({
                    "label": label,
                    "test": test,
                    "generated": llvm_generated,
                    "executable": coverage_executable,
                    "profile": raw,
                    "failure": args.build / (
                        coverage_executable.stem + ".BAD"),
                    "output": args.build / (
                        coverage_executable.stem + ".OUT"),
                    "coverage": args.build / (stem + ".coverage.json"),
                })
                continue
            environment = os.environ.copy()
            environment["LLVM_PROFILE_FILE"] = str(raw)
            run_step(
                failures, stem + " LLVM test",
                lambda: command([str(coverage_executable)], environment))
            if raw.is_file():
                run_step(
                    failures, stem + " LLVM coverage",
                    lambda: llvm_coverage(
                        coverage_executable, raw, test["source"],
                        test["function"], args.platform, llvm_generated,
                        args.build / (stem + ".coverage.json"),
                        args.allow_proposed_coverage_waivers))
    if windows_runs:
        done = args.build / "UTDONE.OK"
        done.unlink(missing_ok=True)
        for record in windows_runs:
            record["report"].unlink(missing_ok=True)
            record["failure"].unlink(missing_ok=True)
            record["output"].unlink(missing_ok=True)
        batch = args.build / "UTRUN.CMD"
        batch.write_bytes(windows_batch([
            (record["executable"].name, record["report"].name)
            for record in windows_runs
        ]).encode("ascii"))
        batch_ran = run_step(
            failures, "Windows unit batch",
            lambda: command(windows_batch_arguments(batch, args.wine)))
        if batch_ran and not done.is_file():
            failures.append(
                "Windows unit batch: UTDONE.OK was not produced")
        for record in windows_runs:
            label = record["label"]
            if (record["failure"].is_file() and
                    record["failure"].stat().st_size != 0):
                detail = ""
                if record["output"].is_file():
                    detail = record["output"].read_text(
                        encoding="ascii", errors="replace").strip()
                suffix = f": {detail}" if detail else ""
                failures.append(
                    f"{label} Windows test returned failure{suffix}")
            if not record["report"].is_file():
                failures.append(
                    f"{label} Windows coverage report is missing")
                continue
            run_step(
                failures, label + " portable coverage",
                lambda record=record: native_coverage(
                    record["report"],
                    record["generated"].with_suffix(".model.json"),
                    args.platform,
                    args.build / (record["executable"].stem +
                                  ".native-coverage.json"),
                    args.allow_proposed_coverage_waivers))
    if windows_llvm_runs:
        done = args.build / "UTLLVM.OK"
        done.unlink(missing_ok=True)
        for record in windows_llvm_runs:
            record["profile"].unlink(missing_ok=True)
            record["failure"].unlink(missing_ok=True)
            record["output"].unlink(missing_ok=True)
        batch = args.build / "UTLLVM.CMD"
        batch.write_bytes(windows_llvm_batch([
            (record["executable"].name, record["profile"].name)
            for record in windows_llvm_runs
        ]).encode("ascii"))
        batch_ran = run_step(
            failures, "Windows LLVM unit batch",
            lambda: command(windows_batch_arguments(batch, args.wine)))
        if batch_ran and not done.is_file():
            failures.append(
                "Windows LLVM unit batch: UTLLVM.OK was not produced")
        for record in windows_llvm_runs:
            label = record["label"]
            if (record["failure"].is_file() and
                    record["failure"].stat().st_size != 0):
                detail = ""
                if record["output"].is_file():
                    detail = record["output"].read_text(
                        encoding="ascii", errors="replace").strip()
                suffix = f": {detail}" if detail else ""
                failures.append(
                    f"{label} Windows LLVM test returned failure{suffix}")
            if not record["profile"].is_file():
                failures.append(f"{label} Windows LLVM profile is missing")
                continue
            run_step(
                failures, label + " LLVM coverage",
                lambda record=record: llvm_coverage(
                    record["executable"], record["profile"],
                    record["test"]["source"], record["test"]["function"],
                    args.platform, record["generated"], record["coverage"],
                    args.allow_proposed_coverage_waivers))
    if dos_runs:
        if args.dosbox is None:
            parser.error("DOS execution requires --dosbox (or use --compile-only)")
        done = args.build / "UTDONE.OK"
        done.unlink(missing_ok=True)
        for record in dos_runs:
            record["report"].unlink(missing_ok=True)
            record["failure"].unlink(missing_ok=True)
            record["output"].unlink(missing_ok=True)
        if args.platform == "dos32":
            extender = Path(watcom) / "binw" / "dos4gw.exe"
            if not extender.is_file():
                raise RuntimeError(f"DOS/4GW runtime not found: {extender}")
            shutil.copyfile(extender, args.build / "DOS4GW.EXE")
        fixture_sources = []
        for record in dos_runs:
            fixture = record["test"]["configuration"].get("dos_tsr_fixture")
            if fixture is not None:
                source = ROOT / str(fixture)
                if source not in fixture_sources:
                    fixture_sources.append(source)
        fixture_names = assemble_dos_tsr_fixtures(
            fixture_sources, args.build, watcom)
        batch = args.build / "UTRUN.BAT"
        batch.write_bytes(dos_batch([
            (record["executable"].name, record["report"].name)
            for record in dos_runs
        ], fixture_names).encode("ascii"))
        dosbox_configuration = args.build / "dosbox.conf"
        dosbox_configuration.write_text(
            "[midi]\nmididevice=none\n"
            "[joystick]\njoysticktype=none\n"
            "[serial]\nserial1=dummy\n"
            "[autoexec]\n",
            encoding="ascii")
        log = args.build / "dosbox.log"

        def execute_dosbox() -> None:
            arguments = dosbox_arguments(
                args.dosbox, args.build, batch, dosbox_configuration)
            print("+", " ".join(arguments), flush=True)
            with log.open("wb") as output:
                subprocess.run(arguments, cwd=args.build, check=True,
                               stdout=output, stderr=subprocess.STDOUT,
                               timeout=args.dos_timeout)

        emulator_ran = run_step(failures, "DOSBox unit batch", execute_dosbox)
        if emulator_ran and not done.is_file():
            failures.append("DOSBox unit batch: UTDONE.OK was not produced")
        for record in dos_runs:
            label = record["label"]
            if (record["failure"].is_file() and
                    record["failure"].stat().st_size != 0):
                detail = ""
                if record["output"].is_file():
                    detail = record["output"].read_text(
                        encoding="ascii", errors="replace").strip()
                suffix = f": {detail}" if detail else ""
                failures.append(
                    f"{label} DOS test returned failure{suffix}")
            if not record["report"].is_file():
                failures.append(f"{label} DOS coverage report is missing")
                continue
            run_step(
                failures, label + " portable coverage",
                lambda record=record: native_coverage(
                    record["report"],
                    record["generated"].with_suffix(".model.json"),
                    args.platform,
                    args.build / (record["executable"].stem +
                                  ".native-coverage.json"),
                    args.allow_proposed_coverage_waivers))
    if failures:
        print(f"{len(failures)} unit-test step(s) failed:", file=sys.stderr)
        for failure in failures:
            print(" -", failure, file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
