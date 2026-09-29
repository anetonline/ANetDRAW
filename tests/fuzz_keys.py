#!/usr/bin/env python3
"""Random-keystroke robustness run: thousands of keys (printables,
control codes, arrows, F-keys, PgUp/PgDn, Tab, Esc, tool menu picks,
SGR mouse presses/drags/releases/wheel at any coordinates)
through the real socket path. Run it against the ASan/UBSan build; the
door must stay responsive and still exit cleanly on Esc, Y.

Usage: tests/fuzz_keys.py path/to/anetdraw [seed] [count]
"""
import os
import random
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from phase1_session import Session, check, FAILS  # noqa: E402

KEYS = [b"\x1b[A", b"\x1b[B", b"\x1b[C", b"\x1b[D", b"\x1b[H", b"\x1b[F", b"\x1b[5~", b"\x1b[6~",
        b"\x1b[2~", b"\x1b[3~", b"\t", b"\r", b"\x7f", b"\x08", b" "] + \
       [b"\x1b[%d~" % n for n in (11, 12, 13, 14, 15, 17, 18, 19, 20, 21, 23, 24)] + \
       [bytes([c]) for c in b"\x01\x02\x05\x06\x0c\x0e\x10\x12\x15\x16\x17\x19\x1a\x0b\x07\x0f"] + \
       [b"\x14" + bytes([c]) for c in b"dlbefspkDLBEFSPK"]


def main():
    binary = os.path.abspath(sys.argv[1])
    seed = int(sys.argv[2]) if len(sys.argv) > 2 else 1
    count = int(sys.argv[3]) if len(sys.argv) > 3 else 3000
    rng = random.Random(seed)
    cols = int(os.environ.get("FUZZ_COLS", "80"))
    rows = int(os.environ.get("FUZZ_ROWS", "25"))
    s = Session(binary, rows=rows, cols=cols)
    s.read(s.startup_secs)
    s.send(b" ", 0.5)
    for i in range(count):
        r = rng.random()
        if r < 0.12:
            # SGR mouse report: press/drag/release/wheel, any button,
            # coordinates on the canvas, the status bar, and way off-screen
            b = rng.choice([0, 1, 2, 3, 32, 34, 35, 64, 65, 4, 8, 16])
            x = rng.choice([rng.randint(1, cols), rng.randint(1, cols), rng.randint(cols - 50, cols), 0, cols + 1, 250, 99999])
            y = rng.choice([rng.randint(1, rows), rng.randint(1, rows), rows, 0, rows + 1, 99999])
            k = b"\x1b[<%d;%d;%d%s" % (b, x, y, rng.choice([b"M", b"M", b"m"]))
        elif r < 0.55:
            k = rng.choice(KEYS)
        elif r < 0.95:
            k = bytes([rng.randint(0x20, 0x7e)])
        else:
            k = bytes([rng.randint(0, 255)])  # junk, incl. high bytes
        s.sock.sendall(k)
        # Drain like a real terminal would. Without this the door
        # blocks writing screen updates while we block sending keys --
        # a test-harness deadlock, not a door bug.
        s.read(0.01)
    s.read(2.0)
    check(s.proc.poll() is None, "door survived %d random keys (seed %d)" % (count, seed))
    # get out of whatever state we're in: Esc until the main menu, Q, then
    # Y at "Quit?" or N at "Save your drawing first?"
    answer = b"y"
    for _ in range(8):
        if any("Quit ANetDRAW?" in s.row(y) for y in range(rows)):
            break
        if any("Save your drawing before" in s.row(y) for y in range(rows)):
            answer = b"n"
            break
        if any("Q Quit" in s.row(y) for y in range(rows)):
            s.send(b"q", 0.8)
        else:
            s.send(b"\x1b", 0.8)
    s.send(answer, 2.0)
    try:
        rc = s.proc.wait(10)
    except Exception:
        s.proc.kill()
        rc = "HUNG"
    check(rc == 0, "clean exit after fuzzing (rc=%s)" % rc)
    print("\n%d failure(s)" % len(FAILS))
    sys.exit(1 if FAILS else 0)


if __name__ == "__main__":
    main()
