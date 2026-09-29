#!/usr/bin/env python3
"""Phase 4 (save / open / SAUCE) end-to-end test for ANetDRAW, over the
real DOOR32.SYS socket path. Each session gets its own --data folder.

Usage: tests/phase4_files.py path/to/anetdraw
"""
import os
import sys
import tempfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from phase1_session import check, FAILS  # noqa: E402
from phase2_tools import Door, ctrl, ESC, ENTER, DOWN, BKSP  # noqa: E402


def sauce(path):
    data = open(path, "rb").read()
    r = data[-128:]
    return {
        "id": r[:7], "title": r[7:42].decode("cp437").rstrip(), "author": r[42:62].decode("cp437").rstrip(),
        "group": r[62:82].decode("cp437").rstrip(), "w": r[96] | r[97] << 8, "h": r[98] | r[99] << 8,
        "ice": r[105] & 1, "art": data[:r[90] | r[91] << 8 | r[92] << 16],
    }


def all_files(root):
    out = []
    for d, _, fs in os.walk(root):
        out += [os.path.relpath(os.path.join(d, f), root) for f in fs]
    return sorted(out)


def screen_has(d, text):
    return any(text in d.row(y) for y in range(d.screen.lines))


def menu(d, key):
    d.send(ESC, 1.2)
    d.send(key.encode(), 1.0)


def main():
    binary = os.path.abspath(sys.argv[1])
    root = tempfile.mkdtemp(prefix="anetdraw_files_")
    data = os.path.join(root, "data")

    # ---------------- a regular caller
    d = Door(binary, extra_args=("--data", data))
    d.read(d.startup_secs)
    d.send(b" ", 1.0)
    d.send(ctrl("F") * 5, 0.5)  # fg 12 (bright red)
    d.send(b"Hi", 0.8)
    d.send(ESC, 1.2)
    check(screen_has(d, "Untitled *"), "menu title shows an unsaved Untitled drawing")
    d.send(b"a", 1.2)
    check(screen_has(d, "Your drawings") and screen_has(d, "no drawings here yet"),
          "caller Save As shows only their own (empty) folder, no path")
    d.send(b"smiley", 0.5)
    d.send(ENTER, 1.2)
    check(screen_has(d, "Save options") and screen_has(d, "Clear the screen first") and screen_has(d, "full speed"),
          "Save As then asks TheDraw's save options")
    d.send(b"c", 0.4)            # clear the screen first: Yes
    d.send(b"s" * 6, 0.6)        # display speed: 9600
    check(screen_has(d, "Yes") and screen_has(d, "9600 bps"), "C and S set clear screen and 9600 bps")
    d.send(ENTER, 1.2)
    udir = os.path.join(data, "users", "1_test_artist")
    f = os.path.join(udir, "smiley.ans")
    check(os.path.isfile(f), "saved into the caller's own folder: %s" % all_files(data))
    check("Saved smiley.ans" in d.status(), "status confirms the save: %r" % d.status()[16:60])
    if os.path.isfile(f):
        sc = sauce(f)
        check(sc["id"] == b"SAUCE00" and sc["title"] == "smiley" and sc["author"] == "Test Artist",
              "SAUCE: title from the file name, author from the caller (%r / %r)" % (sc["title"], sc["author"]))
        check(b"\x1b[" in sc["art"] and b"Hi" in sc["art"] and b"1;31" in sc["art"],
              "the art is ANSI with the bright red 'Hi'")
        check(sc["art"].startswith(b"\x1b[0;6*r\x1b[0m\x1b[2J\x1b[1;1H") and sc["art"].endswith(b"\x1b[0;0*r"),
              "the file starts with 9600 bps + clear screen and ends back at full speed: %r" % sc["art"][:24])
    d.send(ESC, 1.2)
    check(screen_has(d, "smiley.ans") and not screen_has(d, "smiley.ans *"), "menu title now names the saved file")
    d.send(ESC, 0.8)

    # path-escape attempts stay inside the caller's folder
    d.send(b"!", 0.5)  # a change, so Save As isn't a no-op
    menu(d, "a")
    d.send(BKSP * 20, 0.3)
    d.send(b"../../../hack/..\\x", 0.5)
    d.send(ENTER, 1.2)
    d.send(ENTER, 1.2)   # save options: keep them
    outside = [p for p in all_files(root) if not p.startswith(os.path.join("data", "users", "1_test_artist"))]
    check(outside == [], "nothing written outside the caller's folder: %r" % outside)
    check(len(all_files(udir)) == 2, "the escape attempt became a plain file in their folder: %r" % all_files(udir))

    # New, then Open with the save prompt
    menu(d, "n")
    check(d.cell(0, 0).data == " " and d.cell(1, 0).data == " ", "New gives a blank canvas")
    d.send(b"X", 0.5)
    menu(d, "o")
    check("Save your drawing before you open another" in d.status(), "unsaved changes: Open asks to save first")
    d.send(b"n", 1.2)
    check(screen_has(d, "Open drawing") and screen_has(d, "smiley.ans"), "open browser lists the caller's files")
    rows = [d.row(y) for y in range(24)]
    target = next(i for i, r in enumerate(rows) if "smiley.ans" in r)
    first = next(i for i, r in enumerate(rows) if ".ans" in r)
    d.send(DOWN * (target - first), 0.6)
    d.send(ENTER, 1.5)
    check(d.row(0).startswith("Hi") and d.cell(0, 0).fg == "red" and d.cell(0, 0).bold,
          "Open loads smiley.ans back exactly: %r" % d.row(0)[:5])
    check("Opened smiley.ans (80x25" in d.status(), "a saved sketch reopens as a full 80x25 canvas: %r" % d.status()[16:60])

    # SAUCE info, then plain Save (no browser: same file)
    menu(d, "i")
    check(screen_has(d, "SAUCE info"), "SAUCE info dialog opens")
    d.send(BKSP * 40, 0.4)
    d.send(b"Boat at Night", 0.4)
    d.send(b"\t\t", 0.3)
    d.send(b"A-Net", 0.4)
    d.send(ENTER, 1.0)
    menu(d, "s")
    sc = sauce(f)
    check(sc["title"] == "Boat at Night" and sc["group"] == "A-Net",
          "Save writes the edited SAUCE into the same file (%r / %r)" % (sc["title"], sc["group"]))
    check(sc["art"].startswith(b"\x1b[0;6*r\x1b[0m\x1b[2J"),
          "reopened + plain Save keeps the file's save options without asking")

    # quit with unsaved work asks first
    d.send(b"Z", 0.5)
    menu(d, "q")
    check("Save your drawing before you quit" in d.status(), "quitting with unsaved work asks to save")
    d.send(b"n", 1.5)
    rc = d.proc.wait(5)
    check(rc == 0, "answering N quits cleanly (rc=%s)" % rc)

    # ---------------- the sysop (--sysop-level reached: DOOR32 line 8 is 100)
    s = Door(binary, extra_args=("--data", data, "--sysop-level", "100"))
    s.read(s.startup_secs)
    s.send(b" ", 1.0)
    s.send(b"S", 0.5)
    menu(s, "a")
    check(screen_has(s, os.path.basename(data)) and screen_has(s, "(up a folder)") and screen_has(s, "users"),
          "sysop Save As shows the real path, '..' and sub-folders")
    rows = [s.row(y) for y in range(24)]
    up = next(i for i, r in enumerate(rows) if "(up a folder)" in r)
    first_item = up
    s.send(b"\x1b[H", 0.4)  # Home: select '..'
    s.send(ENTER, 1.0)       # list has focus after Home -> go up
    check(screen_has(s, os.path.basename(root)), "Enter on '..' goes up a folder")
    s.send(b"sysop_piece", 0.4)
    s.send(ENTER, 1.2)
    s.send(ENTER, 1.2)   # save options
    check(os.path.isfile(os.path.join(root, "sysop_piece.ans")),
          "sysop saved outside the data folder: %r" % all_files(root))
    s.send(ESC, 1.2)
    s.send(b"q", 1.0)
    s.send(b"y" if "Quit ANetDRAW?" in s.status() else b"n", 1.5)
    s.proc.wait(5)

    # ---------------- a caller WITH --sysop-level but below it: still restricted
    c = Door(binary, extra_args=("--data", data, "--sysop-level", "200"))
    c.read(c.startup_secs)
    c.send(b" ", 1.0)
    c.send(b"c", 0.4)
    menu(c, "a")
    check(screen_has(c, "Your drawings") and not screen_has(c, "(up a folder)"),
          "security 100 < --sysop-level 200: caller browser, no filesystem access")
    c.send(ESC, 1.0)
    c.sock.close()
    c.proc.wait(5)

    print("\n%d failure(s)" % len(FAILS))
    sys.exit(1 if FAILS else 0)


if __name__ == "__main__":
    main()
