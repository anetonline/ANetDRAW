#!/usr/bin/env python3
"""ZMODEM upload and image import through the door. lrzsz's sz plays the
caller's terminal (it answers the door's ZRINIT); the sysop imports a PNG
from disk through the file browser.

Usage: tests/phase8_upload.py path/to/anetdraw path/to/lsz
"""
import os
import select
import shutil
import subprocess
import sys
import tempfile
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from phase1_session import check, FAILS  # noqa: E402
from phase2_tools import Door, ESC, ENTER, DOWN  # noqa: E402
from phase6_gallery import caller  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ZRINIT = b"\x18B01"


def screen_has(d, t):
    return any(t in d.row(y) for y in range(d.screen.lines))


def telnet_encode(data):
    """What a telnet client does to bytes it sends (not in binary mode):
    IAC doubled, a CR followed by NUL -- OpenDoors' telnet socket decodes it."""
    return data.replace(b"\xff", b"\xff\xff").replace(b"\r", b"\r\x00")


def telnet_binary(data):
    """What SyncTERM does once it has switched telnet to binary for a
    transfer: IAC doubled, CR *not* padded. OpenDoors' telnet input still
    treats CR specially, so this only works if the sender escapes control
    bytes -- which the door now asks for (ESCCTL)."""
    return data.replace(b"\xff", b"\xff\xff")


def anetbbs(data):
    """ANetBBS's door relay (games/door_runner.py): DoorSession.write()
    strips every 0x03 (Ctrl-C) on its way to the door's PTY."""
    return data.replace(b"\x03", b"")


def line(data, telnet):
    if telnet == "binary":
        return telnet_binary(data)
    if telnet == "anetbbs":
        return anetbbs(data)
    return telnet_encode(data) if telnet else data


def upload(d, sz, path, timeout=30, telnet=True):
    """Waits for the door's ZRINIT, then runs sz on the line."""
    buf = b""
    end = time.time() + 8
    while ZRINIT not in buf and time.time() < end:
        r, _, _ = select.select([d.sock], [], [], 0.1)
        if r:
            got = d.sock.recv(65536)
            if not got:
                break
            buf += got
    if ZRINIT not in buf:
        return None
    p = subprocess.Popen([sz, "-q", os.path.basename(path)], stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                         stderr=subprocess.DEVNULL, cwd=os.path.dirname(path), bufsize=0)
    i = buf.find(ZRINIT)
    p.stdin.write(buf[max(0, i - 4):])
    out_fd = p.stdout.fileno()
    end = time.time() + timeout
    while time.time() < end:
        r, _, _ = select.select([d.sock, out_fd], [], [], 0.1)
        if d.sock in r:
            got = d.sock.recv(65536)
            if not got:
                break
            try:
                p.stdin.write(got)
            except BrokenPipeError:
                pass
        if out_fd in r:
            got = os.read(out_fd, 65536)
            if got:
                d.sock.sendall(line(got, telnet))
        if p.poll() is not None:
            while True:
                r, _, _ = select.select([out_fd], [], [], 0.05)
                if not r:
                    break
                got = os.read(out_fd, 65536)
                if not got:
                    break
                d.sock.sendall(line(got, telnet))
            break
    return p.wait(5)


def main():
    binary = os.path.abspath(sys.argv[1])
    sz = os.path.abspath(sys.argv[2])
    root = tempfile.mkdtemp(prefix="anetdraw_up_")
    data = os.path.join(root, "data")
    files = os.path.join(root, "files")
    os.makedirs(files)
    shutil.copy(os.path.join(ROOT, "samples", "harbor_moon.ans"), files)
    shutil.copy(os.path.join(ROOT, "samples", "harbor_moon.png"), files)
    udir = os.path.join(data, "users", "1_test_artist")

    d = Door(binary, extra_args=("--data", data))
    d.read(d.startup_secs)
    d.send(b" ", 1.0)
    d.send(ESC, 1.2)
    check(screen_has(d, "U Upload (ZMODEM)") and screen_has(d, "M Image import"), "the menu has Upload and Import")
    d.sock.sendall(b"u")
    rc = upload(d, sz, os.path.join(files, "harbor_moon.ans"))
    d.read(2.0)
    check(rc == 0, "sz finished (rc=%s)" % rc)
    check("Uploaded harbor_moon.ans (80x25)" in d.status(), "an uploaded drawing opens: %r" % d.status()[16:60])
    saved = os.path.join(udir, "harbor_moon.ans")
    check(os.path.isfile(saved) and open(saved, "rb").read() == open(os.path.join(files, "harbor_moon.ans"), "rb").read(),
          "and is kept in the caller's folder, byte for byte")
    check("▄" in d.row(6) or "▀" in d.row(6) or "█" in d.row(6), "the art shows: %r" % d.row(6)[50:70])

    # an image: the import dialog, then it becomes the drawing
    d.send(ESC, 1.2)
    d.sock.sendall(b"u")
    rc = upload(d, sz, os.path.join(files, "harbor_moon.png"))
    d.read(2.0)
    check(screen_has(d, "Import an image") and screen_has(d, "harbor_moon.png"), "an uploaded image opens the import dialog")
    d.send(ENTER, 2.0)
    check("Imported 1280x800 image as 80x25" in d.status(), "Enter imports it: %r" % d.status()[16:70])
    blocks = sum(1 for y in range(24) for x in range(80) if d.cell(x, y).data in "▀▄█")
    check(blocks > 1500, "the picture is made of half blocks (%d cells)" % blocks)
    check(not os.path.exists(os.path.join(udir, "harbor_moon.png")), "images aren't kept, just imported")

    # a second upload of the same drawing doesn't overwrite the first
    d.send(ESC, 1.2)
    d.sock.sendall(b"u")
    d.read(1.0)
    if "Save your drawing" in d.status():
        d.sock.sendall(b"n")
    upload(d, sz, os.path.join(files, "harbor_moon.ans"))
    d.read(2.0)
    check(os.path.isfile(os.path.join(udir, "harbor_moon-2.ans")), "a second upload is kept as harbor_moon-2.ans: %r" % os.listdir(udir))

    # M for a caller: asks for the upload; Esc from the terminal cancels
    d.send(ESC, 1.2)
    d.send(b"m", 1.5)
    if "Save your drawing" in d.status():
        d.send(b"n", 1.5)
    d.send(ESC, 2.0)
    check("Upload cancelled" in d.status() or "No upload" in d.status(), "a caller's Import is an upload; Esc cancels: %r" % d.status()[16:60])
    d.sock.close()
    d.proc.wait(5)

    # ---------------- the sysop imports from disk
    shutil.copy(os.path.join(files, "harbor_moon.png"), os.path.join(data, "moon.png"))
    s = caller(binary, data, (1, "Sys Op", 250), "--sysop-level", "200")
    s.send(ESC, 1.2)
    s.send(b"m", 1.5)
    check(screen_has(s, "Import an image") and screen_has(s, "moon.png"), "the sysop gets an image browser listing moon.png")
    rows = [s.row(y) for y in range(25)]
    t = next((i for i, r in enumerate(rows) if "moon.png" in r), 0)
    first = next((i for i, r in enumerate(rows) if "(up a folder)" in r), t)
    s.send(DOWN * (t - first), 0.4)
    s.send(ENTER, 1.2)
    s.send(b"w", 0.4)                 # width 80 -> 100
    s.send(ENTER, 2.0)
    check("image as 100x" in s.status(), "the sysop imports it at 100 columns: %r" % s.status()[16:70])
    s.sock.close()
    s.proc.wait(5)

    # ---------------- a telnet client in binary mode (what SyncTERM does)
    t = Door(binary, extra_args=("--data", os.path.join(root, "data3")))
    t.read(t.startup_secs)
    t.send(b" ", 1.0)
    t.send(ESC, 1.2)
    t.sock.sendall(b"u")
    rc = upload(t, sz, os.path.join(files, "harbor_moon.ans"), telnet="binary")
    t.read(2.0)
    check(rc == 0 and "Uploaded harbor_moon.ans" in t.status(),
          "telnet in binary mode (CRs not padded): the upload still arrives: %r" % t.status()[16:60])
    t.sock.close()
    t.proc.wait(5)

    # ---------------- the stdio PTY path (ANetBBS native doors): raw bytes
    from phase6_zmodem import PtyDoor
    q = PtyDoor(binary, extra_args=("--data", os.path.join(root, "data2")))
    q.read(q.startup_secs)
    q.send(b" ", 1.0)
    q.send(ESC, 1.2)
    q.sock.sendall(b"u")
    rc = upload(q, sz, os.path.join(files, "harbor_moon.png"), telnet=False)
    q.read(2.0)
    q.send(ENTER, 2.0)
    check(rc == 0 and "Imported 1280x800 image" in q.status(), "stdio PTY: an image upload imports: %r" % q.status()[16:60])
    q.proc.kill()

    # ---------------- behind ANetBBS's relay, which drops every 0x03
    r = PtyDoor(binary, extra_args=("--data", os.path.join(root, "data4")))
    r.read(r.startup_secs)
    r.send(b" ", 1.0)
    r.send(ESC, 1.2)
    r.sock.sendall(b"u")
    rc = upload(r, sz, os.path.join(files, "harbor_moon.png"), telnet="anetbbs")
    r.read(2.0)
    r.send(ENTER, 2.0)
    check(rc == 0 and "Imported 1280x800 image" in r.status(),
          "ANetBBS relay (0x03 stripped): the upload still arrives: %r" % r.status()[16:60])
    r.proc.kill()

    print("\n%d failure(s)" % len(FAILS))
    sys.exit(1 if FAILS else 0)


if __name__ == "__main__":
    main()
