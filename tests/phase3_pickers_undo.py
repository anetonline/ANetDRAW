#!/usr/bin/env python3
"""Phase 3 (pickers + undo/redo) end-to-end test for ANetDRAW, over the
real DOOR32.SYS socket path with a pyte-emulated terminal.

Usage: tests/phase3_pickers_undo.py path/to/anetdraw
"""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from phase1_session import check, FAILS  # noqa: E402
from phase2_tools import Door, ctrl, RIGHT, LEFT, UP, DOWN, ESC, TAB, ENTER  # noqa: E402

F1 = b"\x1b[11~"


def main():
    binary = os.path.abspath(sys.argv[1])
    d = Door(binary)
    d.read(d.startup_secs)
    d.send(b" ", 1.0)
    d.send(ctrl("L"), 0.8)

    # ---- undo / redo, one keypress per step
    d.send(b"abc", 1.0)
    d.send(ctrl("Z"), 0.8)
    check(d.row(0).startswith("ab "), "^Z undoes the last keypress: %r" % d.row(0)[:4])
    d.send(ctrl("Z") * 2, 1.0)
    check(d.row(0).startswith("   "), "^Z twice more undoes back to blank: %r" % d.row(0)[:4])
    d.send(ctrl("Z"), 0.8)
    check("Nothing to undo" in d.status(), "extra ^Z says there's nothing to undo")
    d.send(ctrl("Y"), 0.8)
    check(d.row(0).startswith("a  "), "^Y redoes: %r" % d.row(0)[:4])
    d.send(RIGHT + LEFT + DOWN + UP, 1.0)  # moving around must not cost the redo steps
    d.send(ctrl("Y"), 0.8)
    check(d.row(0).startswith("ab "), "redo survives cursor movement: %r" % d.row(0)[:4])
    d.goto(5, 0)
    d.send(b"X", 0.8)
    d.send(ctrl("Y"), 0.8)
    check("Nothing to redo" in d.status(), "a new edit drops the redo history")

    # ---- a flood fill undoes as one step
    d.tool("B")
    d.send(TAB * 2)  # single-line box
    d.goto(10, 2)
    d.send(b" ")
    d.goto(20, 6)
    d.send(b" ", 1.0)
    d.tool("F")
    d.send(b"*")
    d.goto(15, 4)
    d.send(b" ", 1.0)
    check(all(d.ch(x, y) == "*" for x in range(11, 20) for y in range(3, 6)), "fill painted the box interior")
    d.send(ctrl("Z"), 1.0)
    check(all(d.ch(x, y) == " " for x in range(11, 20) for y in range(3, 6)) and d.ch(10, 2) == "┌",
          "one ^Z undoes the whole fill, box stays")
    d.send(ctrl("Z"), 1.0)
    check(d.ch(10, 2) == " " and d.ch(20, 6) == " ", "next ^Z undoes the whole box")
    d.send(ctrl("Y") * 2, 1.2)
    check(d.ch(10, 2) == "┌" and d.ch(15, 4) == "*", "^Y ^Y brings box and fill back")

    # ---- color picker
    d.tool("D")
    d.send(ctrl("K"), 1.0)
    check(any("Colors (iCE)" in d.row(y) for y in range(24)), "^K opens the color picker (iCE)")
    d.send(RIGHT * 3 + DOWN * 2, 1.0)  # fg 7 -> 10, bg 0 -> 2
    check(any("F10/B02" in d.row(y) for y in range(24)), "arrows move the picker selection")
    d.send(ENTER, 1.0)
    check(not any("Colors" in d.row(y) for y in range(24)), "Enter closes the picker")
    d.goto(0, 10)
    d.send(b"Q", 0.8)
    c = d.cell(0, 10)
    check(c.data == "Q" and c.fg == "green" and c.bold and c.bg == "green",
          "picked colors are used (fg=%s bold=%s bg=%s)" % (c.fg, c.bold, c.bg))
    d.send(ctrl("K"), 0.8)
    d.send(RIGHT * 5, 0.8)
    d.send(ESC, 1.0)
    d.send(b"R", 0.8)
    c = d.cell(1, 10)
    check(c.fg == "green" and c.bg == "green", "Esc leaves the colors alone")

    # ---- character picker: brush + custom F-key slot
    d.send(ctrl("G"), 1.0)
    check(any("Characters" in d.row(y) for y in range(24)), "^G opens the character picker")
    d.send(UP * 6 + LEFT * 24, 1.0)  # from #219 to #003 (heart)
    check(any("#003" in d.row(y) for y in range(24)), "picker navigates to glyph #003")
    d.send(F1, 1.0)  # put the heart in F1 of the current set
    d.send(ENTER, 1.0)
    check(d.ch(2, 10) == "♥", "Enter places the picked glyph in the Draw tool: %r" % d.ch(2, 10))
    d.send(ctrl("L"), 0.8)
    check("1♥" in d.status(), "F1 slot of the set now shows the heart")
    d.send(F1, 0.8)
    check(d.ch(3, 10) == "♥", "F1 now places the heart")

    # ---- pixel painting undoes cleanly
    d.tool("P")
    d.goto(40, 12)
    d.send(b" ", 0.8)
    d.send(ctrl("Z"), 0.8)
    check(d.ch(40, 12) == " ", "pixel paint undoes")

    # ---- wire hygiene
    bare = sum(1 for i, b in enumerate(d.raw) if b == 10 and (i == 0 or d.raw[i - 1] != 13))
    check(bare == 0, "no bare LF")
    check(d.screen.buffer[24][79].data == " ", "bottom-right cell never written")

    d.send(ESC, 0.8)
    d.send(ESC, 1.0)
    d.send(b"q", 1.0)
    d.send(b"n", 1.5)  # don't save
    rc = d.proc.wait(5)
    check(rc == 0, "clean exit (rc=%s)" % rc)
    print("\n%d failure(s)" % len(FAILS))
    sys.exit(1 if FAILS else 0)


if __name__ == "__main__":
    main()
