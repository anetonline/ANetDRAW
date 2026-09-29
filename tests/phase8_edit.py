#!/usr/bin/env python3
"""Insert/delete line & column (^D) and the Select tool's block extras
(flip, swap FG/BG, center, replace a color), over the real DOOR32.SYS
socket path at 80x25.

Usage: tests/phase8_edit.py path/to/anetdraw
"""
import os
import sys
import tempfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from phase1_session import check, FAILS  # noqa: E402
from phase2_tools import Door, ctrl, ESC, ENTER  # noqa: E402

RIGHT, LEFT = b"\x1b[C", b"\x1b[D"


def press(b, x, y):
    return ("\x1b[<%d;%d;%dM" % (b, x + 1, y + 1)).encode()


def drag(b, x, y):
    return ("\x1b[<%d;%d;%dM" % (b + 32, x + 1, y + 1)).encode()


def release(b, x, y):
    return ("\x1b[<%d;%d;%dm" % (b, x + 1, y + 1)).encode()


def mark(d, x0, y0, x1, y1):
    d.send(press(0, x0, y0) + drag(0, x1, y1) + release(0, x1, y1), 0.6)


def text(d, y, x0, n):
    return "".join(d.cell(x, y).data for x in range(x0, x0 + n))


def main():
    binary = os.path.abspath(sys.argv[1])
    root = tempfile.mkdtemp(prefix="anetdraw_edit_")
    d = Door(binary, extra_args=("--data", os.path.join(root, "data")))
    d.read(d.startup_secs)
    d.send(b" ", 1.0)

    # ---------------- ^D: lines and columns
    d.send(b"AAA\rBBB\rCCC", 0.8)
    d.goto(1, 1)
    d.send(ctrl("D"), 0.8)
    check(any("Lines & columns" in d.row(y) for y in range(25)), "^D opens the lines & columns menu")
    d.send(b"i", 0.8)
    check([text(d, y, 0, 3) for y in range(4)] == ["AAA", "   ", "BBB", "CCC"], "I inserts a line at the cursor: %r"
          % [text(d, y, 0, 3) for y in range(4)])
    d.send(ctrl("D") + b"y", 0.8)
    check([text(d, y, 0, 3) for y in range(3)] == ["AAA", "BBB", "CCC"], "Y deletes it again")
    d.goto(1, 0)
    d.send(ctrl("D") + b"c", 0.8)
    check(text(d, 0, 0, 4) == "A AA" and text(d, 1, 0, 4) == "B BB", "C inserts a column: %r" % text(d, 0, 0, 4))
    d.send(ctrl("D") + b"x", 0.8)
    check(text(d, 0, 0, 4) == "AAA " and text(d, 2, 0, 4) == "CCC ", "X deletes it")
    d.send(ctrl("Z"), 0.8)
    check(text(d, 0, 0, 4) == "A AA", "one ^Z undoes a whole column delete")
    d.send(ctrl("Z"), 0.8)

    # ---------------- block extras (Select tool)
    d.goto(0, 5)
    d.send(b"(AB>", 0.6)
    d.send(ctrl("T") + b"k", 0.8)
    mark(d, 0, 5, 3, 5)
    d.send(b"h", 0.8)
    check(text(d, 5, 0, 4) == "<BA)", "H flips left-right, mirroring the glyphs: %r" % text(d, 5, 0, 4))
    d.send(b"s", 0.8)
    d.send(ESC, 0.6)                   # unmark (a marked block shows inverted)
    c = d.cell(1, 5)
    check(c.fg == "black" and c.bg == "white", "S swaps FG and BG (%s on %s)" % (c.fg, c.bg))
    mark(d, 0, 5, 3, 5)
    d.send(b"s", 0.8)
    # V: two rows swap places
    d.send(ESC, 0.5)
    d.goto(0, 7)
    d.send(ctrl("T") + b"d", 0.5)
    d.send(b"top\rbot", 0.6)
    d.send(ctrl("T") + b"k", 0.8)
    mark(d, 0, 7, 2, 8)
    d.send(b"v", 0.8)
    check(text(d, 7, 0, 3) == "bot" and text(d, 8, 0, 3) == "top", "V flips upside-down")
    # N: center within the block
    d.send(ESC, 0.5)
    d.goto(0, 10)
    d.send(ctrl("T") + b"d", 0.5)
    d.send(b"AB", 0.5)
    d.send(ctrl("T") + b"k", 0.8)
    mark(d, 0, 10, 5, 10)
    d.send(b"n", 0.8)
    check(text(d, 10, 0, 6) == "  AB  ", "N centers the row within the block: %r" % text(d, 10, 0, 6))
    # L: replace color 7 (light gray) with 12 in the foreground
    mark(d, 0, 10, 5, 10)
    d.send(b"l", 0.8)
    check(any("Replace a color" in d.row(y) for y in range(25)), "L opens Replace a color")
    d.send(b"\t", 0.3)                 # to "Change to"
    d.send(RIGHT * 5, 0.5)             # 7 -> 12
    d.send(ENTER, 0.8)
    msg = d.status()
    d.send(ESC, 0.6)                   # unmark to see the real colors
    c = d.cell(2, 10)
    check(c.data == "A" and c.fg == "red" and c.bold and "Replaced color 7 with 12" in msg,
          "Enter replaces it in the block: %s/%s %r" % (c.fg, c.bold, msg[16:60]))
    # the right-click menu lists them
    mark(d, 0, 10, 5, 10)
    d.send(press(2, 2, 10) + release(2, 2, 10), 0.8)
    rows = "".join(d.row(y) for y in range(25))
    check(all(t in rows for t in ("Flip left-right", "Flip upside-down", "Swap FG / BG", "Replace a color", "Center each row")),
          "the Block menu lists the new commands")
    d.send(ESC, 0.6)

    for _ in range(5):                 # back out: mark > tool > main menu
        d.send(ESC, 1.0)
        if any("Q Quit" in d.row(y) for y in range(25)):
            break
    d.send(b"q", 1.0)
    d.send(b"n" if "Save your drawing" in d.status() else b"y", 1.5)
    d.proc.wait(5)
    print("\n%d failure(s)" % len(FAILS))
    sys.exit(1 if FAILS else 0)


if __name__ == "__main__":
    main()
