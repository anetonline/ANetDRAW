#!/usr/bin/env python3
"""Select tool with the mouse, and the docked toolbox, over the real
DOOR32.SYS socket path at 132x37 (SGR mouse reports, as SyncTERM sends).

Usage: tests/select_test.py path/to/anetdraw
"""
import os
import sys
import tempfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from phase1_session import check, FAILS, answer_canvas_choice  # noqa: E402
from phase2_tools import Door, ctrl, ESC  # noqa: E402

COLS, ROWS = 132, 37


def press(b, x, y):   # 0-based cell -> SGR report
    return ("\x1b[<%d;%d;%dM" % (b, x + 1, y + 1)).encode()


def drag(b, x, y):
    return ("\x1b[<%d;%d;%dM" % (b + 32, x + 1, y + 1)).encode()


def release(b, x, y):
    return ("\x1b[<%d;%d;%dm" % (b, x + 1, y + 1)).encode()


def text_at(d, y, x0, n):
    return "".join(d.cell(x, y).data for x in range(x0, x0 + n))


def status(d):
    return d.row(d.screen.lines - 1)


def screen_has(d, t):
    return any(t in d.row(y) for y in range(d.screen.lines))


def main():
    binary = os.path.abspath(sys.argv[1])
    root = tempfile.mkdtemp(prefix="anetdraw_sel_")
    d = Door(binary, extra_args=("--data", os.path.join(root, "data")), cols=COLS, rows=ROWS)
    d.read(d.startup_secs)
    d.send(b" ", 1.0)
    check(answer_canvas_choice(d), "a wide screen asks for the canvas size first")

    # ---------------- docked toolbox
    div = [x for x in range(COLS) if d.cell(x, 3).data == "│"]
    check(div and div[-1] == COLS - 38 - 1, "toolbox docked at the right edge, divider at col %s" % div)
    check(d.cell(80, 3).data == "\u2591" and d.cell(92, 3).data == "\u2591",
          "a dim desk fills the drawing area past the 80-col canvas")
    check("ANetDRAW" in d.row(0)[COLS - 38:], "toolbox title in the last 38 columns")
    d.send(press(0, 85, 3) + release(0, 85, 3), 0.6)
    check(d.cell(85, 3).data == "\u2591" and d.cell(79, 3).data == " ", "a click past the canvas edge draws nothing")

    # ---------------- mark with the mouse
    d.send(b"HELLO", 0.6)
    d.send(ctrl("T"), 0.6)
    d.send(b"k", 0.8)
    check("drag to mark" in status(d), "Select explains itself: %r" % status(d)[16:70])
    d.send(press(0, 0, 0) + drag(0, 2, 0) + drag(0, 4, 0) + release(0, 4, 0), 0.8)
    check("Marked 5x1" in status(d), "a drag marks a block: %r" % status(d)[16:70])
    check(screen_has(d, "5x1 Copy Cut Del Fill Move"), "the toolbox shows the block's buttons")

    # right-click menu -> Copy
    d.send(press(2, 2, 0) + release(2, 2, 0), 0.8)
    check(screen_has(d, "Block") and screen_has(d, "Copy") and screen_has(d, "Select all"), "right-click opens the Block menu")
    d.send(b"c", 0.8)
    check("Copied 5x1" in status(d), "menu Copy copies: %r" % status(d)[16:60])

    # right-click again (nothing marked) -> Paste, then click to stamp
    d.send(press(2, 10, 5) + release(2, 10, 5), 0.8)
    d.send(b"p", 0.6)
    d.send(press(0, 10, 5) + release(0, 10, 5), 0.8)
    d.send(ESC, 0.6)
    check(text_at(d, 5, 10, 5) == "HELLO", "menu Paste, then a click stamps it there: %r" % text_at(d, 5, 10, 5))

    # mark it and press Delete
    d.send(press(0, 10, 5) + drag(0, 14, 5) + release(0, 14, 5), 0.6)
    d.send(b"\x1b[3~", 0.8)
    check(text_at(d, 5, 10, 5) == "     " and "Deleted 5x1" in status(d), "Delete erases the marked block")

    # toolbox buttons: Copy the original, Paste lower down
    d.send(press(0, 0, 0) + drag(0, 4, 0) + release(0, 4, 0), 0.6)
    y23 = d.row(23)
    cx = y23.index("Copy")
    d.send(press(0, cx, 23) + release(0, cx, 23), 0.8)
    check("Copied 5x1" in status(d), "the toolbox Copy button works: %r" % status(d)[16:60])
    px = d.row(23).index("Paste")
    d.send(press(0, px, 23) + release(0, px, 23), 0.6)
    d.send(press(0, 3, 8) + release(0, 3, 8), 0.6)
    d.send(ESC, 0.6)
    check(text_at(d, 8, 3, 5) == "HELLO", "the toolbox Paste button, then a click: %r" % text_at(d, 8, 3, 5))

    # select all + cut, then undo it
    d.send(b"a", 0.6)
    check("Marked everything" in status(d), "A marks everything")
    d.send(b"x", 0.8)
    check(text_at(d, 0, 0, 5) == "     " and text_at(d, 8, 3, 5) == "     ", "X cuts it all")
    d.send(ctrl("Z"), 0.8)
    check(text_at(d, 0, 0, 5) == "HELLO" and text_at(d, 8, 3, 5) == "HELLO", "^Z brings it back")

    d.send(ESC, 0.8)
    d.send(ESC, 1.0)
    d.send(b"q", 1.0)
    d.send(b"n" if "Save your drawing" in status(d) else b"y", 1.5)
    d.proc.wait(5)
    # ---------------- Colorize: recolor by dragging, the drawing stays
    g = Door(binary, extra_args=("--data", os.path.join(root, "data")), cols=COLS, rows=ROWS)
    g.read(g.startup_secs)
    g.send(b" ", 1.0)
    answer_canvas_choice(g)
    g.send(b"HELLO", 0.6)
    g.send(ctrl("T"), 0.6)
    check(screen_has(g, "C Colorize"), "^T menu offers C Colorize")
    g.send(b"c", 0.8)
    check("Colorize" in status(g), "Colorize explains itself: %r" % status(g)[16:70])
    g.send(ctrl("F") * 5, 0.4)          # fg -> 12 bright red
    g.send(ctrl("B"), 0.4)              # bg -> 1 blue
    g.send(press(0, 0, 0) + drag(0, 2, 0) + drag(0, 4, 0) + release(0, 4, 0), 0.8)
    cells = [g.cell(x, 0) for x in range(6)]
    check("".join(c.data for c in cells[:5]) == "HELLO", "a drag keeps the letters: %r" % "".join(c.data for c in cells[:5]))
    check(all(c.fg == "red" and c.bold and c.bg == "blue" for c in cells[:5]),
          "and gives them the current colors: %s/%s/%s" % (cells[2].fg, cells[2].bold, cells[2].bg))
    check(cells[5].bg != "blue", "cells not dragged over keep their colors")
    g.send(ctrl("Z"), 0.8)
    check(g.cell(2, 0).data == "L" and g.cell(2, 0).bg != "blue", "one ^Z undoes the whole drag")
    # FG only: the background stays
    g.send(b"\t", 0.4)
    check("Colorize FG" in g.row(15)[COLS - 38:] or "Colorize FG" in status(g), "Tab switches to FG only")
    g.send(press(0, 0, 0) + drag(0, 4, 0) + release(0, 4, 0), 0.8)
    check(g.cell(1, 0).fg == "red" and g.cell(1, 0).bg != "blue", "FG only leaves the background alone")
    # Select a block, R recolors it
    g.send(ctrl("T"), 0.6)
    g.send(b"k", 0.8)
    g.send(press(0, 0, 0) + drag(0, 4, 0) + release(0, 4, 0), 0.6)
    g.send(ctrl("F"), 0.4)              # fg -> 13
    g.send(b"r", 0.8)
    check(g.cell(3, 0).data == "L" and g.cell(3, 0).fg == "magenta" and "Recolored 5x1" in status(g),
          "R recolors a marked block: %s %r" % (g.cell(3, 0).fg, status(g)[16:50]))
    g.sock.close()
    g.proc.wait(5)

    # ---------------- the toolbox's File / Canvas buttons (tall screens)
    f = Door(binary, extra_args=("--data", os.path.join(root, "data")), cols=COLS, rows=ROWS)
    f.read(f.startup_secs)
    f.send(b" ", 1.0)
    answer_canvas_choice(f)
    rowof = lambda t: next((y for y in range(ROWS) if t in f.row(y)[COLS - 38:]), -1)
    check(rowof("Gallery Publish Download") > 0 and rowof("Undo Redo Fonts Colors Chars") > 0,
          "the toolbox has File and Canvas buttons below")
    f.send(b"ABC", 0.6)
    y = rowof("Undo Redo")
    ux = COLS - 38 + f.row(y)[COLS - 38:].index("Undo")
    f.send(press(0, ux, y) + release(0, ux, y), 0.8)
    check(text_at(f, 0, 0, 3) == "AB ", "the Undo button undoes one step: %r" % text_at(f, 0, 0, 3))
    f.send(press(0, ux + 5, y) + release(0, ux + 5, y), 0.8)
    check(text_at(f, 0, 0, 3) == "ABC", "the Redo button redoes it: %r" % text_at(f, 0, 0, 3))
    y = rowof("Size")
    sx = COLS - 38 + f.row(y)[COLS - 38:].index("Size")
    f.send(press(0, sx, y) + release(0, sx, y), 0.8)
    check(screen_has(f, "Canvas size") and screen_has(f, "Fit the screen"), "the Size button opens Canvas size")
    f.send(ESC, 0.8)
    y = rowof("Save as")
    ax = COLS - 38 + f.row(y)[COLS - 38:].index("Save as")
    f.send(press(0, ax, y) + release(0, ax, y), 0.8)
    check(screen_has(f, "Save drawing"), "the Save as button opens the save browser")
    f.send(ESC, 0.8)
    f.sock.close()
    f.proc.wait(5)

    # ---------------- the canvas question's other answers
    for key, want, toolbox in ((b"b", "93x37", True), (b"c", "132x37", False)):
        e = Door(binary, extra_args=("--data", os.path.join(root, "data")), cols=COLS, rows=ROWS)
        e.read(e.startup_secs)
        e.send(b" ", 1.0)
        check(any("Fill the space beside the toolbox" in e.row(y) for y in range(ROWS)) and
              any("Your whole screen" in e.row(y) for y in range(ROWS)), "the question offers B and C")
        e.send(key, 1.0)
        check(("Canvas %s" % want) in status(e), "%s gives a %s canvas: %r" % (key, want, status(e)[16:60]))
        check(("ANetDRAW" in e.row(0)[COLS - 38:]) == toolbox, "%s: toolbox %s" % (key, "shown" if toolbox else "tucked away"))
        x = 120 if not toolbox else 90
        e.send(press(0, x, 5) + release(0, x, 5), 0.6)
        check(e.cell(x, 5).data == "\u2588", "%s: drawing reaches column %d" % (key, x + 1))
        e.sock.close()
        e.proc.wait(5)

    print("\n%d failure(s)" % len(FAILS))
    sys.exit(1 if FAILS else 0)


if __name__ == "__main__":
    main()
