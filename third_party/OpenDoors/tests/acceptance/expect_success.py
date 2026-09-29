#!/usr/bin/env python3
"""Require a child program to succeed and emit a completion marker."""

import argparse
import subprocess


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--launcher", action="append", default=[])
    parser.add_argument("--marker", required=True)
    parser.add_argument("program")
    args = parser.parse_args()

    try:
        completed = subprocess.run(
            [*args.launcher, args.program],
            check=False,
            capture_output=True,
            text=True,
            timeout=15,
        )
    except subprocess.TimeoutExpired as error:
        stdout = error.stdout.decode(errors="replace") if isinstance(
            error.stdout, bytes
        ) else (error.stdout or "")
        stderr = error.stderr.decode(errors="replace") if isinstance(
            error.stderr, bytes
        ) else (error.stderr or "")
        raise RuntimeError(
            f"{args.program} timed out before reporting {args.marker!r}:\n"
            f"{stdout}{stderr}"
        ) from error
    output = completed.stdout + completed.stderr
    if completed.returncode != 0:
        raise RuntimeError(
            f"{args.program} failed with status {completed.returncode}:\n{output}"
        )
    if args.marker not in output:
        raise RuntimeError(
            f"{args.program} exited before reporting {args.marker!r}:\n{output}"
        )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
