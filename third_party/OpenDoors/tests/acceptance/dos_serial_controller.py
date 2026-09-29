#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-2.0-or-later
"""Drive the interactive acceptance door through DOSBox COM1 emulation."""

from __future__ import annotations

import argparse
import socket
import subprocess
import sys
import tempfile
import time
from pathlib import Path

from transport_scenarios import SCENARIOS, create_fixtures, drive_scenario
from transport_scenarios import remove_fixtures


def print_log_tail(log_path: Path) -> None:
    if not log_path.exists():
        return
    contents = log_path.read_bytes()[-16384:].decode(errors="replace")
    if contents:
        print("DOSBox output (last 16 KiB):", file=sys.stderr)
        print(contents, file=sys.stderr)


def main() -> int:
    parser = argparse.ArgumentParser(
        description="drive an OpenDoors acceptance door through DOSBox COM1"
    )
    parser.add_argument("dosbox", type=Path)
    parser.add_argument("door", type=Path)
    parser.add_argument(
        "--pre-command",
        action="append",
        default=[],
        help="DOS command to run before the acceptance door (repeatable)",
    )
    parser.add_argument(
        "--scenario",
        action="append",
        choices=SCENARIOS,
        default=[],
        help="scenario to run (repeatable; defaults to the complete suite)",
    )
    args = parser.parse_args()

    dosbox = args.dosbox.resolve()
    door = args.door.resolve()
    scenarios = args.scenario or list(SCENARIOS)
    if not dosbox.is_file():
        raise RuntimeError(f"DOSBox executable not found: {dosbox}")
    if not door.is_file():
        raise RuntimeError(f"DOS acceptance door not found: {door}")

    build_directory = door.parent
    answer = next(
        (
            candidate
            for candidate in (
                build_directory / "odanswer.exe",
                build_directory / "ODANSWER.EXE",
            )
            if candidate.is_file()
        ),
        build_directory / "odanswer.exe",
    )
    log_path = build_directory / f"dos-serial-{door.stem.lower()}.log"
    failure_path = build_directory / "ODFAIL.TXT"
    modem_ready_path = build_directory / "MODREADY.OK"
    batch_path = build_directory / "ODACTEST.BAT"
    if not answer.is_file():
        raise RuntimeError(f"DOS modem-answer helper not found: {answer}")
    failure_path.unlink(missing_ok=True)
    modem_ready_path.unlink(missing_ok=True)
    batch_path.unlink(missing_ok=True)
    create_fixtures(build_directory)

    batch_lines = [
        "@echo off",
        answer.name,
        "if errorlevel 1 goto failed",
        *args.pre_command,
    ]
    for scenario in scenarios:
        batch_lines.extend(
            (
                f"{door.name} {scenario}",
                "if errorlevel 1 goto failed",
            )
        )
    batch_lines.extend(("exit", ":failed", "exit"))
    batch_path.write_bytes(("\r\n".join(batch_lines) + "\r\n").encode("ascii"))

    reservation = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    reservation.bind(("127.0.0.1", 0))
    port = reservation.getsockname()[1]
    reservation.close()

    process: subprocess.Popen[bytes] | None = None
    peer: socket.socket | None = None
    try:
        with tempfile.TemporaryDirectory(prefix="opendoors-dos-serial-") as temp:
            config = Path(temp) / "dosbox.conf"
            config.write_text(
                "[serial]\n"
                f"serial1=modem listenport:{port}\n",
                encoding="ascii",
            )
            with log_path.open("wb") as log:
                commands = [
                    str(dosbox),
                    "-noconsole",
                    "-conf",
                    str(config),
                    "-c",
                    f'mount c "{build_directory}"',
                    "-c",
                    "c:",
                ]
                commands.extend(("-c", batch_path.name))
                process = subprocess.Popen(
                    commands,
                    stdout=log,
                    stderr=subprocess.STDOUT,
                )
                deadline = time.monotonic() + 30
                while not modem_ready_path.exists():
                    if process.poll() is not None:
                        raise RuntimeError(
                            "DOSBox exited before the modem was ready"
                        )
                    if time.monotonic() >= deadline:
                        raise RuntimeError("DOS modem did not become ready")
                    time.sleep(0.01)
                peer = socket.create_connection(("127.0.0.1", port), 10)
                for scenario in scenarios:
                    drive_scenario(peer, scenario, timeout=30)
                    if scenario == "session":
                        peer.close()
                        peer = None
                return_code = process.wait(timeout=30)
                if return_code != 0:
                    raise RuntimeError(f"DOSBox exited with {return_code}")
    except Exception:
        if failure_path.exists():
            print(failure_path.read_text(errors="replace"), file=sys.stderr)
        print_log_tail(log_path)
        raise
    finally:
        if peer is not None:
            peer.close()
        if process is not None and process.poll() is None:
            process.terminate()
            try:
                process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait()
        modem_ready_path.unlink(missing_ok=True)
        batch_path.unlink(missing_ok=True)
        remove_fixtures(build_directory)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
