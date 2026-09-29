#!/usr/bin/env python3
"""Input latency regression test (OPENDOORS_LOCAL_PATCHES.md #3): mouse
drag events must be handled in a few ms each, a realistic 80 Hz drag
stream must never leave the screen behind the mouse, and an idle door
must not spin the CPU.

Usage: tests/latency_test.py path/to/anetdraw
"""
import os
import select
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from phase1_session import check, FAILS  # noqa: E402
from phase2_tools import Door  # noqa: E402


def cpu_seconds(pid):
    f = open("/proc/%d/stat" % pid).read().split(")")[1].split()
    return (int(f[11]) + int(f[12])) / os.sysconf("SC_CLK_TCK")


def main():
    d = Door(os.path.abspath(sys.argv[1]))
    d.read(d.startup_secs)
    d.send(b" ", 1.5)
    d.read(0.5)

    d.sock.sendall(b"\x1b[<0;2;3M")
    d.read(0.3)
    lat = []
    for i in range(15):
        t0 = time.time()
        d.sock.sendall(b"\x1b[<32;%d;3M" % (3 + i))
        select.select([d.sock], [], [], 3)
        lat.append((time.time() - t0) * 1000)
        d.read(0.1)
    d.sock.sendall(b"\x1b[<0;17;3m")
    d.read(0.3)
    med = sorted(lat)[len(lat) // 2]
    check(med < 8, "single mouse drag event handled in %.1f ms (limit 8)" % med)

    d.sock.sendall(b"\x1b[<0;2;8M")
    d.read(0.2)
    for i in range(150):
        d.sock.sendall(b"\x1b[<32;%d;%dM" % (2 + i % 75, 8 + i // 75))
        end = time.time() + 0.012
        while time.time() < end:
            r, _, _ = select.select([d.sock], [], [], max(0, end - time.time()))
            if r:
                d.stream.feed(d.sock.recv(65536).decode("cp437", "replace"))
    t_sent = last = time.time()
    while time.time() - last < 0.5:
        r, _, _ = select.select([d.sock], [], [], 0.05)
        if r:
            d.stream.feed(d.sock.recv(65536).decode("cp437", "replace"))
            last = time.time()
    behind = max(0, last - t_sent) * 1000
    check(behind < 150, "80 Hz drag stream: screen %.0f ms behind the mouse at the end (limit 150)" % behind)
    painted = sum(1 for x in range(80) for y in (7, 8) if d.cell(x, y).data == "█")
    check(painted >= 150, "all 150 drag cells painted (%d)" % painted)
    d.sock.sendall(b"\x1b[<0;77;9m")
    d.read(0.3)

    # a burst (what a real network link delivers): 60 drag events in one
    # packet must all be applied, drawn with far fewer updates than events
    d.sock.sendall(b"\x1b[<0;2;12M")
    d.read(0.3)
    d.raw = b""
    d.sock.sendall(b"".join(b"\x1b[<32;%d;12M" % (3 + i) for i in range(60)))
    d.read(1.5)
    d.sock.sendall(b"\x1b[<0;62;12m")
    d.read(0.3)
    burst_bytes = len(d.raw)
    painted = sum(1 for x in range(80) if d.cell(x, 11).data == "\u2588")
    check(painted >= 61, "burst of 60 drags fully applied (%d cells)" % painted)
    check(burst_bytes < 60 * 20, "burst drawn as a few updates, not 60 (%d bytes)" % burst_bytes)

    c0 = cpu_seconds(d.proc.pid)
    time.sleep(3)
    used = cpu_seconds(d.proc.pid) - c0
    check(used < 0.15, "idle door uses no CPU (%.2f s over 3 s)" % used)
    d.sock.close()
    d.proc.wait(5)
    print("\n%d failure(s)" % len(FAILS))
    sys.exit(1 if FAILS else 0)


if __name__ == "__main__":
    main()
