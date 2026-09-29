#!/usr/bin/env python3
"""Screen-size test for ANetDRAW: 132x37 from CHAIN.TXT and BBSDEV.DRP
(with the terminal NOT answering the size query, so the dropfile has to
be what's used), splash centering, the wide-screen sidebar and its
clicks, horizontal scrolling of a wide canvas, ^L re-detecting a new
size, and live SIGWINCH resizing in local (-L) mode through a real PTY.

Usage: tests/screen_size_test.py path/to/anetdraw
"""
import fcntl
import os
import pty
import select
import signal
import socket
import struct
import subprocess
import sys
import tempfile
import termios
import time
import tty

import pyte

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from phase1_session import AnsweringScreen, Session, check, cp437_to_str, FAILS, answer_canvas_choice  # noqa: E402
from phase2_tools import Door, ctrl  # noqa: E402


def press(x, y, b=0):
    return b"\x1b[<%d;%d;%dM" % (b, x + 1, y + 1)


def release(x, y, b=0):
    return b"\x1b[<%d;%d;%dm" % (b, x + 1, y + 1)


class DropSession(Door):
    """A session started from a CHAIN.TXT or BBSDEV.DRP instead of
    DOOR32.SYS, on a cols x rows emulated terminal."""

    def __init__(self, binary, kind, cols, rows, answer_cpr=False):
        self.dir = tempfile.mkdtemp(prefix="anetdraw_drop_")
        self.screen = AnsweringScreen(cols, rows)
        self.screen.answer = answer_cpr
        self.stream = pyte.Stream(self.screen)
        self.raw = b""
        self.startup_secs = 2.5
        self.sock, child = socket.socketpair()
        child.set_inheritable(True)
        fd = child.fileno()
        env = dict(os.environ)
        if kind == "chain":
            # WWIV CHAIN.TXT, 31 lines; width/height are lines 9/10
            lines = ["1", "Artist", "Test Artist", "", "30", "M", "0", "09/28/26",
                     str(cols), str(rows), "100", "0", "0", "1", "1", "3600",
                     "/bbs/gfiles/", "/bbs/data/", "", "38400", "1", "ANetBBS", "Sysop",
                     "00:00:00", "0", "0", "0", "0", "0", "8N1", "38400"]
            path = os.path.join(self.dir, "chain.txt")
            args = ["-D", path, "-SOCKET", str(fd)]
        else:
            # BBSDEV.DRP 1.x, 19 lines; found via $BBSDEV_DRP
            lines = ["1.0", "socket", str(fd), "Test Artist", "user-key-1", str(cols), str(rows),
                     "Y", "N", "", "", "CP437", "en", "ANetBBS 1.0", "A-Net Online", "Sysop",
                     "100", "1", "N"]
            path = os.path.join(self.dir, "bbsdev.drp")
            env["BBSDEV_DRP"] = path
            args = []
        with open(path, "w") as f:
            f.write("\r\n".join(lines) + "\r\n")
        self.proc = subprocess.Popen([binary] + args, pass_fds=(fd,), cwd=self.dir, env=env,
                                     stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL,
                                     stderr=subprocess.PIPE)
        child.close()
        self.screen.sock = self.sock

    def status(self):
        return self.row(self.screen.lines - 1)


def dropfile_checks(binary, kind):
    d = DropSession(binary, kind, 132, 37)
    d.read(d.startup_secs)
    # splash centered: art is 80x24 -> offset (26, 6)
    check("A-Net Online presents" in d.row(7) and d.row(7).index("A-Net") == 26 + 29,
          "%s: splash centered on 132x37 (row 8: %r)" % (kind, d.row(7)[40:70]))
    check("press any key" in d.row(29), "%s: splash prompt on row 30" % kind)
    d.send(b" ", 1.5)
    answer_canvas_choice(d)
    check(" (1,1)" in d.row(36), "%s: status bar on row 37 of 37: %r" % (kind, d.row(36)[:12]))
    check("ANetDRAW" in d.row(0)[95:] and d.cell(93, 0).data == "│" and d.cell(80, 0).data == "░",
          "%s: toolbox docked right, dim desk past the 80-col canvas (%r)" % (kind, d.row(0)[78:100]))
    d.sock.close()
    d.proc.wait(5)


def sidebar_checks(binary):
    d = DropSession(binary, "bbsdev", 132, 37)
    d.read(d.startup_secs)
    d.send(b" ", 1.5)
    answer_canvas_choice(d)
    # this terminal never answers the size query: the first ^L waits it
    # out once (1.5s), after that ^L must be instant
    d.send(ctrl("L"), 2.5)
    t0 = time.time()
    d.send(ctrl("L"), 0.0)
    d.send(press(0, 30), 0.3)   # a click right behind the ^L
    d.read(0.5)
    check(d.screen.cursor.y == 30 and time.time() - t0 < 2.0,
          "after one unanswered size query, ^L no longer stalls or eats clicks")
    rows = [d.row(y) for y in range(37)]
    x0 = 132 - 38 + 1  # sidebar content column (docked at the right edge, 38 wide)
    # FG swatch 12 at x0+3+12*2, row 3; right-click swatch 1 -> bg 1
    d.send(press(x0 + 3 + 12 * 2, 3), 0.8)
    d.send(press(x0 + 3 + 1 * 2, 3, 2), 0.8)
    d.goto(0, 0)
    d.send(b"W", 0.8)
    c = d.cell(0, 0)
    check(c.fg == "red" and c.bold and c.bg == "blue",
          "sidebar: left-click sets FG, right-click sets BG (fg=%s bold=%s bg=%s)" % (c.fg, c.bold, c.bg))
    boxrow = next(y for y, r in enumerate(rows) if " B Box" in r)
    boxcol = rows[boxrow].index(" B Box")
    d.send(press(boxcol + 2, boxrow), 0.8)
    check("Box" in d.status(), "sidebar: clicking a tool selects it: %r" % d.status()[16:32])
    # F4 glyph: F-keys start at row 17; F4 is the 4th row of the left column
    d.send(press(x0 + 1, 17 + 3), 0.8)
    d.send(ctrl("L"), 1.0)
    check(d.cell(14, 36).data == "█", "sidebar: clicking F4 sets the brush: %r" % d.cell(14, 36).data)
    setrow = d.row(16)
    nx = setrow.index("►")
    d.send(press(nx, 16), 0.8)
    check("Set 07" in d.row(16), "sidebar: the set arrow moves to set 07: %r" % d.row(16)[82:106])
    d.sock.close()
    d.proc.wait(5)


def wide_canvas_checks(binary):
    d = Door(binary, extra_args=("--width", "120"))
    d.read(d.startup_secs)
    d.send(b" ", 1.0)
    answer_canvas_choice(d)
    d.send(ctrl("L"), 0.8)
    check(" (1,1)" in d.status(), "--width 120: status shows the position: %r" % d.status()[:10])
    d.send(b"\x1b[F", 1.0)  # End -> column 120
    check("(120,1)" in d.status(), "End goes to column 120")
    check(d.screen.cursor.x == 79, "view scrolled right: cursor at the right edge (x=%d)" % d.screen.cursor.x)
    d.send(b"R", 0.8)
    check(d.cell(79, 0).data == "R", "typing at column 120 shows at the screen's right edge")
    d.send(b"\x1b[H", 1.0)  # Home
    check(d.screen.cursor.x == 0 and d.cell(0, 0).data != "R", "Home scrolls back to column 1")
    d.sock.close()
    d.proc.wait(5)


def ctrl_l_redetect(binary):
    d = Door(binary)  # 80x25, answers CPR
    d.read(d.startup_secs)
    d.send(b" ", 1.0)
    answer_canvas_choice(d)
    d.screen.resize(37, 132)
    d.send(ctrl("L"), 2.0)
    check(" (1,1)" in d.row(36) and "ANetDRAW" in d.row(0)[81:],
          "^L re-detects a 132x37 terminal and re-lays-out (%r)" % d.row(36)[:12])
    check("Screen is now 132x37" in d.row(36), "^L reports the new size")
    d.sock.close()
    d.proc.wait(5)


def local_resize(binary):
    """-L in a real PTY: the door gets the size from the OS, and
    resizing the window (TIOCSWINSZ -> SIGWINCH) re-lays-out live."""
    pid, m = pty.fork()
    if pid == 0:
        tty.setraw(0)
        os.environ["LC_ALL"] = "C"
        os.execv(binary, [binary, "-L"])
    fcntl.ioctl(m, termios.TIOCSWINSZ, struct.pack("HHHH", 25, 80, 0, 0))
    scr = pyte.Screen(80, 25)
    st = pyte.Stream(scr)

    def rd(t):
        end = time.time() + t
        while time.time() < end:
            r, _, _ = select.select([m], [], [], 0.05)
            if r:
                try:
                    data = os.read(m, 65536)
                except OSError:
                    return
                st.feed(cp437_to_str(data))

    def row(y):
        return "".join(scr.buffer[y][x].data for x in range(scr.columns))

    rd(2.5)
    os.write(m, b" ")
    rd(1.5)
    check(" (1,1)" in row(24), "-L: starts at the OS window size (80x25)")
    os.write(m, b"hi")
    rd(0.8)
    for cols, lines, sidebar in ((132, 37, True), (100, 30, False), (200, 60, True)):
        scr.resize(lines, cols)
        fcntl.ioctl(m, termios.TIOCSWINSZ, struct.pack("HHHH", lines, cols, 0, 0))
        rd(1.5)
        ok = " (3,1)" in row(lines - 1) and row(0).startswith("hi")
        has_sb = "ANetDRAW" in row(0)[80:]
        check(ok and has_sb == sidebar, "-L: window resized to %dx%d re-lays-out live (status %r, sidebar %s)"
              % (cols, lines, row(lines - 1)[:9], has_sb))
    os.write(m, b"\x1b")
    rd(1.2)
    os.write(m, b"q")
    rd(1.0)
    os.write(m, b"n")  # typed "hi": don't save
    rd(1.5)
    _, status = os.waitpid(pid, 0)
    check(os.WEXITSTATUS(status) == 0, "-L: clean exit after resizing")


def canvas_size_checks(binary):
    d = Door(binary)
    d.read(d.startup_secs)
    d.send(b" ", 1.0)
    answer_canvas_choice(d)
    d.send(ctrl("L"), 0.8)
    d.goto(60, 2)
    d.send(b"X", 0.5)
    d.send(b"\x1b", 1.2)
    d.send(b"c", 1.0)
    check(any("Canvas size" in d.row(y) for y in range(24)), "Esc, C opens the canvas size dialog")
    d.send(b"b", 0.6)  # preset 80x50
    d.send(b"\r", 1.0)
    check("Canvas is now 80x50" in d.status(), "preset B makes an 80x50 canvas: %r" % d.status()[16:60])
    d.send(b"\x1b[6~" * 3, 1.0)
    check(",50)" in d.status() or ",49)" in d.status(), "PgDn reaches row 50: %r" % d.status()[:10])
    d.send(b"\x1b[5~" * 4, 1.0)
    # shrink to 40 wide: the X at column 61 would be cropped -> needs a second Enter
    d.send(b"\x1b", 1.2)
    d.send(b"c", 1.0)
    d.send(b"40\r", 1.0)
    check(any("crops your art" in d.row(y) for y in range(24)), "shrinking below the art asks first")
    d.send(b"\r", 1.2)
    check("Canvas is now 40x50" in d.status(), "second Enter crops to 40 wide: %r" % d.status()[16:60])
    # 80 - 40 leaves room for the toolbox: canvas 0-39, desk at 40, divider 41
    check(d.cell(39, 0).data != "\u2591" and d.cell(40, 0).data == "\u2591" and d.cell(41, 0).data == "\u2502"
          and all("X" not in d.row(y)[:40] for y in range(4)),
          "the canvas ends at column 40 (desk, then the toolbox); cropped art is gone: %r" % d.row(0)[36:44])
    d.send(b"\x1b[F", 0.8)
    check("(40,1)" in d.status(), "End stops at column 40: %r" % d.status()[:10])
    d.send(ctrl("Z"), 0.8)
    check("Nothing to undo" in d.status(), "resizing cleared the undo history")
    d.sock.close()
    d.proc.wait(5)


def collapse_checks(binary):
    d = DropSession(binary, "bbsdev", 132, 37, answer_cpr=True)
    d.read(d.startup_secs)
    d.send(b" ", 1.5)
    answer_canvas_choice(d)
    row0 = d.row(0)
    hx = row0.index("hide")
    d.send(press(hx, 0), 1.0)
    check("ANetDRAW" not in d.row(0)[81:] and d.cell(131, 0).data == "\u25c4" and d.cell(131, 1).data == "T",
          "clicking 'hide' collapses the toolbox to a tab at the right edge (%r)" % d.row(0)[120:132])
    check(all(d.cell(x, 10).data != "\u2502" or x == 80 for x in range(81, 131)),
          "collapsed: no divider in the middle, the drawing area runs to the edge")
    d.send(ctrl("W"), 1.0)
    check("ANetDRAW" in d.row(0)[81:], "^W brings the toolbox back")
    d.send(press(132 - 38 - 1, 10), 1.0)   # the divider left of the docked toolbox
    check("ANetDRAW" not in d.row(0)[81:], "clicking the divider collapses it again")
    d.send(press(131, 0), 1.0)
    check("ANetDRAW" in d.row(0)[81:], "clicking the tab expands it")
    # a canvas too wide for the toolbox: it goes away by itself
    d.send(b"\x1b", 1.2)
    d.send(b"c", 1.0)
    d.send(b"d\r", 1.2)  # 160x50
    check("Canvas is now 160x50" in d.status() and "ANetDRAW" not in d.row(0)[120:],
          "a 160-wide canvas fills the 132-col screen (no room for the toolbox)")
    d.sock.close()
    d.proc.wait(5)


def main():
    binary = os.path.abspath(sys.argv[1])
    dropfile_checks(binary, "chain")
    dropfile_checks(binary, "bbsdev")
    sidebar_checks(binary)
    wide_canvas_checks(binary)
    ctrl_l_redetect(binary)
    canvas_size_checks(binary)
    collapse_checks(binary)
    local_resize(binary)
    print("\n%d failure(s)" % len(FAILS))
    sys.exit(1 if FAILS else 0)


if __name__ == "__main__":
    main()
