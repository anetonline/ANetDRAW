#!/usr/bin/env python3
"""Stage clean OpenDoors release trees from build and documentation output."""

import argparse
import os
import shutil
import subprocess
from importlib.metadata import distribution
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
EXAMPLE_SOURCES = (
    "ex_chat.c", "ex_diag.c", "ex_hello.c", "ex_music.c", "ex_ski.c",
    "ex_vote.c", "ex_votio.c", "ex_votio.h",
)
PERSONALITY_SOURCES = ("ODStand.c", "ODPCB.c", "ODRA.c", "ODWCat.c")


def version():
    return (ROOT / "VERSION").read_text(encoding="utf-8").strip()


def traditional_version():
    major, minor, patch = version().split(".")
    return f"{major}.{minor}{patch}"


def commit():
    if os.environ.get("GITHUB_SHA"):
        return os.environ["GITHUB_SHA"]
    return subprocess.check_output(
        ["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip()


def reset_directory(path):
    if path.exists():
        shutil.rmtree(path)
    path.mkdir(parents=True)


def write_buildinfo(destination, args):
    lines = [
        f"OpenDoors release: {version()}",
        f"Traditional version: {traditional_version()}",
        f"Source commit: {commit()}",
        f"Platform: {args.platform}",
        f"Architecture: {args.architecture}",
        f"Toolchain: {args.toolchain}",
    ]
    if args.runtime:
        lines.append(f"Runtime: {args.runtime}")
    destination.write_text("\n".join(lines) + "\n", encoding="utf-8")


def metadata(args):
    destination = Path(args.prefix)
    if not destination.is_dir():
        raise SystemExit(f"install prefix does not exist: {destination}")
    shutil.copy2(ROOT / "VERSION", destination / "VERSION")
    write_buildinfo(destination / "BUILDINFO.txt", args)


def dos(args):
    destination = Path(args.destination)
    reset_directory(destination)
    (destination / "include").mkdir()
    (destination / "lib").mkdir()
    (destination / "share" / "licenses" / "OpenDoors").mkdir(parents=True)
    shutil.copy2(ROOT / "include" / "OpenDoor.h", destination / "include")
    shutil.copy2(ROOT / "include" / "ODStat.h", destination / "include")
    shutil.copy2(args.library, destination / "lib" / "ODOORL.LIB")
    shutil.copy2(ROOT / "LICENSE",
                 destination / "share" / "licenses" / "OpenDoors")
    shutil.copy2(ROOT / "VERSION", destination / "VERSION")
    write_buildinfo(destination / "BUILDINFO.txt", args)


def dos32(args):
    destination = Path(args.destination)
    reset_directory(destination)
    include = destination / "include"
    libraries = destination / "lib"
    examples = destination / "examples"
    licenses = destination / "licenses"
    include.mkdir()
    libraries.mkdir()
    examples.mkdir()
    licenses.mkdir()

    shutil.copy2(ROOT / "include" / "OpenDoor.h", include)
    shutil.copy2(ROOT / "include" / "ODStat.h", include)
    shutil.copy2(args.register_library, libraries / "ODOOR32R.LIB")
    shutil.copy2(args.stack_library, libraries / "ODOOR32S.LIB")
    examples_source = Path(args.examples_directory)
    example_files = {}
    for entry in examples_source.iterdir():
        if not entry.is_file():
            continue
        folded_name = entry.name.lower()
        if folded_name in example_files:
            raise SystemExit(
                f"duplicate case-insensitive DOS32 filename: {entry.name}")
        example_files[folded_name] = entry
    for name in ("ex_chat.exe", "ex_diag.exe", "ex_hello.exe",
                 "ex_music.exe", "ex_vote.exe"):
        source = example_files.get(name)
        if source is None:
            raise SystemExit(
                f"DOS32 example is missing: {examples_source / name}")
        if b"DOS/32A" not in source.read_bytes():
            raise SystemExit(f"DOS32 example does not embed DOS/32A: {source}")
        shutil.copy2(source, examples / name.upper())
    shutil.copy2(ROOT / "LICENSE", licenses / "OpenDoors.txt")
    shutil.copy2(args.dos32a_license, licenses / "DOS32A.txt")
    shutil.copy2(args.watcom_license, licenses / "OpenWatcom.txt")
    shutil.copy2(ROOT / "VERSION", destination / "VERSION")
    write_buildinfo(destination / "BUILDINFO.txt", args)

    readme = (
        f"OpenDoors {version()} 32-bit DOS SDK\n\n"
        "This SDK targets 32-bit flat-model DOS programs built with Open "
        "Watcom.\n"
        "Link lib/ODOOR32R.LIB when compiling with -3r (register calling "
        "convention), or lib/ODOOR32S.LIB when compiling with -3s (stack "
        "calling convention). Do not mix conventions.\n\n"
        "The example programs use the register convention and are native "
        "DOS/32A LX executables with the extender embedded. The libraries "
        "are validated with both Open Watcom's DOS/4GW and DOS/32A linker "
        "systems.\n\n"
        "The DOS32 serial implementation supports FOSSIL drivers. Direct UART "
        "access (COM_INTERNAL) is not supported. FOSSIL block transfers use a "
        "DPMI conventional-memory buffer when available and automatically "
        "fall back to byte-at-a-time calls.\n\n"
        "This product uses DOS/32 Advanced DOS Extender technology.\n"
    )
    (destination / "README.TXT").write_text(readme, encoding="ascii")


def copy_distribution_license(name, destination):
    package = distribution(name)
    matches = [entry for entry in package.files or ()
               if str(entry).lower().endswith("dist-info/licenses/license")]
    if len(matches) != 1:
        raise SystemExit(f"could not identify the {name} license")
    source = Path(package.locate_file(matches[0]))
    shutil.copy2(source, destination)


def companion(args):
    site = Path(args.site)
    if not (site / "index.html").is_file():
        raise SystemExit(f"rendered documentation is missing: {site}")

    destination = Path(args.destination)
    reset_directory(destination)
    shutil.copytree(site, destination / "docs")
    examples = destination / "examples"
    example_data = examples / "data"
    personalities = examples / "personalities"
    personalities.mkdir(parents=True)
    example_data.mkdir()

    for name in EXAMPLE_SOURCES:
        shutil.copy2(ROOT / "examples" / name, examples / name)
    for name in PERSONALITY_SOURCES:
        shutil.copy2(ROOT / "src" / name, personalities / name)
    for name in ("door.cfg", "DORINFO1.DEF"):
        shutil.copy2(ROOT / "examples" / "data" / name, example_data / name)
    shutil.copy2(ROOT / "release" / "examples" / "CMakeLists.txt", examples)
    shutil.copy2(ROOT / "release" / "examples" / "README.md", examples)
    shutil.copy2(ROOT / "LICENSE", destination / "LICENSE")
    shutil.copy2(ROOT / "README.md", destination / "README.md")
    shutil.copy2(ROOT / "CHANGELOG.md", destination / "CHANGELOG.md")
    shutil.copy2(ROOT / "release" / "RELEASE_NOTES.md", destination)
    shutil.copy2(ROOT / "VERSION", destination / "VERSION")

    readme = (
        f"OpenDoors {version()} examples and documentation\n\n"
        "Open docs/index.html in a web browser for the complete reference.\n"
        "See examples/README.md for example build instructions.\n"
    )
    (destination / "README.txt").write_text(readme, encoding="utf-8")

    licenses = destination / "third-party-licenses"
    licenses.mkdir()
    copy_distribution_license("mkdocs", licenses / "MkDocs-LICENSE")
    copy_distribution_license(
        "mkdocs-material", licenses / "MkDocs-Material-LICENSE")


def add_metadata_arguments(parser):
    parser.add_argument("--platform", required=True)
    parser.add_argument("--architecture", required=True)
    parser.add_argument("--toolchain", required=True)
    parser.add_argument("--runtime", default="")


def main():
    parser = argparse.ArgumentParser()
    subparsers = parser.add_subparsers(dest="command", required=True)

    metadata_parser = subparsers.add_parser("metadata")
    metadata_parser.add_argument("--prefix", required=True)
    add_metadata_arguments(metadata_parser)
    metadata_parser.set_defaults(function=metadata)

    dos_parser = subparsers.add_parser("dos")
    dos_parser.add_argument("--library", required=True)
    dos_parser.add_argument("--destination", required=True)
    add_metadata_arguments(dos_parser)
    dos_parser.set_defaults(function=dos)

    dos32_parser = subparsers.add_parser("dos32")
    dos32_parser.add_argument("--register-library", required=True)
    dos32_parser.add_argument("--stack-library", required=True)
    dos32_parser.add_argument("--examples-directory", required=True)
    dos32_parser.add_argument("--dos32a-license", required=True)
    dos32_parser.add_argument("--watcom-license", required=True)
    dos32_parser.add_argument("--destination", required=True)
    add_metadata_arguments(dos32_parser)
    dos32_parser.set_defaults(function=dos32)

    companion_parser = subparsers.add_parser("companion")
    companion_parser.add_argument("--site", required=True)
    companion_parser.add_argument("--destination", required=True)
    companion_parser.set_defaults(function=companion)

    args = parser.parse_args()
    args.function(args)


if __name__ == "__main__":
    main()
