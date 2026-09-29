#!/usr/bin/env python3
"""Require a child program to fail with a specified diagnostic."""

import argparse
import subprocess


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--launcher", action="append", default=[])
    parser.add_argument("--expected", required=True)
    parser.add_argument("program")
    args = parser.parse_args()

    completed = subprocess.run(
        [*args.launcher, args.program],
        check=False,
        capture_output=True,
        text=True,
        timeout=15,
    )
    output = completed.stdout + completed.stderr
    if completed.returncode == 0:
        raise RuntimeError(f"{args.program} unexpectedly succeeded")
    if args.expected not in output:
        raise RuntimeError(
            f"{args.program} did not report {args.expected!r}:\n{output}"
        )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
