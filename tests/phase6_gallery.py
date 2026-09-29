#!/usr/bin/env python3
"""Phase 6 gallery test: two callers and the sysop share one --data
folder over the real DOOR32.SYS socket path. Publish (and replace),
browse, view, download by ZMODEM (to lrzsz's rz), open a copy, and who
may remove what.

Usage: tests/phase6_gallery.py path/to/anetdraw path/to/rz
"""
import os
import socket
import subprocess
import sys
import tempfile
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from phase1_session import check, FAILS  # noqa: E402
from phase2_tools import Door, ctrl, ESC, ENTER, BKSP, DOWN  # noqa: E402
from phase6_zmodem import receive  # noqa: E402


class Caller(Door):
    """A Door whose DOOR32.SYS names a given user (record number, name, security)."""
    who = (1, "Test Artist", 100)

    def _start_posix(self, binary, extra_args):
        self.startup_secs = 2.0
        self.sock, child = socket.socketpair()
        child.set_inheritable(True)
        num, name, sec = self.who
        drop = os.path.join(self.dir, "door32.sys")
        with open(drop, "w") as f:
            f.write("2\n%d\n38400\nANetBBS\n%d\n%s\n%s\n%d\n60\n1\n1\n"
                    % (child.fileno(), num, name, name.split()[0], sec))
        self.proc = subprocess.Popen([binary, "-D", drop] + list(extra_args), pass_fds=(child.fileno(),),
                                     cwd=self.dir, stdin=subprocess.DEVNULL,
                                     stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
        child.close()


def caller(binary, data, who, *extra):
    cls = type("C", (Caller,), {"who": who})
    d = cls(binary, extra_args=("--data", data) + extra)
    d.read(d.startup_secs)
    d.send(b" ", 1.0)
    return d


def screen_has(d, text):
    return any(text in d.row(y) for y in range(d.screen.lines))


def find_row(d, text):
    return next((y for y in range(d.screen.lines) if text in d.row(y)), -1)


def menu(d, key, secs=1.2):
    d.send(ESC, 1.2)
    d.send(key.encode(), secs)


def publish(d, title):
    menu(d, "p")
    d.send(BKSP * 40, 0.4)
    d.send(title.encode(), 0.4)
    d.send(ENTER, 1.2)


def quit_door(d):
    d.send(ESC, 1.0)
    d.send(b"q", 1.0)
    d.send(b"n" if "Save your drawing" in d.status() else b"y", 1.5)
    try:
        d.proc.wait(5)
    except subprocess.TimeoutExpired:
        d.proc.kill()
    d.sock.close()


def main():
    binary = os.path.abspath(sys.argv[1])
    rz = os.path.abspath(sys.argv[2])
    root = tempfile.mkdtemp(prefix="anetdraw_gal_")
    data = os.path.join(root, "data")
    gal = os.path.join(data, "gallery")

    # ---------------- caller A publishes
    a = caller(binary, data, (1, "Test Artist", 100))
    check(os.path.isdir(gal), "the gallery folder is created at start")
    menu(a, "p")
    check("canvas is empty" in a.status(), "publishing a blank canvas is refused: %r" % a.status()[16:60])
    a.send(ctrl("F") * 3, 0.4)   # cyan-ish
    a.send(b"BOAT", 0.6)
    menu(a, "p")
    check(screen_has(a, "Publish to the gallery") and screen_has(a, "Everyone on the BBS"),
          "P opens the publish form")
    a.send(BKSP * 40, 0.4)
    a.send(b"Boat at Night", 0.4)
    a.send(ENTER, 1.2)
    f_boat = os.path.join(gal, "1_test_artist-Boat at Night.ans")
    check(os.path.isfile(f_boat), "published as <owner>-<title>.ans: %r" % os.listdir(gal))
    check('Published "Boat at Night"' in a.status(), "status confirms: %r" % a.status()[16:60])
    first = open(f_boat, "rb").read() if os.path.isfile(f_boat) else b""
    check(first[-128:-121] == b"SAUCE00" and first[-121:-86].rstrip() == b"Boat at Night"
          and first[-86:-66].rstrip() == b"Test Artist", "SAUCE carries the title and the artist")

    # publish the same title again: asks, then replaces
    a.send(b"!", 0.4)
    publish(a, "Boat at Night")
    check("already published" in a.status(), "same title again asks first: %r" % a.status()[16:70])
    a.send(b"y", 1.2)
    check(b"BOAT!" in open(f_boat, "rb").read(), "answering Y replaces the published piece")

    # a title that tries to leave the gallery folder
    publish(a, "../../../etc/evil")
    outside = [p for p in os.listdir(root) if p != "data"] + [p for p in os.listdir(data) if p not in ("gallery", "users")]
    check(outside == [] and os.path.isfile(os.path.join(gal, "1_test_artist-etcevil.ans")),
          "a path in the title stays a plain name in the gallery: %r %r" % (outside, sorted(os.listdir(gal))))
    quit_door(a)

    # ---------------- the sysop drops a piece in by hand (no owner prefix)
    art = b"\x1b[0;1;33mSAMPLE\x1b[0m\r\n\x1a"
    rec = bytearray(b"SAUCE00" + b"Sample Piece".ljust(35) + b"ANet".ljust(20) + b"".ljust(20)
                    + b"20260101" + (len(art) - 1).to_bytes(4, "little") + bytes([1, 1])
                    + (80).to_bytes(2, "little") + (1).to_bytes(2, "little") + bytes(4) + bytes([0, 0])
                    + bytes(22))
    assert len(rec) == 128
    with open(os.path.join(gal, "sample.ans"), "wb") as f:
        f.write(art + bytes(rec))
    os.utime(os.path.join(gal, "sample.ans"), (time.time() - 3600, time.time() - 3600))

    # ---------------- caller B browses
    b = caller(binary, data, (2, "Other Person", 100))
    menu(b, "b")
    check(screen_has(b, "Gallery") and screen_has(b, "3 pieces"), "B opens the gallery with 3 pieces")
    rows = [find_row(b, t) for t in ("etc/evil", "Boat at Night", "Sample Piece")]
    check(-1 not in rows and rows == sorted(rows), "newest first, sysop piece last: rows %r" % rows)
    y = find_row(b, "Boat at Night")
    check("Test Artist" in b.row(y) and "80x25" in b.row(y), "a row shows title, artist and size: %r" % b.row(y).strip())
    check(screen_has(b, "O=open a copy") and not screen_has(b, "R=remove"),
          "B can't remove A's piece (no R in the footer)")
    b.send(DOWN, 0.5)                      # select Boat at Night
    b.send(b"r", 0.8)
    check(os.path.isfile(f_boat), "pressing R anyway removes nothing")

    # view it
    b.send(ENTER, 1.2)
    check(b.row(0).startswith("BOAT!") and "Boat at Night by Test Artist" in b.row(b.screen.lines - 1),
          "Enter views the piece full screen: %r / %r" % (b.row(0)[:10], b.row(b.screen.lines - 1)[:50]))
    b.send(ESC, 1.0)
    check(screen_has(b, "Gallery"), "Esc from the viewer goes back to the list")

    # download it by ZMODEM
    dest = os.path.join(root, "got")
    os.makedirs(dest)
    b.sock.sendall(b"d")
    seen, rc, _ = receive(b, rz, dest)
    got = os.path.join(dest, "Boat at Night.ans")
    check(seen and rc == 0 and os.path.isfile(got) and open(got, "rb").read() == open(f_boat, "rb").read(),
          "D downloads it by ZMODEM without the owner prefix: %r" % os.listdir(dest))
    b.read(2.0)
    check(screen_has(b, "Gallery"), "back in the gallery list after the download")

    # open a copy
    b.send(b"o", 1.5)
    check(b.row(0).startswith("BOAT!") and "Opened a copy" in b.status(), "O opens a copy in the editor: %r" % b.status()[16:60])
    menu(b, "a")
    check(screen_has(b, "Your drawings"), "saving the copy goes to B's own folder")
    b.send(ESC, 0.8)
    quit_door(b)

    # ---------------- A removes their own piece
    a = caller(binary, data, (1, "Test Artist", 100))
    menu(a, "b")
    a.send(DOWN, 0.5)
    check(screen_has(a, "R=remove"), "A may remove their own piece (R shown)")
    a.send(b"r", 0.8)
    check("Remove \"Boat at Night\"" in a.status(), "R asks first: %r" % a.status()[16:60])
    a.send(b"y", 1.2)
    check(not os.path.exists(f_boat) and screen_has(a, "2 pieces"), "Y removes it and the list updates")
    quit_door(a)

    # ---------------- the sysop may remove anything
    s = caller(binary, data, (1, "Sys Op", 250), "--sysop-level", "200")
    menu(s, "b")
    y = find_row(s, "Sample Piece")
    s.send(DOWN * 1, 0.5)
    check(screen_has(s, "R=remove"), "the sysop sees R on someone else's piece")
    s.send(b"r", 0.8)
    s.send(b"y", 1.2)
    check(not os.path.exists(os.path.join(gal, "sample.ans")), "the sysop removed the hand-placed sample")
    quit_door(s)

    print("\n%d failure(s)" % len(FAILS))
    sys.exit(1 if FAILS else 0)


if __name__ == "__main__":
    main()
