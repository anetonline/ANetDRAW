#!/usr/bin/env python3
"""Phase 2 (draw tools) end-to-end test for ANetDRAW.

Same real-BBS-path harness as phase1_session.py (DOOR32.SYS socket,
pyte VT emulator answering the size query). Every check is about what
the caller's screen shows after real keystrokes.

Usage: tests/phase2_tools.py path/to/anetdraw
"""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from phase1_session import Session, check, FAILS  # noqa: E402

UP, DOWN, RIGHT, LEFT = b"\x1b[A", b"\x1b[B", b"\x1b[C", b"\x1b[D"
HOME, INSERT, F4 = b"\x1b[H", b"\x1b[2~", b"\x1b[14~"
ESC, TAB, ENTER, BKSP = b"\x1b", b"\t", b"\r", b"\x7f"


def ctrl(c):
    return bytes([ord(c.upper()) - 64])


class Door(Session):
    def goto(self, x, y):
        dx = x - self.screen.cursor.x
        dy = y - self.screen.cursor.y
        seq = (RIGHT * dx if dx > 0 else LEFT * -dx) + (DOWN * dy if dy > 0 else UP * -dy)
        if seq:
            self.send(seq, 0.8)

    def tool(self, letter):
        self.send(ctrl("T"), 0.8)
        self.send(letter.encode(), 0.8)

    def ch(self, x, y):
        return self.cell(x, y).data

    def status(self):
        return self.row(24)


def main():
    binary = os.path.abspath(sys.argv[1])
    d = Door(binary)
    d.read(d.startup_secs)
    d.send(b" ", 1.0)  # past the splash
    d.send(ctrl("L"), 0.8)

    # ---- Line: preview, cancel, commit
    d.tool("L")
    check("Line Brush" in d.status(), "^T L selects the Line tool: %r" % d.status()[16:31])
    d.send(b"#")
    d.goto(0, 0)
    d.send(b" ")
    d.goto(5, 2)
    check(d.ch(0, 0) == "#" and d.ch(5, 2) == "#", "line previews live while the anchor is set")
    d.send(ESC, 1.0)
    check(d.ch(0, 0) == " " and d.ch(5, 2) == " ", "Esc cancels the line -- preview gone, nothing drawn")
    d.goto(0, 0)
    d.send(b" ")
    d.goto(5, 2)
    d.send(ENTER, 1.0)
    check(d.ch(0, 0) == "#" and d.ch(5, 2) == "#" and d.ch(3, 1) == "#", "Enter commits the line")

    # ---- Box, double-line style
    d.tool("B")
    d.send(TAB * 3)
    check("Box Double" in d.status(), "Tab x3 -> Box Double: %r" % d.status()[16:31])
    d.goto(10, 0)
    d.send(b" ")
    d.goto(15, 3)
    d.send(b" ", 1.0)
    corners = (d.ch(10, 0), d.ch(15, 0), d.ch(10, 3), d.ch(15, 3), d.ch(12, 0), d.ch(10, 1))
    check(corners == ("╔", "╗", "╚", "╝", "═", "║"), "double box drawn with correct corners: %r" % (corners,))

    # ---- Fill inside the box (glyph mode, brush '.')
    d.tool("F")
    d.send(b".")
    d.goto(12, 1)
    d.send(b" ", 1.0)
    inside = {d.ch(x, y) for x in range(11, 15) for y in (1, 2)}
    check(inside == {"."}, "flood fill filled the box interior: %r" % inside)
    check(d.ch(16, 1) == " " and d.ch(9, 1) == " ", "fill stayed inside the box walls")

    # ---- Shade brush
    d.tool("S")
    d.goto(20, 0)
    d.send(b"   ", 1.0)
    check(d.ch(20, 0) == "▓", "3 shade steps -> dark shade: %r" % d.ch(20, 0))
    d.send(BKSP, 1.0)
    check(d.ch(20, 0) == "▒", "Backspace lightens one step: %r" % d.ch(20, 0))

    # ---- Pixel (half-block) painting
    d.goto(25, 0)
    d.tool("P")
    check("Pixel Top" in d.status(), "Pixel tool starts on the top half")
    d.send(b" ", 1.0)
    check(d.ch(25, 0) == "▀", "top pixel -> upper half block: %r" % d.ch(25, 0))
    d.send(DOWN, 0.8)
    check("Pixel Bot" in d.status() and d.screen.cursor.y == 0, "Down moves to the bottom half of the same cell")
    d.send(ctrl("F"))  # fg 7 -> 8 (dark gray, a bright color)
    d.send(b" ", 1.0)
    c = d.cell(25, 0)
    check(c.data == "▄" and c.fg == "black" and c.bold and c.bg == "white",
          "gray bottom under light-gray top -> lower half block, non-bright bg (%s fg=%s bold=%s bg=%s)"
          % (c.data, c.fg, c.bold, c.bg))
    for _ in range(15):
        d.send(ctrl("F"), 0.1)  # back to fg 7
    d.read(0.5)

    # ---- Pixel ellipse, solid
    d.tool("E")
    d.send(TAB * 3)
    check("Ellipse PxSolid" in d.status(), "Tab x3 -> Ellipse PxSolid: %r" % d.status()[16:31])
    d.goto(30, 0)
    d.send(b" ")
    d.goto(40, 4)
    d.send(DOWN, 0.5)  # bottom half of row 4
    d.send(b" ", 1.0)
    check(d.ch(35, 2) == "█", "solid ellipse is solid in the middle: %r" % d.ch(35, 2))
    check(d.ch(30, 0) == " " and d.ch(40, 0) == " ", "ellipse corners stay empty")
    check(any(d.ch(x, 0) in "▄▀█" for x in range(30, 41)), "top edge uses half blocks")

    # ---- Draw tool pen mode with an F-key brush
    d.tool("D")
    d.goto(0, 6)
    d.send(TAB)
    check("Draw Pen" in d.status(), "Tab turns the Draw pen on")
    d.send(F4, 0.8)  # full block: placed + becomes the brush
    d.send(RIGHT * 3, 1.0)
    row6 = d.row(6)[:6]
    # F4 at col 0, then 3 moves: cols 0-4 all painted (no gap at col 1), col 5 untouched
    check(row6 == "█████ ", "pen paints a continuous stroke from the F-key glyph on: %r" % row6)
    d.send(TAB)

    # ---- Mirror left/right, with glyph flipping
    d.send(ctrl("R"))
    d.goto(2, 10)
    d.send(b"Z(", 1.0)
    check(d.ch(77, 10) == "Z" and d.ch(76, 10) == ")", "mirror H copies to the other side, ( becomes ): %r %r"
          % (d.ch(77, 10), d.ch(76, 10)))
    d.send(ctrl("R") * 3, 0.8)
    check("M" not in d.status()[34:36], "^R cycles back to mirror off")

    # ---- Select / copy / paste / flip / move
    d.tool("K")
    d.goto(10, 0)
    d.send(b" ")
    d.goto(15, 3)
    check("Mark 6x4" in d.status(), "marking shows the block size: %r" % d.status()[16:31])
    d.send(b"c", 1.0)
    d.goto(40, 5)
    d.send(ctrl("V"), 1.0)
    check(d.ch(40, 5) == "╔", "^V floats the clipboard at the cursor")
    d.send(ctrl("L"), 0.8)
    check("PASTE 6x4" in d.status(), "status shows PASTE 6x4: %r" % d.status()[16:32])
    d.send(ENTER, 0.8)
    d.send(ESC, 1.0)
    check(d.ch(40, 5) == "╔" and d.ch(45, 8) == "╝" and d.ch(42, 6) == ".", "Enter stamps the paste")
    # the diagonal line from (0,0) to (5,2) is asymmetric -- flip it
    d.tool("K")
    d.goto(0, 0)
    d.send(b" ")
    d.goto(5, 2)
    d.send(b"c", 1.0)
    d.send(ctrl("V"))
    d.send(b"h")
    d.goto(50, 12)
    d.send(ENTER)
    d.send(ESC, 1.0)
    check(d.ch(55, 12) == "#" and d.ch(50, 12) == " " and d.ch(50, 14) == "#",
          "H flips the clipboard left/right: %r" % d.row(12)[50:56])
    d.tool("K")
    d.goto(40, 5)
    d.send(b" ")
    d.goto(45, 8)
    d.send(b"m", 1.0)
    d.send(RIGHT * 20, 1.0)
    d.send(ENTER)
    d.send(ESC, 1.0)
    check(d.ch(40, 5) == " " and d.ch(60, 5) == "╔", "M moves the block (old spot blank)")

    # ---- Insert mode in Draw
    d.tool("D")
    d.goto(0, 12)
    d.send(b"abc")
    d.send(HOME)
    d.send(INSERT)
    check("INS" in d.status(), "Insert toggles insert mode")
    d.send(b"X", 1.0)
    check(d.row(12).startswith("Xabc"), "insert mode pushes the row right: %r" % d.row(12)[:6])
    d.send(BKSP, 1.0)
    check(d.row(12).startswith("abc"), "Backspace in insert mode pulls the row left: %r" % d.row(12)[:6])
    d.send(INSERT)

    # ---- Esc backs out one level at a time
    d.tool("L")
    d.send(b" ")
    check("*Line" in d.status(), "anchor shows as *Line")
    d.send(ESC, 1.0)
    check("Line" in d.status() and "*Line" not in d.status(), "Esc 1: drops the anchor")
    d.send(ESC, 1.0)
    check("Draw" in d.status(), "Esc 2: back to the Draw tool")
    d.send(ESC, 1.0)
    check(any("Q Quit" in d.row(y) for y in range(24)), "Esc 3: main menu")
    d.send(b"q", 1.0)
    check("Save your drawing before you quit" in d.status(), "menu Q: asks to save the unsaved drawing")
    d.send(ESC, 1.0)

    # ---- Popups draw and restore
    d.send(ctrl("O"), 1.0)
    check(any("ANetDRAW keys" in d.row(y) for y in range(24)), "^O shows the help popup")
    d.send(b" ", 1.0)
    check(not any("ANetDRAW keys" in d.row(y) for y in range(24)) and d.ch(10, 0) == "╔",
          "help closes and the drawing is back")
    d.send(ctrl("T"), 1.0)
    check(any("Select" in d.row(y) and "Tools" in "".join(d.row(yy) for yy in range(24)) for y in range(24)),
          "^T shows the tools popup")
    d.send(ESC, 1.0)
    check(d.ch(10, 0) == "╔", "Esc closes the tools popup cleanly")

    # ---- wire hygiene over the whole session
    bare = sum(1 for i, b in enumerate(d.raw) if b == 10 and (i == 0 or d.raw[i - 1] != 13))
    check(bare == 0, "no bare LF in the whole session")
    check(0xE2 not in d.raw, "pure CP437 output (no UTF-8)")
    check(d.screen.buffer[24][79].data == " ", "bottom-right cell never written")

    d.send(ESC, 1.0)
    d.send(b"q", 1.0)
    d.send(b"n", 1.5)
    rc = d.proc.wait(5)
    check(rc == 0, "clean exit (rc=%s)" % rc)

    print("\n%d failure(s)" % len(FAILS))
    sys.exit(1 if FAILS else 0)


if __name__ == "__main__":
    main()
