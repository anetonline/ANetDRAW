#!/usr/bin/env python3
"""Compile and execute generated unit tests with Turbo C 2.01 in DOSBox."""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import shutil
import subprocess
import sys
from pathlib import Path

from inventory import ROOT
from run import (TESTS_FILE, dos_aux_stem, expand_configurations,
                 native_coverage)


SHORT_STEM = re.compile(r"^U[0-9A-F]{7}$")


def dos_text(text: str) -> bytes:
    normalized = text.replace("\r\n", "\n").replace("\r", "\n")
    if not normalized.endswith("\n"):
        normalized += "\n"
    return normalized.replace("\n", "\r\n").encode("latin-1")


def turbo_compile_batch(runs: list[tuple[str, str, int, Path]]) -> str:
    lines = [
        "@ECHO OFF",
        "SET PATH=C:\\TC\\BIN",
        "SET INCLUDE=C:\\UNITSRC;C:\\SRC;C:\\SRC\\TRIO;C:\\TC\\INCLUDE",
        "SET LIB=C:\\TC\\LIB",
        "IF EXIST C:\\TCUNIT.LOG DEL C:\\TCUNIT.LOG",
        "IF EXIST C:\\UNITCC.OK DEL C:\\UNITCC.OK",
        "IF EXIST C:\\UNITDONE.OK DEL C:\\UNITDONE.OK",
        "CD \\UNITSRC",
        "TCC -ml -O -G -Z -d -w -c "
        "-I.;C:\\SRC;C:\\SRC\\TRIO;C:\\TC\\INCLUDE "
        "UT.C >>C:\\TCUNIT.LOG",
        "TCC -ms -O -G -Z -d -w -eASMFIX.EXE "
        "-I.;C:\\TC\\INCLUDE -LC:\\TC\\LIB ASMFIX.C "
        ">>C:\\TCUNIT.LOG",
        "IF ERRORLEVEL 1 GOTO ALLDONE",
        "CD \\UNITOUT",
    ]
    for index, (stem, source_stem, shard, unused_model) in enumerate(runs):
        model = json.loads(unused_model.read_text(encoding="utf-8"))
        options = []
        if shard:
            options.append(f"-DUT_TURBO_SHARD={shard}")
        option_text = "".join(value + " " for value in options)
        lines.extend([
            f"IF EXIST {stem}.BAD DEL {stem}.BAD",
            f"IF EXIST {stem}.COV DEL {stem}.COV",
            f"IF EXIST {stem}.OUT DEL {stem}.OUT",
            f"IF EXIST C:\\UNITASM\\{stem}.ASM "
            f"DEL C:\\UNITASM\\{stem}.ASM",
            "CD \\UNITSRC",
            "TCC -ml -S -O -G -Z -d -w "
            f"{option_text}"
            "-I.;C:\\SRC;C:\\SRC\\TRIO;C:\\TC\\INCLUDE "
            "-LC:\\TC\\LIB "
            f"{source_stem}.C "
            ">>C:\\TCUNIT.LOG",
            f"IF ERRORLEVEL 1 GOTO FAILED{index}",
            f"ASMFIX {source_stem}.ASM >>C:\\TCUNIT.LOG",
            f"IF ERRORLEVEL 1 GOTO FAILED{index}",
            f"COPY {source_stem}.ASM C:\\UNITASM\\{stem}.ASM >NUL",
            f"IF NOT ERRORLEVEL 1 GOTO COMPILED{index}",
            f":FAILED{index}",
            "CD \\UNITOUT",
            f"ECHO COMPILE>{stem}.BAD",
            f"GOTO NEXT{index}",
            f":COMPILED{index}",
            "CD \\UNITOUT",
            f":NEXT{index}",
        ])
    lines.extend(["ECHO DONE>C:\\UNITCC.OK", ":ALLDONE"])
    return "\r\n".join(lines) + "\r\n"


def turbo_link_batch(runs: list[tuple[str, str, int, Path]],
                     tsr_fixtures: list[str] | None = None) -> str:
    lines = [
        "@ECHO OFF",
        "SET PATH=C:\\TC\\BIN",
        "SET LIB=C:\\TC\\LIB",
        "IF EXIST C:\\UNITDONE.OK DEL C:\\UNITDONE.OK",
        "CD \\UNITOUT",
    ]
    for fixture in tsr_fixtures or []:
        lines.append("C:\\" + fixture)
    for index, (stem, unused_source_stem, unused_shard,
                unused_model) in enumerate(runs):
        del unused_shard
        metadata = json.loads(unused_model.read_text(encoding="utf-8"))
        assembly_object = ""
        if Path(metadata["source"]).suffix.lower() == ".asm":
            assembly_object = " " + dos_aux_stem(unused_source_stem) + ".OBJ"
        lines.extend([
            f"IF EXIST {stem}.BAD GOTO NEXT{index}",
            "CD \\UNITSRC",
            "TCC -ml -O -G -Z -d -w -LC:\\TC\\LIB "
            f"-eC:\\UNITOUT\\{stem}.EXE UT.OBJ {stem}.OBJ"
            f"{assembly_object} "
            ">>C:\\TCUNIT.LOG",
            f"IF NOT ERRORLEVEL 1 GOTO LINKED{index}",
            "CD \\UNITOUT",
            f"ECHO LINK>{stem}.BAD",
            f"GOTO NEXT{index}",
            f":LINKED{index}",
            "CD \\UNITOUT",
            f"{stem}.EXE {stem}.COV >{stem}.OUT",
            f"IF NOT ERRORLEVEL 1 GOTO NEXT{index}",
            f"ECHO RUN>{stem}.BAD",
            f":NEXT{index}",
        ])
    lines.append("ECHO DONE>C:\\UNITDONE.OK")
    return "\r\n".join(lines) + "\r\n"


def execution_units(units: list[tuple[str, Path, Path]],
                    manifest: dict[str, object]) -> list[tuple[str, str, int, Path]]:
    """Expand explicitly large tests into deterministic Turbo-only runs."""
    counts = {
        (test["source"], test["function"]): test.get("turbo_shards", 1)
        for test in manifest.get("tests", [])
    }
    result = []
    used = set()
    for source_stem, unused_source, model in units:
        del unused_source
        metadata = json.loads(model.read_text(encoding="utf-8"))
        count = counts.get((metadata["source"], metadata["function"]), 1)
        if count == 1:
            run_stem = source_stem
            if run_stem in used:
                raise RuntimeError(f"duplicate Turbo unit stem: {run_stem}")
            used.add(run_stem)
            result.append((run_stem, source_stem, 0, model))
            continue
        for shard in range(1, count + 1):
            identity = f"{source_stem}\0{shard}".encode("ascii")
            run_stem = "U" + hashlib.sha256(identity).hexdigest()[:7].upper()
            if run_stem in used:
                raise RuntimeError(f"duplicate Turbo shard stem: {run_stem}")
            used.add(run_stem)
            result.append((run_stem, source_stem, shard, model))
    return result


def turbo_fixture_sources(manifest: dict[str, object],
                          unit_keys: set[tuple[str, str]]) -> list[Path]:
    """Return fixtures belonging to generated DOS16 configurations only."""
    sources = []
    for test in manifest.get("tests", []):
        if (test.get("source"), test.get("function")) not in unit_keys:
            continue
        for expanded in expand_configurations(test, "dos16"):
            fixture = expanded["configuration"].get("turbo_tsr_fixture")
            if fixture is not None:
                path = (ROOT / fixture).resolve()
                if path not in sources:
                    sources.append(path)
    return sources


def merge_coverage_reports(reports: list[Path], output: Path) -> None:
    """Combine shard observations for one unchanged instrumentation model."""
    header = "OPENDOORS-UNIT-COVERAGE 1"
    observations = []
    for report in reports:
        lines = report.read_text(encoding="ascii").splitlines()
        if not lines or lines[0] != header:
            raise RuntimeError(f"invalid portable coverage report: {report}")
        observations.extend(lines[1:])
    output.write_text(
        header + "\n" + "".join(line + "\n" for line in observations),
        encoding="ascii")


def generated_units(directory: Path) -> list[tuple[str, Path, Path]]:
    result = []
    for source in sorted(directory.glob("U???????.c")):
        stem = source.stem.upper()
        model = source.with_suffix(".model.json")
        if not SHORT_STEM.fullmatch(stem):
            raise RuntimeError(f"invalid Turbo unit stem: {stem}")
        if not model.is_file():
            raise RuntimeError(f"missing unit model: {model}")
        result.append((stem, source, model))
    if not result:
        raise RuntimeError(f"no generated Turbo unit sources in {directory}")
    return result


def stage_sources(units: list[tuple[str, Path, Path]],
                  runs: list[tuple[str, str, int, Path]], stage: Path,
                  tsr_fixtures: list[str] | None = None) -> None:
    source_directory = stage / "UNITSRC"
    assembly_directory = stage / "UNITASM"
    output_directory = stage / "UNITOUT"
    source_directory.mkdir(parents=True, exist_ok=True)
    assembly_directory.mkdir(parents=True, exist_ok=True)
    output_directory.mkdir(parents=True, exist_ok=True)
    for path in source_directory.glob("U???????.C"):
        path.unlink()
    for path in source_directory.glob("U???????.OBJ"):
        path.unlink()
    for path in assembly_directory.glob("U???????.ASM"):
        path.unlink()
    for stem, source, unused_model in units:
        del unused_model
        (source_directory / (stem + ".C")).write_bytes(dos_text(
            source.read_text(encoding="latin-1")))
        assembly = source.with_suffix(".unit.asm")
        if assembly.is_file():
            assembly_stem = dos_aux_stem(stem)
            (source_directory / (assembly_stem + ".ASM")).write_bytes(
                dos_text(assembly.read_text(encoding="latin-1")))
    for source, name in ((ROOT / "unit" / "framework" / "ut.c", "UT.C"),
                         (ROOT / "unit" / "framework" / "ut.h", "UT.H"),
                         (ROOT / "unit" / "framework" / "utasm.h", "UTASM.H"),
                         (ROOT / "unit" / "framework" / "tcasmfix.c",
                          "ASMFIX.C")):
        (source_directory / name).write_bytes(dos_text(
            source.read_text(encoding="latin-1")))
    (stage / "TCUNIT1.BAT").write_bytes(
        turbo_compile_batch(runs).encode("ascii"))
    (stage / "TCUNIT2.BAT").write_bytes(
        turbo_link_batch(runs, tsr_fixtures).encode("ascii"))


def assemble_tsr_fixtures(sources: list[Path], stage: Path,
                          assembler: str) -> list[str]:
    """Build real-mode TSR fixtures which must be resident for unit runs."""
    linker = str(Path(assembler).with_name("wlink"))
    if not Path(linker).is_file():
        raise RuntimeError(f"Open Watcom linker is missing: {linker}")
    names = []
    for source in sources:
        stem = "".join(character for character in source.stem.upper()
                       if character.isalnum())[:8]
        object_file = stage / (stem + ".OBJ")
        executable = stage / (stem + ".COM")
        commands = [
            [assembler, "-q", "-0", f"-fo={object_file}", str(source)],
            [linker, "format", "dos", "com", "option", "quiet",
             "option", "nodefaultlibs", "name", str(executable),
             "file", str(object_file)],
        ]
        for arguments in commands:
            print("+", " ".join(arguments), flush=True)
            subprocess.run(arguments, cwd=ROOT, check=True)
        names.append(executable.name)
    return names


def assemble_units(runs: list[tuple[str, str, int, Path]], stage: Path,
                   assembler: str) -> None:
    assembly_directory = stage / "UNITASM"
    source_directory = stage / "UNITSRC"
    output_directory = stage / "UNITOUT"
    log = stage / "WASM.LOG"
    with log.open("wb") as output:
        assembled_auxiliary = set()
        for stem, unused_source_stem, unused_shard, unused_model in runs:
            del unused_shard
            failure = output_directory / (stem + ".BAD")
            if failure.is_file() and failure.stat().st_size:
                continue
            assembly = assembly_directory / (stem + ".ASM")
            object_file = source_directory / (stem + ".OBJ")
            if not assembly.is_file():
                failure.write_text("ASSEMBLE\n", encoding="ascii")
                continue
            command = [assembler, "-q", f"-fo={object_file}", str(assembly)]
            print("+", " ".join(command), flush=True)
            result = subprocess.run(command, stdout=output,
                                    stderr=subprocess.STDOUT, check=False)
            if result.returncode != 0 or not object_file.is_file():
                failure.write_text("ASSEMBLE\n", encoding="ascii")
                continue
            metadata = json.loads(unused_model.read_text(encoding="utf-8"))
            if Path(metadata["source"]).suffix.lower() != ".asm":
                continue
            auxiliary_stem = dos_aux_stem(unused_source_stem)
            if auxiliary_stem in assembled_auxiliary:
                continue
            assembled_auxiliary.add(auxiliary_stem)
            auxiliary_source = source_directory / (auxiliary_stem + ".ASM")
            auxiliary_object = source_directory / (auxiliary_stem + ".OBJ")
            auxiliary_command = [
                assembler, "-q", "-0", "-dLCODE", "-dLDATA",
                f"-fo={auxiliary_object}", str(auxiliary_source),
            ]
            print("+", " ".join(auxiliary_command), flush=True)
            auxiliary_result = subprocess.run(
                auxiliary_command, stdout=output, stderr=subprocess.STDOUT,
                check=False)
            if (auxiliary_result.returncode != 0 or
                    not auxiliary_object.is_file()):
                failure.write_text("ASSEMBLE\n", encoding="ascii")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--generated", type=Path, required=True)
    parser.add_argument("--stage", type=Path, required=True)
    parser.add_argument("--dosbox", type=Path, required=True)
    parser.add_argument("--wasm", default="wasm",
                        help="host Open Watcom assembler executable")
    parser.add_argument("--dosbox-config", type=Path)
    parser.add_argument("--timeout", type=int, default=900)
    parser.add_argument(
        "--allow-proposed-coverage-waivers", action="store_true",
        help="continue past coverage gaps recorded as proposed waivers")
    args = parser.parse_args()

    generated = args.generated.resolve()
    stage = args.stage.resolve()
    if not (stage / "TC" / "BIN" / "TCC.EXE").is_file():
        raise RuntimeError("staged Turbo C compiler is missing")
    if not (stage / "SRC" / "OpenDoor.h").is_file():
        raise RuntimeError("staged OpenDoors headers are missing")
    assembler = shutil.which(args.wasm)
    if assembler is None:
        raise RuntimeError(f"Open Watcom assembler is missing: {args.wasm}")
    units = generated_units(generated)
    manifest = json.loads(TESTS_FILE.read_text(encoding="utf-8"))
    runs = execution_units(units, manifest)
    unit_keys = {(metadata["source"], metadata["function"])
                 for unused_stem, unused_source, model in units
                 for metadata in [json.loads(model.read_text(encoding="utf-8"))]}
    fixture_sources = turbo_fixture_sources(manifest, unit_keys)
    fixture_names = assemble_tsr_fixtures(
        fixture_sources, stage, assembler)
    stage_sources(units, runs, stage, fixture_names)

    def dosbox_arguments(batch: str) -> list[str]:
        arguments = [str(args.dosbox.resolve()), "-noconsole"]
        if args.dosbox_config is not None:
            arguments.extend(["-conf", str(args.dosbox_config.resolve())])
        arguments.extend(["-c", f'mount c "{stage}"', "-c", "c:",
                          "-c", f"CALL {batch}", "-c", "exit"])
        return arguments

    log = stage / "tcunit-dosbox.log"
    arguments = dosbox_arguments("TCUNIT1.BAT")
    print("+", " ".join(arguments), flush=True)
    with log.open("wb") as output:
        subprocess.run(arguments, check=True, stdout=output,
                       stderr=subprocess.STDOUT, timeout=args.timeout)
    if not (stage / "UNITCC.OK").is_file():
        raise RuntimeError("Turbo C unit compile batch did not complete")

    assemble_units(runs, stage, assembler)

    arguments = dosbox_arguments("TCUNIT2.BAT")
    print("+", " ".join(arguments), flush=True)
    with log.open("ab") as output:
        subprocess.run(arguments, check=True, stdout=output,
                       stderr=subprocess.STDOUT, timeout=args.timeout)

    failures = []
    if not (stage / "UNITDONE.OK").is_file():
        failures.append("Turbo C unit batch did not complete")
    output_directory = stage / "UNITOUT"
    reports_by_model = {}
    for stem, unused_source_stem, unused_shard, model in runs:
        del unused_source_stem, unused_shard
        failure = output_directory / (stem + ".BAD")
        report = output_directory / (stem + ".COV")
        diagnostic = output_directory / (stem + ".OUT")
        metadata = json.loads(model.read_text(encoding="utf-8"))
        label = f"{metadata['source']}:{metadata['function']}"
        if failure.is_file() and failure.stat().st_size:
            detail = failure.read_text(encoding="ascii", errors="replace").strip()
            message = f"{label} Turbo C {detail.lower()} failure"
            if diagnostic.is_file() and diagnostic.stat().st_size:
                output = diagnostic.read_text(
                    encoding="latin-1", errors="replace").strip()
                message += ": " + output.replace("\r", "").replace("\n", "; ")
            failures.append(message)
        if not report.is_file():
            failures.append(f"{label} Turbo C coverage report is missing")
            continue
        reports_by_model.setdefault(model, []).append(report)
    for source_stem, unused_source, model in units:
        del unused_source
        metadata = json.loads(model.read_text(encoding="utf-8"))
        label = f"{metadata['source']}:{metadata['function']}"
        reports = reports_by_model.get(model, [])
        if not reports:
            continue
        report = (reports[0] if len(reports) == 1 else
                  generated / (source_stem + ".turbo-merged.cov"))
        try:
            if len(reports) > 1:
                merge_coverage_reports(reports, report)
            native_coverage(
                report, model, "dos16",
                generated / (source_stem + ".turbo-native-coverage.json"),
                args.allow_proposed_coverage_waivers)
        except RuntimeError as error:
            failures.append(f"{label} Turbo C coverage: {error}")
    if failures:
        if (stage / "TCUNIT.LOG").is_file():
            print((stage / "TCUNIT.LOG").read_text(
                encoding="latin-1", errors="replace"), file=sys.stderr)
        for failure in failures:
            print("FAIL", failure, file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
