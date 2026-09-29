#!/usr/bin/env python3
"""Save formats through the door: Save as PCBoard, reopen it, plain Save
keeps the format, Save as PNG and Ctrl-A -- over the DOOR32.SYS socket.

Usage: tests/phase8_formats.py path/to/anetdraw
"""
import os
import sys
import tempfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from phase1_session import check, FAILS  # noqa: E402
from phase2_tools import Door, ctrl, ESC, ENTER, BKSP, DOWN  # noqa: E402


def screen_has(d, t):
    return any(t in d.row(y) for y in range(d.screen.lines))


def save_as(d, name, format_presses):
    d.send(ESC, 1.2)
    d.send(b"a", 1.2)
    d.send(BKSP * 30, 0.3)
    d.send(name.encode(), 0.4)
    d.send(ENTER, 1.2)
    d.send(b"f" * format_presses, 0.6)
    d.send(ENTER, 1.2)


def main():
    binary = os.path.abspath(sys.argv[1])
    root = tempfile.mkdtemp(prefix="anetdraw_fmt_")
    data = os.path.join(root, "data")
    udir = os.path.join(data, "users", "1_test_artist")
    d = Door(binary, extra_args=("--data", data))
    d.read(d.startup_secs)
    d.send(b" ", 1.0)
    d.send(ctrl("F") * 5, 0.4)      # bright red
    d.send(b"MENU", 0.6)

    # Save as -> format PCBoard (ANSI, ASCII, BIN, XBin, PCBoard: 4 presses)
    d.send(ESC, 1.2)
    d.send(b"a", 1.2)
    d.send(b"menu", 0.4)
    d.send(ENTER, 1.2)
    check(screen_has(d, "Format") and screen_has(d, "ANSI"), "Save options starts with the Format row")
    d.send(b"f" * 4, 0.6)
    check(screen_has(d, "PCBoard @X") and screen_has(d, "Saves as .pcb"), "F steps to PCBoard @X (.pcb)")
    d.send(ENTER, 1.2)
    f = os.path.join(udir, "menu.pcb")
    check(os.path.isfile(f), "saved as menu.pcb: %r" % (os.listdir(udir) if os.path.isdir(udir) else None))
    if os.path.isfile(f):
        body = open(f, "rb").read()
        check(body.startswith(b"@X07") and b"@X0CMENU" in body and b"SAUCE" not in body,
              "the file is PCBoard @X codes, no SAUCE: %r" % body[:24])

    # open it back
    d.send(ESC, 1.2)
    d.send(b"n", 1.2)
    d.send(ESC, 1.2)
    d.send(b"o", 1.2)
    check(screen_has(d, "menu.pcb"), "Open lists the .pcb file")
    rows = [d.row(y) for y in range(25)]
    target = next((i for i, r in enumerate(rows) if "menu.pcb" in r), 0)
    first = next((i for i, r in enumerate(rows) if ".pcb" in r or ".ans" in r), 0)
    d.send(DOWN * (target - first), 0.4)
    d.send(ENTER, 1.5)
    c = d.cell(0, 0)
    check(d.row(0).startswith("MENU") and c.fg == "red" and c.bold, "menu.pcb reopens with its colors: %r" % d.row(0)[:6])
    # plain Save keeps PCBoard
    d.goto(4, 0)
    d.send(b"!", 0.4)
    d.send(ESC, 1.2)
    d.send(b"s", 1.2)
    check(b"MENU!" in open(f, "rb").read() and not os.path.exists(os.path.join(udir, "menu.ans")),
          "plain Save writes the .pcb again, in PCBoard")

    # Save as PNG (7 presses from PCBoard: pip, msg, png = 3)
    save_as(d, "menu", 3)
    png = os.path.join(udir, "menu.png")
    check(os.path.isfile(png) and open(png, "rb").read(8) == b"\x89PNG\r\n\x1a\n", "Save as PNG writes a real PNG")
    # Save as Ctrl-A (from PNG: ANSI, ASCII, BIN, XBin, PCB, PIP, MSG = 7)
    save_as(d, "menu", 7)
    msg = os.path.join(udir, "menu.msg")
    check(os.path.isfile(msg) and b"\x01N\x01H\x01RMENU!" in open(msg, "rb").read(), "Save as Ctrl-A writes Synchronet codes")

    d.send(ESC, 1.0)
    d.send(b"q", 1.0)
    d.send(b"n" if "Save your drawing" in d.status() else b"y", 1.5)
    d.proc.wait(5)
    print("\n%d failure(s)" % len(FAILS))
    sys.exit(1 if FAILS else 0)


if __name__ == "__main__":
    main()
