#!/usr/bin/env python3
"""The shared wall: two callers on different nodes (two door processes,
one data folder) draw on it together, see each other's work and cursors,
come and go; a third caller later finds the art still there.

Usage: tests/phase8_wall.py path/to/anetdraw
"""
import os
import sys
import tempfile
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from phase1_session import check, FAILS  # noqa: E402
from phase2_tools import ESC  # noqa: E402
from phase6_gallery import Caller  # noqa: E402


def node(binary, data, who, nodenum):
    cls = type("N", (Caller,), {"who": who})
    d = cls(binary, extra_args=("--data", data, "-N", str(nodenum)))
    d.read(d.startup_secs)
    d.send(b" ", 1.0)
    return d


def status(d):
    return d.row(d.screen.lines - 1)


def text(d, y, x0, n):
    return "".join(d.cell(x, y).data for x in range(x0, x0 + n))


def wait_for(d, cond, secs=4.0):
    end = time.time() + secs
    while time.time() < end:
        d.read(0.3)
        if cond():
            return True
    return False


def main():
    binary = os.path.abspath(sys.argv[1])
    root = tempfile.mkdtemp(prefix="anetdraw_wall_")
    data = os.path.join(root, "data")

    a = node(binary, data, (1, "Alice Artist", 100), 1)
    a.send(ESC, 1.2)
    check(any("J Shared wall" in a.row(y) for y in range(25)), "the menu offers J Shared wall")
    a.send(b"j", 1.2)
    check("shared wall" in status(a), "Alice joins the wall: %r" % status(a)[16:70])
    a.send(b"HELLO", 0.8)

    b = node(binary, data, (2, "Bob Builder", 100), 2)
    b.send(ESC, 1.2)
    b.send(b"j", 1.5)
    check(text(b, 0, 0, 5) == "HELLO", "Bob joins and sees Alice's HELLO: %r" % text(b, 0, 0, 5))
    check("1 other" in status(b), "Bob is told someone else is here: %r" % status(b)[16:70])
    check(wait_for(a, lambda: "Bob Builder joined the wall" in status(a)), "Alice is told Bob joined: %r" % status(a)[16:60])

    # Bob writes; Alice sees it without pressing anything
    b.goto(0, 2)
    b.send(b"WORLD", 0.8)
    check(wait_for(a, lambda: text(a, 2, 0, 5) == "WORLD"), "Alice sees Bob's WORLD appear: %r" % text(a, 2, 0, 5))
    # Alice's cursor shows on Bob's screen as her initial, on a color
    a.goto(10, 5)
    check(wait_for(b, lambda: b.cell(10, 5).data == "A" and b.cell(10, 5).bg != "default", 5.0),
          "Bob sees Alice's cursor as an 'A' marker at (11,6): %r/%s" % (b.cell(10, 5).data, b.cell(10, 5).bg))

    # New is blocked on the wall
    b.send(ESC, 1.2)
    b.send(b"n", 1.0)
    check("leave it first" in status(b), "New is blocked on the wall: %r" % status(b)[16:70])

    # Bob leaves
    b.send(ESC, 1.2)
    b.send(b"j", 1.2)
    check("You left the wall" in status(b), "Bob leaves the wall: %r" % status(b)[16:60])
    check(wait_for(a, lambda: "Bob Builder left the wall" in status(a), 5.0), "Alice is told Bob left: %r" % status(a)[16:60])

    # quitting on the wall doesn't ask to save (it's always saved)
    a.send(ESC, 1.2)
    a.send(b"q", 1.2)
    check("Quit ANetDRAW?" in status(a) or "Save your drawing" not in status(a), "no save question when leaving from the wall")
    a.send(b"y", 1.5)
    a.proc.wait(5)
    b.sock.close()
    b.proc.wait(5)

    # a third caller later: the art is there
    c = node(binary, data, (3, "Cara Coder", 100), 3)
    c.send(ESC, 1.2)
    c.send(b"j", 1.5)
    check(text(c, 0, 0, 5) == "HELLO" and text(c, 2, 0, 5) == "WORLD", "later, Cara finds HELLO and WORLD still on the wall")
    c.sock.close()
    c.proc.wait(5)

    print("\n%d failure(s)" % len(FAILS))
    sys.exit(1 if FAILS else 0)


if __name__ == "__main__":
    main()
