#!/usr/bin/env python3
"""Regression test for the OpenDoors escape-sequence buffer overflow
(third_party/OPENDOORS_LOCAL_PATCHES.md #1): ESC followed by a burst of
bytes must not corrupt memory, and real key sequences in the same burst
must still decode. Run against the ASan build.

Usage: tests/esc_burst.py path/to/anetdraw
"""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from phase1_session import Session, check, FAILS  # noqa: E402


def main():
    s = Session(os.path.abspath(sys.argv[1]))
    s.read(s.startup_secs)
    s.send(b" ", 1.0)
    # ESC + 20 bytes in a single write: the overflow trigger
    # 'x' is not a main-menu hotkey, so the menu the ESC opens just closes
    s.sock.sendall(b"\x1b" + b"x" * 20)
    s.read(2.0)
    check(s.proc.poll() is None, "door survives ESC + 20-byte burst")
    # the ESC opened the main menu, the first 'x' closed it, the rest were typed
    check("xxxx" in s.row(0), "burst bytes after ESC are delivered as keys: %r" % s.row(0)[:22])
    # real sequences inside a burst still decode: 5 x Right then 'Z'
    s.send(b"\x1b[H" + b"\x1b[C" * 5 + b"Z", 1.5)
    check(s.row(0)[5] == "Z", "arrow sequences in a burst still decode: %r" % s.row(0)[:8])
    # a long burst of junk escapes
    s.sock.sendall((b"\x1b[" + b"9" * 15 + b"~") * 5 + b"\x1bOP" * 3)
    s.read(2.0)
    check(s.proc.poll() is None, "door survives malformed long CSI bursts")
    s.send(b"\x1b", 1.0)
    s.send(b"q", 1.0)
    s.send(b"n", 1.5)  # the burst typed onto the canvas: don't save
    try:
        rc = s.proc.wait(5)
    except Exception:
        s.proc.kill()
        rc = "HUNG"
    check(rc == 0, "clean exit (rc=%s)" % rc)
    print("\n%d failure(s)" % len(FAILS))
    sys.exit(1 if FAILS else 0)


if __name__ == "__main__":
    main()
