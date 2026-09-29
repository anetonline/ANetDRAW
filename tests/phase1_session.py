#!/usr/bin/env python3
"""Phase 1 end-to-end session test for ANetDRAW.

Drives the real compiled door over the real BBS path (DOOR32.SYS, comm
type 2, socket handle) -- not -L local mode, which OpenDoors silently
switches to UTF-8 output under a UTF-8 locale -- and feeds everything
the door sends into a pyte VT emulator so assertions are about what a
caller's screen actually shows, not just which bytes went by.

Usage: tests/phase1_session.py path/to/anetdraw
"""
import os
import re
import select
import socket
import subprocess
import sys
import tempfile
import time

import pyte

# CP437 glyphs for 0x01-0x1F. The door only sends these as raw bytes
# for codes SyncTERM draws as glyphs (never 07/08/09/0A/0D/1B -- those
# get substituted by ad_display_glyph()), so the harness maps them to
# glyphs too instead of letting pyte treat e.g. 0x0E as Shift-Out.
LOW_GLYPHS = " ☺☻♥♦♣♠•◘○◙♂♀♪♫☼" \
             "►◄↕‼¶§▬↨↑↓→←∟↔▲▼"
REAL_CONTROLS = {0x07, 0x08, 0x09, 0x0A, 0x0D, 0x1B}


def cp437_to_str(data):
    out = []
    for b in data:
        if b in REAL_CONTROLS:
            out.append(chr(b))
        elif 0 < b < 0x20:
            out.append(LOW_GLYPHS[b])
        else:
            out.append(bytes([b]).decode("cp437"))
    return "".join(out)


class AnsweringScreen(pyte.Screen):
    """A pyte screen that answers device status reports (CSI 6n) back
    down the socket, like a real terminal does."""
    sock = None
    answer = True

    def write_process_input(self, data):
        if self.sock is not None and self.answer:
            self.sock.sendall(data.encode("ascii"))


class Session:
    def __init__(self, binary, rows=25, answer_cpr=True, extra_args=(), cols=80):
        self.dir = tempfile.mkdtemp(prefix="anetdraw_test_")
        self.screen = AnsweringScreen(cols, rows)
        self.screen.answer = answer_cpr
        self.stream = pyte.Stream(self.screen)
        self.raw = b""
        self._start_posix(binary, extra_args)
        self.screen.sock = self.sock

    def _start_posix(self, binary, extra_args):
        self.startup_secs = 2.0
        self.sock, child = socket.socketpair()
        child.set_inheritable(True)
        drop = os.path.join(self.dir, "door32.sys")
        with open(drop, "w") as f:
            f.write("2\n%d\n38400\nANetBBS\n1\nTest Artist\nArtist\n100\n60\n1\n1\n"
                    % child.fileno())
        self.proc = subprocess.Popen([binary, "-D", drop] + list(extra_args), pass_fds=(child.fileno(),),
                                     cwd=self.dir, stdin=subprocess.DEVNULL,
                                     stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
        child.close()

    def read(self, secs=0.6):
        got = b""
        end = time.time() + secs
        while time.time() < end:
            r, _, _ = select.select([self.sock], [], [], 0.05)
            if r:
                d = self.sock.recv(65536)
                if not d:
                    break
                got += d
                # Feed as it arrives, so device status reports get
                # answered immediately like a real terminal would.
                self.stream.feed(cp437_to_str(d))
        self.raw += got
        return got

    def send(self, data, secs=0.6):
        self.sock.sendall(data)
        return self.read(secs)

    def cell(self, x, y):
        return self.screen.buffer[y][x]

    def row(self, y):
        return "".join(self.screen.buffer[y][x].data for x in range(self.screen.columns))


FAILS = []


def answer_canvas_choice(d, key=b"a", secs=1.0):
    """On a wide screen a new drawing asks for a canvas size first; pick
    one (default: A, the classic 80x25). No-op when it isn't showing."""
    if any("New drawing: canvas size" in d.row(y) for y in range(d.screen.lines)):
        d.send(key, secs)
        return True
    return False


def check(cond, what):
    print(("PASS " if cond else "FAIL ") + what)
    if not cond:
        FAILS.append(what)


def main():
    binary = os.path.abspath(sys.argv[1])
    s = Session(binary)
    s.read(s.startup_secs)

    # --- splash screen
    splash = open(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "util", "splash.ans"), "rb").read()
    check(splash in s.raw, "door sends the generated splash bytes exactly (%d bytes)" % len(splash))
    check(0xE2 not in s.raw, "splash is pure CP437 (no UTF-8 lead bytes)")
    check("A-Net Online presents" in s.row(1), "splash shows 'A-Net Online presents'")
    check("press any key to start drawing" in s.row(23), "splash prompt on row 24")
    check("\u2588" in s.row(4) or "\u2580" in s.row(3) or "\u2584" in s.row(3), "splash wordmark is drawn in block glyphs")
    check(s.screen.buffer[23][79].data == " " and s.screen.buffer[24][79].data == " ", "splash never paints column 80")
    s.send(b"q", 1.5)  # any key -- must NOT be typed onto the canvas
    check(not s.row(0).startswith("q"), "the splash key press is not drawn on the canvas")

    status = s.row(24)
    check(" (1,1)" in status, "status bar shows the cursor as (col,row) (1,1): %r" % status[:12])
    check("Welcome, Test Artist" in status, "status bar greets the caller by dropfile name")
    check(b"\x1b[?33h" in s.raw, "iCE mode (CSI ?33h) sent at startup")
    check(s.screen.buffer[24][79].data == " ", "bottom-right cell never written")

    s.send(b"H")
    out = s.send(b"i")
    check(s.row(0).startswith("Hi"), "typing 'Hi' lands on row 1: %r" % s.row(0)[:10])
    check((s.screen.cursor.x, s.screen.cursor.y) == (2, 0), "cursor advanced to col 3")
    check(len(out) < 60, "one keystroke sends a small diff (%d bytes: %r)" % (len(out), out))
    check("\u2591" in s.row(24) and "\u2593" in s.row(24) and "^T Tools" in s.row(24),
          "after the first key, status shows set 06 shade glyphs + ^T Tools")
    check("S06" in s.row(24), "status bar shows default F-key set 06")

    # ^F x3: fg 7 -> 10 (bright green). 'X' should be bold green.
    s.send(b"\x06\x06\x06X")
    c = s.cell(2, 0)
    check(c.data == "X" and c.fg == "green" and c.bold, "^F x3 + X -> bright green X (fg=%s bold=%s)" % (c.fg, c.bold))

    # F1 in two wire forms -> set 06 glyph 1 (0xB0 light shade).
    s.send(b"\x1b[11~")
    s.send(b"\x1bOP")
    check(s.cell(3, 0).data == "░" and s.cell(4, 0).data == "░",
          "F1 as ESC[11~ and ESC OP both place the light-shade glyph")
    # ^A 4 -> glyph 4 of set 06 (0xDB full block).
    s.send(b"\x014")
    check(s.cell(5, 0).data == "█", "^A then 4 places full block (F-key fallback)")

    # ^B -> bg 1 (blue); space paints blue background.
    s.send(b"\x02 ")
    check(s.cell(6, 0).bg == "blue", "^B + space paints a blue background (bg=%s)" % s.cell(6, 0).bg)
    # 8 more ^B -> bg 9 (bright blue), iCE on -> SGR 104.
    s.raw = b""
    s.send(b"\x02" * 8 + b" ")
    c = s.cell(7, 0)
    check(c.bg == "brightblue" or c.bg == "blue" and b"104m" in s.raw,
          "bright-blue background via SGR 104 with iCE on (bg=%s)" % c.bg)
    sgr_params = [m.split(b";") for m in re.findall(rb"\x1b\[([0-9;]*)m", s.raw)]
    check(b";104m" in s.raw and not any(b"5" in p for p in sgr_params), "iCE on: SGR 104, no blink (SGR 5)")

    # ^E -> iCE off: CSI ?33l, full redraw, the bright-bg cell becomes blink + blue.
    s.raw = b""
    s.send(b"\x05", 1.0)
    check(b"\x1b[?33l" in s.raw, "^E sends CSI ?33l when iCE turns off")
    c = s.cell(7, 0)
    check(c.blink and c.bg == "blue", "with iCE off, bright bg shows as blink over blue (blink=%s bg=%s)" % (c.blink, c.bg))
    s.send(b"\x0c", 1.0)  # ^L redraw -- also clears the one-shot message
    check("BLK" in s.row(24), "status bar shows BLK (iCE off) once the message clears")
    s.send(b"\x05", 1.0)

    # Enter -> next line col 1; CR LF counts as a single Enter.
    s.send(b"\r\n")
    check((s.screen.cursor.x, s.screen.cursor.y) == (0, 1), "CR LF is one Enter (cursor %r)" % ((s.screen.cursor.x, s.screen.cursor.y),))

    # Backspace as 0x7F (PuTTY/xterm) and 0x08 (SyncTERM).
    s.send(b"abc\x7f")
    check(s.row(1).startswith("ab "), "0x7F backspace erases left: %r" % s.row(1)[:5])
    s.send(b"\x08")
    check(s.row(1).startswith("a  "), "0x08 backspace erases left: %r" % s.row(1)[:5])
    # ^E/^D must NOT be arrow keys (OpenDoors' WordStar mapping is off).
    s.send(b"\x04")
    check(any("Lines & columns" in s.row(y) for y in range(24)),
          "^D is not an arrow key: it opens Lines & columns")
    s.send(b"\x1b", 1.0)   # close that menu
    check((s.screen.cursor.x, s.screen.cursor.y) == (1, 1), "^D does not move the cursor")

    # Scroll: 30 downs, row 1's content scrolls off, status bar intact.
    s.send(b"\x1b[B" * 30, 1.5)
    check(",32)" in s.row(24)[:10], "status shows row 32 after 30 downs: %r" % s.row(24)[:10])
    check(not s.row(0).startswith("Hi"), "viewport scrolled with the cursor")
    s.send(b"\x1b[5~\x1b[5~", 1.0)
    check(s.row(0).startswith("Hi"), "PgUp scrolls back to the top: %r" % s.row(0)[:10])

    # F-key set cycling.
    s.send(b"\x0e")
    check("S07" in s.row(24), "^N moves to set 07")
    s.send(b"\x10\x10")
    check("S05" in s.row(24), "^P twice moves to set 05")

    # Nothing ever scrolled the physical screen: status still on the last row.
    check("S05" in s.row(24) and "^T Tools" in s.row(24), "status bar still on the bottom row")

    # No bare LF anywhere in the session.
    bare = sum(1 for i, b in enumerate(s.raw) if b == 10 and (i == 0 or s.raw[i - 1] != 13))
    check(bare == 0, "no bare LF in output")

    # ESC -> main menu; Q with unsaved work -> "save first?"; Esc there
    # keeps editing; ESC Q N quits without saving, exit 0.
    s.send(b"\x1b", 1.0)
    check(any("Q Quit" in s.row(y) for y in range(24)), "ESC opens the main menu")
    s.send(b"q", 1.0)
    check("Save your drawing before you quit" in s.row(24), "Q with unsaved work asks to save first")
    s.send(b"\x1b", 1.0)
    check(s.proc.poll() is None and "^T Tools" in s.row(24), "Esc at that prompt keeps editing")
    s.raw = b""
    s.send(b"\x1b", 1.0)
    s.send(b"q", 1.0)
    s.send(b"n", 1.5)
    try:
        rc = s.proc.wait(5)
    except subprocess.TimeoutExpired:
        s.proc.kill()
        rc = "HUNG"
    check(rc == 0, "ESC Y exits cleanly (rc=%s)" % rc)
    check(b"\x1b[?33l" in s.raw, "terminal blink mode restored on exit")

    # Hangup: a caller dropping carrier must end the process promptly,
    # without spinning the CPU.
    h = Session(binary)
    h.read(h.startup_secs + 1.5)
    h.send(b" ", 1.0)
    h.sock.close()
    t0 = time.time()
    try:
        rc = h.proc.wait(10)
        check(rc == 0, "process exits cleanly after hangup in %.1fs (rc=%s)" % (time.time() - t0, rc))
    except subprocess.TimeoutExpired:
        h.proc.kill()
        check(False, "process exits after hangup")

    # Terminal that never answers the size query -> safe 24-row layout.
    n = Session(binary, answer_cpr=False)
    n.read(n.startup_secs + 1.5)
    n.send(b" ", 1.0)
    check(" (1,1)" in n.row(23) and n.row(24).strip() == "",
          "no CPR reply -> 24-row fallback, status on row 24")
    n.sock.close(); n.proc.wait(5)

    # Taller terminal answers 40 -> status on row 40.
    t = Session(binary, rows=40)
    t.read(t.startup_secs + 1.5)
    t.send(b" ", 1.0)
    check(" (1,1)" in t.row(39), "40-row terminal -> status on row 40")
    t.sock.close(); t.proc.wait(5)

    # Sysop override wins over the terminal's own answer.
    o = Session(binary, extra_args=("--rows", "22"))
    o.read(o.startup_secs + 1.5)
    o.send(b" ", 1.0)
    check(" (1,1)" in o.row(21), "--rows 22 override -> status on row 22")
    o.sock.close(); o.proc.wait(5)

    print("\n%d failure(s)" % len(FAILS))
    sys.exit(1 if FAILS else 0)


if __name__ == "__main__":
    main()
