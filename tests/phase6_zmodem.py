#!/usr/bin/env python3
"""Phase 6 ZMODEM download test: the door sends the drawing to a real
receiver (lrzsz's rz) over both BBS paths -- the DOOR32.SYS telnet socket
and stdio on a PTY (how ANetBBS runs native doors: comm type 2, handle -1).

The harness plays the terminal: it watches for ZRQINIT the way SyncTERM
does (ZDLE 'B' '0' '0'), then hands the line to rz until it exits.

Usage: tests/phase6_zmodem.py path/to/anetdraw path/to/rz
"""
import os
import pty
import select
import subprocess
import sys
import tempfile
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from phase1_session import check, FAILS  # noqa: E402
from phase2_tools import Door, ctrl, ESC, ENTER, BKSP  # noqa: E402

ZRQINIT = b"\x18B00"


class FdSock:
    """Just enough of a socket for Session, over a PTY master fd."""
    def __init__(self, fd):
        self.fd = fd

    def fileno(self):
        return self.fd

    def recv(self, n):
        try:
            return os.read(self.fd, n)
        except OSError:
            return b""

    def sendall(self, data):
        while data:
            data = data[os.write(self.fd, data):]

    def close(self):
        os.close(self.fd)


class PtyDoor(Door):
    def _start_posix(self, binary, extra_args):
        self.startup_secs = 2.0
        drop = os.path.join(self.dir, "door32.sys")
        with open(drop, "w") as f:
            f.write("2\n-1\n38400\nANetBBS\n1\nTest Artist\nArtist\n100\n60\n1\n1\n")
        master, slave = pty.openpty()
        self.proc = subprocess.Popen([binary, "-D", drop] + list(extra_args), stdin=slave, stdout=slave,
                                     stderr=subprocess.DEVNULL, cwd=self.dir)
        os.close(slave)
        self.sock = FdSock(master)


def receive(d, rz, dest, answer=True, timeout=30):
    """Waits for ZRQINIT from the door, then runs rz on the line. Returns
    (seen_zrqinit, rz_exit_code, bytes the door sent before ZRQINIT)."""
    buf = b""
    end = time.time() + 8
    while ZRQINIT not in buf and time.time() < end:
        r, _, _ = select.select([d.sock], [], [], 0.1)
        if r:
            got = d.sock.recv(65536)
            if not got:
                break
            buf += got
    i = buf.find(ZRQINIT)
    if i < 0:
        return False, None, buf
    if not answer:
        return True, None, buf[:i]
    start = buf.rfind(b"*", 0, i)
    start = start - 1 if start > 0 and buf[start - 1:start] == b"*" else start
    rz_proc = subprocess.Popen([rz, "-y", "-q"], stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                               stderr=subprocess.DEVNULL, cwd=dest, bufsize=0)
    rz_proc.stdin.write(buf[start:])
    pumped = b""
    out_fd = rz_proc.stdout.fileno()
    end = time.time() + timeout
    while time.time() < end:
        r, _, _ = select.select([d.sock, out_fd], [], [], 0.1)
        if d.sock in r:
            got = d.sock.recv(65536)
            if not got:
                break
            pumped += got
            try:
                rz_proc.stdin.write(got)
            except BrokenPipeError:
                pass
        if out_fd in r:
            got = os.read(out_fd, 65536)
            if got:
                d.sock.sendall(got)
        if rz_proc.poll() is not None:
            # hand over anything rz still had queued
            while True:
                r, _, _ = select.select([out_fd], [], [], 0.05)
                if not r:
                    break
                got = os.read(out_fd, 65536)
                if not got:
                    break
                d.sock.sendall(got)
            break
    rc = rz_proc.wait(5)
    d.raw += pumped
    return True, rc, buf[:i]


def screen_has(d, text):
    return any(text in d.row(y) for y in range(d.screen.lines))


def session(kind, cls, binary, rz):
    root = tempfile.mkdtemp(prefix="anetdraw_zm_")
    data, dest = os.path.join(root, "data"), os.path.join(root, "got")
    os.makedirs(dest)
    d = cls(binary, extra_args=("--data", data))
    d.read(d.startup_secs)
    d.send(b" ", 1.0)
    d.send(ctrl("F") * 5, 0.5)       # bright red
    d.send(b"Hi", 0.8)
    d.send(ESC, 1.2)
    d.send(b"a", 1.2)
    d.send(b"smiley", 0.4)
    d.send(ENTER, 1.2)
    d.send(ENTER, 1.2)   # save options
    saved = os.path.join(data, "users", "1_test_artist", "smiley.ans")
    check(os.path.isfile(saved), "%s: saved smiley.ans first" % kind)

    d.send(ESC, 1.2)
    check(screen_has(d, "D Download"), "%s: the menu offers D Download (ZMODEM)" % kind)
    d.sock.sendall(b"d")
    seen, rc, before = receive(d, rz, dest)
    check(seen, "%s: the door starts with ZRQINIT" % kind)
    check(b"\x1b[?1002l" in before, "%s: mouse reporting is switched off before the transfer" % kind)
    check(rc == 0, "%s: rz finished cleanly (rc=%s)" % (kind, rc))
    got = os.path.join(dest, "smiley.ans")
    ok = os.path.isfile(got) and open(got, "rb").read() == open(saved, "rb").read()
    check(ok, "%s: the downloaded smiley.ans matches the saved file byte for byte (%s)"
          % (kind, os.listdir(dest)))
    d.read(2.0)
    check("Downloaded smiley.ans" in d.status(), "%s: status confirms: %r" % (kind, d.status()[16:60]))
    check(d.row(0).startswith("Hi") and d.cell(0, 0).fg == "red", "%s: the drawing is repainted" % kind)
    check(b"\x1b[?1002h" in d.raw[-4000:], "%s: mouse reporting is back on" % kind)
    d.send(b"!", 0.6)
    check(d.row(0).startswith("Hi!"), "%s: the editor still takes keys after the transfer" % kind)

    # a terminal that can't do ZMODEM: the caller presses Esc
    d.send(ESC, 1.2)
    d.sock.sendall(b"d")
    seen, _, _ = receive(d, rz, dest, answer=False)
    t0 = time.time()
    d.send(ESC, 1.5)
    check(seen and "Download cancelled" in d.status(),
          "%s: Esc at a plain terminal cancels at once: %r" % (kind, d.status()[16:60]))
    check(time.time() - t0 < 3 and d.row(0).startswith("Hi!"), "%s: and the screen comes back" % kind)

    d.send(ESC, 1.0)
    d.send(b"q", 1.0)
    d.send(b"n", 1.5)
    d.proc.wait(5)
    d.sock.close()


def main():
    binary = os.path.abspath(sys.argv[1])
    rz = os.path.abspath(sys.argv[2])
    session("telnet socket", Door, binary, rz)
    session("stdio pty", PtyDoor, binary, rz)
    print("\n%d failure(s)" % len(FAILS))
    sys.exit(1 if FAILS else 0)


if __name__ == "__main__":
    main()
