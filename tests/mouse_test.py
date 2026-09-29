#!/usr/bin/env python3
"""Mouse support test for ANetDRAW: real SGR mouse reports
(CSI < b ; x ; y M/m, 1-based, as SyncTERM/xterm send them with
DECSET 1002 + 1006) over the DOOR32.SYS socket path.

Usage: tests/mouse_test.py path/to/anetdraw
"""
import os
import re
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from phase1_session import Session, check, FAILS  # noqa: E402
from phase2_tools import Door, ctrl, ESC  # noqa: E402


def press(x, y, b=0):   # 0-based cell -> 1-based report
    return b"\x1b[<%d;%d;%dM" % (b, x + 1, y + 1)


def drag(x, y, b=0):
    return b"\x1b[<%d;%d;%dM" % (b + 32, x + 1, y + 1)


def release(x, y, b=0):
    return b"\x1b[<%d;%d;%dm" % (b, x + 1, y + 1)


def main():
    binary = os.path.abspath(sys.argv[1])
    d = Door(binary)
    d.read(d.startup_secs)
    d.raw = b""
    d.send(b" ", 1.0)
    check(b"\x1b[?1002h" in d.raw and b"\x1b[?1006h" in d.raw, "editor turns on button-event + SGR mouse reporting")
    d.send(ctrl("L"), 0.8)

    # ---- freehand stroke in Draw, undone as ONE step
    d.send(press(4, 1), 0.6)
    check(d.ch(4, 1) == "█", "left press paints the brush: %r" % d.ch(4, 1))
    d.send(drag(9, 1) + drag(14, 1), 0.8)  # fast drag: jumps 5 cells at a time
    check(all(d.ch(x, 1) == "█" for x in range(4, 15)), "drag paints a gap-free stroke: %r" % d.row(1)[:16])
    d.send(release(14, 1), 0.6)
    d.send(ctrl("Z"), 0.8)
    check(all(d.ch(x, 1) == " " for x in range(4, 15)), "one ^Z undoes the whole stroke")
    d.send(ctrl("Y"), 0.8)

    # ---- Line tool: press, rubber-band, release commits
    d.tool("L")
    d.send(b"#")
    d.send(press(1, 5), 0.5)
    d.send(drag(10, 8), 0.8)
    check(d.ch(1, 5) == "#" and d.ch(10, 8) == "#", "dragging rubber-bands the line preview")
    d.send(drag(12, 8), 0.8)
    check(d.ch(10, 8) != "#" or d.ch(12, 8) == "#", "preview follows the pointer")
    d.send(release(12, 8), 0.8)
    d.send(ctrl("L"), 0.8)
    check(d.ch(1, 5) == "#" and d.ch(12, 8) == "#" and "*Line" not in d.status(),
          "release commits the line and clears the anchor")

    # ---- right-click eyedropper
    d.tool("D")
    d.send(ctrl("F") * 5, 0.5)  # fg 7 -> 12
    d.send(press(3, 1, 2), 0.8)  # right-click the light-gray stroke
    check("Picked up F07" in d.status(), "right-click picks up colors: %r" % d.status()[16:50])

    # ---- status bar clicks
    d.send(press(10, 24), 1.0)
    check(any("Colors" in d.row(y) for y in range(24)), "clicking the color sample opens the color picker")
    rows = [d.row(y) for y in range(24)]
    top = next(y for y, r in enumerate(rows) if "Colors" in r)
    col0 = rows[top].index("╔") if "╔" in rows[top] else 14
    # swatch fg=12 bg=1 sits at col0+2+12*3+1, row top+1+1
    d.send(press(col0 + 2 + 12 * 3 + 1, top + 2), 0.2)
    d.send(release(col0 + 2 + 12 * 3 + 1, top + 2), 1.0)
    check(not any("Colors" in d.row(y) for y in range(24)), "clicking a swatch picks it and closes (release ignored)")
    d.goto(30, 12)
    d.send(b"Q", 0.8)
    c = d.cell(30, 12)
    check(c.fg == "red" and c.bold and c.bg == "blue", "clicked colors used (fg=%s bold=%s bg=%s)" % (c.fg, c.bold, c.bg))

    d.send(press(20, 24), 0.3)
    d.send(release(20, 24), 1.0)
    check(any("Select" in d.row(y) for y in range(24)), "clicking the tool name opens the tools menu (release ignored)")
    rows = [d.row(y) for y in range(24)]
    boxrow = next(y for y, r in enumerate(rows) if "B Box" in r)
    d.send(press(30, boxrow), 1.0)
    check("Box" in d.status(), "clicking a tool line picks that tool: %r" % d.status()[16:32])

    d.send(press(38, 24), 0.8)  # F1 slot of set 06
    d.send(ctrl("L"), 0.6)
    check(d.cell(14, 24).data == "░", "clicking the F-key strip sets the brush: %r" % d.cell(14, 24).data)

    # ---- wheel scrolls
    d.tool("D")
    before = d.status()[1:9]
    d.send(b"\x1b[<65;10;10M" * 4, 1.0)
    after = d.status()[1:9]
    row = lambda st: int(re.search(r",(\d+)\)", st).group(1))
    # 4 notches x 3 rows: the view (and cursor) move down 12 rows
    check(row(after) == row(before) + 12, "wheel down scrolls 3 rows a notch: %r -> %r" % (before, after))
    d.send(b"\x1b[<64;10;10M" * 4, 1.0)

    # ---- a report split across two packets still parses
    d.goto(0, 15)
    d.sock.sendall(b"\x1b[<0;41;1")
    time.sleep(0.02)
    d.sock.sendall(b"7M" + release(40, 16))
    d.read(1.0)
    check(d.ch(40, 16) == "░", "split mouse report still parsed: %r" % d.ch(40, 16))

    # ---- malformed report: keys are not lost, no crash
    d.goto(0, 18)
    d.sock.sendall(b"\x1b[<ab")
    d.read(1.5)
    d.send(b"xyz", 1.0)
    check(d.proc.poll() is None, "malformed report doesn't crash the door")
    check("xyz" in d.row(18), "typing after a malformed report still works: %r" % d.row(18)[:10])

    # ---- lone Esc still means Esc
    d.send(ESC, 1.2)
    check(any("Q Quit" in d.row(y) for y in range(24)), "a lone Esc still opens the main menu")
    d.send(b"q", 1.0)
    d.raw = b""
    d.send(b"n", 1.5)  # don't save
    rc = d.proc.wait(5)
    check(rc == 0 and b"\x1b[?1002l" in d.raw and b"\x1b[?1006l" in d.raw,
          "exit turns mouse reporting back off (rc=%s)" % rc)

    # ---- --no-mouse
    n = Door(binary, extra_args=("--no-mouse",))
    n.read(n.startup_secs)
    n.raw = b""
    n.send(b" ", 1.0)
    check(b"\x1b[?1002h" not in n.raw, "--no-mouse leaves mouse reporting off")
    n.sock.close()
    n.proc.wait(5)

    print("\n%d failure(s)" % len(FAILS))
    sys.exit(1 if FAILS else 0)


if __name__ == "__main__":
    main()
