#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-2.0-or-later
"""Drive one public transport scenario through a hosted socket."""

from __future__ import annotations

import argparse
import os
import socket
import subprocess
import tempfile
from pathlib import Path

from transport_scenarios import SCENARIOS, create_fixtures, drive_scenario


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--launcher", action="append", default=[])
    parser.add_argument("--tcp", action="store_true")
    parser.add_argument("scenario", choices=SCENARIOS)
    parser.add_argument("door", type=Path)
    args = parser.parse_args()

    door = args.door.resolve()
    kwargs: dict[str, object] = {"close_fds": True}
    listener: socket.socket | None = None
    child_socket: socket.socket | None = None
    peer: socket.socket | None = None
    process: subprocess.Popen[bytes] | None = None
    transcript = bytearray()

    with tempfile.TemporaryDirectory(prefix="opendoors-transport-") as temp:
        work_directory = Path(temp)
        create_fixtures(work_directory)
        if args.tcp:
            listener = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            listener.bind(("127.0.0.1", 0))
            listener.listen(1)
            listener.settimeout(10)
            child_argument = str(listener.getsockname()[1])
        else:
            peer, child_socket = socket.socketpair()
            child_socket.set_inheritable(True)
            child_argument = str(child_socket.fileno())
            if os.name == "posix":
                kwargs["pass_fds"] = (child_socket.fileno(),)
            else:
                kwargs["close_fds"] = False

        try:
            process = subprocess.Popen(
                [
                    *args.launcher,
                    str(door),
                    args.scenario,
                    child_argument,
                ],
                cwd=work_directory,
                **kwargs,
            )
            if child_socket is not None:
                child_socket.close()
                child_socket = None
            if listener is not None:
                peer, _ = listener.accept()
                listener.close()
                listener = None
            assert peer is not None
            transcript = drive_scenario(peer, args.scenario)
            if args.scenario == "session":
                peer.close()
                peer = None
            return_code = process.wait(timeout=12)
            if return_code != 0:
                raise RuntimeError(
                    f"{args.scenario} door exited with {return_code}"
                )
        except Exception:
            print(
                f"transport scenario {args.scenario!r} transcript: "
                f"{bytes(transcript)!r}"
            )
            raise
        finally:
            if listener is not None:
                listener.close()
            if child_socket is not None:
                child_socket.close()
            if peer is not None:
                peer.close()
            if process is not None and process.poll() is None:
                process.kill()
                process.wait()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
